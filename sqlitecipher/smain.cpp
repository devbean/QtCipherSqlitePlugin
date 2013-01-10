#include <qsqldriverplugin.h>
#include <qstringlist.h>                                                                                                                           
#include "qsql_sqlite.h"

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
        QSQLiteDriver* driver = new QSQLiteDriver();
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
#endif

#if (QT_VERSION < 0x050000)
Q_EXPORT_STATIC_PLUGIN(SqliteCipherDriverPlugin)
Q_EXPORT_PLUGIN2(qsqlite, SqliteCipherDriverPlugin)
#endif

QT_END_NAMESPACE

#include "smain.moc"
