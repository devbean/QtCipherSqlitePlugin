#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

#ifdef Q_OS_IOS
#  include <QtPlugin>

Q_IMPORT_PLUGIN(SqliteCipherDriverPlugin)
#endif

class TestSqliteCipherPlugin: public QObject
{
    Q_OBJECT
private slots:
    void initTestCase() // will run once before the first test
    {
        // Check that the driver exists
        QVERIFY2(QSqlDatabase::isDriverAvailable("SQLITECIPHER"), "SQLITECIPHER driver not found.");
        // Set the database file
        QString dbname = QDir(tmpDir.path()).absoluteFilePath("test_plugin.db");
//        QString dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
//        QString dbname = dir + "/local.db";
//        qDebug() << dbname;
        QSqlDatabase db = QSqlDatabase::addDatabase("SQLITECIPHER", "db");
        db.setDatabaseName(dbname);
    }
    void cleanup()
    {
        QSqlDatabase db = QSqlDatabase::database("db", false);
        db.setConnectOptions();
        db.setPassword(QString());
        db.close();
    }
    void createDbWithPassphrase();
    void updatePassphrase();
    void removePassphrase();
    void refuseToReadWithoutPassphrase();
    void refuseToReadWithIncorrectPassphrase();
    void allowToReadWithPassphrase();

    void cleanupTestCase()
    {
        QSqlDatabase::removeDatabase("db");
    }
private:
    QTemporaryDir tmpDir;
};

void TestSqliteCipherPlugin::createDbWithPassphrase()
{
    QSqlDatabase db = QSqlDatabase::database("db", false);
    db.setPassword("foobar");
    db.setConnectOptions("QSQLITE_CREATE_KEY");

    QVERIFY2(db.open(), db.lastError().text().toLatin1().constData());
    QSqlQuery q(db);
    QStringList queries;
    queries << "create table IF NOT EXISTS foo(bar integer)"
            << "insert into foo values (42)";
    for(const QString& qs : qAsConst(queries))
    {
        QVERIFY2(q.exec(qs), q.lastError().text().toLatin1().constData());
    }
}

void TestSqliteCipherPlugin::updatePassphrase()
{
    QSqlDatabase db = QSqlDatabase::database("db", false);
    db.setPassword("You shall not pass!");
    db.setConnectOptions("QSQLITE_UPDATE_KEY=foobar");

    QVERIFY2(!db.open(), db.lastError().driverText().toLatin1().constData());

    db.setPassword("foobar");
    db.setConnectOptions("QSQLITE_UPDATE_KEY=foobar");
    QVERIFY(db.open());
}

void TestSqliteCipherPlugin::removePassphrase()
{
    QSqlDatabase db = QSqlDatabase::database("db", false);
    db.setPassword("You shall not pass!");
    db.setConnectOptions("QSQLITE_REMOVE_KEY");
    QVERIFY2(!db.open(), db.lastError().driverText().toLatin1().constData());

    db.setPassword("foobar");
    db.setConnectOptions("QSQLITE_REMOVE_KEY");
    QVERIFY(db.open());

    // Recovery password
    db.setPassword("foobar");
    db.setConnectOptions("QSQLITE_CREATE_KEY");
    QVERIFY(db.open());
}

void TestSqliteCipherPlugin::refuseToReadWithoutPassphrase()
{
    QSqlDatabase db = QSqlDatabase::database("db", false);
    QVERIFY(db.open());
    QSqlQuery q(db);
    QVERIFY2(!q.exec("select bar from foo"), q.lastError().text().toLatin1().constData());
}

void TestSqliteCipherPlugin::refuseToReadWithIncorrectPassphrase()
{
    QSqlDatabase db = QSqlDatabase::database("db", false);
    db.setPassword("You shall not pass!");
    QVERIFY2(!db.open(), db.lastError().driverText().toLatin1().constData());
}

void TestSqliteCipherPlugin::allowToReadWithPassphrase()
{
    QSqlDatabase db = QSqlDatabase::database("db", false);
    db.setPassword("foobar");
    QVERIFY(db.open());
    QSqlQuery q(db);
    QVERIFY2(q.exec("select bar from foo"), q.lastError().text().toLatin1().constData());
    QVERIFY(q.next());
    QVERIFY(q.value(0).toInt() == 42);
}

QTEST_GUILESS_MAIN(TestSqliteCipherPlugin)
#include "main.moc"
