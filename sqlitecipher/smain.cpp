#include <qsqldriverplugin.h>
#include <qstringlist.h>                                                                                                                           
#include "qsql_sqlite.h"
 
QT_BEGIN_NAMESPACE
 
class SqliteCipherDriverPlugin : public QSqlDriverPlugin
{
public:
    SqliteCipherDriverPlugin();
 
    QSqlDriver* create(const QString &);
    QStringList keys() const;
};
 
SqliteCipherDriverPlugin::SqliteCipherDriverPlugin()
    : QSqlDriverPlugin()
{
}
 
QSqlDriver* SqliteCipherDriverPlugin::create(const QString &name)
{
    if (name == QLatin1String("SQLITECIPHER")) {
        QSQLiteDriver* driver = new QSQLiteDriver();
        return driver;
    }
    return 0;
}
 
QStringList SqliteCipherDriverPlugin::keys() const
{
    QStringList l;
    l  << QLatin1String("SQLITECIPHER");
    return l;
}
 
Q_EXPORT_STATIC_PLUGIN(SqliteCipherDriverPlugin)
Q_EXPORT_PLUGIN2(qsqlite, SqliteCipherDriverPlugin)
 
QT_END_NAMESPACE
