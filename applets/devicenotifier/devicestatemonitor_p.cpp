/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "devicestatemonitor_p.h"

#include "devicenotifier_debug.h"

#include <QTimer>

#include <Solid/Camera>
#include <Solid/Device>
#include <Solid/DeviceNotifier>
#include <Solid/GenericInterface>
#include <Solid/OpticalDisc>
#include <Solid/OpticalDrive>
#include <Solid/PortableMediaPlayer>
#include <Solid/StorageAccess>
#include <Solid/StorageDrive>
#include <Solid/StorageVolume>

DevicesStateMonitor::DevicesStateMonitor(QObject *parent)
    : QObject(parent)
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Devices State Monitor created";
}

DevicesStateMonitor::~DevicesStateMonitor()
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Devices State Monitor removed";
}

std::shared_ptr<DevicesStateMonitor> DevicesStateMonitor::instance()
{
    static std::weak_ptr<DevicesStateMonitor> s_clip;
    if (s_clip.expired()) {
        std::shared_ptr<DevicesStateMonitor> ptr{new DevicesStateMonitor};
        s_clip = ptr;
        return ptr;
    }
    return s_clip.lock();
}

void DevicesStateMonitor::addMonitoringDevice(const QString &udi)
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Devices State Monitor : addDevice signal arrived for " << udi;
    if (auto it = m_devicesStates.constFind(udi); it != m_devicesStates.constEnd()) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Devices State Monitor : Device " << udi << "is already monitoring. Don't add another one";
        return;
    }

    Solid::Device device(udi);

    DeviceInfo deviceInfo{
        .isBusy = false,
        .isRemovable = false,
        .isMounted = false,
        .isChecked = false,
        .needRepair = false,
        .operationResult = Solid::NoError,
        .operationInfo = QVariant(),
        .state = Idle,
        .deviceTimeStamp = QDateTime::currentDateTimeUtc(),
    };

    auto it = m_devicesStates.emplace(udi, deviceInfo);

    if (device.is<Solid::OpticalDisc>()) {
        Solid::OpticalDrive *drive = getAncestorAs<Solid::OpticalDrive>(device);
        if (drive) {
            connect(drive, &Solid::OpticalDrive::ejectRequested, this, &DevicesStateMonitor::setUnmountingState);
            connect(drive, &Solid::OpticalDrive::ejectDone, this, &DevicesStateMonitor::setIdleState);
        }
    }

    if (device.is<Solid::StorageVolume>()) {
        Solid::StorageAccess *access = device.as<Solid::StorageAccess>();
        if (access) {
            connect(access, &Solid::StorageAccess::accessibilityChanged, this, &DevicesStateMonitor::setAccessibilityState);
            connect(access, &Solid::StorageAccess::setupRequested, this, &DevicesStateMonitor::setMountingState);
            connect(access, &Solid::StorageAccess::setupDone, this, &DevicesStateMonitor::setIdleState);
            connect(access, &Solid::StorageAccess::teardownRequested, this, &DevicesStateMonitor::setUnmountingState);
            connect(access, &Solid::StorageAccess::teardownDone, this, &DevicesStateMonitor::setIdleState);
            if (access->canCheck()) {
                connect(access, &Solid::StorageAccess::checkRequested, this, &DevicesStateMonitor::setCheckingState);
                connect(access, &Solid::StorageAccess::checkDone, this, &DevicesStateMonitor::setIdleState);
            }
            if (access->canRepair()) {
                connect(access, &Solid::StorageAccess::repairRequested, this, &DevicesStateMonitor::setRepairingState);
                connect(access, &Solid::StorageAccess::repairDone, this, &DevicesStateMonitor::setIdleState);
            }
            qCDebug(APPLETS::DEVICENOTIFIER) << "Devices State Monitor : Device " << udi << " state : " << access->isAccessible();
            it->isMounted = access->isAccessible();
        }

        Solid::StorageVolume *storagevolume = device.as<Solid::StorageVolume>();
        if (storagevolume) {
            // Check if the volume is part of an encrypted container
            // This needs to trigger an update for the encrypted container volume since
            // libsolid cannot notify us when the accessibility of the container changes
            Solid::Device encryptedContainer = storagevolume->encryptedContainer();
            if (encryptedContainer.isValid()) {
                if (!m_encryptedContainerMap.contains(udi)) {
                    const QString containerUdi = encryptedContainer.udi();
                    m_encryptedContainerMap[udi] = containerUdi;
                    updateEncryptedContainer(containerUdi);
                }
            }
        }
    }

    if (device.is<Solid::StorageDrive>()) {
        Solid::StorageDrive *storagedrive = device.as<Solid::StorageDrive>();
        if (storagedrive) {
            it->isRemovable = storagedrive->isRemovable();
        }
    }

    Solid::StorageDrive *drive = getAncestorAs<Solid::StorageDrive>(device);
    if (drive) {
        // remove check for isHotpluggable() when plasmoids are changed to check for both properties
        it->isRemovable = drive->isRemovable() || drive->isHotpluggable();
    }

    if (device.is<Solid::Camera>()) {
        Solid::Camera *camera = device.as<Solid::Camera>();
        if (camera) {
            it->isRemovable = true;
        }
    }

    if (device.is<Solid::PortableMediaPlayer>()) {
        Solid::PortableMediaPlayer *mediaplayer = device.as<Solid::PortableMediaPlayer>();
        if (mediaplayer) {
            it->isRemovable = true;
        }
    }

    qCDebug(APPLETS::DEVICENOTIFIER) << "Devices State Monitor : Device " << udi << "successfully added";
    Q_EMIT stateChanged(udi);
}

void DevicesStateMonitor::removeMonitoringDevice(const QString &udi)
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Devices State Monitor : Remove Signal arrived for " << udi;
    if (auto it = m_devicesStates.constFind(udi); it != m_devicesStates.constEnd()) {
        m_devicesStates.erase(it);

        // libsolid cannot notify us when an encrypted container is closed,
        // hence we trigger an update when a device contained in an encrypted container device dies
        if (auto it = m_encryptedContainerMap.constFind(udi); it != m_encryptedContainerMap.constEnd()) {
            updateEncryptedContainer(it.value());
            m_encryptedContainerMap.erase(it);
        }

        Solid::Device device(udi);
        if (device.is<Solid::StorageVolume>()) {
            Solid::StorageAccess *access = device.as<Solid::StorageAccess>();
            if (access) {
                disconnect(access, nullptr, this, nullptr);
            }
        } else if (device.is<Solid::OpticalDisc>()) {
            Solid::OpticalDrive *drive = getAncestorAs<Solid::OpticalDrive>(device);
            if (drive) {
                disconnect(drive, nullptr, this, nullptr);
            }
        }

        Q_EMIT stateChanged(udi);

        qCDebug(APPLETS::DEVICENOTIFIER) << "Devices State Monitor : Device " << udi << " successfully removed";
    } else {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Devices State Monitor : Device " << udi << " was not monitored";
    }
}

bool DevicesStateMonitor::isBusy(const QString &udi) const
{
    if (auto it = m_devicesStates.constFind(udi); it != m_devicesStates.constEnd()) {
        return it->isBusy;
    }
    return false;
}

bool DevicesStateMonitor::isRemovable(const QString &udi) const
{
    if (auto it = m_devicesStates.constFind(udi); it != m_devicesStates.constEnd()) {
        return it->isRemovable;
    }
    return false;
}

bool DevicesStateMonitor::isMounted(const QString &udi) const
{
    if (auto it = m_devicesStates.constFind(udi); it != m_devicesStates.constEnd()) {
        return it->isMounted;
    }
    return false;
}

bool DevicesStateMonitor::isChecked(const QString &udi) const
{
    if (auto it = m_devicesStates.constFind(udi); it != m_devicesStates.constEnd()) {
        return it->isChecked;
    }
    return false;
}

bool DevicesStateMonitor::needRepair(const QString &udi) const
{
    if (auto it = m_devicesStates.constFind(udi); it != m_devicesStates.constEnd()) {
        return it->needRepair;
    }
    return false;
}

QDateTime DevicesStateMonitor::getDeviceTimeStamp(const QString &udi) const
{
    if (auto it = m_devicesStates.constFind(udi); it != m_devicesStates.constEnd()) {
        return it->deviceTimeStamp;
    }
    return {};
}

DevicesStateMonitor::State DevicesStateMonitor::getState(const QString &udi) const
{
    if (auto it = m_devicesStates.constFind(udi); it != m_devicesStates.constEnd()) {
        return it->state;
    }
    return NotPresent;
}

Solid::ErrorType DevicesStateMonitor::getOperationResult(const QString &udi) const
{
    if (auto it = m_devicesStates.constFind(udi); it != m_devicesStates.constEnd()) {
        return it->operationResult;
    }
    return Solid::NoError;
}

QVariant DevicesStateMonitor::getOperationInfo(const QString &udi) const
{
    if (auto it = m_devicesStates.constFind(udi); it != m_devicesStates.constEnd()) {
        return it->operationInfo;
    }
    return {};
}

void DevicesStateMonitor::setMountingState(const QString &udi)
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Devices State Monitor : Device " << udi << " state changed";
    if (auto it = m_devicesStates.find(udi); it != m_devicesStates.end()) {
        it->isBusy = true;
        it->state = Mounting;
        Q_EMIT stateChanged(udi);
    }
}

void DevicesStateMonitor::setUnmountingState(const QString &udi)
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Devices State Monitor : Device " << udi << " state changed";
    if (auto it = m_devicesStates.find(udi); it != m_devicesStates.end()) {
        it->isBusy = true;
        it->state = Unmounting;
        Q_EMIT stateChanged(udi);
    }
}

void DevicesStateMonitor::setCheckingState(const QString &udi)
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Devices State Monitor : Device " << udi << " state changed";
    if (auto it = m_devicesStates.find(udi); it != m_devicesStates.end()) {
        it->isBusy = true;
        it->state = Checking;
        Q_EMIT stateChanged(udi);
    }
}

void DevicesStateMonitor::setRepairingState(const QString &udi)
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Devices State Monitor : Device " << udi << " state changed";
    if (auto it = m_devicesStates.find(udi); it != m_devicesStates.end()) {
        it->isBusy = true;
        it->state = Repairing;
        Q_EMIT stateChanged(udi);
    }
}

void DevicesStateMonitor::setAccessibilityState(bool isAccessible, const QString &udi)
{
    if (auto it = m_devicesStates.find(udi); it != m_devicesStates.end()) {
        if (it->isMounted != isAccessible) {
            it->isMounted = isAccessible;
            Q_EMIT stateChanged(udi);
        }
    }
}

void DevicesStateMonitor::setIdleState(Solid::ErrorType operationResult, QVariant operationInfo, const QString &udi)
{
    Solid::Device device(udi);

    if (!device.isValid()) {
        return;
    }

    if (auto it = m_devicesStates.find(udi); it != m_devicesStates.end()) {
        it->isBusy = false;
        it->operationResult = operationResult;
        it->operationInfo = operationInfo;
        qCDebug(APPLETS::DEVICENOTIFIER) << "Devices State Monitor : Device " << udi << " Operation result is: " << operationResult
                                         << " operation info: " << it->operationInfo;
        if (it->state == Checking) {
            Solid::StorageAccess *access = device.as<Solid::StorageAccess>();
            it->isChecked = true;
            it->needRepair = (operationResult == Solid::NoError) ? !operationInfo.toBool() && access->canRepair() : false;
            qCDebug(APPLETS::DEVICENOTIFIER) << "Devices State Monitor : Device " << udi << " check done, need repair : " << it->needRepair;
            it->state = CheckDone;
        } else if (it->state == Repairing) {
            it->needRepair = (operationResult != Solid::NoError);
            qCDebug(APPLETS::DEVICENOTIFIER) << "Devices State Monitor : Device " << udi << " repair done, need repair : " << it->needRepair;
            it->state = RepairDone;
        } else if (it->state == Mounting) {
            auto access = device.as<Solid::StorageAccess>();
            it->isMounted = access->isAccessible();
            qCDebug(APPLETS::DEVICENOTIFIER) << "Devices State Monitor : Device " << udi << " Mount signal arrived. State changed : " << access->isAccessible();
            it->state = MountDone;
        } else if (it->state == Unmounting) {
            auto access = device.as<Solid::StorageAccess>();
            it->isMounted = access->isAccessible();
            qCDebug(APPLETS::DEVICENOTIFIER) << "Devices State Monitor : Device " << udi
                                             << " Unmount signal arrived. State changed : " << access->isAccessible();
            it->state = UnmountDone;
        } else {
            it->state = Idle;
        }
        Q_EMIT stateChanged(udi);
    }
}

void DevicesStateMonitor::updateEncryptedContainer(const QString &udi)
{
    if (auto it = m_devicesStates.find(udi); it != m_devicesStates.end()) {
        Solid::Device device(udi);
        if (!device.isValid()) {
            return;
        }

        it->state = Idle;

        Solid::StorageAccess *storageaccess = device.as<Solid::StorageAccess>();
        if (storageaccess) {
            it->isMounted = storageaccess->isAccessible();
        }

        Q_EMIT stateChanged(udi);
    }
}

#include "moc_devicestatemonitor_p.cpp"
