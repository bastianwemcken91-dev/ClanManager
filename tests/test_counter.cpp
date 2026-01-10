#include <QtTest/QtTest>
#include "MainWindow.h"

class TestCounter : public QObject
{
    Q_OBJECT
private slots:
    void test_no_response_threshold_flag()
    {
        QStandardPaths::setTestModeEnabled(true);
        MainWindow w;
        // Erzeuge Spieler knapp unter Threshold
        int threshold = w.noResponseThresholdValue();
        Player p1;
        p1.name = "Spieler1";
        p1.noResponseCounter = threshold - 1;
        w.testAddPlayer(p1);
        Player p2;
        p2.name = "Spieler2";
        p2.noResponseCounter = threshold - 1;
        w.testAddPlayer(p2);
        w.testRebuildModel();
        QVERIFY(!w.isPlayerFlaggedNoResponse("Spieler1"));
        // Simuliere Nicht-Auswahl (beide nicht ausgewählt -> Counter++ )
        w.simulateNoResponseIncrement(QStringList());
        // Jetzt sollte Flag erscheinen
        QVERIFY(w.isPlayerFlaggedNoResponse("Spieler1"));
    }
};
QTEST_MAIN(TestCounter)
#include "test_counter.moc"
