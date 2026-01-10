#include "MainWindow.h"
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QMenu>
#include <QAction>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QLabel>
#include <QMenu>
#include <QContextMenuEvent>
#include "EditPlayerDialog.h"
#include <QSet>

#include <QMenuBar>
#include <QCoreApplication>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(6);
    m_table->setColumnCount(9);
    QStringList headers{"ID", tr("Spielername"), tr("Level"), tr("Gruppe"), tr("Teilnahmen"), tr("Kommentar"), tr("Beitrittsdatum"), tr("Dienstrang"), tr("Aktionen")};
    m_table->setHorizontalHeaderLabels(headers);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);
    m_search = new QLineEdit(this);
    m_search->setPlaceholderText(tr("Suche: Name, T17 oder Gruppe..."));
    m_rankFilter = new QComboBox(this);
    m_rankFilter->addItem(tr("Alle Ränge"));

    connect(m_search, &QLineEdit::textChanged, this, &MainWindow::filterChanged);
    connect(m_rankFilter, &QComboBox::currentTextChanged, this, &MainWindow::filterChanged);

    m_importBtn = new QPushButton(tr("Import (CSV)"), this);
    m_exportBtn = new QPushButton(tr("Export (CSV)"), this);
    m_addBtn = new QPushButton(tr("Neuen Spieler hinzufügen"), this);
    m_refreshBtn = new QPushButton(tr("Aktualisieren"), this);
    m_editBtn = new QPushButton(tr("Spieler bearbeiten"), this);
    m_createTrainingBtn = new QPushButton(tr("Training/Event erstellen"), this);
    m_manageGroupsBtn = new QPushButton(tr("Gruppen verwalten"), this);
    m_showSoldbuchBtn = new QPushButton(tr("Soldbuch anzeigen"), this);
    m_deleteBtn = new QPushButton(tr("Spieler löschen"), this);

    connect(m_importBtn, &QPushButton::clicked, this, &MainWindow::importCsv);
    connect(m_exportBtn, &QPushButton::clicked, this, &MainWindow::exportCsv);
    connect(m_addBtn, &QPushButton::clicked, this, &MainWindow::addPlayer);
    connect(m_refreshBtn, &QPushButton::clicked, this, &MainWindow::refresh);
    connect(m_editBtn, &QPushButton::clicked, this, &MainWindow::editPlayer);
    connect(m_showSoldbuchBtn, &QPushButton::clicked, this, &MainWindow::showPlayerSoldbuch);
    connect(m_deleteBtn, &QPushButton::clicked, this, &MainWindow::deleteSelected);

    connect(m_table, &QTableWidget::customContextMenuRequested, [this](const QPoint &pos)
            {
        int row = m_table->rowAt(pos.y());
        if (row < 0) return;
        m_table->setCurrentCell(row, 0);
        QMenu menu(this);
        QAction *actEdit = menu.addAction(tr("Spieler bearbeiten"));
        QAction *actPromote = menu.addAction(tr("Befördern"));
        QAction *actDemote = menu.addAction(tr("Degradieren"));
        QAction *actSoldbuch = menu.addAction(tr("Soldbuch anzeigen"));
        QAction *actDelete = menu.addAction(tr("Löschen"));
        QAction *sel = menu.exec(m_table->viewport()->mapToGlobal(pos));
        if (!sel) return;
        if (sel == actEdit) editPlayer();
        else if (sel == actPromote) promoteSelected();
        else if (sel == actDemote) demoteSelected();
        else if (sel == actSoldbuch) showPlayerSoldbuch(); });

    connect(m_table, &QTableWidget::doubleClicked, [this](const QModelIndex &idx)
            {
        if (!idx.isValid()) return;
        m_table->setCurrentCell(idx.row(), 0);
        editPlayer(); });

    QHBoxLayout *btnRow = new QHBoxLayout;
    btnRow->addWidget(m_importBtn);
    btnRow->addWidget(m_exportBtn);
    // Menu bar
    QMenuBar *mb = menuBar();
    QMenu *file = mb->addMenu(tr("&Datei"));
    file->addAction(tr("Import..."), this, &MainWindow::importCsv);
    file->addAction(tr("Export..."), this, &MainWindow::exportCsv);
    file->addSeparator();
    file->addAction(tr("Beenden"), qApp, &QCoreApplication::quit);

    QMenu *edit = mb->addMenu(tr("&Bearbeiten"));
    edit->addAction(tr("Neuen Spieler"), this, &MainWindow::addPlayer);
    edit->addAction(tr("Spieler bearbeiten"), this, &MainWindow::editPlayer);
    edit->addAction(tr("Spieler löschen"), this, &MainWindow::deleteSelected);
    btnRow->addWidget(m_addBtn);
    btnRow->addWidget(m_editBtn);
    btnRow->addWidget(m_createTrainingBtn);
    btnRow->addWidget(m_manageGroupsBtn);
    btnRow->addWidget(m_showSoldbuchBtn);
    btnRow->addWidget(m_deleteBtn);
    btnRow->addWidget(m_refreshBtn);

    QHBoxLayout *filterRow = new QHBoxLayout;
    filterRow->addWidget(new QLabel(tr("Suche:"), this));
    filterRow->addWidget(m_search);
    filterRow->addWidget(new QLabel(tr("Filter:"), this));
    filterRow->addWidget(m_rankFilter);

    QVBoxLayout *layout = new QVBoxLayout(central);
    layout->addLayout(filterRow);
    layout->addWidget(m_table);
    layout->addLayout(btnRow);

    setWindowTitle(tr("Clan Manager - Minimal"));
    resize(900, 600);

    // persistent data file in current directory
    m_dataFile = QDir::current().filePath("data.csv");
    if (QFile::exists(m_dataFile))
    {
        m_players.loadFromCsv(m_dataFile);
        refreshTable();
    }
}

QWidget *MainWindow::createActionsWidget(int row)
{
    QWidget *w = new QWidget(this);
    QHBoxLayout *l = new QHBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(4);

    // capture player id to find the current row dynamically
    int pid = -1;
    if (row >= 0 && row < m_players.players().size())
        pid = m_players.players().at(row).id;

    auto findRow = [this, pid]() -> int
    {
        if (pid < 0)
            return -1;
        for (int r = 0; r < m_table->rowCount(); ++r)
        {
            QTableWidgetItem *it = m_table->item(r, 0);
            if (!it)
                continue;
            if (it->text().toInt() == pid)
                return r;
        }
        return -1;
    };

    auto makeBtn = [&](const QString &txt, int wdt)
    { QPushButton *btn = new QPushButton(txt, w); btn->setFixedWidth(wdt); return btn; };

    QPushButton *bEdit = makeBtn(QString::fromUtf8("✎"), 28);
    bEdit->setToolTip(tr("Bearbeiten"));
    l->addWidget(bEdit);
    connect(bEdit, &QPushButton::clicked, [this, findRow]()
            {
        int r = findRow(); if (r < 0) return; m_table->setCurrentCell(r, 0); editPlayer(); });

    QPushButton *bProm = makeBtn(QString::fromUtf8("▲"), 28);
    bProm->setToolTip(tr("Befördern"));
    l->addWidget(bProm);
    connect(bProm, &QPushButton::clicked, [this, findRow]()
            {
        int r = findRow(); if (r < 0) return; m_table->setCurrentCell(r, 0); promoteSelected(); });

    QPushButton *bDem = makeBtn(QString::fromUtf8("▼"), 28);
    bDem->setToolTip(tr("Degradieren"));
    l->addWidget(bDem);
    connect(bDem, &QPushButton::clicked, [this, findRow]()
            {
        int r = findRow(); if (r < 0) return; m_table->setCurrentCell(r, 0); demoteSelected(); });

    QPushButton *bAddT = makeBtn(QString::fromUtf8("+T"), 34);
    bAddT->setToolTip(tr("+ Teilnahme"));
    l->addWidget(bAddT);
    connect(bAddT, &QPushButton::clicked, [this, findRow]()
            {
        int r = findRow(); if (r < 0) return;
        int idx = r;
        Player &pp = m_players.playersMutable()[idx];
        pp.attendance += 1;
        refreshTable(); });

    QPushButton *bSubT = makeBtn(QString::fromUtf8("-T"), 34);
    bSubT->setToolTip(tr("- Teilnahme"));
    l->addWidget(bSubT);
    connect(bSubT, &QPushButton::clicked, [this, findRow]()
            {
        int r = findRow(); if (r < 0) return;
        int idx = r;
        Player &pp = m_players.playersMutable()[idx];
        if (pp.attendance > 0) pp.attendance -= 1;
        refreshTable(); });

    QPushButton *bSold = makeBtn(QString::fromUtf8("S"), 28);
    bSold->setToolTip(tr("Soldbuch"));
    l->addWidget(bSold);
    connect(bSold, &QPushButton::clicked, [this, findRow]()
            {
        int r = findRow(); if (r < 0) return; m_table->setCurrentCell(r, 0); showPlayerSoldbuch(); });

    QPushButton *bDel = makeBtn(QString::fromUtf8("🗑"), 34);
    bDel->setToolTip(tr("Löschen"));
    l->addWidget(bDel);
    connect(bDel, &QPushButton::clicked, [this, findRow]()
            {
        int r = findRow(); if (r < 0) return; m_table->setCurrentCell(r, 0); deleteSelected(); });

    return w;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!m_dataFile.isEmpty())
    {
        m_players.saveToCsv(m_dataFile);
    }
    QMainWindow::closeEvent(event);
}

void MainWindow::importCsv()
{
    QString fn = QFileDialog::getOpenFileName(this, tr("CSV Datei wählen"), QString(), tr("CSV Files (*.csv);;Text Files (*.txt);;All Files (*)"));
    if (fn.isEmpty())
        return;
    if (!m_players.loadFromCsv(fn))
    {
        QMessageBox::warning(this, tr("Import Fehler"), tr("Konnte Datei nicht lesen."));
        return;
    }
    refreshTable();
    updateRankFilter();
}

void MainWindow::exportCsv()
{
    QString fn = QFileDialog::getSaveFileName(this, tr("CSV Datei speichern"), QString(), tr("CSV Files (*.csv);;Text Files (*.txt);;All Files (*)"));
    if (fn.isEmpty())
        return;
    if (!m_players.saveToCsv(fn))
    {
        QMessageBox::warning(this, tr("Export Fehler"), tr("Konnte Datei nicht schreiben."));
    }
}

void MainWindow::addPlayer()
{
    bool ok = false;
    QString name = QInputDialog::getText(this, tr("Neuer Spieler"), tr("Name:"), QLineEdit::Normal, QString(), &ok);
    if (!ok || name.trimmed().isEmpty())
        return;
    Player p;
    p.id = m_players.players().size() + 1;
    p.name = name.trimmed();
    p.joinDate = QDate::currentDate();
    m_players.addPlayer(p);
    refreshTable();
}

void MainWindow::refresh()
{
    refreshTable();
}

void MainWindow::refreshTable()
{
    const auto &pl = m_players.players();
    m_table->setRowCount(pl.size());
    for (int r = 0; r < pl.size(); ++r)
    {
        const Player &p = pl.at(r);
        m_table->setItem(r, 0, new QTableWidgetItem(QString::number(p.id)));
        m_table->setItem(r, 1, new QTableWidgetItem(p.name));
        m_table->setItem(r, 2, new QTableWidgetItem(QString::number(p.level)));
        m_table->setItem(r, 3, new QTableWidgetItem(p.group));
        m_table->setItem(r, 4, new QTableWidgetItem(QString::number(p.attendance)));
        m_table->setItem(r, 5, new QTableWidgetItem(p.comment));
        m_table->setItem(r, 6, new QTableWidgetItem(p.joinDate.toString(Qt::ISODate)));
        m_table->setItem(r, 7, new QTableWidgetItem(p.rank));
        QWidget *actW = createActionsWidget(r);
        m_table->setCellWidget(r, 8, actW);
    }
    updateRankFilter();
}

void MainWindow::updateRankFilter()
{
    QSet<QString> ranks;
    for (const Player &p : m_players.players())
    {
        if (!p.rank.trimmed().isEmpty())
            ranks.insert(p.rank.trimmed());
    }
    QString current = m_rankFilter->currentText();
    m_rankFilter->blockSignals(true);
    m_rankFilter->clear();
    m_rankFilter->addItem(tr("Alle Ränge"));
    for (const QString &r : ranks.toList())
        m_rankFilter->addItem(r);
    int idx = m_rankFilter->findText(current);
    if (idx >= 0)
        m_rankFilter->setCurrentIndex(idx);
    m_rankFilter->blockSignals(false);
}

void MainWindow::filterChanged(const QString & /*text*/)
{
    QString search = m_search->text().trimmed().toLower();
    QString rank = m_rankFilter->currentText();
    bool filterAll = (rank.isEmpty() || rank == tr("Alle Ränge"));
    for (int r = 0; r < m_table->rowCount(); ++r)
    {
        bool show = true;
        QTableWidgetItem *nameItem = m_table->item(r, 1);
        QTableWidgetItem *groupItem = m_table->item(r, 3);
        QTableWidgetItem *rankItem = m_table->item(r, 7);
        QString name = nameItem ? nameItem->text().toLower() : QString();
        QString group = groupItem ? groupItem->text().toLower() : QString();
        QString rankText = rankItem ? rankItem->text() : QString();
        if (!search.isEmpty())
        {
            if (!name.contains(search) && !group.contains(search) && !rankText.toLower().contains(search))
                show = false;
        }
        if (show && !filterAll)
        {
            if (rankText != rank)
                show = false;
        }
        m_table->setRowHidden(r, !show);
    }
}

void MainWindow::promoteSelected()
{
    int row = m_table->currentRow();
    if (row < 0)
        return;
    Player &p = m_players.at(row);
    if (p.rank.isEmpty())
        p.rank = "T+";
    else
        p.rank += "+";
    refreshTable();
}

void MainWindow::demoteSelected()
{
    int row = m_table->currentRow();
    if (row < 0)
        return;
    Player &p = m_players.at(row);
    if (p.rank.isEmpty())
        p.rank = "D-";
    else
        p.rank += "-";
    refreshTable();
}

void MainWindow::editPlayer()
{
    int row = m_table->currentRow();
    if (row < 0)
        return;
    Player &p = m_players.at(row);
    EditPlayerDialog dlg(this);
    dlg.setPlayer(p);
    if (dlg.exec() == QDialog::Accepted)
    {
        Player edited = dlg.player();
        // keep id
        edited.id = p.id;
        m_players.playersMutable()[row] = edited;
        refreshTable();
    }
}

void MainWindow::showPlayerSoldbuch()
{
    int row = m_table->currentRow();
    if (row < 0)
        return;
    Player &p = m_players.at(row);
    QMessageBox::information(this, tr("Soldbuch"), tr("Soldbuch für %1\n(Platzhalter)").arg(p.name));
}

void MainWindow::deleteSelected()
{
    int row = m_table->currentRow();
    if (row < 0)
        return;
    const Player &p = m_players.at(row);
    if (QMessageBox::question(this, tr("Spieler löschen"), tr("Spieler %1 wirklich löschen?").arg(p.name)) == QMessageBox::Yes)
    {
        m_players.playersMutable().remove(row);
        refreshTable();
    }
}
