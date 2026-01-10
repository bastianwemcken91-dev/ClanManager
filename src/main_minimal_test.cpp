// Minimal startup test for ClanManager
// This version skips loading files and complex widgets to identify the crash

#include <QApplication>
#include <QMainWindow>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    
    // Create data directory
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dataDir.isEmpty()) {
        dataDir = QDir::homePath() + "/.clanmanager";
    }
    QDir().mkpath(dataDir);
    
    // Create minimal window to test if Qt works
    QMainWindow *window = new QMainWindow();
    QWidget *central = new QWidget(window);
    window->setCentralWidget(central);
    
    QVBoxLayout *layout = new QVBoxLayout(central);
    QLabel *label = new QLabel("ClanManager Minimal Test\n\nWenn Sie dieses Fenster sehen, funktioniert Qt6 grundsätzlich.\n\nDatenverzeichnis: " + dataDir, central);
    label->setWordWrap(true);
    layout->addWidget(label);
    
    window->setWindowTitle("ClanManager - Minimal Test");
    window->resize(500, 300);
    window->show();
    
    QMessageBox::information(window, "Test", "Qt6 funktioniert! Das Problem liegt im MainWindow-Konstruktor.");
    
    return app.exec();
}
