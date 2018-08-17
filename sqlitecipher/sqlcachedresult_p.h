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

#ifndef SQLCACHEDRESULT_H
#define SQLCACHEDRESULT_H

#include <QSqlResult>

#include "sqlitecipher_global.h"

#if (QT_VERSION < 0x050000)
QT_BEGIN_HEADER
#endif

QT_BEGIN_NAMESPACE

class QVariant;
template <typename T> class QVector;

class SqlCachedResultPrivate;

class SqlCachedResult : public QSqlResult
{
public:
    virtual ~SqlCachedResult();

    typedef QVector<QVariant> ValueCache;

protected:
    SqlCachedResult(const QSqlDriver * db);

    void init(int colCount);
    void cleanup();
    void clearValues();

    virtual bool gotoNext(ValueCache &values, int index) = 0;

    QVariant data(int i) DECL_OVERRIDE;
    bool isNull(int i) DECL_OVERRIDE;
    bool fetch(int i) DECL_OVERRIDE;
    bool fetchNext() DECL_OVERRIDE;
    bool fetchPrevious() DECL_OVERRIDE;
    bool fetchFirst() DECL_OVERRIDE;
    bool fetchLast() DECL_OVERRIDE;

    int colCount() const;
    ValueCache &cache();

    void virtual_hook(int id, void *data) DECL_OVERRIDE;
    void detachFromResultSet() DECL_OVERRIDE;
    void setNumericalPrecisionPolicy(QSql::NumericalPrecisionPolicy policy) DECL_OVERRIDE;
private:
    bool cacheNext();
    SqlCachedResultPrivate *d;
};

QT_END_NAMESPACE

#if (QT_VERSION < 0x050000)
QT_END_HEADER
#endif

#endif // SQLCACHEDRESULT_H