# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS Test)

remove_definitions(-DQT_NO_CAST_FROM_ASCII)

ecm_add_test(kxftconfigtest.cpp TEST_NAME kxftconfigtest
    LINK_LIBRARIES Qt::Test kcm_fonts_static)
