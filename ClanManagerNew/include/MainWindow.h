#pragma once

#include <QMainWindow>
#include "PlayerList.h"

class QTableWidget;
class QPushButton;
class QLineEdit;
class QComboBox;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void importCsv();
    void exportCsv();
    void addPlayer();
    void editPlayer();
    void promoteSelected();
    void demoteSelected();
    void showPlayerSoldbuch();
    void filterChanged(const QString &text = QString());
    void refresh();
    void deleteSelected();
    void updateRankFilter();

private:
    PlayerList m_players;
    QTableWidget *m_table = nullptr;
    QPushButton *m_importBtn = nullptr;
    QPushButton *m_exportBtn = nullptr;
    QPushButton *m_addBtn = nullptr;
    QPushButton *m_refreshBtn = nullptr;
    QLineEdit *m_search = nullptr;
    QComboBox *m_rankFilter = nullptr;
    QPushButton *m_editBtn = nullptr;
    QPushButton *m_createTrainingBtn = nullptr;
    QPushButton *m_manageGroupsBtn = nullptr;
    QPushButton *m_showSoldbuchBtn = nullptr;
    QPushButton *m_deleteBtn = nullptr;
    QString m_dataFile;

    QWidget *createActionsWidget(int row);

protected:
    void closeEvent(QCloseEvent *event) override;

    void refreshTable();
};
