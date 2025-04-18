/*
    SPDX-FileCopyrightText: 2003-2007 Craig Drummond <craig@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "KCmFontInst.h"
#include "DuplicatesDialog.h"
#include "FcEngine.h"
#include "FontFilter.h"
#include "FontList.h"
#include "FontPreview.h"
#include "FontsPackage.h"
#include "KfiConstants.h"
#include "Misc.h"
#include "PreviewList.h"
#include "PreviewSelectAction.h"
#include "PrintDialog.h"
#include <KAboutData>
#include <KActionMenu>
#include <KConfigGroup>
#include <KGuiItem>
#include <KIO/StatJob>
#include <KIconLoader>
#include <KJobWidgets>
#include <KMessageBox>
#include <KPluginFactory>
#include <KStandardActions>
#include <KToolBar>
#include <KZip>
#include <QAction>
#include <QApplication>
#include <QBoxLayout>
#include <QCoreApplication>
#include <QFileDialog>
#include <QGridLayout>
#include <QIcon>
#include <QInputDialog>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QProcess>
#include <QProgressBar>
#include <QProgressDialog>
#include <QPushButton>
#include <QSplitter>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QTextStream>

#define CFG_GROUP "Main Settings"
#define CFG_PREVIEW_SPLITTER_SIZES "PreviewSplitterSizes"
#define CFG_GROUP_SPLITTER_SIZES "GroupSplitterSizes"
#define CFG_FONT_SIZE "FontSize"

K_PLUGIN_CLASS_WITH_JSON(KFI::CKCmFontInst, "kcm_fontinst.json")

using namespace Qt::StringLiterals;

namespace KFI
{
static QString partialIcon(bool load = true)
{
    QString name = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/kfi/partial.png"_L1;

    if (Misc::fExists(name)) {
        if (!load) {
            QFile::remove(name);
        }
    } else if (load) {
        QPixmap pix = KIconLoader::global()->loadIcon(u"dialog-ok"_s, KIconLoader::Small, KIconLoader::SizeSmall, KIconLoader::DisabledState);

        pix.save(name, "PNG");
    }

    return name;
}

class CPushButton : public QPushButton
{
public:
    CPushButton(const KGuiItem &item, QWidget *parent)
        : QPushButton(parent)
    {
        KGuiItem::assign(this, item);
        theirHeight = qMax(theirHeight, QPushButton::sizeHint().height());
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    }

    QSize sizeHint() const override
    {
        QSize sh(QPushButton::sizeHint());

        sh.setHeight(theirHeight);
        if (sh.width() < sh.height()) {
            sh.setWidth(sh.height());
        } else if (text().isEmpty()) {
            sh.setWidth(theirHeight);
        }
        return sh;
    }

    static int theirHeight;
};

class CToolBar : public KToolBar
{
public:
    CToolBar(QWidget *parent)
        : KToolBar(parent)
    {
        setMovable(false);
        setFloatable(false);
        setToolButtonStyle(Qt::ToolButtonIconOnly);
        setFont(QApplication::font());
    }

    void paintEvent(QPaintEvent *) override
    {
        QColor col(palette().color(backgroundRole()));

        col.setAlphaF(0.0);
        QPainter(this).fillRect(rect(), col);
    }
};

class CProgressBar : public QProgressBar
{
public:
    CProgressBar(QWidget *p, int h)
        : QProgressBar(p)
        , m_height((int)(h * 0.6))
    {
    }

    ~CProgressBar() override
    {
    }

    int height() const
    {
        return m_height;
    }
    QSize sizeHint() const override
    {
        return QSize(100, m_height);
    }

private:
    int m_height;
};

int CPushButton::theirHeight = 0;

CKCmFontInst::CKCmFontInst(QObject *parent, const KPluginMetaData &data)
    : KCModule(parent, data)
    , m_preview(nullptr)
    , m_config(KFI_UI_CFG_FILE)
    , m_job(nullptr)
    , m_progress(nullptr)
    , m_updateDialog(nullptr)
    , m_tempDir(nullptr)
    , m_printProc(nullptr)
{
    setButtons(Help);

    KIconLoader::global()->addAppDir(QStringLiteral(KFI_NAME));

    KConfigGroup cg(&m_config, QStringLiteral(CFG_GROUP));

    m_groupSplitter = new QSplitter(widget());
    m_groupSplitter->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    QWidget *groupWidget = new QWidget(m_groupSplitter), *fontWidget = new QWidget(m_groupSplitter);

    m_previewSplitter = new QSplitter(fontWidget);
    m_previewSplitter->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    connect(widget(), &QObject::destroyed, this, [this] { // this signal fires before the children destroy!
        KConfigGroup cg(&m_config, QStringLiteral(CFG_GROUP));

        cg.writeEntry(CFG_PREVIEW_SPLITTER_SIZES, m_previewSplitter->sizes());
        cg.writeEntry(CFG_GROUP_SPLITTER_SIZES, m_groupSplitter->sizes());
    });

    QWidget *fontControlWidget = new QWidget(fontWidget);
    auto mainLayout = new QVBoxLayout(widget());
    auto fontsLayout = new QVBoxLayout(fontWidget);
    auto fontControlLayout = new QHBoxLayout(fontControlWidget);

    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    fontsLayout->setSpacing(0);
    fontsLayout->setContentsMargins(0, 0, 0, 0);

    m_filter = new CFontFilter(widget());

    // Details - Groups...
    auto groupsLayout = new QVBoxLayout(groupWidget);
    groupsLayout->setSpacing(0);
    groupsLayout->setContentsMargins({});

    m_groupList = new CGroupList(groupWidget);
    m_groupListView = new CGroupListView(groupWidget, m_groupList);
    m_groupListView->setProperty("_breeze_borders_sides", QVariant::fromValue(QFlags{Qt::BottomEdge}));
    groupsLayout->addWidget(m_groupListView);

    auto groupsButtonLayout = new QHBoxLayout;
    groupsButtonLayout->setSpacing(widget()->style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing));
    groupsButtonLayout->setContentsMargins(widget()->style()->pixelMetric(QStyle::PM_LayoutLeftMargin),
                                           widget()->style()->pixelMetric(QStyle::PM_LayoutTopMargin),
                                           widget()->style()->pixelMetric(QStyle::PM_LayoutRightMargin),
                                           widget()->style()->pixelMetric(QStyle::PM_LayoutBottomMargin));
    QPushButton *createGroup = new CPushButton(KGuiItem(QString(), u"list-add"_s, i18n("Create New Group…")), groupWidget);
    groupsButtonLayout->addWidget(createGroup);

    m_deleteGroupControl = new CPushButton(KGuiItem(QString(), u"list-remove"_s, i18n("Remove Group…")), groupWidget);
    groupsButtonLayout->addWidget(m_deleteGroupControl);

    m_enableGroupControl = new CPushButton(KGuiItem(QString(), u"font-enable"_s, i18n("Enable Fonts in Group…")), groupWidget);
    groupsButtonLayout->addWidget(m_enableGroupControl);

    m_disableGroupControl = new CPushButton(KGuiItem(QString(), u"font-disable"_s, i18n("Disable Fonts in Group…")), groupWidget);
    groupsButtonLayout->addWidget(m_disableGroupControl);

    groupsLayout->addLayout(groupsButtonLayout);

    m_previewWidget = new QWidget(widget());
    auto previewWidgetLayout = new QVBoxLayout(m_previewWidget);
    previewWidgetLayout->setContentsMargins(0, 0, 0, 0);
    previewWidgetLayout->setSpacing(0);

    // Preview
    QFrame *previewFrame = new QFrame(m_previewWidget);
    QBoxLayout *previewFrameLayout = new QBoxLayout(QBoxLayout::LeftToRight, previewFrame);

    previewFrameLayout->setContentsMargins(0, 0, 0, 0);
    previewFrameLayout->setSpacing(0);
    previewFrame->setFrameShape(QFrame::StyledPanel);
    previewFrame->setFrameShadow(QFrame::Sunken);
    previewFrame->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);

    m_preview = new CFontPreview(previewFrame);
    m_preview->setWhatsThis(i18n("This displays a preview of the selected font."));
    m_preview->setContextMenuPolicy(Qt::CustomContextMenu);
    previewFrameLayout->addWidget(m_preview);
    previewWidgetLayout->addWidget(previewFrame);
    m_preview->engine()->readConfig(m_config);

    // List-style preview...
    m_previewList = new CPreviewListView(m_preview->engine(), m_previewWidget);
    previewWidgetLayout->addWidget(m_previewList);
    m_previewList->setVisible(false);

    // Font List...
    m_fontList = new CFontList(fontWidget);
    m_fontListView = new CFontListView(m_previewSplitter, m_fontList);
    m_fontListView->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    m_scanDuplicateFontsControl =
        new CPushButton(KGuiItem(i18n("Find Duplicates…"), u"edit-duplicate"_s, i18n("Scan for Duplicate Fonts…")), fontControlWidget);

    m_addFontControl = new CPushButton(KGuiItem(i18n("Install from File…"), u"document-import"_s, i18n("Install fonts from a local file")), fontControlWidget);
    m_getNewFontsControl = new KNSWidgets::Button(i18n("Get New Fonts…"), QStringLiteral("kfontinst.knsrc"), widget());
    m_getNewFontsControl->setToolTip(i18n("Download new fonts"));

    m_deleteFontControl = new CPushButton(KGuiItem(QString(), u"edit-delete"_s, i18n("Delete Selected Fonts…")), fontControlWidget);

    m_previewSplitter->addWidget(m_previewWidget);
    m_previewSplitter->setCollapsible(1, true);

    m_statusLabel = new QLabel(widget());
    m_statusLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    m_listingProgress = new CProgressBar(widget(), m_statusLabel->height());
    m_listingProgress->setRange(0, 100);

    // Layout widgets...
    mainLayout->addWidget(m_filter);

    auto separator = new QFrame(widget());
    separator->setFrameShape(QFrame::HLine);
    separator->setMaximumHeight(1);
    mainLayout->addWidget(separator);

    mainLayout->addWidget(m_groupSplitter);

    fontControlLayout->addWidget(m_deleteFontControl);
    fontControlLayout->addStretch();
    fontControlLayout->addWidget(m_listingProgress);
    fontControlLayout->addWidget(m_statusLabel);
    fontControlLayout->addWidget(m_scanDuplicateFontsControl);
    fontControlLayout->addWidget(m_addFontControl);
    fontControlLayout->addWidget(m_getNewFontsControl);

    fontsLayout->addWidget(m_previewSplitter);
    separator = new QFrame(widget());
    separator->setFrameShape(QFrame::HLine);
    separator->setMaximumHeight(1);
    fontsLayout->addWidget(separator);
    fontsLayout->addWidget(fontControlWidget);

    // Set size of widgets...
    m_previewSplitter->setChildrenCollapsible(false);
    m_groupSplitter->setChildrenCollapsible(false);
    m_groupSplitter->setStretchFactor(0, 0);
    m_groupSplitter->setStretchFactor(1, 1);
    m_previewSplitter->setStretchFactor(0, 1);
    m_previewSplitter->setStretchFactor(1, 0);

    // Set sizes for 3 views...
    QList<int> defaultSizes;
    defaultSizes += 300;
    defaultSizes += 220;
    m_previewSplitter->setSizes(cg.readEntry(CFG_PREVIEW_SPLITTER_SIZES, defaultSizes));
    m_previewHidden = m_previewSplitter->sizes().at(1) < 8;

    defaultSizes.clear();
    defaultSizes += 110;
    defaultSizes += 350;
    m_groupSplitter->setSizes(cg.readEntry(CFG_GROUP_SPLITTER_SIZES, defaultSizes));

    // Preview widget pop-up menu
    m_previewMenu = new QMenu(m_preview);
    QAction *zoomIn = KStandardActions::create(KStandardActions::ZoomIn, m_preview, &CFontPreview::zoomIn, this),
            *zoomOut = KStandardActions::create(KStandardActions::ZoomOut, m_preview, &CFontPreview::zoomOut, this);

    m_previewMenu->addAction(zoomIn);
    m_previewMenu->addAction(zoomOut);
    m_previewMenu->addSeparator();
    CPreviewSelectAction *prevSel = new CPreviewSelectAction(m_previewMenu);
    m_previewMenu->addAction(prevSel);
    QAction *changeTextAct = new QAction(QIcon::fromTheme(u"edit-rename"_s), i18n("Change Preview Text…"), this);
    m_previewMenu->addAction(changeTextAct),

        m_previewListMenu = new QMenu(m_previewList);
    m_previewListMenu->addAction(changeTextAct),

        // Connect signals...
        connect(m_preview, &CFontPreview::atMax, zoomIn, &QAction::setDisabled);
    connect(m_preview, &CFontPreview::atMin, zoomOut, &QAction::setDisabled);
    connect(prevSel, &CPreviewSelectAction::range, m_preview, &CFontPreview::setUnicodeRange);
    connect(changeTextAct, &QAction::triggered, this, &CKCmFontInst::changeText);
    connect(m_filter, &CFontFilter::queryChanged, m_fontListView, &CFontListView::filterText);
    connect(m_filter, &CFontFilter::criteriaChanged, m_fontListView, &CFontListView::filterCriteria);
    connect(m_groupListView, &CGroupListView::del, this, &CKCmFontInst::removeGroup);
    connect(m_groupListView, &CGroupListView::print, this, &CKCmFontInst::printGroup);
    connect(m_groupListView, &CGroupListView::enable, this, &CKCmFontInst::enableGroup);
    connect(m_groupListView, &CGroupListView::disable, this, &CKCmFontInst::disableGroup);
    connect(m_groupListView, &CGroupListView::moveFonts, this, &CKCmFontInst::moveFonts);
    connect(m_groupListView, &CGroupListView::zip, this, &CKCmFontInst::zipGroup);
    connect(m_groupListView, &CGroupListView::itemSelected, this, &CKCmFontInst::groupSelected);
    connect(m_groupListView, &CGroupListView::info, this, &CKCmFontInst::showInfo);
    connect(m_groupList, &CGroupList::refresh, this, &CKCmFontInst::refreshFontList);
    connect(m_fontList, &CFontList::listingPercent, this, &CKCmFontInst::listingPercent);
    connect(m_fontList, &QAbstractItemModel::layoutChanged, this, &CKCmFontInst::setStatusBar);
    connect(m_fontListView, &CFontListView::del, this, &CKCmFontInst::deleteFonts);
    connect(m_fontListView, SIGNAL(print()), SLOT(print()));
    connect(m_fontListView, &CFontListView::enable, this, &CKCmFontInst::enableFonts);
    connect(m_fontListView, &CFontListView::disable, this, &CKCmFontInst::disableFonts);
    connect(m_fontListView, &CFontListView::fontsDropped, this, qOverload<const QSet<QUrl> &>(&CKCmFontInst::addFonts));
    connect(m_fontListView, &CFontListView::itemsSelected, this, &CKCmFontInst::fontsSelected);
    connect(m_fontListView, &CFontListView::refresh, this, &CKCmFontInst::setStatusBar);
    connect(m_groupListView, &CGroupListView::unclassifiedChanged, m_fontListView, &CFontListView::refreshFilter);
    connect(createGroup, &QAbstractButton::clicked, this, &CKCmFontInst::addGroup);
    connect(m_deleteGroupControl, &QAbstractButton::clicked, this, &CKCmFontInst::removeGroup);
    connect(m_enableGroupControl, &QAbstractButton::clicked, this, &CKCmFontInst::enableGroup);
    connect(m_disableGroupControl, &QAbstractButton::clicked, this, &CKCmFontInst::disableGroup);
    connect(m_addFontControl, &QAbstractButton::clicked, this, qOverload<>(&CKCmFontInst::addFonts));
    connect(m_getNewFontsControl, &KNSWidgets::Button::dialogFinished, this, &CKCmFontInst::downloadFonts);
    connect(m_deleteFontControl, &QAbstractButton::clicked, this, &CKCmFontInst::deleteFonts);
    connect(m_scanDuplicateFontsControl, &QAbstractButton::clicked, this, &CKCmFontInst::duplicateFonts);
    // connect(validateFontsAct, SIGNAL(triggered(bool)), SLOT(validateFonts()));
    connect(m_preview, &QWidget::customContextMenuRequested, this, &CKCmFontInst::previewMenu);
    connect(m_previewList, &CPreviewListView::showMenu, this, &CKCmFontInst::previewMenu);
    connect(m_previewSplitter, &QSplitter::splitterMoved, this, &CKCmFontInst::splitterMoved);

    selectMainGroup();
    m_fontList->load();
}

CKCmFontInst::~CKCmFontInst()
{
    // NOTE: do not access children of widget() here; it may have been destroyed already!
    delete m_tempDir;
    partialIcon(false);
}

void CKCmFontInst::previewMenu(const QPoint &pos)
{
    if (m_previewList->isHidden()) {
        m_previewMenu->popup(m_preview->mapToGlobal(pos));
    } else {
        m_previewListMenu->popup(m_previewList->mapToGlobal(pos));
    }
}

void CKCmFontInst::splitterMoved()
{
    if (m_previewWidget->width() > 8 && m_previewHidden) {
        m_previewHidden = false;
        fontsSelected(m_fontListView->getSelectedItems());
    } else if (!m_previewHidden && m_previewWidget->width() < 8) {
        m_previewHidden = true;
    }
}

void CKCmFontInst::fontsSelected(const QModelIndexList &list)
{
    if (!m_previewHidden) {
        if (!list.isEmpty()) {
            if (list.count() < 2) {
                CFontModelItem *mi = static_cast<CFontModelItem *>(list.last().internalPointer());
                CFontItem *font = mi->parent() ? static_cast<CFontItem *>(mi) : (static_cast<CFamilyItem *>(mi))->regularFont();

                if (font) {
                    m_preview->showFont(font->isEnabled() ? font->family() : font->fileName(), font->styleInfo(), font->index());
                }
            } else {
                m_previewList->showFonts(list);
            }
        }
        m_previewList->setVisible(list.count() > 1);
        m_preview->parentWidget()->setVisible(list.count() < 2);
    }

    m_deleteFontControl->setEnabled(list.count());
}

void CKCmFontInst::addFonts()
{
    QFileDialog dlg(widget(), i18n("Add Fonts"));
    dlg.setFileMode(QFileDialog::ExistingFiles);
    dlg.setMimeTypeFilters(CFontList::fontMimeTypes);
    QList<QUrl> list;
    if (dlg.exec() == QDialog::Accepted) {
        list = dlg.selectedUrls();
    }

    if (!list.isEmpty()) {
        QSet<QUrl> urls;
        QList<QUrl>::Iterator it(list.begin()), end(list.end());

        for (; it != end; ++it) {
            if (QLatin1String(KFI_KIO_FONTS_PROTOCOL) != (*it).scheme()) // Do not try to install from fonts:/ !!!
            {
                auto job = KIO::mostLocalUrl(*it);
                KJobWidgets::setWindow(job, widget());
                job->exec();
                QUrl url = job->mostLocalUrl();

                if (url.isLocalFile()) {
                    QString file(url.toLocalFile());

                    if (Misc::isPackage(file)) { // If its a package we need to unzip 1st...
                        urls += FontsPackage::extract(url.toLocalFile(), &m_tempDir);
                    } else if (!Misc::isMetrics(url)) {
                        urls.insert(url);
                    }
                } else if (!Misc::isMetrics(url)) {
                    urls.insert(url);
                }
            }
        }
        if (!urls.isEmpty()) {
            addFonts(urls);
        }
        delete m_tempDir;
        m_tempDir = nullptr;
    }
}

void CKCmFontInst::groupSelected(const QModelIndex &index)
{
    CGroupListItem *grp = nullptr;

    if (index.isValid()) {
        grp = static_cast<CGroupListItem *>(index.internalPointer());
    } else {
        return;
    }

    m_fontListView->setFilterGroup(grp);
    setStatusBar();

    //
    // Check fonts listed within group are still valid!
    if (grp->isCustom() && !grp->validated()) {
        QSet<QString> remList;
        QSet<QString>::Iterator it(grp->families().begin()), end(grp->families().end());

        for (; it != end; ++it) {
            if (!m_fontList->hasFamily(*it)) {
                remList.insert(*it);
            }
        }
        it = remList.begin();
        end = remList.end();
        for (; it != end; ++it) {
            m_groupList->removeFromGroup(grp, *it);
        }
        grp->setValidated();
    }

    m_getNewFontsControl->setEnabled(grp->isPersonal() || grp->isAll());
}

void CKCmFontInst::print(bool all)
{
    //
    // In order to support printing of newly installed/enabled fonts, the actual printing
    // is carried out by the kfontinst helper app. This way we know Qt's font list will be
    // up to date.
    if ((!m_printProc || QProcess::NotRunning == m_printProc->state()) && !Misc::app(KFI_PRINTER).isEmpty()) {
        QSet<Misc::TFont> fonts;

        m_fontListView->getPrintableFonts(fonts, !all);

        if (!fonts.isEmpty()) {
            CPrintDialog dlg(widget());
            KConfigGroup cg(&m_config, QStringLiteral(CFG_GROUP));

            if (dlg.exec(cg.readEntry(CFG_FONT_SIZE, 1))) {
                static const int constSizes[] = {0, 12, 18, 24, 36, 48};
                QSet<Misc::TFont>::ConstIterator it(fonts.begin()), end(fonts.end());
                QTemporaryFile tmpFile;
                bool useFile(fonts.count() > 16), startProc(true);
                QStringList args;

                if (!m_printProc) {
                    m_printProc = new QProcess(this);
                } else {
                    m_printProc->kill();
                }

                QString title = QGuiApplication::applicationDisplayName();
                if (title.isEmpty()) {
                    title = QCoreApplication::applicationName();
                }

                //
                // If we have lots of fonts to print, pass kfontinst a temporary groups file to print
                // instead of passing font by font...
                if (useFile) {
                    if (tmpFile.open()) {
                        QTextStream str(&tmpFile);

                        for (; it != end; ++it) {
                            str << (*it).family << Qt::endl << (*it).styleInfo << Qt::endl;
                        }

                        args << u"--embed"_s << QStringLiteral("0x%1").arg((unsigned int)widget()->window()->winId(), 0, 16) << u"--qwindowtitle"_s << title
                             << u"--qwindowicon"_s << u"preferences-desktop-font-installer"_s << u"--size"_s
                             << QString::number(constSizes[dlg.chosenSize() < 6 ? dlg.chosenSize() : 2]) << u"--listfile"_s << tmpFile.fileName()
                             << u"--deletefile"_s;
                    } else {
                        KMessageBox::error(widget(), i18n("Failed to save list of fonts to print."));
                        startProc = false;
                    }
                } else {
                    args << u"--embed"_s << QStringLiteral("0x%1").arg((unsigned int)widget()->window()->winId(), 0, 16) << u"--qwindowtitle"_s << title
                         << u"--qwindowicon"_s << u"preferences-desktop-font-installer"_s << u"--size"_s
                         << QString::number(constSizes[dlg.chosenSize() < 6 ? dlg.chosenSize() : 2]);

                    for (; it != end; ++it) {
                        args << u"--pfont"_s << QString((*it).family + u',' + QString().setNum((*it).styleInfo));
                    }
                }

                if (startProc) {
                    m_printProc->start(Misc::app(KFI_PRINTER), args);

                    if (m_printProc->waitForStarted(1000)) {
                        if (useFile) {
                            tmpFile.setAutoRemove(false);
                        }
                    } else {
                        KMessageBox::error(widget(), i18n("Failed to start font printer."));
                    }
                }
                cg.writeEntry(CFG_FONT_SIZE, dlg.chosenSize());
            }
        } else {
            KMessageBox::information(widget(),
                                     i18n("There are no printable fonts.\n"
                                          "You can only print non-bitmap and enabled fonts."),
                                     i18n("Cannot Print"));
        }
    }
}

void CKCmFontInst::deleteFonts()
{
    CJobRunner::ItemList urls;
    QStringList fontNames;
    QSet<Misc::TFont> fonts;

    m_deletedFonts.clear();
    m_fontListView->getFonts(urls, fontNames, &fonts, true);

    if (urls.isEmpty()) {
        KMessageBox::information(widget(), i18n("You did not select anything to delete."), i18n("Nothing to Delete"));
    } else {
        QSet<Misc::TFont>::ConstIterator it(fonts.begin()), end(fonts.end());
        bool doIt = false;

        for (; it != end; ++it) {
            m_deletedFonts.insert((*it).family);
        }

        switch (fontNames.count()) {
        case 0:
            break;
        case 1:
            doIt = KMessageBox::Continue
                == KMessageBox::warningContinueCancel(widget(),
                                                      i18n("<p>Do you really want to "
                                                           "delete</p><p>\'<b>%1</b>\'?</p>",
                                                           fontNames.first()),
                                                      i18n("Delete Font"),
                                                      KStandardGuiItem::del());
            break;
        default:
            doIt = KMessageBox::Continue
                == KMessageBox::warningContinueCancelList(
                       widget(),
                       i18np("Do you really want to delete this font?", "Do you really want to delete these %1 fonts?", fontNames.count()),
                       fontNames,
                       i18n("Delete Fonts"),
                       KStandardGuiItem::del());
        }

        if (doIt) {
            m_statusLabel->setText(i18n("Deleting font(s)…"));
            doCmd(CJobRunner::CMD_DELETE, urls);
        }
    }
}

void CKCmFontInst::moveFonts()
{
    CJobRunner::ItemList urls;
    QStringList fontNames;

    m_deletedFonts.clear();
    m_fontListView->getFonts(urls, fontNames, nullptr, true);

    if (urls.isEmpty()) {
        KMessageBox::information(widget(), i18n("You did not select anything to move."), i18n("Nothing to Move"));
    } else {
        bool doIt = false;

        switch (fontNames.count()) {
        case 0:
            break;
        case 1:
            doIt = KMessageBox::Continue
                == KMessageBox::warningContinueCancel(widget(),
                                                      i18n("<p>Do you really want to "
                                                           "move</p><p>\'<b>%1</b>\'</p><p>from <i>%2</i> to <i>%3</i>?</p>",
                                                           fontNames.first(),
                                                           m_groupListView->isSystem() ? KFI_KIO_FONTS_SYS.toString() : KFI_KIO_FONTS_USER.toString(),
                                                           m_groupListView->isSystem() ? KFI_KIO_FONTS_USER.toString() : KFI_KIO_FONTS_SYS.toString()),
                                                      i18n("Move Font"),
                                                      KGuiItem(i18n("Move")));
            break;
        default:
            doIt = KMessageBox::Continue
                == KMessageBox::warningContinueCancelList(widget(),
                                                          i18np("<p>Do you really want to move this font from <i>%2</i> to <i>%3</i>?</p>",
                                                                "<p>Do you really want to move these %1 fonts from <i>%2</i> to <i>%3</i>?</p>",
                                                                fontNames.count(),
                                                                m_groupListView->isSystem() ? KFI_KIO_FONTS_SYS.toString() : KFI_KIO_FONTS_USER.toString(),
                                                                m_groupListView->isSystem() ? KFI_KIO_FONTS_USER.toString() : KFI_KIO_FONTS_SYS.toString()),
                                                          fontNames,
                                                          i18n("Move Fonts"),
                                                          KGuiItem(i18n("Move")));
        }

        if (doIt) {
            m_statusLabel->setText(i18n("Moving font(s)…"));
            doCmd(CJobRunner::CMD_MOVE, urls, !m_groupListView->isSystem());
        }
    }
}

void CKCmFontInst::zipGroup()
{
    QModelIndex idx(m_groupListView->currentIndex());

    if (idx.isValid()) {
        CGroupListItem *grp = static_cast<CGroupListItem *>(idx.internalPointer());

        if (grp) {
            QFileDialog dlg(widget(), i18n("Export Group"));
            dlg.setAcceptMode(QFileDialog::AcceptSave);
            dlg.setDirectoryUrl(QUrl::fromLocalFile(grp->name()));
            dlg.setMimeTypeFilters(QStringList() << QStringLiteral("application/zip"));
            QString fileName;
            if (dlg.exec() == QDialog::Accepted) {
                fileName = dlg.selectedFiles().value(0);
            }

            if (!fileName.isEmpty()) {
                KZip zip(fileName);

                if (zip.open(QIODevice::WriteOnly)) {
                    QSet<QString> files;

                    files = m_fontListView->getFiles();

                    if (!files.isEmpty()) {
                        QMap<QString, QString> map = Misc::getFontFileMap(files);
                        QMap<QString, QString>::ConstIterator it(map.constBegin()), end(map.constEnd());

                        for (; it != end; ++it) {
                            zip.addLocalFile(it.value(), it.key());
                        }
                        zip.close();
                    } else {
                        KMessageBox::error(widget(), i18n("No files?"));
                    }
                } else {
                    KMessageBox::error(widget(), i18n("Failed to open %1 for writing", fileName));
                }
            }
        }
    }
}

void CKCmFontInst::enableFonts()
{
    toggleFonts(true);
}

void CKCmFontInst::disableFonts()
{
    toggleFonts(false);
}

void CKCmFontInst::addGroup()
{
    bool ok;
    QString name(QInputDialog::getText(widget(), i18n("Create New Group"), i18n("Name of new group:"), QLineEdit::Normal, i18n("New Group"), &ok));

    if (ok && !name.isEmpty()) {
        m_groupList->createGroup(name);
    }
}

void CKCmFontInst::removeGroup()
{
    if (m_groupList->removeGroup(m_groupListView->currentIndex())) {
        selectMainGroup();
    }
}

void CKCmFontInst::enableGroup()
{
    toggleGroup(true);
}

void CKCmFontInst::disableGroup()
{
    toggleGroup(false);
}

void CKCmFontInst::changeText()
{
    bool status;
    QString oldStr(m_preview->engine()->getPreviewString()),
        newStr(QInputDialog::getText(widget(), i18n("Preview Text"), i18n("Please enter new text:"), QLineEdit::Normal, oldStr, &status));

    if (status && oldStr != newStr) {
        m_preview->engine()->setPreviewString(newStr);

        m_preview->showFont();
        m_previewList->refreshPreviews();
    }
}

void CKCmFontInst::duplicateFonts()
{
    CDuplicatesDialog(widget(), m_fontList).exec();
}

// void CKCmFontInst::validateFonts()
//{
//}

void CKCmFontInst::downloadFonts(const QList<KNSCore::Entry> &changedEntries)
{
    if (changedEntries.isEmpty()) {
        return;
    }

    // Ask dbus helper for the current fonts folder name...
    // We then sym-link our knewstuff3 download folder into the fonts folder...
    QString destFolder = CJobRunner::folderName(false);
    if (!destFolder.isEmpty()) {
        destFolder += "kfontinst"_L1;
        if (!QFile::exists(destFolder)) {
            QFile _file(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1Char('/') + "kfontinst"_L1);
            _file.link(destFolder);
        }
    }

    doCmd(CJobRunner::CMD_UPDATE, CJobRunner::ItemList());
}

void CKCmFontInst::print()
{
    print(false);
}

void CKCmFontInst::printGroup()
{
    print(true);
}

void CKCmFontInst::listingPercent(int p)
{
    if (0 == p) {
        showInfo(i18n("Scanning font list…"));
        m_listingProgress->show();
    } else if (100 == p && p != m_listingProgress->value()) {
        removeDeletedFontsFromGroups();

        QSet<QString> foundries;

        m_fontList->getFoundries(foundries);
        m_filter->setFoundries(foundries);
        refreshFamilies();
        m_listingProgress->hide();
        m_fontListView->selectFirstFont();
    }
    m_listingProgress->setValue(p);
}

void CKCmFontInst::refreshFontList()
{
    m_fontListView->refreshFilter();
    refreshFamilies();
}

void CKCmFontInst::refreshFamilies()
{
    QSet<QString> enabledFamilies, disabledFamilies, partialFamilies;

    m_fontList->getFamilyStats(enabledFamilies, disabledFamilies, partialFamilies);
    m_groupList->updateStatus(enabledFamilies, disabledFamilies, partialFamilies);
    setStatusBar();
}

void CKCmFontInst::showInfo(const QString &info)
{
    if (info.isEmpty()) {
        if (m_lastStatusBarMsg.isEmpty()) {
            setStatusBar();
        } else {
            m_statusLabel->setText(m_lastStatusBarMsg);
            m_lastStatusBarMsg = QString();
        }
    } else {
        if (m_lastStatusBarMsg.isEmpty()) {
            m_lastStatusBarMsg = m_statusLabel->text();
        }
        m_statusLabel->setText(info);
    }
}

void CKCmFontInst::setStatusBar()
{
    if (m_fontList->slowUpdates()) {
        return;
    }

    int enabled = 0, disabled = 0, partial = 0;
    bool selectedEnabled = false, selectedDisabled = false;

    m_statusLabel->setToolTip(QString());
    if (0 == m_fontList->families().count()) {
        m_statusLabel->setText(i18n("No fonts"));
    } else {
        m_fontListView->stats(enabled, disabled, partial);
        m_fontListView->selectedStatus(selectedEnabled, selectedDisabled);

        QString text(i18np("1 Font", "%1 Fonts", enabled + disabled + partial));

        if (disabled || partial) {
            text += QLatin1String(" (<img src=\"%1\" />%2").arg(KIconLoader::global()->iconPath(u"dialog-ok"_s, -KIconLoader::SizeSmall)).arg(enabled)
                + QLatin1String(" <img src=\"%1\" />%2").arg(KIconLoader::global()->iconPath(u"dialog-cancel"_s, -KIconLoader::SizeSmall)).arg(disabled);
            if (partial) {
                text += QLatin1String(" <img src=\"%1\" />%2").arg(partialIcon()).arg(partial);
            }
            text += QLatin1Char(')');
            m_statusLabel->setToolTip(partial ? i18n("<table>"
                                                     "<tr><td align=\"right\">Enabled:</td><td>%1</td></tr>"
                                                     "<tr><td align=\"right\">Disabled:</td><td>%2</td></tr>"
                                                     "<tr><td align=\"right\">Partially enabled:</td><td>%3</td></tr>"
                                                     "<tr><td align=\"right\">Total:</td><td>%4</td></tr>"
                                                     "</table>",
                                                     enabled,
                                                     disabled,
                                                     partial,
                                                     enabled + disabled + partial)
                                              : i18n("<table>"
                                                     "<tr><td align=\"right\">Enabled:</td><td>%1</td></tr>"
                                                     "<tr><td align=\"right\">Disabled:</td><td>%2</td></tr>"
                                                     "<tr><td align=\"right\">Total:</td><td>%3</td></tr>"
                                                     "</table>",
                                                     enabled,
                                                     disabled,
                                                     enabled + disabled));
        }

        m_statusLabel->setText(disabled || partial ? "<p>"_L1 + text + "</p>"_L1 : text);
    }

    CGroupListItem::EType type(m_groupListView->getType());

    bool isStd(CGroupListItem::CUSTOM == type);

    m_addFontControl->setEnabled(CGroupListItem::ALL == type || CGroupListItem::UNCLASSIFIED == type || CGroupListItem::PERSONAL == type
                                 || CGroupListItem::SYSTEM == type);
    m_deleteGroupControl->setEnabled(isStd);
    m_enableGroupControl->setEnabled(disabled || partial);
    m_disableGroupControl->setEnabled(isStd && (enabled || partial));

    m_groupListView->controlMenu(m_deleteGroupControl->isEnabled(),
                                 m_enableGroupControl->isEnabled(),
                                 m_disableGroupControl->isEnabled(),
                                 enabled || partial,
                                 enabled || disabled);

    m_deleteFontControl->setEnabled(selectedEnabled || selectedDisabled);
}

void CKCmFontInst::addFonts(const QSet<QUrl> &src)
{
    if (!src.isEmpty() && !m_groupListView->isCustom()) {
        bool system;

        if (Misc::root()) {
            system = true;
        } else {
            switch (m_groupListView->getType()) {
            case CGroupListItem::ALL:
            case CGroupListItem::UNCLASSIFIED:
                switch (KMessageBox::questionTwoActionsCancel(widget(),
                                                              i18n("Do you wish to install the font(s) for personal use "
                                                                   "(only available to you), or "
                                                                   "system-wide (available to all users)?"),
                                                              i18n("Where to Install"),
                                                              KGuiItem(KFI_KIO_FONTS_USER.toString()),
                                                              KGuiItem(KFI_KIO_FONTS_SYS.toString()))) {
                case KMessageBox::PrimaryAction:
                    system = false;
                    break;
                case KMessageBox::SecondaryAction:
                    system = true;
                    break;
                default:
                case KMessageBox::Cancel:
                    return;
                }
                break;
            case CGroupListItem::PERSONAL:
                system = false;
                break;
            case CGroupListItem::SYSTEM:
                system = true;
                break;
            default:
                return;
            }
        }

        QSet<QUrl> copy;
        QSet<QUrl>::ConstIterator it, end(src.end());

        //
        // Check if font has any associated AFM or PFM file...
        m_statusLabel->setText(i18n("Looking for any associated files…"));

        if (!m_progress) {
            m_progress = new QProgressDialog(widget());
            m_progress->setWindowTitle(i18n("Scanning Files…"));
            m_progress->setLabelText(i18n("Looking for additional files to install…"));
            m_progress->setModal(true);
            m_progress->setAutoReset(true);
            m_progress->setAutoClose(true);
        }

        m_progress->setCancelButton(nullptr);
        m_progress->setMinimumDuration(500);
        m_progress->setRange(0, src.size());
        m_progress->setValue(0);

        int steps = src.count() < 200 ? 1 : src.count() / 10;
        for (it = src.begin(); it != end; ++it) {
            QList<QUrl> associatedUrls;

            m_progress->setLabelText(i18n("Looking for files associated with %1", (*it).url()));
            m_progress->setValue(m_progress->value() + 1);
            if (1 == steps || 0 == (m_progress->value() % steps)) {
                bool dialogVisible(m_progress->isVisible());
                QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
                if (dialogVisible && !m_progress->isVisible()) { // User closed dialog! re-open!!!
                    m_progress->show();
                }
            }

            CJobRunner::getAssociatedUrls(*it, associatedUrls, false);
            copy.insert(*it);

            QList<QUrl>::Iterator aIt(associatedUrls.begin()), aEnd(associatedUrls.end());

            for (; aIt != aEnd; ++aIt) {
                copy.insert(*aIt);
            }
        }
        m_progress->close();

        CJobRunner::ItemList installUrls;

        end = copy.end();
        for (it = copy.begin(); it != end; ++it) {
            installUrls.append(*it);
        }

        m_statusLabel->setText(i18n("Installing font(s)…"));
        doCmd(CJobRunner::CMD_INSTALL, installUrls, system);
    }
}

void CKCmFontInst::removeDeletedFontsFromGroups()
{
    if (!m_deletedFonts.isEmpty()) {
        QSet<QString>::Iterator it(m_deletedFonts.begin()), end(m_deletedFonts.end());

        for (; it != end; ++it) {
            if (!m_fontList->hasFamily(*it)) {
                m_groupList->removeFamily(*it);
            }
        }

        m_deletedFonts.clear();
    }
}

void CKCmFontInst::selectGroup(CGroupListItem::EType grp)
{
    QModelIndex current(m_groupListView->currentIndex());

    if (current.isValid()) {
        CGroupListItem *grpItem = static_cast<CGroupListItem *>(current.internalPointer());

        if (grpItem && grp == grpItem->type()) {
            return;
        } else {
            m_groupListView->selectionModel()->select(current, QItemSelectionModel::Deselect);
        }
    }

    QModelIndex idx(m_groupList->index(grp));

    m_groupListView->selectionModel()->select(idx, QItemSelectionModel::Select);
    m_groupListView->setCurrentIndex(idx);
    groupSelected(idx);
    m_fontListView->refreshFilter();
    setStatusBar();
}

void CKCmFontInst::toggleGroup(bool enable)
{
    QModelIndex idx(m_groupListView->currentIndex());

    if (idx.isValid()) {
        CGroupListItem *grp = static_cast<CGroupListItem *>(idx.internalPointer());

        if (grp) {
            toggleFonts(enable, grp->name());
        }
    }
}

void CKCmFontInst::toggleFonts(bool enable, const QString &grp)
{
    CJobRunner::ItemList urls;
    QStringList fonts;

    m_fontListView->getFonts(urls, fonts, nullptr, grp.isEmpty(), !enable, enable);

    if (urls.isEmpty()) {
        KMessageBox::information(widget(),
                                 enable ? i18n("You did not select anything to enable.") : i18n("You did not select anything to disable."),
                                 enable ? i18n("Nothing to Enable") : i18n("Nothing to Disable"));
    } else {
        toggleFonts(urls, fonts, enable, grp);
    }
}

void CKCmFontInst::toggleFonts(CJobRunner::ItemList &urls, const QStringList &fonts, bool enable, const QString &grp)
{
    bool doIt = false;

    switch (fonts.count()) {
    case 0:
        break;
    case 1: {
        const QString text = grp.isEmpty() ? enable ? i18n("<p>Do you really want to enable</p><p>\'<b>%1</b>\'?</p>", fonts.first())
                                                    : i18n("<p>Do you really want to disable</p><p>\'<b>%1</b>\'?</p>", fonts.first())
            : enable ? i18n("<p>Do you really want to enable</p><p>\'<b>%1</b>\', contained within group \'<b>%2</b>\'?</p>", fonts.first(), grp)
                     : i18n("<p>Do you really want to disable</p><p>\'<b>%1</b>\', contained within group \'<b>%2</b>\'?</p>", fonts.first(), grp);
        const QString title = enable ? i18n("Enable Font") : i18n("Disable Font");
        const KGuiItem buttonContinue = enable ? KGuiItem(i18n("Enable"), u"font-enable"_s, i18n("Enable Font")) //
                                               : KGuiItem(i18n("Disable"), u"font-disable"_s, i18n("Disable Font"));
        doIt = KMessageBox::Continue == KMessageBox::warningContinueCancel(widget(), text, title, buttonContinue);
        break;
    }
    default: {
        const QString text = grp.isEmpty() ? enable
                ? i18np("Do you really want to enable this font?", "Do you really want to enable these %1 fonts?", urls.count())
                : i18np("Do you really want to disable this font?", "Do you really want to disable these %1 fonts?", urls.count())
            : enable // no group, differentiate based on enable
            ? i18np("<p>Do you really want to enable this font contained within group \'<b>%2</b>\'?</p>",
                    "<p>Do you really want to enable these %1 fonts contained within group \'<b>%2</b>\'?</p>",
                    urls.count(),
                    grp)
            : i18np("<p>Do you really want to disable this font contained within group \'<b>%2</b>\'?</p>",
                    "<p>Do you really want to disable these %1 fonts contained within group \'<b>%2</b>\'?</p>",
                    urls.count(),
                    grp);
        const QString title = enable ? i18n("Enable Fonts") : i18n("Disable Fonts");
        const KGuiItem buttonContinue =
            enable ? KGuiItem(i18n("Enable"), u"font-enable"_s, i18n("Enable Fonts")) : KGuiItem(i18n("Disable"), u"font-disable"_s, i18n("Disable Fonts"));
        doIt = KMessageBox::Continue == KMessageBox::warningContinueCancelList(widget(), text, fonts, title, buttonContinue);
    }
    }

    if (doIt) {
        if (enable) {
            m_statusLabel->setText(i18n("Enabling font(s)…"));
        } else {
            m_statusLabel->setText(i18n("Disabling font(s)…"));
        }

        doCmd(enable ? CJobRunner::CMD_ENABLE : CJobRunner::CMD_DISABLE, urls);
    }
}

void CKCmFontInst::selectMainGroup()
{
    selectGroup(/*Misc::root()
                    ? */
                CGroupListItem::ALL /* : CGroupListItem::PERSONAL*/);
}

void CKCmFontInst::doCmd(CJobRunner::ECommand cmd, const CJobRunner::ItemList &urls, bool system)
{
    m_fontList->setSlowUpdates(true);
    CJobRunner runner(widget());

    connect(&runner, &CJobRunner::configuring, m_fontList, &CFontList::unsetSlowUpdates);
    runner.exec(cmd, urls, system);
    m_fontList->setSlowUpdates(false);
    refreshFontList();
    if (CJobRunner::CMD_DELETE == cmd) {
        m_fontListView->clearSelection();
    }
    CFcEngine::setDirty();
    setStatusBar();
    delete m_tempDir;
    m_tempDir = nullptr;
    m_fontListView->repaint();
    removeDeletedFontsFromGroups();
}

}

#include "KCmFontInst.moc"

#include "moc_KCmFontInst.cpp"
