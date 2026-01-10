#include <QtTest/QtTest>
#include "MainWindow.h"

class TestGrouping : public QObject
{
    Q_OBJECT
private slots:
    void test_grouping_snapshot()
    {
        QStandardPaths::setTestModeEnabled(true);
        MainWindow w;
        Player a;
        a.name = "A";
        a.group = "Alpha";
        w.testAddPlayer(a);
        Player b;
        b.name = "B";
        b.group = "Alpha";
        w.testAddPlayer(b);
        Player c;
        c.name = "C";
        c.group = "Beta";
        w.testAddPlayer(c);
        Player d;
        d.name = "D";
        d.group = "";
        w.testAddPlayer(d); // Ohne Gruppe
        QMap<QString, QStringList> snap = w.groupingSnapshot();
        QVERIFY(snap.contains("Alpha"));
        QVERIFY(snap.contains("Beta"));
        QVERIFY(snap.contains("Ohne Gruppe"));
        QCOMPARE(snap.value("Alpha").size(), 2);
        QCOMPARE(snap.value("Beta").size(), 1);
        QCOMPARE(snap.value("Ohne Gruppe").size(), 1);
    }
};
QTEST_MAIN(TestGrouping)
#include "test_grouping.moc"
