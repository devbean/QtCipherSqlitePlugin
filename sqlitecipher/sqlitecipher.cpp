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

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QSqlError>
#include <QSqlField>
#include <QSqlIndex>
#include <QSqlQuery>
#include <QtSql/private/qsqlcachedresult_p.h>
#include <QtSql/private/qsqldriver_p.h>

#include "sqlitecipher_p.h"
#ifdef REGULAR_EXPRESSION_ENABLED
  #include <qcache.h>
  #include <qregularexpression.h>
#endif
#ifdef TIMEZONE_ENABLED
  #include <QTimeZone>
#endif

#include <QScopedValueRollback>
#include <QVariant>

#if defined Q_OS_WIN
# include <qt_windows.h>
#else
# include <unistd.h>
#endif

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
  #define VALUE_TYPE_BOOL      QMetaType::Bool
  #define VALUE_TYPE_INT       QMetaType::Int
  #define VALUE_TYPE_DOUBLE    QMetaType::Double
  #define VALUE_TYPE_STRING    QMetaType::QString
  #define VALUE_TYPE_BYTEARRAY QMetaType::QByteArray
  #define VALUE_TYPE_UINT      QMetaType::UInt
  #define VALUE_TYPE_LONGLONG  QMetaType::LongLong
  #define VALUE_TYPE_DATETIME  QMetaType::QDateTime
  #define VALUE_TYPE_TIME      QMetaType::QTime
  #define VALUE_TYPE_INVALID   QMetaType::UnknownType
#else
  #define VALUE_TYPE_BOOL      QVariant::Bool
  #define VALUE_TYPE_INT       QVariant::Int
  #define VALUE_TYPE_DOUBLE    QVariant::Double
  #define VALUE_TYPE_STRING    QVariant::String
  #define VALUE_TYPE_BYTEARRAY QVariant::ByteArray
  #define VALUE_TYPE_UINT      QVariant::UInt
  #define VALUE_TYPE_LONGLONG  QVariant::LongLong
  #define VALUE_TYPE_DATETIME  QVariant::DateTime
  #define VALUE_TYPE_TIME      QVariant::Time
  #define VALUE_TYPE_INVALID   QVariant::Invalid
#endif

extern "C" {
# include "sqlite3mc_amalgamation.h"
}

Q_DECLARE_METATYPE(sqlite3*)
Q_DECLARE_METATYPE(sqlite3_stmt*)
#if (QT_VERSION >= 0x050000)
Q_DECLARE_OPAQUE_POINTER(sqlite3*)
Q_DECLARE_OPAQUE_POINTER(sqlite3_stmt*)
#endif

#define CHECK_SQLITE_KEY \
    do { \
        sqlite3_key(d->access, password.toUtf8().constData(), password.size()); \
        int result = sqlite3_exec(d->access, QStringLiteral("SELECT count(*) FROM sqlite_master LIMIT 1").toUtf8().constData(), nullptr, nullptr, nullptr); \
        if (result != SQLITE_OK) { \
            if (d->access) { sqlite3_close(d->access); d->access = nullptr; } \
            setLastError(qMakeError(d->access, tr("Invalid password."), QSqlError::ConnectionError, result)); setOpenError(true); return false; \
        } \
    } while (0)

QT_BEGIN_NAMESPACE

static QString _q_escapeIdentifier(const QString &identifier)
{
    QString res = identifier;
    if (!identifier.isEmpty() && !identifier.startsWith(QLatin1Char('"')) && !identifier.endsWith(QLatin1Char('"'))) {
        res.replace(QLatin1Char('"'), QLatin1String("\"\""));
        res.prepend(QLatin1Char('"')).append(QLatin1Char('"'));
        res.replace(QLatin1Char('.'), QLatin1String("\".\""));
    }
    return res;
}

static
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
QMetaType::Type
#else
QVariant::Type
#endif
qGetColumnType(const QString &tpName)
{
    const QString typeName = tpName.toLower();

    if (typeName == QLatin1String("integer")
        || typeName == QLatin1String("int"))
        return VALUE_TYPE_INT;
    if (typeName == QLatin1String("double")
        || typeName == QLatin1String("float")
        || typeName == QLatin1String("real")
        || typeName.startsWith(QLatin1String("numeric")))
        return VALUE_TYPE_DOUBLE;
    if (typeName == QLatin1String("blob"))
        return VALUE_TYPE_BYTEARRAY;
    if (typeName == QLatin1String("boolean")
        || typeName == QLatin1String("bool"))
        return VALUE_TYPE_BOOL;
    return VALUE_TYPE_STRING;
}

static QSqlError qMakeError(sqlite3 *access, const QString &descr, QSqlError::ErrorType type,
                            int errorCode)
{
    return QSqlError(descr,
                     QString(reinterpret_cast<const QChar *>(sqlite3_errmsg16(access))),
                     type, QString::number(errorCode));
}

class SQLiteResultPrivate;

class SQLiteResult : public QSqlCachedResult
{
    Q_DECLARE_PRIVATE(SQLiteResult)
    friend class SQLiteCipherDriver;
public:
    explicit SQLiteResult(const SQLiteCipherDriver* db);
    ~SQLiteResult();
    QVariant handle() const DECL_OVERRIDE;

protected:
    bool gotoNext(QSqlCachedResult::ValueCache& row, int idx) DECL_OVERRIDE;
    bool reset(const QString &query) DECL_OVERRIDE;
    bool prepare(const QString &query) DECL_OVERRIDE;
    bool execBatch(bool arrayBind) DECL_OVERRIDE;
    bool exec() DECL_OVERRIDE;
    int size() DECL_OVERRIDE;
    int numRowsAffected() DECL_OVERRIDE;
    QVariant lastInsertId() const DECL_OVERRIDE;
    QSqlRecord record() const DECL_OVERRIDE;
#if (QT_VERSION >= 0x050000)
    void detachFromResultSet() DECL_OVERRIDE;
#endif
    void virtual_hook(int id, void *data) DECL_OVERRIDE;
};

class SQLiteCipherDriverPrivate : public QSqlDriverPrivate
{
    Q_DECLARE_PUBLIC(SQLiteCipherDriver)
public:
    inline SQLiteCipherDriverPrivate() : QSqlDriverPrivate(), access(nullptr) {}
    sqlite3 *access;
    QList <SQLiteResult *> results;
    QStringList notificationid;
};


class SQLiteResultPrivate : public QSqlCachedResultPrivate
{
    Q_DECLARE_PUBLIC(SQLiteResult)
public:
    Q_DECLARE_SQLDRIVER_PRIVATE(SQLiteCipherDriver)
    SQLiteResultPrivate(SQLiteResult *q, const SQLiteCipherDriver *drv);
    void cleanup();
    bool fetchNext(QSqlCachedResult::ValueCache &values, int idx, bool initialFetch);
    // initializes the recordInfo and the cache
    void initColumns(bool emptyResultset);
    void finalize();

    sqlite3_stmt *stmt;

    bool skippedStatus; // the status of the fetchNext() that's skipped
    bool skipRow; // skip the next fetchNext()?
    QSqlRecord rInf;
    QVector<QVariant> firstRow;
};

SQLiteResultPrivate::SQLiteResultPrivate(SQLiteResult *q, const SQLiteCipherDriver *drv)
    : QSqlCachedResultPrivate(q, drv),
      stmt(nullptr),
      skippedStatus(false),
      skipRow(false)
{
}

void SQLiteResultPrivate::cleanup()
{
    Q_Q(SQLiteResult);
    finalize();
    rInf.clear();
    skippedStatus = false;
    skipRow = false;
    q->setAt(QSql::BeforeFirstRow);
    q->setActive(false);
    q->cleanup();
}

void SQLiteResultPrivate::finalize()
{
    if (!stmt)
        return;

    sqlite3_finalize(stmt);
    stmt = nullptr;
}

void SQLiteResultPrivate::initColumns(bool emptyResultset)
{
    Q_Q(SQLiteResult);
    int nCols = sqlite3_column_count(stmt);
    if (nCols <= 0)
        return;

    q->init(nCols);

    for (int i = 0; i < nCols; ++i) {
        QString colName = QString(reinterpret_cast<const QChar *>(
                    sqlite3_column_name16(stmt, i))
                    ).remove(QLatin1Char('"'));
        const QString tableName = QString(reinterpret_cast<const QChar *>(
                            sqlite3_column_table_name16(stmt, i))
                            ).remove(QLatin1Char('"'));

        // must use typeName for resolving the type to match SQLiteCipherDriver::record
        QString typeName = QString(reinterpret_cast<const QChar *>(
                    sqlite3_column_decltype16(stmt, i)));
        // sqlite3_column_type is documented to have undefined behavior if the result set is empty
        int stp = emptyResultset ? -1 : sqlite3_column_type(stmt, i);

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
        int fieldType;
#else
        QVariant::Type fieldType;
#endif

        if (!typeName.isEmpty()) {
            fieldType = qGetColumnType(typeName);
        } else {
            // Get the proper type for the field based on stp value
            switch (stp) {
            case SQLITE_INTEGER:
                fieldType = VALUE_TYPE_INT;
                break;
            case SQLITE_FLOAT:
                fieldType = VALUE_TYPE_DOUBLE;
                break;
            case SQLITE_BLOB:
                fieldType = VALUE_TYPE_BYTEARRAY;
                break;
            case SQLITE_TEXT:
                fieldType = VALUE_TYPE_STRING;
                break;
            case SQLITE_NULL:
            default:
                fieldType = VALUE_TYPE_INVALID;
                break;
            }
        }

#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
        QSqlField fld(colName, fieldType);
#elif QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        QSqlField fld(colName, fieldType, tableName);
#else // QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        QSqlField fld(colName, QMetaType(fieldType), tableName);
#endif
        fld.setSqlType(stp);
        rInf.append(fld);
    }
}

bool SQLiteResultPrivate::fetchNext(QSqlCachedResult::ValueCache &values, int idx, bool initialFetch)
{
    Q_Q(SQLiteResult);
    int res;
    int i;

    if (skipRow) {
        // already fetched
        Q_ASSERT(!initialFetch);
        skipRow = false;
        for(int i = 0; i < firstRow.count(); i++)
            values[i] = firstRow[i];
        return skippedStatus;
    }
    skipRow = initialFetch;

    if(initialFetch) {
        firstRow.clear();
        firstRow.resize(sqlite3_column_count(stmt));
    }

    if (!stmt) {
        q->setLastError(QSqlError(QCoreApplication::translate("SQLiteResult", "Unable to fetch row"),
                                  QCoreApplication::translate("SQLiteResult", "No query"), QSqlError::ConnectionError));
        q->setAt(QSql::AfterLastRow);
        return false;
    }
    res = sqlite3_step(stmt);

    switch(res) {
    case SQLITE_ROW:
        // check to see if should fill out columns
        if (rInf.isEmpty())
            // must be first call.
            initColumns(false);
        if (idx < 0 && !initialFetch)
            return true;
        for (i = 0; i < rInf.count(); ++i) {
            switch (sqlite3_column_type(stmt, i)) {
            case SQLITE_BLOB:
                values[i + idx] = QByteArray(static_cast<const char *>(
                            sqlite3_column_blob(stmt, i)),
                            sqlite3_column_bytes(stmt, i));
                break;
            case SQLITE_INTEGER:
                values[i + idx] = sqlite3_column_int64(stmt, i);
                break;
            case SQLITE_FLOAT:
                switch(q->numericalPrecisionPolicy()) {
                    case QSql::LowPrecisionInt32:
                        values[i + idx] = sqlite3_column_int(stmt, i);
                        break;
                    case QSql::LowPrecisionInt64:
                        values[i + idx] = sqlite3_column_int64(stmt, i);
                        break;
                    case QSql::LowPrecisionDouble:
                    case QSql::HighPrecision:
                    default:
                        values[i + idx] = sqlite3_column_double(stmt, i);
                        break;
                };
                break;
            case SQLITE_NULL:
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
                values[i + idx] = QVariant(QMetaType::fromType<QString>());
#else
                values[i + idx] = QVariant(VALUE_TYPE_STRING);
#endif
                break;
            default:
                values[i + idx] = QString(reinterpret_cast<const QChar *>(
                            sqlite3_column_text16(stmt, i)),
                            sqlite3_column_bytes16(stmt, i) / sizeof(QChar));
                break;
            }
        }
        return true;
    case SQLITE_DONE:
        if (rInf.isEmpty())
            // must be first call.
            initColumns(true);
        q->setAt(QSql::AfterLastRow);
        sqlite3_reset(stmt);
        return false;
    case SQLITE_CONSTRAINT:
    case SQLITE_ERROR:
        // SQLITE_ERROR is a generic error code and we must call sqlite3_reset()
        // to get the specific error message.
        res = sqlite3_reset(stmt);
        q->setLastError(qMakeError(drv_d_func()->access, QCoreApplication::translate("SQLiteResult",
                        "Unable to fetch row"), QSqlError::ConnectionError, res));
        q->setAt(QSql::AfterLastRow);
        return false;
    case SQLITE_MISUSE:
    case SQLITE_BUSY:
    default:
        // something wrong, don't get col info, but still return false
        q->setLastError(qMakeError(drv_d_func()->access, QCoreApplication::translate("SQLiteResult",
                        "Unable to fetch row"), QSqlError::ConnectionError, res));
        sqlite3_reset(stmt);
        q->setAt(QSql::AfterLastRow);
        return false;
    }
    return false;
}

SQLiteResult::SQLiteResult(const SQLiteCipherDriver* db)
    : QSqlCachedResult(*new SQLiteResultPrivate(this, db))
{
    Q_D(SQLiteResult);
    const_cast<SQLiteCipherDriverPrivate*>(d->drv_d_func())->results.append(this);
}

SQLiteResult::~SQLiteResult()
{
    Q_D(SQLiteResult);
    if (d->drv_d_func())
        const_cast<SQLiteCipherDriverPrivate*>(d->drv_d_func())->results.removeOne(this);
    d->cleanup();
}

void SQLiteResult::virtual_hook(int id, void *data)
{
#if (QT_VERSION >= 0x050000)
    QSqlCachedResult::virtual_hook(id, data);
#else
    switch (id) {
    case QSqlResult::DetachFromResultSet:
        if (d->stmt)
            sqlite3_reset(d->stmt);
        break;
    default:
        SqlCachedResult::virtual_hook(id, data);
    }
#endif
}

bool SQLiteResult::reset(const QString &query)
{
    if (!prepare(query))
        return false;
    return exec();
}

bool SQLiteResult::prepare(const QString &query)
{
    Q_D(SQLiteResult);
    if (!driver() || !driver()->isOpen() || driver()->isOpenError())
        return false;

    d->cleanup();

    setSelect(false);

    const void *pzTail = nullptr;

#if (SQLITE_VERSION_NUMBER >= 3003011)
    int res = sqlite3_prepare16_v2(d->drv_d_func()->access, query.constData(), (query.size() + 1) * sizeof(QChar),
                                   &d->stmt, &pzTail);
#else
    int res = sqlite3_prepare16(d->access, query.constData(), (query.size() + 1) * sizeof(QChar),
                                &d->stmt, &pzTail);
#endif

    if (res != SQLITE_OK) {
        setLastError(qMakeError(d->drv_d_func()->access, QCoreApplication::translate("SQLiteResult",
                     "Unable to execute statement"), QSqlError::StatementError, res));
        d->finalize();
        return false;
    } else if (pzTail && !QString(reinterpret_cast<const QChar *>(pzTail)).trimmed().isEmpty()) {
        setLastError(qMakeError(d->drv_d_func()->access, QCoreApplication::translate("SQLiteResult",
            "Unable to execute multiple statements at a time"), QSqlError::StatementError, SQLITE_MISUSE));
        d->finalize();
        return false;
    }
    return true;
}

static QString secondsToOffset(int seconds)
{
    const QChar sign = ushort(seconds < 0 ? '-' : '+');
    seconds = qAbs(seconds);
    const int hours = seconds / 3600;
    const int minutes = (seconds % 3600) / 60;

    return QString(QStringLiteral("%1%2:%3")).arg(sign).arg(hours, 2, 10, QLatin1Char('0')).arg(minutes, 2, 10, QLatin1Char('0'));
}

static QString timespecToString(const QDateTime &dateTime)
{
    switch (dateTime.timeSpec()) {
    case Qt::LocalTime:
        return QString();
    case Qt::UTC:
        return QStringLiteral("Z");
    case Qt::OffsetFromUTC:
        return secondsToOffset(dateTime.offsetFromUtc());
#ifdef TIMEZONE_ENABLED
    case Qt::TimeZone:
        return secondsToOffset(dateTime.timeZone().offsetFromUtc(dateTime));
#endif
    default:
        return QString();
    }
}

bool SQLiteResult::execBatch(bool arrayBind)
{
    Q_UNUSED(arrayBind)
    Q_D(QSqlResult);
    QScopedValueRollback<decltype(d->values)> valuesScope(d->values);
    QVector<QVariant> values = d->values;
    if (values.count() == 0)
        return false;

    for (int i = 0; i < values.at(0).toList().count(); ++i) {
        d->values.clear();
        QScopedValueRollback<decltype(d->indexes)> indexesScope(d->indexes);
        decltype(d->indexes)::const_iterator it = d->indexes.constBegin();
        while (it != d->indexes.constEnd()) {
            bindValue(it.key(), values.at(it.value().first()).toList().at(i), QSql::In);
            ++it;
        }
        if (!exec())
            return false;
    }
    return true;
}

bool SQLiteResult::exec()
{
    Q_D(SQLiteResult);
    QVector<QVariant> values = boundValues();

    d->skippedStatus = false;
    d->skipRow = false;
    d->rInf.clear();
    clearValues();
    setLastError(QSqlError());

    int res = sqlite3_reset(d->stmt);
    if (res != SQLITE_OK) {
        setLastError(qMakeError(d->drv_d_func()->access, QCoreApplication::translate("SQLiteResult",
                     "Unable to reset statement"), QSqlError::StatementError, res));
        d->finalize();
        return false;
    }
    int paramCount = sqlite3_bind_parameter_count(d->stmt);
    bool paramCountIsValid = paramCount == values.count();
#if (SQLITE_VERSION_NUMBER >= 3003011)
    // In the case of the reuse of a named placeholder
    // We need to check explicitly that paramCount is greater than or equal to 1, as sqlite
    // can end up in a case where for virtual tables it returns 0 even though it
    // has parameters
    if (paramCount >= 1 && paramCount < values.count()) {
        const auto countIndexes = [](int counter, const auto indexList) {
                                      return counter + indexList.length();
                                  };

        const int bindParamCount = std::accumulate(d->indexes.cbegin(),
                                                   d->indexes.cend(),
                                                   0,
                                                   countIndexes);

        paramCountIsValid = bindParamCount == values.count();
        // When using named placeholders, it will reuse the index for duplicated
        // placeholders. So we need to ensure the QVector has only one instance of
        // each value as SQLite will do the rest for us.
        QVector<QVariant> prunedValues;
        QVector<int> handledIndexes;
        for (int i = 0, currentIndex = 0; i < values.size(); ++i) {
            if (handledIndexes.contains(i))
                continue;
            const char *parameterName = sqlite3_bind_parameter_name(d->stmt, currentIndex + 1);
            if (!parameterName) {
                paramCountIsValid = false;
                continue;
            }
            const auto placeHolder = QString::fromUtf8(parameterName);
            const auto &indexes = d->indexes.value(placeHolder);
#if QT_VERSION <= QT_VERSION_CHECK(5, 11, 0)
            handledIndexes << QVector<int>::fromList(indexes);
#else
            handledIndexes << indexes;
#endif
            prunedValues << values.at(indexes.first());
            ++currentIndex;
        }
        values = prunedValues;
    }
#endif

    if (paramCountIsValid) {
        for (int i = 0; i < paramCount; ++i) {
            res = SQLITE_OK;
            const QVariant value = values.at(i);

            if (value.isNull()) {
                res = sqlite3_bind_null(d->stmt, i + 1);
            } else {
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
                switch (value.userType()) {
#else
                switch (value.type()) {
#endif
                case VALUE_TYPE_BYTEARRAY: {
                    const QByteArray *ba = static_cast<const QByteArray*>(value.constData());
                    res = sqlite3_bind_blob(d->stmt, i + 1, ba->constData(),
                                            ba->size(), SQLITE_STATIC);
                    break; }
                case VALUE_TYPE_INT:
                case VALUE_TYPE_BOOL:
                    res = sqlite3_bind_int(d->stmt, i + 1, value.toInt());
                    break;
                case VALUE_TYPE_DOUBLE:
                    res = sqlite3_bind_double(d->stmt, i + 1, value.toDouble());
                    break;
                case VALUE_TYPE_UINT:
                case VALUE_TYPE_LONGLONG:
                    res = sqlite3_bind_int64(d->stmt, i + 1, value.toLongLong());
                    break;
                case VALUE_TYPE_DATETIME: {
                    const QDateTime dateTime = value.toDateTime();
                    const QString str = dateTime.toString(QLatin1String("yyyy-MM-ddThh:mm:ss.zzz") + timespecToString(dateTime));
                    res = sqlite3_bind_text16(d->stmt, i + 1, str.utf16(),
                                              str.size() * sizeof(ushort), SQLITE_TRANSIENT);
                    break;
                }
                case VALUE_TYPE_TIME: {
                    const QTime time = value.toTime();
                    const QString str = time.toString(QStringLiteral("hh:mm:ss.zzz"));
                    res = sqlite3_bind_text16(d->stmt, i + 1, str.utf16(),
                                              str.size() * sizeof(ushort), SQLITE_TRANSIENT);
                    break;
                }
                case VALUE_TYPE_STRING: {
                    // lifetime of string == lifetime of its qvariant
                    const QString *str = static_cast<const QString*>(value.constData());
                    res = sqlite3_bind_text16(d->stmt, i + 1, str->utf16(),
                                              (str->size()) * sizeof(QChar), SQLITE_STATIC);
                    break; }
                default: {
                    QString str = value.toString();
                    // SQLITE_TRANSIENT makes sure that sqlite buffers the data
                    res = sqlite3_bind_text16(d->stmt, i + 1, str.utf16(),
                                              (str.size()) * sizeof(QChar), SQLITE_TRANSIENT);
                    break; }
                }
            }
            if (res != SQLITE_OK) {
                setLastError(qMakeError(d->drv_d_func()->access, QCoreApplication::translate("SQLiteResult",
                             "Unable to bind parameters"), QSqlError::StatementError, res));
                d->finalize();
                return false;
            }
        }
    } else {
        setLastError(QSqlError(QCoreApplication::translate("SQLiteResult",
                        "Parameter count mismatch"), QString(), QSqlError::StatementError));
        return false;
    }
    d->skippedStatus = d->fetchNext(d->firstRow, 0, true);
    if (lastError().isValid()) {
        setSelect(false);
        setActive(false);
        return false;
    }
    setSelect(!d->rInf.isEmpty());
    setActive(true);
    return true;
}

bool SQLiteResult::gotoNext(QSqlCachedResult::ValueCache& row, int idx)
{
    Q_D(SQLiteResult);
    return d->fetchNext(row, idx, false);
}

int SQLiteResult::size()
{
    return -1;
}

int SQLiteResult::numRowsAffected()
{
    Q_D(const SQLiteResult);
    return sqlite3_changes(d->drv_d_func()->access);
}

QVariant SQLiteResult::lastInsertId() const
{
    Q_D(const SQLiteResult);
    if (isActive()) {
        qint64 id = sqlite3_last_insert_rowid(d->drv_d_func()->access);
        if (id)
            return id;
    }
    return QVariant();
}

QSqlRecord SQLiteResult::record() const
{
    Q_D(const SQLiteResult);
    if (!isActive() || !isSelect())
        return QSqlRecord();
    return d->rInf;
}

#if (QT_VERSION >= 0x050000)
void SQLiteResult::detachFromResultSet()
{
    Q_D(SQLiteResult);
    if (d->stmt)
        sqlite3_reset(d->stmt);
}
#endif

QVariant SQLiteResult::handle() const
{
    Q_D(const SQLiteResult);
    return QVariant::fromValue(d->stmt);
}

/////////////////////////////////////////////////////////

#ifdef REGULAR_EXPRESSION_ENABLED
static void _q_regexp(sqlite3_context* context, int argc, sqlite3_value** argv)
{
    if (Q_UNLIKELY(argc != 2)) {
        sqlite3_result_int(context, 0);
        return;
    }

    const QString pattern = QString::fromUtf8(
        reinterpret_cast<const char*>(sqlite3_value_text(argv[0])));
    const QString subject = QString::fromUtf8(
        reinterpret_cast<const char*>(sqlite3_value_text(argv[1])));

    auto cache = static_cast<QCache<QString, QRegularExpression>*>(sqlite3_user_data(context));
    auto regexp = cache->object(pattern);
    const bool wasCached = regexp;

    if (!wasCached) {
#if QT_VERSION < QT_VERSION_CHECK(5, 12, 0)
        regexp = new QRegularExpression(pattern, QRegularExpression::DontCaptureOption | QRegularExpression::OptimizeOnFirstUsageOption);
#else
        regexp = new QRegularExpression(pattern, QRegularExpression::DontCaptureOption);
#endif
    }

    const bool found = subject.contains(*regexp);

    if (!wasCached)
        cache->insert(pattern, regexp);

    sqlite3_result_int(context, int(found));
}

static void _q_regexp_cleanup(void *cache)
{
    delete static_cast<QCache<QString, QRegularExpression>*>(cache);
}
#endif

SQLiteCipherDriver::SQLiteCipherDriver(QObject * parent)
    : QSqlDriver(*new SQLiteCipherDriverPrivate, parent)
{
}

SQLiteCipherDriver::SQLiteCipherDriver(sqlite3 *connection, QObject *parent)
    : QSqlDriver(*new SQLiteCipherDriverPrivate, parent)
{
    Q_D(SQLiteCipherDriver);
    d->access = connection;
    setOpen(true);
    setOpenError(false);
}


SQLiteCipherDriver::~SQLiteCipherDriver()
{
    close();
}

bool SQLiteCipherDriver::hasFeature(DriverFeature f) const
{
    switch (f) {
    case BLOB:
    case Transactions:
    case Unicode:
    case LastInsertId:
    case PreparedQueries:
    case PositionalPlaceholders:
    case SimpleLocking:
    case FinishQuery:
    case LowPrecisionNumbers:
    case EventNotifications:
        return true;
    case QuerySize:
    case BatchOperations:
    case MultipleResultSets:
    case CancelQuery:
        return false;
    case NamedPlaceholders:
#if (SQLITE_VERSION_NUMBER < 3003011)
        return false;
#else
        return true;
#endif
    }
    return false;
}

enum QtSqliteCipher {
    UNKNOWN_CIPHER = 0,
    AES_128_CBC,
    AES_256_CBC,
    CHACHA20,
    SQLCIPHER,
    RC4
};

static int _cipherNameToValue(const QString &name) {
    const QString lowerName = name.toLower();
    if (lowerName == QStringLiteral("aes128cbc")) {
        return AES_128_CBC;
    } else if (lowerName == QStringLiteral("aes256cbc")) {
        return AES_256_CBC;
    } else if (lowerName == QStringLiteral("chacha20")) {
        return CHACHA20;
    } else if (lowerName == QStringLiteral("sqlcipher")) {
        return SQLCIPHER;
    } else if (lowerName == QStringLiteral("rc4")) {
        return RC4;
    } else {
        return UNKNOWN_CIPHER;
    }
}

static QString _cipherValueToName(const QtSqliteCipher &cipher) {
    switch (cipher) {
    case AES_128_CBC:
        return QStringLiteral("AES_128_CBC");
    case AES_256_CBC:
        return QStringLiteral("AES_256_CBC");
    case CHACHA20:
        return QStringLiteral("CHACHA20");
    case SQLCIPHER:
        return QStringLiteral("SQLCIPHER");
    case RC4:
        return QStringLiteral("RC4");
    default:
        return QStringLiteral("UNKNOWN");
    }
}

/*
   SQLite dbs have no user name, hosts or ports.
   just file names and password we need.
*/
bool SQLiteCipherDriver::open(const QString & db, const QString &, const QString &password, const QString &, int, const QString &conOpts)
{
    Q_D(SQLiteCipherDriver);
    if (isOpen())
        close();

    enum KEY_OP {
        OPEN_WITH_KEY = 0,
        CREATE_KEY,
        UPDATE_KEY,
        REMOVE_KEY
    };

    int timeOut = 5000;
    int keyOp = OPEN_WITH_KEY;
    bool sharedCache = false;
    bool openReadOnlyOption = false;
    bool openUriOption = false;
    QString newPassword = QString();
    int cipher = -1;
    // AES128CBC
    bool aes128cbcLegacy = false;
    int aes128cbcLegacyPageSize = 0;
    // AES256CBC
    bool aes256cbcLegacy = false;
    int aes256cbcKdfIter = 4001;
    int aes256cbcLegacyPageSize = 0;
    // CHACHA20
    bool chacha20Legacy = false;
    int chacha20KdfIter = 64007;
    int chacha20LegacyPageSize = 4096;
    // SQLCIPHER
    int sqlcipherLegacy = 0;
    int sqlcipherKdfIter = 256000;
    int sqlcipherFastKdfIter = 2;
    bool sqlcipherHmacUse = true;
    int sqlcipherHmacPgno = 1;
    int sqlcipherHmacSaltMask = 0x3a;
    int sqlcipherLegacyPageSize = 4096;
    int sqlcipherKdfAlgorithm = 2;
    int sqlcipherHmacAlgorithm = 2;
    int sqlcipherPlainTextHeaderSize = 0;
    // RC4
    int rc4LegacyPageSize = 0;

#ifdef REGULAR_EXPRESSION_ENABLED
    static const QLatin1String regexpConnectOption = QLatin1String("QSQLITE_ENABLE_REGEXP");
    bool defineRegexp = false;
    int regexpCacheSize = 25;
#endif

    const auto opts = QString(conOpts).remove(QLatin1Char(' ')).split(QLatin1Char(';'));
#if (QT_VERSION >= 0x050700)
    for (auto option : opts) {
#else
    foreach (auto option : opts) {
#endif
        option = option.trimmed();
        if (option.startsWith(QLatin1String("QSQLITE_BUSY_TIMEOUT"))) {
            option = option.mid(20).trimmed();
            if (option.startsWith(QLatin1Char('='))) {
                bool ok;
                const int v = option.mid(1).trimmed().toInt(&ok);
                if (ok) {
                    timeOut = v;
                }
            }
        }
        if (option.startsWith(QLatin1String("QSQLITE_UPDATE_KEY"))) {
            option = option.mid(18).trimmed();
            if (option.startsWith(QLatin1Char('='))) {
                newPassword = option.mid(1);
            }
            keyOp = UPDATE_KEY;
        }
        if (option.startsWith(QLatin1String("QSQLITE_USE_CIPHER"))) {
            option = option.mid(18).trimmed();
            cipher = _cipherNameToValue(option.mid(1));
        }
        if (option.startsWith(QLatin1String("AES128CBC_LEGACY"))) {
            option = option.mid(16).trimmed();
            if (option.startsWith(QLatin1Char('='))) {
                bool ok;
                const int v = option.mid(1).trimmed().toInt(&ok);
                if (ok) {
                    aes128cbcLegacy = v;
                }
            }
        }
        if (option.startsWith(QLatin1String("AES128CBC_LEGACY_PAGE_SIZE"))) {
            option = option.mid(26).trimmed();
            if (option.startsWith(QLatin1Char('='))) {
                bool ok;
                const int v = option.mid(1).trimmed().toInt(&ok);
                if (ok) {
                    aes128cbcLegacyPageSize = v;
                    if (aes128cbcLegacyPageSize < 0) {
                        aes128cbcLegacyPageSize = 0;
                    } else if (aes128cbcLegacyPageSize > 65536) {
                        aes128cbcLegacyPageSize = 65536;
                    }
                }
            }
        }
        if (option.startsWith(QLatin1String("AES256CBC_LEGACY"))) {
            option = option.mid(16).trimmed();
            if (option.startsWith(QLatin1Char('='))) {
                bool ok;
                const int v = option.mid(1).trimmed().toInt(&ok);
                if (ok) {
                    aes256cbcLegacy = v;
                }
            }
        }
        if (option.startsWith(QLatin1String("AES256CBC_LEGACY_PAGE_SIZE"))) {
            option = option.mid(26).trimmed();
            if (option.startsWith(QLatin1Char('='))) {
                bool ok;
                const int v = option.mid(1).trimmed().toInt(&ok);
                if (ok) {
                    aes256cbcLegacyPageSize = v;
                    if (aes256cbcLegacyPageSize < 0) {
                        aes256cbcLegacyPageSize = 0;
                    } else if (aes256cbcLegacyPageSize > 65536) {
                        aes256cbcLegacyPageSize = 65536;
                    }
                }
            }
        }
        if (option.startsWith(QLatin1String("AES256CBC_KDF_ITER"))) {
            option = option.mid(18).trimmed();
            if (option.startsWith(QLatin1Char('='))) {
                bool ok;
                const int v = option.mid(1).trimmed().toInt(&ok);
                if (ok) {
                    aes256cbcKdfIter = v;
                    if (aes256cbcKdfIter < 1) {
                        aes256cbcKdfIter = 1;
                    }
                }
            }
        }
        if (option.startsWith(QLatin1String("CHACHA20_LEGACY"))) {
            option = option.mid(15).trimmed();
            if (option.startsWith(QLatin1Char('='))) {
                bool ok;
                const int v = option.mid(1).trimmed().toInt(&ok);
                if (ok) {
                    chacha20Legacy = v;
                }
            }
        }
        if (option.startsWith(QLatin1String("CHACHA20_LEGACY_PAGE_SIZE"))) {
            option = option.mid(25).trimmed();
            if (option.startsWith(QLatin1Char('='))) {
                bool ok;
                const int v = option.mid(1).trimmed().toInt(&ok);
                if (ok) {
                    chacha20LegacyPageSize = v;
                    if (chacha20LegacyPageSize < 0) {
                        chacha20LegacyPageSize = 0;
                    } else if (chacha20LegacyPageSize > 65536) {
                        chacha20LegacyPageSize = 65536;
                    }
                }
            }
        }
        if (option.startsWith(QLatin1String("CHACHA20_KDF_ITER"))) {
            option = option.mid(17).trimmed();
            if (option.startsWith(QLatin1Char('='))) {
                bool ok;
                const int v = option.mid(1).trimmed().toInt(&ok);
                if (ok) {
                    chacha20KdfIter = v;
                    if (chacha20KdfIter < 1) {
                        chacha20KdfIter = 1;
                    }
                }
            }
        }
        if (option.startsWith(QLatin1String("SQLCIPHER_LEGACY"))) {
            option = option.mid(16).trimmed();
            if (option.startsWith(QLatin1Char('='))) {
                bool ok;
                const int v = option.mid(1).trimmed().toInt(&ok);
                if (ok) {
                    sqlcipherLegacy = v;
                    if (sqlcipherLegacy < 0) {
                        sqlcipherLegacy = 0;
                    } else if (sqlcipherLegacy > 4) {
                        sqlcipherLegacy = 4;
                    }
                }
            }
        }
        if (option.startsWith(QLatin1String("SQLCIPHER_LEGACY_PAGE_SIZE"))) {
            option = option.mid(26).trimmed();
            if (option.startsWith(QLatin1Char('='))) {
                bool ok;
                const int v = option.mid(1).trimmed().toInt(&ok);
                if (ok) {
                    sqlcipherLegacyPageSize = v;
                    if (sqlcipherLegacyPageSize < 0) {
                        sqlcipherLegacyPageSize = 0;
                    } else if (sqlcipherLegacyPageSize > 65536) {
                        sqlcipherLegacyPageSize = 65536;
                    }
                }
            }
        }
        if (option.startsWith(QLatin1String("SQLCIPHER_KDF_ITER"))) {
            option = option.mid(18).trimmed();
            if (option.startsWith(QLatin1Char('='))) {
                bool ok;
                const int v = option.mid(1).trimmed().toInt(&ok);
                if (ok) {
                    sqlcipherKdfIter = v;
                    if (sqlcipherKdfIter < 1) {
                        sqlcipherKdfIter = 1;
                    }
                }
            }
        }
        if (option.startsWith(QLatin1String("SQLCIPHER_FAST_KDF_ITER"))) {
            option = option.mid(23).trimmed();
            if (option.startsWith(QLatin1Char('='))) {
                bool ok;
                const int v = option.mid(1).trimmed().toInt(&ok);
                if (ok) {
                    sqlcipherFastKdfIter = v;
                    if (sqlcipherFastKdfIter < 1) {
                        sqlcipherFastKdfIter = 1;
                    }
                }
            }
        }
        if (option.startsWith(QLatin1String("SQLCIPHER_HMAC_USE"))) {
            option = option.mid(18).trimmed();
            if (option.startsWith(QLatin1Char('='))) {
                bool ok;
                const int v = option.mid(1).trimmed().toInt(&ok);
                if (ok) {
                    sqlcipherHmacUse = v;
                }
            }
        }
        if (option.startsWith(QLatin1String("SQLCIPHER_HMAC_PGNO"))) {
            option = option.mid(19).trimmed();
            if (option.startsWith(QLatin1Char('='))) {
                bool ok;
                const int v = option.mid(1).trimmed().toInt(&ok);
                if (ok) {
                    sqlcipherHmacPgno = v;
                    if (sqlcipherHmacPgno < 0) {
                        sqlcipherHmacPgno = 0;
                    }
                    if (sqlcipherHmacPgno > 2) {
                        sqlcipherHmacPgno = 2;
                    }
                }
            }
        }
        if (option.startsWith(QLatin1String("SQLCIPHER_HMAC_SALT_MASK"))) {
            option = option.mid(24).trimmed();
            if (option.startsWith(QLatin1Char('='))) {
                bool ok;
                const int v = option.mid(1).trimmed().toInt(&ok);
                if (ok) {
                    sqlcipherHmacSaltMask = v;
                    if (sqlcipherHmacSaltMask < 0) {
                        sqlcipherHmacSaltMask = 0;
                    }
                    if (sqlcipherHmacSaltMask > 255) {
                        sqlcipherHmacSaltMask = 255;
                    }
                }
            }
        }
        if (option.startsWith(QLatin1String("SQLCIPHER_KDF_ALGORITHM"))) {
            option = option.mid(23).trimmed();
            if (option.startsWith(QLatin1Char('='))) {
                bool ok;
                const int v = option.mid(1).trimmed().toInt(&ok);
                if (ok) {
                    sqlcipherKdfAlgorithm = v;
                    if (sqlcipherKdfAlgorithm < 0) {
                        sqlcipherKdfAlgorithm = 0;
                    }
                    if (sqlcipherKdfAlgorithm > 2) {
                        sqlcipherKdfAlgorithm = 2;
                    }
                }
            }
        }
        if (option.startsWith(QLatin1String("SQLCIPHER_HMAC_ALGORITHM"))) {
            option = option.mid(24).trimmed();
            if (option.startsWith(QLatin1Char('='))) {
                bool ok;
                const int v = option.mid(1).trimmed().toInt(&ok);
                if (ok) {
                    sqlcipherHmacAlgorithm = v;
                    if (sqlcipherHmacAlgorithm < 0) {
                        sqlcipherHmacAlgorithm = 0;
                    }
                    if (sqlcipherHmacAlgorithm > 2) {
                        sqlcipherHmacAlgorithm = 2;
                    }
                }
            }
        }
        if (option.startsWith(QLatin1String("SQLCIPHER_PLAINTEXT_HEADER_SIZE"))) {
            option = option.mid(31).trimmed();
            if (option.startsWith(QLatin1Char('='))) {
                bool ok;
                const int v = option.mid(1).trimmed().toInt(&ok);
                if (ok) {
                    sqlcipherPlainTextHeaderSize = v;
                    if (sqlcipherPlainTextHeaderSize < 0) {
                        sqlcipherPlainTextHeaderSize = 0;
                    }
                    if (sqlcipherPlainTextHeaderSize > 100) {
                        sqlcipherPlainTextHeaderSize = 100;
                    }
                }
            }
        }
        if (option.startsWith(QLatin1String("RC4_LEGACY_PAGE_SIZE"))) {
            option = option.mid(20).trimmed();
            if (option.startsWith(QLatin1Char('='))) {
                bool ok;
                const int v = option.mid(1).trimmed().toInt(&ok);
                if (ok) {
                    rc4LegacyPageSize = v;
                    if (rc4LegacyPageSize < 0) {
                        rc4LegacyPageSize = 0;
                    } else if (rc4LegacyPageSize > 65536) {
                        rc4LegacyPageSize = 65536;
                    }
                }
            }
        }

        if (option == QLatin1String("QSQLITE_OPEN_READONLY")) {
            openReadOnlyOption = true;
        } else if (option == QLatin1String("QSQLITE_OPEN_URI")) {
            openUriOption = true;
        } else if (option == QLatin1String("QSQLITE_ENABLE_SHARED_CACHE")) {
            sharedCache = true;
        } else if (option == QLatin1String("QSQLITE_CREATE_KEY")) {
            keyOp = CREATE_KEY;
        } else if (option == QLatin1String("QSQLITE_REMOVE_KEY")) {
            keyOp = REMOVE_KEY;
        }
#ifdef REGULAR_EXPRESSION_ENABLED
        else if (option.startsWith(regexpConnectOption)) {
            option = option.mid(regexpConnectOption.size()).trimmed();
            if (option.isEmpty()) {
                defineRegexp = true;
            } else if (option.startsWith(QLatin1Char('='))) {
                bool ok = false;
                const int cacheSize = option.mid(1).trimmed().toInt(&ok);
                if (ok) {
                    defineRegexp = true;
                    if (cacheSize > 0)
                        regexpCacheSize = cacheSize;
                }
            }
        }
#endif
    }

    int openMode = (openReadOnlyOption ? SQLITE_OPEN_READONLY : (SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE));
    openMode |= (sharedCache ? SQLITE_OPEN_SHAREDCACHE : SQLITE_OPEN_PRIVATECACHE);
    if (openUriOption)
        openMode |= SQLITE_OPEN_URI;

    openMode |= SQLITE_OPEN_NOMUTEX;

    const int openResult = sqlite3_open_v2(db.toUtf8().constData(), &d->access, openMode, nullptr);
    if (openResult == SQLITE_OK) {
        sqlite3_busy_timeout(d->access, timeOut);

        setOpen(true);
        setOpenError(false);
#ifdef REGULAR_EXPRESSION_ENABLED
        if (defineRegexp) {
            auto cache = new QCache<QString, QRegularExpression>(regexpCacheSize);
            sqlite3_create_function_v2(d->access, "regexp", 2, SQLITE_UTF8, cache, &_q_regexp, nullptr,
                                       nullptr, &_q_regexp_cleanup);
        }
#endif
        if (cipher > 0) {
            sqlite3mc_config(d->access, "cipher", cipher);
            switch (cipher) {
            case AES_128_CBC:
            {
                sqlite3mc_config_cipher(d->access, "aes128cbc", "legacy", aes128cbcLegacy ? 1 : 0);
                sqlite3mc_config_cipher(d->access, "aes128cbc", "legacy_page_size", aes128cbcLegacyPageSize);
                break;
            }
            case AES_256_CBC:
            {
                sqlite3mc_config_cipher(d->access, "aes256cbc", "legacy", aes256cbcLegacy ? 1 : 0);
                sqlite3mc_config_cipher(d->access, "aes256cbc", "legacy_page_size", aes256cbcLegacyPageSize);
                sqlite3mc_config_cipher(d->access, "aes256cbc", "kdf_iter", aes256cbcKdfIter);
                break;
            }
            case CHACHA20:
            {
                sqlite3mc_config_cipher(d->access, "chacha20", "legacy", chacha20Legacy ? 1 : 0);
                sqlite3mc_config_cipher(d->access, "chacha20", "legacy_page_size", chacha20LegacyPageSize);
                sqlite3mc_config_cipher(d->access, "chacha20", "kdf_iter", chacha20KdfIter);
                break;
            }
            case SQLCIPHER:
            {
                sqlite3mc_config_cipher(d->access, "sqlcipher", "legacy", sqlcipherLegacy);
                sqlite3mc_config_cipher(d->access, "sqlcipher", "legacy_page_size", sqlcipherLegacyPageSize);
                sqlite3mc_config_cipher(d->access, "sqlcipher", "kdf_iter", sqlcipherKdfIter);
                sqlite3mc_config_cipher(d->access, "sqlcipher", "fast_kdf_iter", sqlcipherFastKdfIter);
                sqlite3mc_config_cipher(d->access, "sqlcipher", "hmac_use", sqlcipherHmacUse ? 1 : 0);
                sqlite3mc_config_cipher(d->access, "sqlcipher", "hmac_pgno", sqlcipherHmacPgno);
                sqlite3mc_config_cipher(d->access, "sqlcipher", "hmac_salt_mask", sqlcipherHmacSaltMask);
                sqlite3mc_config_cipher(d->access, "sqlcipher", "kdf_algorithm", sqlcipherKdfAlgorithm);
                sqlite3mc_config_cipher(d->access, "sqlcipher", "hmac_algorithm", sqlcipherHmacAlgorithm);
                sqlite3mc_config_cipher(d->access, "sqlcipher", "plaintext_header_size", sqlcipherPlainTextHeaderSize);
                break;
            }
            case RC4:
            {
                sqlite3mc_config_cipher(d->access, "rc4", "legacy", 1);
                sqlite3mc_config_cipher(d->access, "rc4", "legacy_page_size", rc4LegacyPageSize);
                break;
            }
            default:
                // Do nothing
                break;
            }
        }

        if (!(password.isNull() || password.isEmpty())) {
            switch (keyOp) {
            case OPEN_WITH_KEY:
            {
                CHECK_SQLITE_KEY;
                break;
            }
            case CREATE_KEY:
            {
                int res = sqlite3_rekey(d->access, password.toUtf8().constData(), password.size());
                if (SQLITE_OK != res) {
                    setLastError(qMakeError(d->access, tr("Cannot create password. Maybe it is encrypted?"), QSqlError::ConnectionError, res));
                    setOpenError(true);
                    setOpen(false);
                    sqlite3_close_v2(d->access);
                    return false;
                }
                break;
            }
            case UPDATE_KEY:
            {
                // verify old password
                CHECK_SQLITE_KEY;
                // set new password
                if (newPassword.isEmpty() || newPassword.isNull()) {
                    sqlite3_rekey(d->access, nullptr, 0);
                } else {
                    sqlite3_rekey(d->access, newPassword.toUtf8().constData(), newPassword.size());
                }
                break;
            }
            case REMOVE_KEY:
            {
                // verify old password
                CHECK_SQLITE_KEY;
                // set new password to null
                sqlite3_rekey(d->access, nullptr, 0);
                break;
            }
            }
        }
        return true;
    } else {
        setLastError(qMakeError(d->access, tr("Error opening database"),
                     QSqlError::ConnectionError, openResult));
        setOpenError(true);
        setOpen(false);
        if (d->access) {
            sqlite3_close(d->access);
            d->access = nullptr;
        }
        return false;
    }
}

void SQLiteCipherDriver::close()
{
    Q_D(SQLiteCipherDriver);
    if (isOpen()) {
#if (QT_VERSION >= 0x050700)
        for (auto result : qAsConst(d->results)) {
#else
        foreach (SQLiteResult *result, d->results) {
#endif
            result->d_func()->finalize();
        }

        if (d->access && (d->notificationid.count() > 0)) {
            d->notificationid.clear();
            sqlite3_update_hook(d->access, nullptr, nullptr);
        }

        const int result = sqlite3_close(d->access);
        if (result != SQLITE_OK)
            setLastError(qMakeError(d->access, tr("Error closing database."), QSqlError::ConnectionError, result));
        d->access = nullptr;
        setOpen(false);
        setOpenError(false);
    }
}

QSqlResult *SQLiteCipherDriver::createResult() const
{
    return new SQLiteResult(this);
}

bool SQLiteCipherDriver::beginTransaction()
{
    if (!isOpen() || isOpenError())
        return false;

    QSqlQuery q(createResult());
    if (!q.exec(QLatin1String("BEGIN"))) {
        setLastError(QSqlError(tr("Unable to begin transaction"),
                               q.lastError().databaseText(), QSqlError::TransactionError));
        return false;
    }

    return true;
}

bool SQLiteCipherDriver::commitTransaction()
{
    if (!isOpen() || isOpenError())
        return false;

    QSqlQuery q(createResult());
    if (!q.exec(QLatin1String("COMMIT"))) {
        setLastError(QSqlError(tr("Unable to commit transaction"),
                               q.lastError().databaseText(), QSqlError::TransactionError));
        return false;
    }

    return true;
}

bool SQLiteCipherDriver::rollbackTransaction()
{
    if (!isOpen() || isOpenError())
        return false;

    QSqlQuery q(createResult());
    if (!q.exec(QLatin1String("ROLLBACK"))) {
        setLastError(QSqlError(tr("Unable to rollback transaction"),
                               q.lastError().databaseText(), QSqlError::TransactionError));
        return false;
    }

    return true;
}

QStringList SQLiteCipherDriver::tables(QSql::TableType type) const
{
    QStringList res;
    if (!isOpen())
        return res;

    QSqlQuery q(createResult());
    q.setForwardOnly(true);

    QString sql = QLatin1String("SELECT name FROM sqlite_master WHERE %1 "
                                "UNION ALL SELECT name FROM sqlite_temp_master WHERE %1");
    if ((type & QSql::Tables) && (type & QSql::Views))
        sql = sql.arg(QLatin1String("type='table' OR type='view'"));
    else if (type & QSql::Tables)
        sql = sql.arg(QLatin1String("type='table'"));
    else if (type & QSql::Views)
        sql = sql.arg(QLatin1String("type='view'"));
    else
        sql.clear();

    if (!sql.isEmpty() && q.exec(sql)) {
        while(q.next())
            res.append(q.value(0).toString());
    }

    if (type & QSql::SystemTables) {
        // there are no internal tables beside this one:
        res.append(QLatin1String("sqlite_master"));
    }

    return res;
}

static QSqlIndex qGetTableInfo(QSqlQuery &q, const QString &tableName, bool onlyPIndex = false)
{
    QString schema;
    QString table(tableName);
    int indexOfSeparator = tableName.indexOf(QLatin1Char('.'));
    if (indexOfSeparator > -1) {
        schema = tableName.left(indexOfSeparator).append(QLatin1Char('.'));
        table = tableName.mid(indexOfSeparator + 1);
    }
    q.exec(QLatin1String("PRAGMA ") + schema + QLatin1String("table_info (") + _q_escapeIdentifier(table) + QLatin1Char(')'));

    QSqlIndex ind;
    while (q.next()) {
        bool isPk = q.value(5).toInt();
        if (onlyPIndex && !isPk)
            continue;
        QString typeName = q.value(2).toString().toLower();
		QString defVal = q.value(4).toString();
        if (!defVal.isEmpty() && defVal.at(0) == QLatin1Char('\'')) {
            const int end = defVal.lastIndexOf(QLatin1Char('\''));
            if (end > 0)
                defVal = defVal.mid(1, end - 1);
        }

#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
        QSqlField fld(q.value(1).toString(), qGetColumnType(typeName));
#elif QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        QSqlField fld(q.value(1).toString(), qGetColumnType(typeName), tableName);
#else // QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        QSqlField fld(q.value(1).toString(), QMetaType(qGetColumnType(typeName)), tableName);
#endif
        if (isPk && (typeName == QLatin1String("integer")))
            // INTEGER PRIMARY KEY fields are auto-generated in sqlite
            // INT PRIMARY KEY is not the same as INTEGER PRIMARY KEY!
            fld.setAutoValue(true);
        fld.setRequired(q.value(3).toInt() != 0);
        fld.setDefaultValue(defVal);
        ind.append(fld);
    }
    return ind;
}

QSqlIndex SQLiteCipherDriver::primaryIndex(const QString &tblname) const
{
    if (!isOpen())
        return QSqlIndex();

    QString table = tblname;
    if (isIdentifierEscaped(table, QSqlDriver::TableName))
        table = stripDelimiters(table, QSqlDriver::TableName);

    QSqlQuery q(createResult());
    q.setForwardOnly(true);
    return qGetTableInfo(q, table, true);
}

QSqlRecord SQLiteCipherDriver::record(const QString &tbl) const
{
    if (!isOpen())
        return QSqlRecord();

    QString table = tbl;
    if (isIdentifierEscaped(table, QSqlDriver::TableName))
        table = stripDelimiters(table, QSqlDriver::TableName);

    QSqlQuery q(createResult());
    q.setForwardOnly(true);
    return qGetTableInfo(q, table);
}

QVariant SQLiteCipherDriver::handle() const
{
    Q_D(const SQLiteCipherDriver);
    return QVariant::fromValue(d->access);
}

QString SQLiteCipherDriver::escapeIdentifier(const QString &identifier, IdentifierType type) const
{
    Q_UNUSED(type);
    return _q_escapeIdentifier(identifier);
}

static void handle_sqlite_callback(void *qobj, int aoperation, char const *adbname, char const *atablename,
                                   sqlite3_int64 arowid)
{
    Q_UNUSED(aoperation);
    Q_UNUSED(adbname);
    SQLiteCipherDriver *driver = static_cast<SQLiteCipherDriver *>(qobj);
    if (driver) {
        QMetaObject::invokeMethod(driver, "handleNotification", Qt::QueuedConnection,
                                  Q_ARG(QString, QString::fromUtf8(atablename)), Q_ARG(qint64, arowid));
    }
}

bool SQLiteCipherDriver::subscribeToNotification(const QString &name)
{
    Q_D(SQLiteCipherDriver);
    if (!isOpen()) {
        qWarning("Database not open.");
        return false;
    }

    if (d->notificationid.contains(name)) {
        qWarning("Already subscribing to '%s'.", qPrintable(name));
        return false;
    }

    //sqlite supports only one notification callback, so only the first is registered
    d->notificationid << name;
    if (d->notificationid.count() == 1)
        sqlite3_update_hook(d->access, &handle_sqlite_callback, reinterpret_cast<void *> (this));

    return true;
}

bool SQLiteCipherDriver::unsubscribeFromNotification(const QString &name)
{
    Q_D(SQLiteCipherDriver);
    if (!isOpen()) {
        qWarning("Database not open.");
        return false;
    }

    if (!d->notificationid.contains(name)) {
        qWarning("Not subscribed to '%s'.", qPrintable(name));
        return false;
    }

    d->notificationid.removeAll(name);
    if (d->notificationid.isEmpty())
        sqlite3_update_hook(d->access, nullptr, nullptr);

    return true;
}

QStringList SQLiteCipherDriver::subscribedToNotifications() const
{
    Q_D(const SQLiteCipherDriver);
    return d->notificationid;
}

void SQLiteCipherDriver::handleNotification(const QString &tableName, qint64 rowid)
{
    Q_D(const SQLiteCipherDriver);
    if (d->notificationid.contains(tableName)) {
#if (QT_VERSION < QT_VERSION_CHECK(5, 15, 0))
        emit notification(tableName);
#endif
        emit notification(tableName, QSqlDriver::UnknownSource, QVariant(rowid));
    }
}

QT_END_NAMESPACE
