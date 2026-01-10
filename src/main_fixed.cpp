#include <QApplication>
#include <QMessageBox>
#include "MainWindow.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    
    try {
        MainWindow w;
        w.show();
        return app.exec();
    } catch (const std::exception& e) {
        QMessageBox::critical(nullptr, "Startup Error", 
            QString("Failed to start ClanManager:\n%1").arg(e.what()));
        return 1;
    } catch (...) {
        QMessageBox::critical(nullptr, "Startup Error", 
            "Unknown error occurred during startup.");
        return 1;
    }
}
