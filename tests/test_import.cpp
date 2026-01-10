#include <QtTest/QtTest>
#include "MainWindow.h"
#include <QTemporaryDir>
#include <QFile>
#include <QTextStream>

class TestImport : public QObject
{
    Q_OBJECT
private slots:
    void test_basic_import_and_merge()
    {
        QStandardPaths::setTestModeEnabled(true);
        MainWindow w; // nicht anzeigen
        // CSV erzeugen
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QString path = dir.filePath("players.csv");
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream ts(&f);
        ts << "Name;Gruppe;Level\n";
        ts << "Alpha;TruppA;5\n"; // neu
        ts << "Bravo;TruppB;3\n"; // neu
        ts << "Alpha;TruppA;7\n"; // merge (Level kann sich ändern)
        f.close();
        int imported = 0, merged = 0, skipped = 0;
        QVERIFY(w.importCsvFile(path, &imported, &merged, &skipped));
        QCOMPARE(imported, 2); // Alpha + Bravo (Alpha initial neu, zweiter Alpha -> merged gezählt)
        QCOMPARE(merged, 1);   // zweiter Alpha
        QVERIFY(skipped >= 0);
        // Prüfe dass Alpha nur einmal existiert
        // Prüfe Alpha nur einmal über groupingSnapshot (indirekt)
        int alphaCount = 0;
        for (const QString &g : w.groupingSnapshot().keys())
        {
            for (const QString &n : w.groupingSnapshot().value(g))
                if (n == "Alpha")
                    ++alphaCount;
        }
        QCOMPARE(alphaCount, 1);
    }
};
QTEST_MAIN(TestImport)
#include "test_import.moc"
