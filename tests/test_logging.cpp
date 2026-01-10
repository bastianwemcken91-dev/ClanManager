#include <QtTest/QtTest>
#include "MainWindow.h"

class TestLogging : public QObject
{
    Q_OBJECT
private slots:
    void test_error_log_append_and_read()
    {
        QStandardPaths::setTestModeEnabled(true);
        MainWindow w;
        QString before = w.readErrorLogContents();
        w.testAppendErrorLog("UnitTest", "Testnachricht");
        QString after = w.readErrorLogContents();
        QVERIFY(after.contains("Testnachricht"));
        QVERIFY(after.length() >= before.length());
    }
};
QTEST_MAIN(TestLogging)
#include "test_logging.moc"
