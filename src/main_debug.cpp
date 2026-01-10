#include <QApplication>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include "MainWindow.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    
    // Ensure data directory exists BEFORE creating MainWindow
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dataDir.isEmpty()) {
        dataDir = QDir::homePath() + "/.clanmanager";
    }
    
    qDebug() << "Data directory:" << dataDir;
    
    if (!QDir().mkpath(dataDir)) {
        QMessageBox::critical(nullptr, "Startup Error", 
            QString("Failed to create data directory:\n%1").arg(dataDir));
        return 1;
    }
    
    try {
        MainWindow w;
        w.show();
        return app.exec();
    } catch (const std::exception& e) {
        QMessageBox::critical(nullptr, "Startup Error", 
            QString("Failed to start ClanManager:\n%1\n\nData directory: %2")
                .arg(e.what()).arg(dataDir));
        return 1;
    } catch (...) {
        QMessageBox::critical(nullptr, "Startup Error", 
            QString("Unknown error occurred during startup.\n\nData directory: %1").arg(dataDir));
        return 1;
    }
}
