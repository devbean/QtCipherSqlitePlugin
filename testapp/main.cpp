#include <QtSql>
#include <QCoreApplication>
#include <QTemporaryDir>

#ifndef QT_DEBUG
#error Must be built in debug mode!
#endif

#ifdef Q_OS_IOS
#  include <QtPlugin>

Q_IMPORT_PLUGIN(SqliteCipherDriverPlugin)
#endif

#define CONNECTION_FAILED -1

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    Q_UNUSED(app)

    qDebug() << QSqlDatabase::drivers();

    Q_ASSERT(QSqlDatabase::isDriverAvailable("QSQLITE")); // from Qt
    Q_ASSERT(QSqlDatabase::isDriverAvailable("SQLITECIPHER")); // from our plugin

    QTemporaryDir tmp;
    Q_ASSERT(tmp.isValid());

    auto withDB = [&](const char *driver, auto fn) {
        QString path = QDir(tmp.path()).absoluteFilePath(QString(driver) + ".db");
        {
            QSqlDatabase db = QSqlDatabase::addDatabase(driver, "db");
            db.setDatabaseName(path);
            Q_ASSERT(db.open());
            fn(db);
        }
        QSqlDatabase::removeDatabase("db");
    };

//    // QSQLITE
//    {
//        // Create a SQLite db
//        withDB("QSQLITE", [](auto db) {
//            db.exec("create table foo (bar integer)");
//            db.exec("insert into foo values (42)");
//        });

//        // Check that we can read from the SQLite db
//        withDB("QSQLITE", [](auto db) {
//            QSqlQuery q = db.exec("select bar from foo");
//            Q_ASSERT(q.next());
//            Q_ASSERT(q.value(0).toInt() == 42);
//        });

//        // Check that SQLite is not SQLCipher
//        withDB("QSQLITE", [](auto db) {
//            QSqlQuery q = db.exec("select sqlcipher_export()");
//            QString errmsg = q.lastError().databaseText();
//            Q_ASSERT(errmsg.startsWith("no such function"));
//        });
//    }

//    // SQLITECIPHER
//    {
//        // Check that SQLiteCipher is not SQLite
//        withDB("SQLITECIPHER", [](auto db) {
//            QSqlQuery q = db.exec("select sqlcipher_export()");
//            QString errmsg = q.lastError().databaseText();
//            qDebug() << errmsg;
//            Q_ASSERT(errmsg.startsWith("wrong number of arguments"));
//        });

//        // Create a SQLiteCipher db with a passphrase
//        withDB("SQLITECIPHER", [](auto db) {
//            db.exec("pragma key='foobar'");
//            db.exec("create table foo (bar integer)");
//            db.exec("insert into foo values (42)");
//        });

//        // Check that we can't read from the SQLiteCipher db without the passphrase
//        withDB("SQLITECIPHER", [](auto db) {
//            QSqlQuery q = db.exec("select bar from foo");
//            Q_ASSERT(!q.next());
//        });

//        // Check that we can read from the SQLiteCipher db with the passphrase
//        withDB("SQLITECIPHER", [](auto db) {
//            db.exec("pragma key='foobar'");
//            QSqlQuery q = db.exec("select bar from foo");
//            Q_ASSERT(q.next());
//            Q_ASSERT(q.value(0).toInt() == 42);
//        });
//    }

    QString dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
//    QString DB_FILE_PATH = dir + "/test_chacha20.db";
    QString DB_FILE_PATH = dir + "/local.db";
    qDebug() << "DB File Path is:" << DB_FILE_PATH;

    QSqlDatabase dbconn = QSqlDatabase::addDatabase("SQLITECIPHER");
    dbconn.setDatabaseName(DB_FILE_PATH); //localDB.db is already existing before running the application
    dbconn.setPassword("pass");
    dbconn.setConnectOptions("QSQLITE_USE_CIPHER=sqlcipher; SQLCIPHER_LEGACY=1; SQLCIPHER_LEGACY_PAGE_SIZE=4096; QSQLITE_CREATE_KEY");

    bool open = dbconn.open();
    qDebug() << "open: " << open;
    qDebug() << "isOpen(): " << dbconn.isOpen() << dbconn.isOpenError();
    qDebug() << "create_key: " << dbconn.lastError();

    if (!dbconn.isOpen())
    {
        qDebug() << "Connection failed: " << dbconn.lastError().driverText();
        exit(CONNECTION_FAILED);
    }

    QSqlQuery query;
//    query.exec("create table test (id int, name varchar)");
//    query.exec("insert into test values (1, 'AAA')");
//    query.exec("insert into test values (2, 'BBB')");
//    query.exec("insert into test values (3, 'CCC')");
//    query.exec("insert into test values (4, 'DDD')");
//    query.exec("insert into test values (5, 'EEE')");
//    query.exec("insert into test values (6, 'FFF')");
//    query.exec("insert into test values (7, 'GGG')");
//    query.exec("select * from test where name regexp '(a|A)$'");
//    if (query.next()) {
//        qDebug() << "Regexp result: " << query.value(0).toInt() << ": " << query.value(1).toString();
//    } else {
//        qDebug() << "This plugin does not support regexp.";
//    }
//    qDebug() << "----------" << endl;
    query.exec("select id, name from test");
    while (query.next()) {
        qDebug() << query.value(0).toInt() << ": " << query.value(1).toString();
    }
//    qDebug() << "----------" << endl;
//    query.exec("update mapping set name='ZZZ' where id=1");
//    query.exec("select id, name from mapping");
//    while (query.next()) {
//        qDebug() << query.value(0).toInt() << ": " << query.value(1).toString();
//    }
//    qDebug() << "----------" << endl;
//    query.exec("delete from mapping where id=4");
//    query.exec("select id, name from mapping");
//    while (query.next()) {
//        qDebug() << query.value(0).toInt() << ": " << query.value(1).toString();
//    }
//    query.exec("drop table mapping");
    dbconn.close();

    return 0;
}
