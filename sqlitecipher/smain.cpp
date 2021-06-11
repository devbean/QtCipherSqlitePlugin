/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtSql module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qsqldriverplugin.h>
#include <qstringlist.h>                                                                                                                           
#include "sqlitecipher_p.h"

QT_BEGIN_NAMESPACE

/*
 * Change the driver name if you like.
 */
static const char DriverName[] = "SQLITECIPHER";

class SqliteCipherDriverPlugin : public QSqlDriverPlugin
{
#if (QT_VERSION >= 0x050000)
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QSqlDriverFactoryInterface" FILE "SqliteCipherDriverPlugin.json")
#endif
public:
    SqliteCipherDriverPlugin();
 
    QSqlDriver* create(const QString &);
#if (QT_VERSION < 0x050000)
    QStringList keys() const;
#endif
};

SqliteCipherDriverPlugin::SqliteCipherDriverPlugin()
    : QSqlDriverPlugin()
{
}

QSqlDriver* SqliteCipherDriverPlugin::create(const QString &name)
{
    if (name == QLatin1String(DriverName)) {
        SQLiteCipherDriver* driver = new SQLiteCipherDriver();
        return driver;
    }
    return 0;
}

#if (QT_VERSION < 0x050000)
QStringList SqliteCipherDriverPlugin::keys() const
{
    QStringList l;
    l  << QLatin1String(DriverName);
    return l;
}

Q_EXPORT_STATIC_PLUGIN(SqliteCipherDriverPlugin)
Q_EXPORT_PLUGIN2(qsqlite, SqliteCipherDriverPlugin)
#endif

QT_END_NAMESPACE

#include "smain.moc"
