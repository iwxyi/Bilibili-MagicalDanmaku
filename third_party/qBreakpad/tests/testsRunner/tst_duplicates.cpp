#include <QtTest>

#include <QProcess>
#include <QFile>

namespace
{
void runTest(const QString &testName)
{
    QProcess testProcess;
    testProcess.start(TEST_RUNNER, QStringList{testName});
    testProcess.waitForFinished();

    const QString &testResult = testProcess.readAllStandardError();
    QVERIFY(!testResult.isEmpty());
    QVERIFY(!testResult.contains("Failed"));
    QVERIFY(testResult.contains("Succeed"));
    const QString dumpPath = testResult.mid(testResult.indexOf("dump path:") + 10).trimmed();
    QVERIFY(QFile::exists(dumpPath) || QFile::exists(dumpPath + ".dmp"));
}
}

class test : public QObject
{
    Q_OBJECT

public:
    test() = default;
    ~test() = default;

private slots:
    void test_duplicates1();
    void test_duplicates2();
};

void test::test_duplicates1()
{
    runTest("1");
}

void test::test_duplicates2()
{
    runTest("2");
}

QTEST_APPLESS_MAIN(test)

#include "tst_duplicates.moc"
