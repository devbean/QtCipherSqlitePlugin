#include <QtSql>
#include <QCoreApplication>

#define CONNECTION_FAILED -1

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    Q_UNUSED(app);

    qDebug() << QSqlDatabase::drivers();

    QSqlDatabase dbconn = QSqlDatabase::addDatabase("SQLITECIPHER");
    dbconn.setDatabaseName("test_c.db");
    dbconn.setPassword("test");
    if (!dbconn.open()) {
        qDebug() << "Can not open connection: " << dbconn.lastError().driverText();
        exit(CONNECTION_FAILED);
    }

    QSqlQuery query;
    query.exec("create table mapping (id int, name varchar)");
    query.exec("insert into mapping values (1, 'AAA')");
    query.exec("insert into mapping values (2, 'BBB')");
    query.exec("insert into mapping values (3, 'CCC')");
    query.exec("insert into mapping values (4, 'DDD')");
    query.exec("insert into mapping values (5, 'EEE')");
    query.exec("insert into mapping values (6, 'FFF')");
    query.exec("insert into mapping values (7, 'GGG')");
    query.exec("select id, name from mapping");
    while (query.next()) {
        qDebug() << query.value(0).toInt() << ": " << query.value(1).toString();
    }
    qDebug() << "----------" << endl;
    query.exec("update mapping set name='ZZZ' where id=1");
    query.exec("select id, name from mapping");
    while (query.next()) {
        qDebug() << query.value(0).toInt() << ": " << query.value(1).toString();
    }
    qDebug() << "----------" << endl;
    query.exec("delete from mapping where id=4");
    query.exec("select id, name from mapping");
    while (query.next()) {
        qDebug() << query.value(0).toInt() << ": " << query.value(1).toString();
    }
    query.exec("drop table mapping");
    dbconn.close();

    return 0;
}
