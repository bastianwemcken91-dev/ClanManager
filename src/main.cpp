#include <QApplication>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <exception>
#include "MainWindow.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    
    // Set organization and application name for proper settings storage
    QCoreApplication::setOrganizationName("ClanManager");
    QCoreApplication::setApplicationName("ClanManager");
    
    // Ensure data directory exists BEFORE creating MainWindow
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dataDir.isEmpty()) {
        dataDir = QDir::homePath() + "/.clanmanager";
    }
    
    qDebug() << "ClanManager starting...";
    qDebug() << "Data directory:" << dataDir;
    
    // Create data directory
    if (!QDir().mkpath(dataDir)) {
        qCritical() << "Failed to create data directory:" << dataDir;
        QMessageBox::critical(nullptr, "Startup Error", 
            QString("Failed to create data directory:\n%1\n\nPlease check permissions.").arg(dataDir));
        return 1;
    }
    
    // Verify data directory is writable
    QFileInfo dirInfo(dataDir);
    if (!dirInfo.isWritable()) {
        qCritical() << "Data directory is not writable:" << dataDir;
        QMessageBox::critical(nullptr, "Startup Error", 
            QString("Data directory is not writable:\n%1\n\nPlease check permissions.").arg(dataDir));
        return 1;
    }
    
    qDebug() << "Data directory OK, creating MainWindow...";
    
    // Create main window with exception handling
    MainWindow *mainWindow = nullptr;
    try {
        mainWindow = new MainWindow();
        qDebug() << "MainWindow created successfully";
        
        mainWindow->show();
        qDebug() << "MainWindow shown successfully";
        
        int result = app.exec();
        
        if (mainWindow) {
            delete mainWindow;
        }
        
        return result;
        
    } catch (const std::exception& e) {
        qCritical() << "Exception during startup:" << e.what();
        QMessageBox::critical(nullptr, "Startup Error", 
            QString("Failed to start ClanManager:\n\n%1\n\nData directory: %2\n\nPlease report this error.")
                .arg(e.what()).arg(dataDir));
        if (mainWindow) {
            delete mainWindow;
        }
        return 1;
        
    } catch (...) {
        qCritical() << "Unknown exception during startup";
        QMessageBox::critical(nullptr, "Startup Error", 
            QString("Unknown error occurred during startup.\n\nData directory: %1\n\nPlease report this error.").arg(dataDir));
        if (mainWindow) {
            delete mainWindow;
        }
        return 1;
    }
}
