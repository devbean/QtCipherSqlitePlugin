#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

#ifdef Q_OS_IOS
#  include <QtPlugin>

Q_IMPORT_PLUGIN(SqliteCipherDriverPlugin)
#endif

class TestSqliteCipher: public QObject
{
    Q_OBJECT
private slots:
    void initTestCase() // will run once before the first test
    {
        // Check that the driver exists
        QVERIFY2(QSqlDatabase::isDriverAvailable("SQLITECIPHER"), "SQLITECIPHER driver not found.");
        // Set the database file
        QString dbname = QDir(tmpDir.path()).absoluteFilePath("test.db");
        QSqlDatabase db = QSqlDatabase::addDatabase("SQLITECIPHER", "db");
        db.setDatabaseName(dbname);
    }
    void cleanup()
    {
        QSqlDatabase db = QSqlDatabase::database("db", false);
        db.close();
    }
    void checkCompileOptions();
    void createDbWithPassphrase();
    void refuseToReadWithoutPassphrase();
    void allowToReadWithPassphrase();

    void cleanupTestCase()
    {
        QSqlDatabase::removeDatabase("db");
    }
private:
    QTemporaryDir tmpDir;
};

void TestSqliteCipher::checkCompileOptions()
{
    QSqlQuery q(QSqlDatabase::database("db"));
    QVERIFY2(q.exec("PRAGMA compile_options"), q.lastError().text().toLatin1().constData());
    bool hasCodec = false;
    while(q.next())
    {
        if(q.value(0).toString() == QString("HAS_CODEC"))
        {
            hasCodec = true;
            break;
        }
    }
    QVERIFY2(hasCodec, "'HAS_CODEC' should be in SQLITECIPHER's compile_options.");
}

void TestSqliteCipher::createDbWithPassphrase()
{
    QSqlQuery q(QSqlDatabase::database("db"));
    QStringList queries;
    queries << "PRAGMA key='foobar'"
            << "create table foo(bar integer)"
            << "insert into foo values (42)";
    for(const QString& qs : queries)
    {
        QVERIFY2(q.exec(qs), q.lastError().text().toLatin1().constData());
    }
}

void TestSqliteCipher::refuseToReadWithoutPassphrase()
{
    QSqlQuery q(QSqlDatabase::database("db"));
    QVERIFY(!q.exec("select bar from foo"));
}

void TestSqliteCipher::allowToReadWithPassphrase()
{
    QSqlQuery q(QSqlDatabase::database("db"));
    QStringList queries;
    queries << "PRAGMA key='foobar'"
            << "select bar from foo";
    for(const QString& qs : queries)
    {
        QVERIFY2(q.exec(qs), q.lastError().text().toLatin1().constData());
    }
    QVERIFY(q.next());
    QVERIFY(q.value(0).toInt() == 42);
}

QTEST_GUILESS_MAIN(TestSqliteCipher)
#include "main.moc"
