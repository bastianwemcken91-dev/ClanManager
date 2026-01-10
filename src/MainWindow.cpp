// Clean MainWindow implementation used as a temporary replacement for a
// corrupted translation unit. Provides minimal UI and stubs for the
// methods declared in include/MainWindow.h so the project can build.
#include "MainWindow.h"
#include <QTableView>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QItemSelectionModel>
#include <QStandardItem>
#include <QPushButton>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QBrush>
#include <QColor>
#include <QDir>
#include <QMap>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonArray>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QLineEdit>
#include <QStyledItemDelegate>
#include <QEvent>
#include <QApplication>
#include <QGridLayout>
#include <QFormLayout>
#include <QTime>
#include <QSet>
#include <QUuid>
#include <QMenu>
#include <QTreeWidget>
#include <QVariant>
#include <QListWidget>
#include <QSizePolicy>
#include <QVector>
#include <QGroupBox>
#include <QInputDialog>
#include <QColorDialog>
#include <type_traits>
#include <QStringConverter>
#include <algorithm>
#include "LineupDialog.h"
#include "LineupExporter.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QProcess>
#include <QDateEdit>
#include <QSpinBox>
#include <QSortFilterProxyModel>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionButton>
#include <QMouseEvent>
#include <QDateTime>
#include <QTextEdit>
#include <QTabWidget>
#include <QScrollArea>
#include <QTextBrowser>

namespace
{
    QString dataFilePath(const QString &fileName)
    {
        QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (dir.isEmpty())
            dir = QDir::homePath() + "/.clanmanager";
        QDir().mkpath(dir);
        return QDir(dir).filePath(fileName);
    }

    void appendDefaultGroups(QStringList &groups, QMap<QString, QString> &categories, QMap<QString, QString> &colors)
    {
        struct DefaultGroup
        {
            const char *name;
            const char *category;
            const char *color;
        };
        static const DefaultGroup defaults[] = {
            {"Anwerber", "Sonstiges", ""},
            {"Fennek", "Angriff", ""},
            {"Fenriswolf", "Panzerwaffe", ""},
            {"Hanibal", "Verteidigung", ""},
            {"Herbstnebel", "Verteidigung", ""},
            {"Kaiser", "Artillerie", ""},
            {"Kaiserwucht", "Artillerie", ""},
            {"Kampfgruppe Zander", "Angriff", ""},
            {"Phönix", "Panzerwaffe", ""},
            {"Steiner", "Angriff", ""},
            {"Tortuga", "Sonstiges", ""}};
        for (const DefaultGroup &def : defaults)
        {
            const QString name = QString::fromUtf8(def.name);
            if (!groups.contains(name))
                groups.append(name);
            const QString category = QString::fromUtf8(def.category);
            if (!category.isEmpty())
                categories.insert(name, category);
            const QString color = QString::fromUtf8(def.color);
            if (!color.isEmpty())
                colors.insert(name, color);
        }
    }

    int indexOfGroupName(const QStringList &list, const QString &name)
    {
        for (int i = 0; i < list.size(); ++i)
        {
            if (list.at(i).compare(name, Qt::CaseInsensitive) == 0)
                return i;
        }
        return -1;
    }
}

QStringList MainWindow::rankOptions()
{
    return {"Anwerber/AW", "AW", "Panzergrenadier", "PzGren", "Obergrenadier", "OGren", "Gefreiter", "Gefr", "Obergefreiter", "OGefr", "Stabsgefreiter", "StGefr", "Unteroffizier", "Uffz", "Stabsunteroffizier (ZBV)", "StUffz", "Unterfeldwebel", "Ufw", "Feldwebel", "Fw", "Oberfeldwebel", "OFw", "Hauptfeldwebel", "HFw", "Stabsfeldwebel", "StFw", "Fähnrich", "Fähn", "Leutnant", "Lt", "Oberleutnant", "OLt", "Hauptmann", "Hptm", "Major", "Maj", "Oberst", "Obst"};
}

// Proxy model used for search/filter and rank-aware sorting
class SortProxy : public QSortFilterProxyModel
{
public:
    SortProxy(const QStringList &ranks, QObject *parent = nullptr) : QSortFilterProxyModel(parent)
    {
        int idx = 0;
        for (const QString &r : ranks)
            rankOrder.insert(r, idx++);
        officerThreshold = INT_MAX;
    }

    void setRankFilter(const QString &f)
    {
        rankFilter = f;
        invalidateFilter();
    }
    void setGroupFilter(const QString &f)
    {
        groupFilter = f;
        invalidateFilter();
    }
    void setTextFilter(const QString &t)
    {
        textFilter = t;
        invalidateFilter();
    }

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override
    {
        const QAbstractItemModel *m = sourceModel();
        if (!m)
            return QSortFilterProxyModel::lessThan(left, right);

        auto stringValue = [&](int row, int column) -> QString
        {
            const int role = (column == 8) ? Qt::EditRole : Qt::DisplayRole;
            return m->index(row, column).data(role).toString();
        };
        auto compareString = [](const QString &a, const QString &b) -> bool
        {
            return QString::localeAwareCompare(a, b) < 0;
        };

        int column = sortColumn();
        if (column < 0)
            column = left.column();

        switch (column)
        {
        case 0:
        case 1:
        case 2:
        case 4:
        case 5:
        case 6:
            return compareString(stringValue(left.row(), column), stringValue(right.row(), column));
        case 3:
        {
            int lv = m->index(left.row(), 3).data(Qt::DisplayRole).toInt();
            int rv = m->index(right.row(), 3).data(Qt::DisplayRole).toInt();
            if (lv != rv)
                return lv < rv;
            return compareString(stringValue(left.row(), 0), stringValue(right.row(), 0));
        }
        case 7:
        {
            QDate dl = QDate::fromString(stringValue(left.row(), 7), Qt::ISODate);
            QDate dr = QDate::fromString(stringValue(right.row(), 7), Qt::ISODate);
            if (dl.isValid() && dr.isValid() && dl != dr)
                return dl < dr;
            return compareString(stringValue(left.row(), 7), stringValue(right.row(), 7));
        }
        case 8:
        {
            QString rl = stringValue(left.row(), 8);
            QString rr = stringValue(right.row(), 8);
            int i1 = rankOrder.value(rl, INT_MAX);
            int i2 = rankOrder.value(rr, INT_MAX);
            if (i1 != i2)
                return i1 < i2;
            int lvl = m->index(left.row(), 3).data(Qt::DisplayRole).toInt();
            int lvr = m->index(right.row(), 3).data(Qt::DisplayRole).toInt();
            if (lvl != lvr)
                return lvl < lvr;
            return compareString(stringValue(left.row(), 0), stringValue(right.row(), 0));
        }
        default:
            break;
        }

        QString n1 = stringValue(left.row(), 0);
        QString n2 = stringValue(right.row(), 0);
        return compareString(n1, n2);
    }

    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override
    {
        const QAbstractItemModel *m = sourceModel();
        if (!m)
            return true;

        if (!groupFilter.isEmpty() && groupFilter != "Alle Gruppen")
        {
            QString g = m->index(source_row, 4, source_parent).data().toString(); // group col
            if (g != groupFilter)
                return false;
        }

        if (!rankFilter.isEmpty() && rankFilter != "Alle Ränge")
        {
            if (rankFilter == "Nur Offiziere")
            {
                QString r = m->index(source_row, 8, source_parent).data(Qt::EditRole).toString();
                int idx = rankOrder.value(r, -1);
                if (idx < officerThreshold)
                    return false;
            }
            else
            {
                QString r = m->index(source_row, 8, source_parent).data(Qt::EditRole).toString();
                if (r != rankFilter)
                    return false;
            }
        }

        if (!textFilter.isEmpty())
        {
            QString txt = textFilter;
            QString n = m->index(source_row, 0, source_parent).data().toString();
            QString t17 = m->index(source_row, 2, source_parent).data().toString();
            QString g = m->index(source_row, 4, source_parent).data().toString();
            if (!n.contains(txt, Qt::CaseInsensitive) && !t17.contains(txt, Qt::CaseInsensitive) && !g.contains(txt, Qt::CaseInsensitive))
                return false;
        }

        return true;
    }

private:
    QMap<QString, int> rankOrder;
    QString rankFilter;
    QString groupFilter;
    QString textFilter;
    int officerThreshold;
};

// Delegate for group selection (column 3)
class GroupDelegate : public QStyledItemDelegate
{
    MainWindow *m_window;

public:
    GroupDelegate(MainWindow *mw, QObject *parent = nullptr) : QStyledItemDelegate(parent), m_window(mw) {}
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const override
    {
        QComboBox *cb = new QComboBox(parent);
        QStringList gr = m_window->getGroups();
        QMap<QString, QString> grpCat = m_window->getGroupCategory();
        for (const QString &g : gr)
        {
            QString cat = grpCat.value(g);
            QString disp = cat.isEmpty() ? g : QString("%1 - %2").arg(cat, g);
            cb->addItem(disp, g);
        }
        cb->setEditable(false);
        return cb;
    }
    void setEditorData(QWidget *editor, const QModelIndex &index) const override
    {
        QComboBox *cb = qobject_cast<QComboBox *>(editor);
        if (!cb)
            return;
        QString cur = index.model()->data(index, Qt::EditRole).toString();
        for (int i = 0; i < cb->count(); ++i)
        {
            if (cb->itemData(i).toString() == cur)
            {
                cb->setCurrentIndex(i);
                return;
            }
        }
        int idx = cb->findText(cur);
        if (idx >= 0)
            cb->setCurrentIndex(idx);
    }
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override
    {
        QComboBox *cb = qobject_cast<QComboBox *>(editor);
        if (!cb)
            return;
        QString grp = cb->currentData().toString();
        model->setData(index, grp, Qt::EditRole);
    }
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const override
    {
        editor->setGeometry(option.rect);
    }
};

// Delegate for last promotion date (column 6)
class DateDelegate : public QStyledItemDelegate
{
public:
    DateDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const override
    {
        QDateEdit *ed = new QDateEdit(parent);
        ed->setCalendarPopup(true);
        ed->setDisplayFormat("yyyy-MM-dd");
        MainWindow *mw = qobject_cast<MainWindow *>(this->parent());
        if (mw)
            ed->setDate(mw->nowDate());
        else
            ed->setDate(QDate::currentDate());
        return ed;
    }
    void setEditorData(QWidget *editor, const QModelIndex &index) const override
    {
        QDateEdit *ed = qobject_cast<QDateEdit *>(editor);
        if (!ed)
            return;
        QString v = index.model()->data(index, Qt::EditRole).toString();
        if (!v.isEmpty())
            ed->setDate(QDate::fromString(v, Qt::ISODate));
    }
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override
    {
        QDateEdit *ed = qobject_cast<QDateEdit *>(editor);
        if (!ed)
            return;
        model->setData(index, ed->date().toString(Qt::ISODate), Qt::EditRole);
    }
};

// Delegate for Kommentar column (column 5)
class CommentDelegate : public QStyledItemDelegate
{
public:
    CommentDelegate(MainWindow *mw, QObject *parent = nullptr) : QStyledItemDelegate(parent), m_main(mw) {}
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const override
    {
        QComboBox *cb = new QComboBox(parent);
        cb->setEditable(true);
        cb->addItems(m_main->commentOptions);
        return cb;
    }
    void setEditorData(QWidget *editor, const QModelIndex &index) const override
    {
        QComboBox *cb = qobject_cast<QComboBox *>(editor);
        if (!cb)
            return;
        QString cur = index.model()->data(index, Qt::EditRole).toString();
        int idx = cb->findText(cur);
        if (idx >= 0)
            cb->setCurrentIndex(idx);
        else
            cb->setEditText(cur);
    }
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override
    {
        QComboBox *cb = qobject_cast<QComboBox *>(editor);
        if (!cb)
            return;
        QString val = cb->currentText().trimmed();
        if (val.isEmpty())
        {
            model->setData(index, QString(), Qt::EditRole);
            return;
        }

        QMessageBox::StandardButton res = QMessageBox::question(m_main, "Kommentar bestätigen",
                                                                QString("Kommentar '%1' hinzufügen und ins Log schreiben?").arg(val),
                                                                QMessageBox::Yes | QMessageBox::No);
        if (res != QMessageBox::Yes)
        {
            model->setData(index, QString(), Qt::EditRole);
            return;
        }

        if (!m_main->commentOptions.contains(val))
        {
            m_main->commentOptions.append(val);
            m_main->saveCommentOptions();
        }

        QAbstractItemModel *m = model;
        QSortFilterProxyModel *proxy = qobject_cast<QSortFilterProxyModel *>(m);
        QModelIndex srcIndex = index;
        if (proxy)
            srcIndex = proxy->mapToSource(index);

        QString playerKey;
        const QStandardItemModel *srcModel = qobject_cast<const QStandardItemModel *>(srcIndex.model());
        if (srcModel && srcIndex.isValid())
        {
            int actionCol = srcModel->columnCount() - 1;
            const QStandardItem *actItem = srcModel->item(srcIndex.row(), actionCol);
            if (actItem)
                playerKey = actItem->data(Qt::UserRole + 1).toString();
        }

        if (!playerKey.isEmpty())
        {
            QJsonObject data;
            data.insert("comment", val);
            data.insert("date", m_main->nowDate().toString(Qt::ISODate));
            m_main->appendSoldbuchEntry(playerKey, "Comment", data, QDateTime::currentDateTime());
            m_main->updateEligibilityForPlayerKey(playerKey);
        }

        model->setData(index, QString(), Qt::EditRole);
    }

private:
    MainWindow *m_main;
};

class ActionButtonDelegate : public QStyledItemDelegate
{
public:
    enum ButtonId
    {
        CreateSession,
        LogTraining,
        LogEvent,
        LogReserve,
        OpenSoldbuch
    };

    ActionButtonDelegate(MainWindow *mw, QObject *parent = nullptr) : QStyledItemDelegate(parent), m_main(mw)
    {
        specs = {

            {LogTraining, QStringLiteral("T"), QStringLiteral("Training eintragen"), 22},
            {LogEvent, QStringLiteral("E"), QStringLiteral("Event eintragen"), 22},
            {LogReserve, QStringLiteral("R"), QStringLiteral("Reserve eintragen"), 24},
            {OpenSoldbuch, QStringLiteral("SB"), QStringLiteral("Soldbuch öffnen"), 30}};
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &) const override
    {
        QStyle *style = option.widget ? option.widget->style() : QApplication::style();
        for (const auto &button : buttonRects(option))
        {
            QStyleOptionButton btn;
            btn.rect = button.rect;
            btn.text = button.label;
            btn.state = QStyle::State_Enabled;
            if (option.state & QStyle::State_Selected)
                btn.state |= QStyle::State_Selected;
            style->drawControl(QStyle::CE_PushButton, &btn, painter);
        }
    }

    bool editorEvent(QEvent *event, QAbstractItemModel *, const QStyleOptionViewItem &option, const QModelIndex &index) override
    {
        if (!m_main)
            return false;
        if (event->type() != QEvent::MouseButtonRelease)
            return false;
        auto *mouse = static_cast<QMouseEvent *>(event);
        for (const auto &button : buttonRects(option))
        {
            if (button.rect.contains(mouse->pos()))
            {
                QModelIndex sourceIndex = index;
                if (auto *proxy = qobject_cast<const QSortFilterProxyModel *>(index.model()))
                    sourceIndex = proxy->mapToSource(index);
                triggerAction(button.id, sourceIndex.row());
                break;
            }
        }
        return true;
    }

private:
    struct ButtonSpec
    {
        ButtonId id;
        QString label;
        QString tooltip;
        int minWidth;
    };

    struct ButtonDef
    {
        ButtonId id;
        QString label;
        QString tooltip;
        QRect rect;
    };

    QVector<ButtonDef> buttonRects(const QStyleOptionViewItem &option) const
    {
        QVector<ButtonDef> result;
        if (specs.isEmpty())
            return result;
        const int spacing = 2;
        const int total = specs.size();
        const int contentWidth = option.rect.width() - spacing * (total + 1);
        const int buttonHeight = qMax(18, option.rect.height() - 10);
        int x = option.rect.left() + spacing;
        int y = option.rect.center().y() - buttonHeight / 2;
        for (const auto &spec : specs)
        {
            ButtonDef def;
            def.id = spec.id;
            def.label = spec.label;
            def.tooltip = spec.tooltip;
            int widthPer = (contentWidth > 0 && total > 0) ? contentWidth / total : spec.minWidth;
            int width = qMax(spec.minWidth, widthPer);
            def.rect = QRect(x, y, width, buttonHeight);
            result.append(def);
            x += width + spacing;
        }
        return result;
    }

    void triggerAction(ButtonId id, int sourceRow) const
    {
        if (!m_main)
            return;
        switch (id)
        {
        case CreateSession:
            m_main->handleCreateSessionButtonForRow(sourceRow);
            break;
        case LogTraining:
            m_main->handleTrainingButtonForRow(sourceRow);
            break;
        case LogEvent:
            m_main->handleEventButtonForRow(sourceRow);
            break;
        case LogReserve:
            m_main->handleReserveButtonForRow(sourceRow);
            break;
        case OpenSoldbuch:
            m_main->handleSoldbuchButtonForRow(sourceRow);
            break;
        }
    }

    MainWindow *m_main = nullptr;
    QVector<ButtonSpec> specs;
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // Initialize all pointers to nullptr FIRST to prevent crashes
    model = nullptr;
    table = nullptr;
    proxy = nullptr;
    searchEdit = nullptr;
    filterModeCombo = nullptr;
    rankFilterCombo = nullptr;
    groupFilterCombo = nullptr;
    sortFieldCombo = nullptr;
    sessionBox = nullptr;
    sessionTemplateCombo = nullptr;
    sessionTypeCombo = nullptr;
    sessionNameEdit = nullptr;
    sessionMapCombo = nullptr;
    sessionDateEdit = nullptr;
    sessionRememberCheck = nullptr;
    sessionPlayerTree = nullptr;
    sessionConfirmedList = nullptr;
    sessionDeclinedList = nullptr;
    sessionNoResponseList = nullptr;
    sessionSummaryLabel = nullptr;
    useTestDate = nullptr;
    testDateEdit = nullptr;
    trainingsSummaryLabel = nullptr;
    
    // Set default values for member variables
    noResponseThreshold = 10;
    fuzzyMatchThreshold = 2;
    autoCreatePlayers = true;
    fuzzyMatchingEnabled = true;
    autoFillMetadata = true;
    ocrLanguage = "deu";
    unassignedGroupName = "Nicht zugewiesen";
    incrementCounterOnNoResponse = true;
    resetCounterOnResponse = true;
    showCounterInTable = true;
    hintColumnName = "Hinweis";
    
    qDebug() << "MainWindow: Starting UI initialization...";
    
    try {
        // Initialize UI widgets
        initializeUI();
        qDebug() << "MainWindow: UI initialized successfully";
        
        // Load data files
        loadDataFiles();
        qDebug() << "MainWindow: Data files loaded successfully";
        
    } catch (const std::exception& e) {
        qCritical() << "MainWindow initialization failed:" << e.what();
        QMessageBox::critical(this, "Initialization Error", 
            QString("Failed to initialize ClanManager:\n%1").arg(e.what()));
        throw; // Re-throw to be caught in main()
    } catch (...) {
        qCritical() << "MainWindow initialization failed with unknown error";
        QMessageBox::critical(this, "Initialization Error", 
            "Unknown error during initialization");
        throw;
    }
}

void MainWindow::initializeUI()
{
    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    model = new QStandardItemModel(this);
    QStringList headers = {"Spielername", hintColumnName, "T17-Name", "Level", "Gruppe", "Einsätze", "Kommentar", "Beitrittsdatum", "Dienstrang", "Aktion"};
    model->setHorizontalHeaderLabels(headers);

    table = new QTableView(this);
    QHeaderView *header = table->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::Stretch);
    header->setSectionResizeMode(1, QHeaderView::Interactive);
    header->setMinimumSectionSize(40);
    header->setSectionsClickable(true);
    header->setSortIndicatorShown(true);
    table->setColumnWidth(1, 80);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked);
    table->setContextMenuPolicy(Qt::CustomContextMenu);

    // setup proxy model with rank ordering
    proxy = new SortProxy(MainWindow::rankOptions(), this);
    proxy->setDynamicSortFilter(true);
    proxy->setSourceModel(model);
    table->setModel(proxy);
    table->setSortingEnabled(true);
    table->sortByColumn(0, Qt::AscendingOrder);

    // Delegate for fixed Dienstrang selection (dropdown) on the rank column
    class RankDelegate : public QStyledItemDelegate
    {
        MainWindow *m_window = nullptr;

    public:
        RankDelegate(MainWindow *mw, QObject *parent = nullptr) : QStyledItemDelegate(parent), m_window(mw) {}
        QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const override
        {
            QComboBox *cb = new QComboBox(parent);
            cb->addItems(MainWindow::rankOptions());
            cb->setEditable(false);
            return cb;
        }
        void setEditorData(QWidget *editor, const QModelIndex &index) const override
        {
            QComboBox *cb = qobject_cast<QComboBox *>(editor);
            if (!cb)
                return;
            QString val = index.model()->data(index, Qt::EditRole).toString();
            int idx = cb->findText(val);
            cb->setCurrentIndex(qMax(0, idx));
        }
        void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override
        {
            QComboBox *cb = qobject_cast<QComboBox *>(editor);
            if (!cb)
                return;
            model->setData(index, cb->currentText(), Qt::EditRole);
            if (m_window)
            {
                QModelIndex sourceIndex = index;
                if (auto *proxyModel = qobject_cast<const QSortFilterProxyModel *>(index.model()))
                    sourceIndex = proxyModel->mapToSource(index);
                m_window->validateRow(sourceIndex.row());
            }
        }
        void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const override
        {
            editor->setGeometry(option.rect);
        }
    };

    table->setItemDelegateForColumn(8, new RankDelegate(this, table));
    table->setItemDelegateForColumn(4, new GroupDelegate(this, table));
    table->setItemDelegateForColumn(6, new CommentDelegate(this, table));
    table->setItemDelegateForColumn(7, new DateDelegate(this));
    table->setItemDelegateForColumn(9, new ActionButtonDelegate(this, table));

    QPushButton *refreshBtn = new QPushButton("Aktualisieren", this);
    // Import/Export wandern in den Einstellungsdialog
    // QPushButton *importBtn = new QPushButton("Import (CSV/TXT)", this);
    // QPushButton *exportBtn = new QPushButton("Export (CSV)", this);
    // QPushButton *rankSettingsBtn = new QPushButton("Rank Settings", this);
    // QPushButton *multiAssignBtn = new QPushButton("Training/Event zuweisen", this);
    QPushButton *settingsBtn = new QPushButton("⚙️ Einstellungen", this);
    QPushButton *addPlayerBtn = new QPushButton("Spieler anlegen", this);
    QPushButton *editPlayerBtn = new QPushButton("Spieler bearbeiten", this);
    QPushButton *manageGroupsBtn = new QPushButton("Gruppen verwalten", this);

    connect(refreshBtn, &QPushButton::clicked, this, [this]()
            {
                refreshModelFromList();
                validateAllRows();
                if (table)
                    table->viewport()->update(); });
    // Import/Export und Rank Settings werden über den Einstellungsdialog angeboten
    connect(table, &QTableView::customContextMenuRequested, this, &MainWindow::showContextMenu);
    connect(settingsBtn, &QPushButton::clicked, this, &MainWindow::showSettingsDialog);
    connect(addPlayerBtn, &QPushButton::clicked, this, &MainWindow::addPlayer);
    connect(editPlayerBtn, &QPushButton::clicked, this, &MainWindow::editPlayer);
    connect(manageGroupsBtn, &QPushButton::clicked, this, &MainWindow::editGroups);

    // filter / search layout above the table
    QHBoxLayout *filterLay = new QHBoxLayout;
    filterLay->setSpacing(6);
    QLabel *searchLbl = new QLabel("Suche:", this);
    searchEdit = new QLineEdit(this);
    searchEdit->setPlaceholderText("Name, T17 oder Gruppe...");

    filterModeCombo = new QComboBox(this);
    filterModeCombo->addItem("Rang");
    filterModeCombo->addItem("Gruppe");

    rankFilterCombo = new QComboBox(this);
    rankFilterCombo->addItem("Alle Ränge");
    rankFilterCombo->addItem("Nur Offiziere");
    for (const QString &r : MainWindow::rankOptions())
        rankFilterCombo->addItem(r, r);
    rankFilterCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);

    groupFilterCombo = new QComboBox(this);
    groupFilterCombo->addItem("Alle Gruppen");
    groupFilterCombo->setVisible(false);

    QLabel *filterModeLbl = new QLabel("Filter:", this);
    QLabel *rankLbl = new QLabel("Rang:", this);
    QLabel *groupLbl = new QLabel("Gruppe:", this);
    groupLbl->setVisible(false);

    QLabel *sortFieldLbl = new QLabel("Sortierung:", this);
    sortFieldCombo = new QComboBox(this);
    sortFieldCombo->addItem("Spielername", 0);
    sortFieldCombo->addItem("T17-Name", 2);
    sortFieldCombo->addItem("Level", 3);
    sortFieldCombo->addItem("Gruppe", 4);
    sortFieldCombo->addItem("Beitritt", 7);
    sortFieldCombo->addItem("Rang", 8);
    QLabel *sortOrderLbl = new QLabel("Reihenfolge:", this);
    sortOrderCombo = new QComboBox(this);
    sortOrderCombo->addItem("Aufsteigend", static_cast<int>(Qt::AscendingOrder));
    sortOrderCombo->addItem("Absteigend", static_cast<int>(Qt::DescendingOrder));

    useTestDate = new QCheckBox("Testdatum", this);
    testDateEdit = new QDateEdit(this);
    testDateEdit->setDisplayFormat("yyyy-MM-dd");
    testDateEdit->setCalendarPopup(true);
    testDateEdit->setDate(QDate::currentDate());
    testDateEdit->setEnabled(false);

    filterLay->addWidget(searchLbl);
    filterLay->addWidget(searchEdit);
    filterLay->addWidget(filterModeLbl);
    filterLay->addWidget(filterModeCombo);
    filterLay->addWidget(rankLbl);
    filterLay->addWidget(rankFilterCombo);
    filterLay->addWidget(groupLbl);
    filterLay->addWidget(groupFilterCombo);
    filterLay->addWidget(sortFieldLbl);
    filterLay->addWidget(sortFieldCombo);
    filterLay->addWidget(sortOrderLbl);
    filterLay->addWidget(sortOrderCombo);
    filterLay->addWidget(useTestDate);
    filterLay->addWidget(testDateEdit);

    sessionBox = new QGroupBox("Letztes Gefecht / Training", this);
    sessionBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    // Feste Höhe verdoppeln: z.B. 600px (statt ~300px)
    sessionBox->setFixedHeight(500);
    QVBoxLayout *sessionLayout = new QVBoxLayout(sessionBox);

    QHBoxLayout *sessionRow1 = new QHBoxLayout;
    sessionTemplateCombo = new QComboBox(sessionBox);
    sessionRow1->addWidget(new QLabel("Vorlage:", sessionBox));
    sessionRow1->addWidget(sessionTemplateCombo, 1);
    sessionTypeCombo = new QComboBox(sessionBox);
    sessionTypeCombo->addItems({"Training", "Event", "Reserve"});
    sessionRow1->addWidget(new QLabel("Typ:", sessionBox));
    sessionRow1->addWidget(sessionTypeCombo);
    sessionDateEdit = new QDateEdit(sessionBox);
    sessionDateEdit->setCalendarPopup(true);
    sessionDateEdit->setDisplayFormat("yyyy-MM-dd");
    sessionDateEdit->setDate(nowDate());
    sessionRow1->addWidget(new QLabel("Datum:", sessionBox));
    sessionRow1->addWidget(sessionDateEdit);
    sessionLayout->addLayout(sessionRow1);

    QHBoxLayout *sessionRow2 = new QHBoxLayout;
    sessionNameEdit = new QLineEdit(sessionBox);
    sessionNameEdit->setPlaceholderText("Name des Trainings/Event");
    sessionRow2->addWidget(new QLabel("Name:", sessionBox));
    sessionRow2->addWidget(sessionNameEdit, 1);
    sessionMapCombo = new QComboBox(sessionBox);
    sessionMapCombo->setEditable(true);
    sessionRow2->addWidget(new QLabel("Karte:", sessionBox));
    sessionRow2->addWidget(sessionMapCombo);
    sessionRememberCheck = new QCheckBox("Als Vorlage speichern", sessionBox);
    sessionRememberCheck->setChecked(true);
    sessionRow2->addWidget(sessionRememberCheck);
    sessionLayout->addLayout(sessionRow2);

    sessionPlayerTree = new QTreeWidget(sessionBox);
    sessionPlayerTree->setColumnCount(2);
    sessionPlayerTree->setHeaderLabels({"Teilnahme", "Spieler"});
    sessionPlayerTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    sessionPlayerTree->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    sessionPlayerTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    sessionPlayerTree->setSelectionMode(QAbstractItemView::NoSelection);
    sessionPlayerTree->setRootIsDecorated(true);
    sessionPlayerTree->setUniformRowHeights(true);
    sessionPlayerTree->setMinimumHeight(260);
    sessionPlayerTree->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sessionPlayerTree->setVisible(false); // Versteckt (alt), durch neue Listen ersetzt
    sessionLayout->addWidget(sessionPlayerTree, 0);

    // Neue 3-Listen-Struktur
    QHBoxLayout *listsLayout = new QHBoxLayout;

    QVBoxLayout *confirmedLayout = new QVBoxLayout;
    QLabel *confirmedHeader = new QLabel("<b>✓ Zugesagt</b>", sessionBox);
    confirmedHeader->setAlignment(Qt::AlignCenter);
    confirmedHeader->setStyleSheet("background:#d4edda; padding:4px; border:1px solid #c3e6cb;");
    sessionConfirmedList = new QListWidget(sessionBox);
    sessionConfirmedList->setSelectionMode(QAbstractItemView::SingleSelection);
    confirmedLayout->addWidget(confirmedHeader);
    confirmedLayout->addWidget(sessionConfirmedList);
    listsLayout->addLayout(confirmedLayout);

    QVBoxLayout *declinedLayout = new QVBoxLayout;
    QLabel *declinedHeader = new QLabel("<b>✗ Abgesagt</b>", sessionBox);
    declinedHeader->setAlignment(Qt::AlignCenter);
    declinedHeader->setStyleSheet("background:#f8d7da; padding:4px; border:1px solid #f5c6cb;");
    sessionDeclinedList = new QListWidget(sessionBox);
    sessionDeclinedList->setSelectionMode(QAbstractItemView::SingleSelection);
    declinedLayout->addWidget(declinedHeader);
    declinedLayout->addWidget(sessionDeclinedList);
    listsLayout->addLayout(declinedLayout);

    QVBoxLayout *noResponseLayout = new QVBoxLayout;
    QLabel *noResponseHeader = new QLabel("<b>? Keine Antwort</b>", sessionBox);
    noResponseHeader->setAlignment(Qt::AlignCenter);
    noResponseHeader->setStyleSheet("background:#fff3cd; padding:4px; border:1px solid #ffeaa7;");
    sessionNoResponseList = new QListWidget(sessionBox);
    sessionNoResponseList->setSelectionMode(QAbstractItemView::SingleSelection);
    noResponseLayout->addWidget(noResponseHeader);
    noResponseLayout->addWidget(sessionNoResponseList);
    listsLayout->addLayout(noResponseLayout);

    sessionLayout->addLayout(listsLayout, 1);

    QHBoxLayout *sessionButtons = new QHBoxLayout;
    QPushButton *sessionSelectBtn = new QPushButton("Aus Liste übernehmen", sessionBox);
    QPushButton *sessionUploadBtn = new QPushButton("Spielerliste hochladen", sessionBox);
    sessionUploadBtn->setToolTip("Bild (Screenshot/Foto) mit Namensliste laden und per OCR Spieler markieren");
    QPushButton *addSessionPlayerBtn = new QPushButton("+ Spieler", sessionBox);
    addSessionPlayerBtn->setToolTip("Spieler manuell zur Session hinzufügen");
    QPushButton *removePlayerBtn = new QPushButton("- Spieler", sessionBox);
    removePlayerBtn->setToolTip("Ausgewählten Spieler entfernen");
    QPushButton *markConfirmedBtn = new QPushButton("→ Zugesagt", sessionBox);
    markConfirmedBtn->setToolTip("Ausgewählten Spieler als zugesagt markieren");
    QPushButton *markDeclinedBtn = new QPushButton("→ Abgesagt", sessionBox);
    markDeclinedBtn->setToolTip("Ausgewählten Spieler als abgesagt markieren");
    QPushButton *sessionApplyBtn = new QPushButton("Session bestätigen", sessionBox);
    sessionButtons->addWidget(sessionSelectBtn);
    sessionButtons->addWidget(sessionUploadBtn);
    sessionButtons->addWidget(addSessionPlayerBtn);
    sessionButtons->addWidget(removePlayerBtn);
    sessionButtons->addWidget(markConfirmedBtn);
    sessionButtons->addWidget(markDeclinedBtn);
    sessionButtons->addStretch();

    // Test-Button zum Erhöhen des NoResponse-Counters
    QPushButton *testCounterBtn = new QPushButton("🧪 Test Counter+", sessionBox);
    testCounterBtn->setToolTip("TEST: Erhöht den NoResponse-Counter für ausgewählte Spieler");
    testCounterBtn->setStyleSheet("background-color: #fff3cd; color: #856404;");
    sessionButtons->addWidget(testCounterBtn);

    sessionButtons->addWidget(sessionApplyBtn);
    sessionLayout->addLayout(sessionButtons);

    sessionSummaryLabel = new QLabel("0 Spieler ausgewählt", sessionBox);
    sessionLayout->addWidget(sessionSummaryLabel);

    connect(sessionTemplateCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int)
            { applySessionTemplateSelection(); });
    connect(sessionNameEdit, &QLineEdit::textChanged, this, [this](const QString &)
            { updateSessionSummary(); });
    connect(sessionDateEdit, &QDateEdit::dateChanged, this, [this](const QDate &)
            { updateSessionSummary(); });
    connect(sessionSelectBtn, &QPushButton::clicked, this, &MainWindow::applySessionSelectionFromTable);
    connect(sessionApplyBtn, &QPushButton::clicked, this, &MainWindow::commitSessionAssignment);
    connect(addSessionPlayerBtn, &QPushButton::clicked, this, &MainWindow::addPlayerToSession);
    connect(testCounterBtn, &QPushButton::clicked, this, [this]()
            {
        // Erhöhe Counter für ausgewählten oder ersten Spieler in der oberen "Keine Antwort" Liste
        QListWidgetItem *item = sessionNoResponseList->currentItem();
        if (!item && sessionNoResponseList->count() > 0) {
            item = sessionNoResponseList->item(0);
            sessionNoResponseList->setCurrentItem(item);
        }
        if (!item) {
            QMessageBox::information(this, "Test Counter", "Keine Spieler in der oberen Liste 'Keine Antwort'.");
            return;
        }
        QString text = item->text();
        // Extrahiere reinen Namen vor dem Suffix " (NR: X)"
        QString playerName = text;
        int paren = text.indexOf(" (NR:");
        if (paren > 0) playerName = text.left(paren);

        bool found = false;
        for (Player &p : list.players) {
            if (p.name == playerName) {
                found = true;
                p.noResponseCounter++;
                int row = rowForPlayerKey(playerName);
                if (row >= 0) validateRow(row);
                break;
            }
        }
        if (!found) {
            // Spieler existiert nicht in Stammliste: anlegen mit Counter=1
            Player np; np.name = playerName; np.group = unassignedGroupName; np.noResponseCounter = 1; np.joinDate = nowDate();
            list.players.push_back(np);
            addPlayerToModel(np);
            validateRow(model->rowCount()-1);
            savePlayers();
        } else {
            savePlayers();
        }
        // Sicherstellen, dass Spieler im oberen Bereich 'Keine Antwort' erfasst ist
        sessionPlayerStatus[playerName] = ResponseStatus::NoResponse;
        refreshSessionPlayerLists();
        // UI-Text aktualisieren mit neuem Counter
        int cnt = 0;
        for (const Player &p : list.players) { if (p.name == playerName) { cnt = p.noResponseCounter; break; } }
        item->setText(QStringLiteral("%1 (NR: %2)").arg(playerName).arg(cnt));
        QMessageBox::information(this, "Test Counter", QStringLiteral("Counter für '%1' ist jetzt %2").arg(playerName).arg(cnt));
        updateSessionSummary(); });
    connect(removePlayerBtn, &QPushButton::clicked, this, &MainWindow::removePlayerFromSession);
    connect(markConfirmedBtn, &QPushButton::clicked, this, &MainWindow::movePlayerToConfirmed);
    connect(markDeclinedBtn, &QPushButton::clicked, this, &MainWindow::movePlayerToDeclined);
    connect(sessionUploadBtn, &QPushButton::clicked, this, [this]()
            {
        QFileDialog::Options opts;
        opts |= QFileDialog::DontUseNativeDialog;
        QString file = QFileDialog::getOpenFileName(this, QStringLiteral("Bild mit Spielerliste wählen"), QDir::homePath(),
                                                    QStringLiteral("Bilder (*.png *.jpg *.jpeg *.bmp *.tif *.tiff);;Alle Dateien (*.*)"), nullptr, opts);
        if (file.isEmpty())
            return;
        QProcess proc;
        QString program = QStringLiteral("tesseract");
        QStringList args;
        args << file << QStringLiteral("stdout") << QStringLiteral("-l") << QStringLiteral("deu");
        proc.start(program, args);
        if (!proc.waitForStarted(5000))
        {
            QMessageBox::warning(this, QStringLiteral("Tesseract fehlt"),
                                 QStringLiteral("Tesseract konnte nicht gestartet werden. Bitte installieren (z.B. via Homebrew: brew install tesseract)."));
            return;
        }
        proc.waitForFinished(-1);
        QByteArray out = proc.readAllStandardOutput();
        QByteArray err = proc.readAllStandardError();
        if (proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0)
        {
            QMessageBox::warning(this, QStringLiteral("OCR-Fehler"),
                                 QStringLiteral("Die Texterkennung ist fehlgeschlagen.\n%1").arg(QString::fromUtf8(err)));
            return;
        }
        QString textRaw = QString::fromUtf8(out);
        QString textLower = textRaw.toLower();

        // Event-Metadaten aus OCR extrahieren (falls vorhanden)
        QString extractedEventName;
        QDate extractedDate;
        QString extractedMap;
        bool isTraining = false;
        
        QStringList allLines = textRaw.split(QRegularExpression("[\\r\\n]+"), Qt::SkipEmptyParts);
        
        // Training vs Event Erkennung
        QStringList trainingKeywords = {"training", "clantraining", "freitagstraining", "montagstraining", "übung", "practice", "drill"};
        QStringList eventKeywords = {"event", "vs", "versus", "match", "scrim", "scrimmage", "gegen"};
        for (const QString &line : allLines) {
            QString lineLower = line.toLower();
            for (const QString &kw : trainingKeywords) {
                if (lineLower.contains(kw)) { isTraining = true; break; }
            }
            if (!isTraining) {
                for (const QString &kw : eventKeywords) {
                    if (lineLower.contains(kw)) { isTraining = false; break; }
                }
            }
        }
        
        // Parse Zusagen/Absagen
        QSet<QString> acceptedPlayers, rejectedPlayers;
        QRegularExpression acceptedRx(QStringLiteral(R"(Akzeptiert\s*\((\d+)\))"), QRegularExpression::CaseInsensitiveOption);
        QRegularExpression tankRx(QStringLiteral(R"(Tank\s*\((\d+)\))"), QRegularExpression::CaseInsensitiveOption);
        QRegularExpression rejectedRx(QStringLiteral(R"(Abgelehnt\s*\((\d+)\))"), QRegularExpression::CaseInsensitiveOption);
        int acceptedIdx = -1, tankIdx = -1, rejectedIdx = -1;
        for (int i = 0; i < allLines.size(); ++i) {
            QString line = allLines[i];
            if (acceptedRx.match(line).hasMatch()) acceptedIdx = i;
            if (tankRx.match(line).hasMatch()) tankIdx = i;
            if (rejectedRx.match(line).hasMatch()) rejectedIdx = i;
        }
        if (acceptedIdx >= 0) {
            int nextIdx = qMin(tankIdx >= 0 ? tankIdx : allLines.size(), rejectedIdx >= 0 ? rejectedIdx : allLines.size());
            for (int i = acceptedIdx + 1; i < nextIdx && i < allLines.size(); ++i) {
                QString line = allLines[i].trimmed();
                if (line.length() >= 3 && line.length() <= 48) acceptedPlayers.insert(line);
            }
        }
        if (tankIdx >= 0) {
            int nextIdx = rejectedIdx >= 0 ? rejectedIdx : allLines.size();
            for (int i = tankIdx + 1; i < nextIdx && i < allLines.size(); ++i) {
                QString line = allLines[i].trimmed();
                if (line.length() >= 3 && line.length() <= 48) acceptedPlayers.insert(line);
            }
        }
        if (rejectedIdx >= 0) {
            for (int i = rejectedIdx + 1; i < allLines.size(); ++i) {
                QString line = allLines[i].trimmed();
                if (line.length() >= 3 && line.length() <= 48) rejectedPlayers.insert(line);
            }
        }
        
        if (!allLines.isEmpty()) {
            QString firstLine = allLines.first().trimmed();
            if (firstLine.length() >= 5 && firstLine.length() <= 80) {
                extractedEventName = firstLine;
            }
        }
        
        QRegularExpression dateRx(QStringLiteral(R"(\b(\d{1,2})[\.\-/](\d{1,2})[\.\-/](\d{4})\b)"));
        QRegularExpressionMatch dateMatch = dateRx.match(textRaw);
        if (dateMatch.hasMatch()) {
            int day = dateMatch.captured(1).toInt();
            int month = dateMatch.captured(2).toInt();
            int year = dateMatch.captured(3).toInt();
            QDate parsedDate(year, month, day);
            if (parsedDate.isValid()) {
                extractedDate = parsedDate;
            }
        }
        
        QStringList knownMaps = {"SME", "Carentan", "Foy", "Kursk", "Stalingrad", "Omaha", "Utah", 
                                 "Purple Heart Lane", "Hill 400", "Hurtgen", "Sainte", "SMDM"};
        for (const QString &map : knownMaps) {
            if (textRaw.contains(map, Qt::CaseInsensitive)) {
                extractedMap = map;
                break;
            }
        }

        // Levenshtein-Distanz Hilfsfunktion
        auto levenshtein = [](const QString &s1, const QString &s2) -> int {
            const int m = s1.size(), n = s2.size();
            QVector<QVector<int>> dp(m+1, QVector<int>(n+1));
            for (int i=0; i<=m; ++i) dp[i][0] = i;
            for (int j=0; j<=n; ++j) dp[0][j] = j;
            for (int i=1; i<=m; ++i) {
                for (int j=1; j<=n; ++j) {
                    int cost = (s1[i-1] == s2[j-1]) ? 0 : 1;
                    dp[i][j] = std::min({dp[i-1][j]+1, dp[i][j-1]+1, dp[i-1][j-1]+cost});
                }
            }
            return dp[m][n];
        };

        QSet<QString> recognized;
        QMap<QString, QString> existingByLower;
        for (const Player &p : list.players)
        {
            if (!p.name.isEmpty()) existingByLower.insert(p.name.toLower(), p.name);
            if (!p.t17name.isEmpty()) existingByLower.insert(p.t17name.toLower(), p.name);
            const QString name = p.name.toLower();
            const QString t17 = p.t17name.toLower();
            if ((!name.isEmpty() && textLower.contains(name)) || (!t17.isEmpty() && textLower.contains(t17)))
                recognized.insert(p.name);
        }
        
        // Rang-Präfixe zum Trennen von Namen
        QStringList rankPrefixes = MainWindow::rankOptions();
        
        // Hilfsfunktion: Trenne Namen mit mehreren Rängen oder Leerzeichen + Großbuchstabe
        auto splitNames = [&rankPrefixes](const QString &text) -> QStringList {
            QStringList result;
            QString current = text.trimmed();
            
            // Finde alle Vorkommen von Rängen
            QList<int> rankPositions;
            for (const QString &rank : rankPrefixes) {
                if (rank.isEmpty()) continue;
                QRegularExpression rx(QRegularExpression::escape(rank), 
                                     QRegularExpression::CaseInsensitiveOption);
                QRegularExpressionMatchIterator it = rx.globalMatch(current);
                while (it.hasNext()) {
                    QRegularExpressionMatch match = it.next();
                    rankPositions << match.capturedStart();
                }
            }
            
            // Wenn mehrere Ränge: trenne bei jedem Rang
            if (rankPositions.size() > 1) {
                std::sort(rankPositions.begin(), rankPositions.end());
                for (int i = 0; i < rankPositions.size(); ++i) {
                    int start = rankPositions[i];
                    int end = (i + 1 < rankPositions.size()) ? rankPositions[i + 1] : current.length();
                    QString segment = current.mid(start, end - start).trimmed();
                    if (!segment.isEmpty() && segment.length() >= 3)
                        result << segment;
                }
                return result;
            }
            
            // Fallback: Trenne bei Leerzeichen + Kleinbuchstabe oder Großbuchstabe
            QRegularExpression splitPattern1("\\s+(?=[a-zäöüß])");
            QStringList parts1 = current.split(splitPattern1, Qt::SkipEmptyParts);
            if (parts1.size() > 1) {
                for (const QString &p : parts1) {
                    if (p.trimmed().length() >= 3)
                        result << p.trimmed();
                }
                if (!result.isEmpty()) return result;
            }
            
            QRegularExpression splitPattern2("\\s+(?=[A-ZÄÖÜ])");
            QStringList parts2 = current.split(splitPattern2, Qt::SkipEmptyParts);
            if (parts2.size() > 1) {
                for (const QString &p : parts2) {
                    if (p.trimmed().length() >= 3)
                        result << p.trimmed();
                }
                if (!result.isEmpty()) return result;
            }
            
            if (!current.isEmpty() && current.length() >= 3)
                result << current;
            return result;
        };
        
        // Alle Spieler aus acceptedPlayers und rejectedPlayers sammeln (mit Trennung)
        QSet<QString> allParsedPlayers;
        for (const QString &player : acceptedPlayers) {
            for (const QString &split : splitNames(player))
                allParsedPlayers.insert(split);
        }
        for (const QString &player : rejectedPlayers) {
            for (const QString &split : splitNames(player))
                allParsedPlayers.insert(split);
        }
        
        // Kandidaten aus Zeilen/Kommas bilden
        QSet<QString> candidates;
        auto addCandidate = [&candidates](const QString &s){ QString c=s; c.replace('\t',' '); c.replace(QRegularExpression("\\s+"), " "); c=c.trimmed(); if(c.size()>=3 && c.size()<=48) candidates.insert(c); };
        
        // Hilfsfunktion: Teile String bei mehrfach vorkommenden Rängen
        auto splitByRanks = [&rankPrefixes](const QString &text) -> QStringList {
            QStringList result;
            QString current = text.trimmed();
            
            // Finde alle Vorkommen von Rängen im Text
            QList<int> rankPositions;
            QList<QString> foundRanks;
            
            for (const QString &rank : rankPrefixes) {
                if (rank.isEmpty()) continue;
                
                // Suche nach Rang-Vorkommen (case-insensitive)
                QRegularExpression rx(QRegularExpression::escape(rank), 
                                     QRegularExpression::CaseInsensitiveOption);
                QRegularExpressionMatchIterator it = rx.globalMatch(current);
                while (it.hasNext()) {
                    QRegularExpressionMatch match = it.next();
                    rankPositions << match.capturedStart();
                    foundRanks << rank;
                }
            }
            
            // Wenn mehr als ein Rang gefunden wurde, trenne bei jedem Rang
            if (rankPositions.size() > 1) {
                // Sortiere nach Position
                QList<QPair<int, QString>> posRankPairs;
                for (int i = 0; i < rankPositions.size(); ++i) {
                    posRankPairs << qMakePair(rankPositions[i], foundRanks[i]);
                }
                std::sort(posRankPairs.begin(), posRankPairs.end());
                
                for (int i = 0; i < posRankPairs.size(); ++i) {
                    int start = posRankPairs[i].first;
                    int end = (i + 1 < posRankPairs.size()) ? posRankPairs[i + 1].first : current.length();
                    QString segment = current.mid(start, end - start).trimmed();
                    if (!segment.isEmpty())
                        result << segment;
                }
                return result;
            }
            
            // Fallback: Trenne bei Leerzeichen gefolgt von Kleinbuchstabe oder Großbuchstabe
            // Dies erfasst sowohl "GefrBuddy eiben" als auch "Buddy Eiben" (Namen ohne Rang)
            if (!current.isEmpty()) {
                // Versuche zuerst Trennung bei Leerzeichen + Kleinbuchstabe
                QRegularExpression splitPattern1("\\s+(?=[a-zäöüß])");
                QStringList parts1 = current.split(splitPattern1, Qt::SkipEmptyParts);
                if (parts1.size() > 1) {
                    result = parts1;
                    return result;
                }
                
                // Dann versuche Trennung bei Leerzeichen + Großbuchstabe (für Namen ohne Rang)
                // Aber nur wenn kein Rang am Anfang des zweiten Teils steht
                QRegularExpression splitPattern2("\\s+(?=[A-ZÄÖÜ])");
                QStringList parts2 = current.split(splitPattern2, Qt::SkipEmptyParts);
                if (parts2.size() > 1) {
                    // Prüfe ob Teile valide Namen sind (nicht nur einzelne Buchstaben)
                    bool allValid = true;
                    for (const QString &part : parts2) {
                        if (part.trimmed().length() < 2) {
                            allValid = false;
                            break;
                        }
                    }
                    if (allValid) {
                        result = parts2;
                        return result;
                    }
                }
            }
            
            if (result.isEmpty())
                result << current;
            return result;
        };
        
        // Falls wir Status-Kategorien gefunden haben, verwende diese als Kandidaten
        if (!allParsedPlayers.isEmpty()) {
            for (const QString &player : allParsedPlayers) {
                QStringList subParts = splitByRanks(player);
                for (const QString &sub : subParts) addCandidate(sub);
            }
        }
        
        // Falls keine Status-Kategorien gefunden wurden, verwende Fallback
        if (candidates.isEmpty()) {
            const QStringList lines = textRaw.split(QRegularExpression("[\\r\\n]+"), Qt::SkipEmptyParts);
            for (const QString &ln : lines) {
                const QStringList parts = ln.split(',', Qt::SkipEmptyParts);
                if (parts.size()>1) { 
                    for (const QString &p : parts) {
                        QStringList subParts = splitByRanks(p);
                        for (const QString &sub : subParts) addCandidate(sub);
                    }
                }
                else { 
                    QStringList subParts = splitByRanks(ln);
                    for (const QString &sub : subParts) addCandidate(sub);
                }
            }
        }
        // Unbekannte Spieler automatisch anlegen in Gruppe "Nicht zugewiesen" (mit Fuzzy-Matching)
        const QString unassignedGroup = QStringLiteral("Nicht zugewiesen");
        QSet<QString> created;
        constexpr int fuzzyThreshold = 2;
        for (const QString &cand : std::as_const(candidates))
        {
            const QString lower = cand.toLower();
            // Prüfe ob Spieler zugesagt hat
            bool isAccepted = false;
            for (const QString &acc : acceptedPlayers) {
                if (levenshtein(acc.toLower(), lower) <= 1) {
                    isAccepted = true;
                    break;
                }
            }
            // Exakte Übereinstimmung prüfen
            if (existingByLower.contains(lower)) {
                if (isAccepted) recognized.insert(existingByLower.value(lower));
                continue;
            }
            // Fuzzy-Matching gegen alle vorhandenen Namen
            bool fuzzyMatch = false;
            for (auto it = existingByLower.cbegin(); it != existingByLower.cend(); ++it)
            {
                if (levenshtein(lower, it.key()) <= fuzzyThreshold)
                {
                    fuzzyMatch = true;
                    if (isAccepted) recognized.insert(it.value());
                    break;
                }
            }
            if (fuzzyMatch)
                continue;
            Player np; np.name = cand; np.group = unassignedGroup; np.joinDate = nowDate();
            if (!MainWindow::rankOptions().isEmpty()) np.rank = MainWindow::rankOptions().first();
            np.totalAttendance = qMax(np.totalAttendance, np.attendance);
            np.totalEvents = qMax(np.totalEvents, np.events);
            np.totalReserve = qMax(np.totalReserve, np.reserve);
            list.players.push_back(np);
            if (ensureGroupRegistered(unassignedGroup)) saveGroups();
            addPlayerToModel(np); validateRow(model->rowCount()-1);
            existingByLower.insert(lower, np.name);
            created.insert(np.name);
            if (isAccepted) recognized.insert(np.name);
        }
        if (!created.isEmpty()) savePlayers();
        
        // Fülle sessionPlayerStatus basierend auf OCR-Ergebnissen
        sessionPlayerStatus.clear();

        // Hilfsfunktion: Zeile in mehrere Spieler auftrennen, wenn mehrere Ränge vorkommen
        auto splitByMultipleRanksLine = [this](const QString &line) -> QStringList {
            const QStringList ranks = MainWindow::rankOptions();
            QString text = line.trimmed();
            QList<int> pos;
            for (const QString &r : ranks) {
                if (r.isEmpty()) continue;
                QRegularExpression rx(QRegularExpression::escape(r), QRegularExpression::CaseInsensitiveOption);
                auto it = rx.globalMatch(text);
                while (it.hasNext()) {
                    auto m = it.next();
                    pos << m.capturedStart();
                }
            }
            if (pos.size() <= 1)
                return {text};
            std::sort(pos.begin(), pos.end());
            QStringList out;
            for (int i = 0; i < pos.size(); ++i) {
                int start = pos[i];
                int end = (i+1<pos.size()) ? pos[i+1] : text.size();
                QString seg = text.mid(start, end-start).trimmed();
                if (!seg.isEmpty()) out << seg;
            }
            return out;
        };

        // Zugesagte Spieler (accepted)
        for (const QString &raw : acceptedPlayers)
        {
            for (const QString &playerName : splitByMultipleRanksLine(raw))
                sessionPlayerStatus.insert(playerName, ResponseStatus::Confirmed);
        }

        // Abgelehnte Spieler (rejected)
        for (const QString &raw : rejectedPlayers)
        {
            for (const QString &playerName : splitByMultipleRanksLine(raw))
                sessionPlayerStatus.insert(playerName, ResponseStatus::Declined);
        }
        
        // WICHTIG: Alle anderen Spieler aus der Gesamtliste als "Keine Antwort" erfassen
        for (const Player &p : list.players)
        {
            // Überspringe Spieler, die bereits zu-/abgesagt haben
            if (sessionPlayerStatus.contains(p.name))
                continue;
            
            // Füge Spieler als "Keine Antwort" hinzu
            sessionPlayerStatus.insert(p.name, ResponseStatus::NoResponse);
        }
        
        // Aktualisiere die drei Listen
        refreshSessionPlayerLists();
        
        // Automatisch erkannte Event-Daten in Session-Felder übernehmen
        bool dataApplied = false;
        if (!extractedEventName.isEmpty()) {
            sessionNameEdit->setText(extractedEventName);
            dataApplied = true;
        }
        if (extractedDate.isValid()) {
            sessionDateEdit->setDate(extractedDate);
            dataApplied = true;
        }
        
        int totalParsed = acceptedPlayers.size() + rejectedPlayers.size();
        QString infoMsg = QStringLiteral("%1 Spieler erkannt (%2 zugesagt, %3 abgesagt).\n%4 Spieler für Session markiert, %5 neu angelegt.")
            .arg(totalParsed > 0 ? totalParsed : (recognized.size() + created.size()))
            .arg(acceptedPlayers.size())
            .arg(rejectedPlayers.size())
            .arg(recognized.size())
            .arg(created.size());
        
        if (isTraining) {
            infoMsg += QStringLiteral("\n\n📋 Typ: TRAINING");
        } else {
            infoMsg += QStringLiteral("\n\n🎮 Typ: EVENT");
        }
        if (dataApplied) {
            infoMsg += QStringLiteral("\n\nEvent-Daten automatisch übernommen:");
            if (!extractedEventName.isEmpty())
                infoMsg += QStringLiteral("\n• Event: %1").arg(extractedEventName);
            if (extractedDate.isValid())
                infoMsg += QStringLiteral("\n• Datum: %1").arg(extractedDate.toString("dd.MM.yyyy"));
            if (!extractedMap.isEmpty())
                infoMsg += QStringLiteral("\n• Map: %1 (in Notizen einfügen)").arg(extractedMap);
        }
        
        QMessageBox::information(this, QStringLiteral("OCR Import"), infoMsg);
        updateSessionSummary(); });
    connect(sessionPlayerTree, &QTreeWidget::itemChanged, this, [this](QTreeWidgetItem *item)
            {
        if (!item)
            return;
        QString key = item->data(0, Qt::UserRole).toString();
        if (key.isEmpty())
            return;
        if (item->checkState(0) == Qt::Checked)
            sessionSelectedPlayers.insert(key);
        else
            sessionSelectedPlayers.remove(key);
        updateSessionSummary(); });

    QGroupBox *menuBox = new QGroupBox("Menü", this);
    QGridLayout *menuLayout = new QGridLayout(menuBox);
    menuLayout->setContentsMargins(8, 8, 8, 8);
    menuLayout->setHorizontalSpacing(8);
    menuLayout->setVerticalSpacing(8);

    QList<QPushButton *> menuButtons = {
        refreshBtn,
        addPlayerBtn,
        editPlayerBtn,
        manageGroupsBtn,
        settingsBtn};

    const int columns = 3;
    for (int i = 0; i < menuButtons.size(); ++i)
    {
        int row = i / columns;
        int column = i % columns;
        menuLayout->addWidget(menuButtons.at(i), row, column);
    }

    // Persönlicher Hinweis im Menü
    QLabel *creditLbl = new QLabel(QStringLiteral("Created by buddy für pg60"), menuBox);
    creditLbl->setAlignment(Qt::AlignRight);
    int nextRow = (menuButtons.size() + columns - 1) / columns; // erste freie Zeile unter den Buttons
    menuLayout->addWidget(creditLbl, nextRow, 0, 1, columns);

    QVBoxLayout *l = new QVBoxLayout;
    l->addLayout(filterLay);
    l->addWidget(table, 15);
    l->addWidget(menuBox, 0);
    // Unterer Bereich "Letztes Gefecht / Training" (vergrößert)
    l->addWidget(sessionBox, 3);
    central->setLayout(l);

    // wire search and filter to proxy
    auto *sp = static_cast<SortProxy *>(proxy);
    connect(searchEdit, &QLineEdit::textChanged, this, [sp](const QString &txt)
            { sp->setTextFilter(txt); });
    connect(rankFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [sp, this](int idx)
            {
        if (idx == 0) { sp->setRankFilter(""); return; }
        if (idx == 1) { sp->setRankFilter("Nur Offiziere"); return; }
        QVariant d = rankFilterCombo->itemData(idx, Qt::UserRole);
        QString raw = d.isValid() ? d.toString() : rankFilterCombo->itemText(idx);
        sp->setRankFilter(raw); });
    connect(groupFilterCombo, &QComboBox::currentTextChanged, this, [sp](const QString &txt)
            { sp->setGroupFilter(txt); });
    auto sortChanged = [this]()
    {
        applySortSettings();
    };
    connect(sortFieldCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [sortChanged](int)
            { sortChanged(); });
    connect(sortOrderCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [sortChanged](int)
            { sortChanged(); });
    connect(filterModeCombo, &QComboBox::currentTextChanged, this, [this, rankLbl, groupLbl, sp](const QString &mode)
            {
        bool isGroup = (mode == "Gruppe");
        if (rankLbl) rankLbl->setVisible(!isGroup);
        if (groupLbl) groupLbl->setVisible(isGroup);
        if (rankFilterCombo) rankFilterCombo->setVisible(!isGroup);
        if (groupFilterCombo) groupFilterCombo->setVisible(isGroup);
        if (isGroup) sp->setGroupFilter(groupFilterCombo->currentText()); else sp->setRankFilter(rankFilterCombo->currentText()); });

    connect(useTestDate, &QCheckBox::toggled, this, [this](bool checked)
            {
        if (testDateEdit)
            testDateEdit->setEnabled(checked);
        validateAllRows(); });
    connect(testDateEdit, &QDateEdit::dateChanged, this, [this](const QDate &)
            {
        if (useTestDate && useTestDate->isChecked())
            validateAllRows(); });

    setWindowTitle("ClanManager - Baseline");
    resize(900, 600);
}

void MainWindow::loadDataFiles()
{
    qDebug() << "loadDataFiles: Loading settings and data...";
    
    // Load settings with error handling
    try {
        loadSettings();
    } catch (...) {
        qWarning() << "Failed to load settings, using defaults";
    }
    
    try {
        loadRankRequirements();
    } catch (...) {
        qWarning() << "Failed to load rank requirements, using defaults";
    }
    
    try {
        loadMaps();
    } catch (...) {
        qWarning() << "Failed to load maps, using defaults";
    }
    
    try {
        loadTrainings();
    } catch (...) {
        qWarning() << "Failed to load trainings";
    }
    
    try {
        loadGroups();
    } catch (...) {
        qWarning() << "Failed to load groups, using defaults";
    }
    
    try {
        loadGroupColors();
    } catch (...) {
        qWarning() << "Failed to load group colors";
    }
    
    try {
        loadAttendance();
    } catch (...) {
        qWarning() << "Failed to load attendance data";
    }
    
    try {
        loadSoldbuch();
    } catch (...) {
        qWarning() << "Failed to load soldbuch data";
    }
    
    try {
        loadPlayers();
    } catch (...) {
        qWarning() << "Failed to load players";
    }
    
    // Only call these if widgets are initialized
    if (model && table) {
        try {
            updateGroupDecorations();
        } catch (...) {
            qWarning() << "Failed to update group decorations";
        }
        
        try {
            validateAllRows();
        } catch (...) {
            qWarning() << "Failed to validate rows";
        }
        
        try {
            applySortSettings();
        } catch (...) {
            qWarning() << "Failed to apply sort settings";
        }
    }
    
    try {
        refreshSessionMapCombo();
    } catch (...) {
        qWarning() << "Failed to refresh session map combo";
    }
    
    try {
        refreshSessionTemplates();
    } catch (...) {
        qWarning() << "Failed to refresh session templates";
    }
    
    try {
        refreshSessionPlayerTable();
    } catch (...) {
        qWarning() << "Failed to refresh session player table";
    }
    
    try {
        applySettingsToUI();
    } catch (...) {
        qWarning() << "Failed to apply settings to UI";
    }
    
    qDebug() << "loadDataFiles: All data files processed";
}

int MainWindow::monthsSinceJoin(const QDate &joinDate)
{
    if (!joinDate.isValid())
        return -1;
    QDate now = nowDate();
    int months = (now.year() - joinDate.year()) * 12 + (now.month() - joinDate.month());
    if (now.day() < joinDate.day())
        months -= 1;
    return qMax(0, months);
}

int MainWindow::monthsRequiredForRank(const QString &rank)
{
    return requirementForRank(rank).minMonths;
}

bool MainWindow::validateRow(int row, QString *outReason)
{
    if (row < 0 || row >= model->rowCount())
        return true;
    QStandardItem *joinItem = model->item(row, 7);
    QStandardItem *rankItem = model->item(row, 8);
    if (!joinItem || !rankItem)
        return true;
    QString joinStr = joinItem->text();
    QString rank = rankItem->data(Qt::EditRole).toString();
    if (rank.isEmpty())
        rank = rankItem->text();
    QDate joinDate = QDate::fromString(joinStr, Qt::ISODate);
    RankRequirement req = requirementForRank(rank);

    // reset background first
    for (int c = 0; c < model->columnCount(); ++c)
        if (QStandardItem *it = model->item(row, c))
            it->setBackground(Qt::NoBrush);

    QStringList reasons;
    // Entferne auffällige Hintergrundfarbe für Warnungen
    QColor warnColor; // ungenutzt
    bool ok = true;

    if (req.minMonths > 0)
    {
        if (!joinDate.isValid())
        {
            reasons << QString("kein Beitrittsdatum (benötigt: %1 Monate)").arg(req.minMonths);
            ok = false;
        }
        else
        {
            int months = monthsSinceJoin(joinDate);
            if (months < req.minMonths)
            {
                reasons << QString("nur %1 Monate im Dienst (benötigt: %2)").arg(months).arg(req.minMonths);
                ok = false;
            }
        }
    }

    Player player = playerFromModelRow(row);
    int playerLevel = player.level;
    if (playerLevel <= 0)
    {
        if (QStandardItem *levelItem = model->item(row, 3))
            playerLevel = levelItem->text().toInt();
    }
    if (req.minLevel > 0 && playerLevel < req.minLevel)
    {
        reasons << QString("Level %1/%2").arg(playerLevel).arg(req.minLevel);
        ok = false;
    }

    QString key = playerKeyForRow(row);
    if (!key.isEmpty() && req.minCombined > 0)
    {
        AttendanceSummary stats = attendanceSummaryForPlayer(key, nowDate());
        int combined = stats.trainings + stats.events + stats.reserve;
        if (combined < req.minCombined)
        {
            reasons << QString("T+E+R: %1/%2").arg(combined).arg(req.minCombined);
            ok = false;
        }
    }

    // Bei Nichterfüllung der Anforderungen wird kein Hintergrund mehr gesetzt
    // (Anzeige bleibt neutral; Details stehen im Tooltip/Status)

    bool hasRequirement = (req.minMonths > 0 || req.minCombined > 0 || req.minLevel > 0);
    updatePromotionIndicatorForRow(row, ok && hasRequirement);

    if (outReason)
        *outReason = reasons.join(", ");
    return ok;
}

void MainWindow::validateAllRows()
{
    // Null check added to prevent crash
    if (!model || !table) {
        qWarning() << "validateAllRows: model or table is null, skipping";
        return;
    }
    
    if (!model)
        return;
    for (int row = 0; row < model->rowCount(); ++row)
        validateRow(row);
}

void MainWindow::refreshModelFromList()
{
    if (!model)
        return;
    model->removeRows(0, model->rowCount());
    for (const auto &p : list.players)
        addPlayerToModel(p);
    updateGroupDecorations();
    refreshSessionPlayerTable();
}

void MainWindow::refreshGroupFilterCombo()
{
    if (!groupFilterCombo)
        return;
    QString previous = groupFilterCombo->currentText();
    groupFilterCombo->blockSignals(true);
    groupFilterCombo->clear();
    groupFilterCombo->addItem("Alle Gruppen");
    for (const QString &groupName : groups)
        groupFilterCombo->addItem(groupName);
    int idx = groupFilterCombo->findText(previous, Qt::MatchExactly);
    groupFilterCombo->setCurrentIndex(idx >= 0 ? idx : 0);
    groupFilterCombo->blockSignals(false);

    if (filterModeCombo && filterModeCombo->currentText() == "Gruppe")
    {
        if (auto *sp = dynamic_cast<SortProxy *>(proxy))
            sp->setGroupFilter(groupFilterCombo->currentText());
    }
}

bool MainWindow::ensureGroupRegistered(const QString &groupName, const QString &category)
{
    const QString trimmed = groupName.trimmed();
    if (trimmed.isEmpty())
        return false;

    int existingIndex = indexOfGroupName(groups, trimmed);
    if (existingIndex >= 0)
    {
        if (!category.isEmpty())
            groupCategory.insert(groups.at(existingIndex), category);
        return false;
    }

    groups.append(trimmed);
    if (!category.isEmpty())
        groupCategory.insert(trimmed, category);
    std::sort(groups.begin(), groups.end(), [](const QString &a, const QString &b)
              { return a.compare(b, Qt::CaseInsensitive) < 0; });
    groups.erase(std::unique(groups.begin(), groups.end(), [](const QString &a, const QString &b)
                             { return a.compare(b, Qt::CaseInsensitive) == 0; }),
                 groups.end());
    refreshGroupFilterCombo();
    return true;
}

void MainWindow::addPlayerToModel(const Player &p)
{
    if (!model)
        return;
    QList<QStandardItem *> row;
    row << new QStandardItem(p.name)
        << new QStandardItem()
        << new QStandardItem(p.t17name)
        << new QStandardItem(QString::number(p.level))
        << new QStandardItem(p.group)
        << new QStandardItem(formatTrainingDisplay(p))
        << new QStandardItem(p.comment)
        << new QStandardItem(p.joinDate.toString(Qt::ISODate))
        << new QStandardItem()
        << new QStandardItem();
    const QString key = p.name;
    for (QStandardItem *item : row)
    {
        if (item)
            item->setData(key, Qt::UserRole + 1);
    }
    if (QStandardItem *statusItem = row.value(1))
    {
        statusItem->setEditable(false);
        statusItem->setTextAlignment(Qt::AlignCenter);
        // Rotes X wenn noResponseCounter >= threshold
        if (p.noResponseCounter >= noResponseThreshold)
        {
            statusItem->setText("❌");
            statusItem->setForeground(QBrush(Qt::red));
        }
    }
    if (QStandardItem *trainingItem = row.value(5))
        trainingItem->setToolTip(trainingTooltip(p));
    if (QStandardItem *rankItem = row.value(8))
    {
        rankItem->setEditable(true);
        rankItem->setData(p.rank, Qt::EditRole);
        rankItem->setData(formatRankDisplay(p, false), Qt::DisplayRole);
    }
    if (QStandardItem *actionItem = row.last())
        actionItem->setEditable(false);
    model->appendRow(row);
    updateGroupDecorations();
}

QString MainWindow::formatTrainingDisplay(const Player &p) const
{
    // Einsätze = Training + Event + Reserve
    int currentCombined = p.attendance + p.events + p.reserve;
    int totalCombined = qMax(p.attendance, p.totalAttendance) + qMax(p.events, p.totalEvents) + qMax(p.reserve, p.totalReserve);
    if (showCounterInTable)
        return QStringLiteral("%1 / %2 [%3]").arg(currentCombined).arg(totalCombined).arg(p.noResponseCounter);
    return QStringLiteral("%1 / %2").arg(currentCombined).arg(totalCombined);
}

QString MainWindow::trainingTooltip(const Player &p) const
{
    int currentCombined = p.attendance + p.events + p.reserve;
    int totalCombined = qMax(p.attendance, p.totalAttendance) + qMax(p.events, p.totalEvents) + qMax(p.reserve, p.totalReserve);
    return QStringLiteral(
               "Einsätze (Training+Event+Reserve): %1 (seit letzter Beförderung/Erstellung) / %2 (Gesamt)\n"
               "Training: %3 / %4  Event: %5 / %6  Reserve: %7 / %8")
        .arg(currentCombined)
        .arg(totalCombined)
        .arg(p.attendance)
        .arg(qMax(p.attendance, p.totalAttendance))
        .arg(p.events)
        .arg(qMax(p.events, p.totalEvents))
        .arg(p.reserve)
        .arg(qMax(p.reserve, p.totalReserve));
}

QString MainWindow::formatRankDisplay(const Player &p, bool eligible) const
{
    QStringList parts;
    QString rank = p.rank.isEmpty() ? QStringLiteral("-") : p.rank;
    parts << rank;
    if (p.level > 0)
        parts << QStringLiteral("Level %1").arg(p.level);
    QString text = parts.join(QStringLiteral(" · "));
    if (eligible)
        text = QStringLiteral("★ %1").arg(text);
    return text;
}

Player MainWindow::playerFromModelRow(int row) const
{
    Player p;
    if (!model)
        return p;
    if (row < 0 || row >= model->rowCount())
        return p;
    QString key = playerKeyForRow(row);
    for (const Player &candidate : list.players)
    {
        if (candidate.name == key)
            return candidate;
    }
    return p;
}

Player *MainWindow::findPlayerByKey(const QString &playerKey)
{
    for (Player &player : list.players)
    {
        if (player.name == playerKey)
            return &player;
    }
    return nullptr;
}

int MainWindow::rowForPlayerKey(const QString &playerKey) const
{
    if (!model)
        return -1;
    for (int row = 0; row < model->rowCount(); ++row)
    {
        if (playerKeyForRow(row) == playerKey)
            return row;
    }
    return -1;
}

bool MainWindow::playerContextForRow(int sourceRow, QString &playerKey, QString &playerName) const
{
    if (!model || sourceRow < 0 || sourceRow >= model->rowCount())
        return false;
    playerKey = playerKeyForRow(sourceRow);
    if (playerKey.isEmpty())
        return false;
    if (QStandardItem *nameItem = model->item(sourceRow, 0))
        playerName = nameItem->text();
    if (playerName.isEmpty())
        playerName = playerKey;
    return true;
}

void MainWindow::handleAttendanceButtonForRow(int sourceRow, const QString &forcedType, bool quickAdd)
{
    QString playerKey;
    QString playerName;
    if (!playerContextForRow(sourceRow, playerKey, playerName))
        return;

    AttendanceDialogResult dialogResult;
    if (quickAdd)
    {
        dialogResult.type = forcedType.isEmpty() ? QStringLiteral("Training") : forcedType;
        dialogResult.name.clear();
        dialogResult.map.clear();
        dialogResult.date = nowDate();
    }
    else
    {
        if (!openAttendanceEntryDialog(playerName, dialogResult, forcedType, false))
            return;
    }

    auto ensureMapKnown = [this](const QString &map)
    {
        if (map.isEmpty())
            return;
        for (const QString &existing : maps)
        {
            if (existing.compare(map, Qt::CaseInsensitive) == 0)
                return;
        }
        maps.append(map);
        saveMaps();
        refreshSessionMapCombo();
    };
    ensureMapKnown(dialogResult.map);

    QDateTime timestamp(dialogResult.date, QTime::currentTime());
    appendAttendanceLog(playerKey, dialogResult.type.toLower(), timestamp, dialogResult.name, dialogResult.map);
    incrementPlayerCounters(playerKey, dialogResult.type);
    validateRow(sourceRow);
}

void MainWindow::incrementPlayerCounters(const QString &playerKey, const QString &type)
{
    Player *player = findPlayerByKey(playerKey);
    if (!player)
        return;
    QString lowerType = type.toLower();
    if (lowerType == "training")
    {
        player->attendance++;
        player->totalAttendance++;
    }
    else if (lowerType == "event")
    {
        player->events++;
        player->totalEvents++;
    }
    else if (lowerType == "reserve")
    {
        player->reserve++;
        player->totalReserve++;
    }

    // Zähler zurücksetzen bei An-/Abmeldung (optional)
    if (resetCounterOnResponse)
        player->noResponseCounter = 0;

    int row = rowForPlayerKey(playerKey);
    if (row >= 0)
    {
        if (QStandardItem *trainingItem = model->item(row, 5))
        {
            trainingItem->setText(formatTrainingDisplay(*player));
            trainingItem->setToolTip(trainingTooltip(*player));
        }
        // Update Hinweis-Spalte (rotes X entfernen)
        if (QStandardItem *statusItem = model->item(row, 1))
        {
            statusItem->setText(QString());
            statusItem->setForeground(QBrush(Qt::NoBrush));
        }
    }
    savePlayers();
}

void MainWindow::updatePromotionIndicatorForRow(int row, bool eligible)
{
    if (!model || row < 0 || row >= model->rowCount())
        return;

    Player player = playerFromModelRow(row);

    if (QStandardItem *statusItem = model->item(row, 1))
    {
        // Priorität: Rotes X bei noResponseCounter >= threshold
        if (player.noResponseCounter >= noResponseThreshold)
        {
            statusItem->setText("❌");
            statusItem->setForeground(QBrush(Qt::red));
            statusItem->setToolTip(QStringLiteral("Keine Rückmeldungen: %1 Mal").arg(player.noResponseCounter));
        }
        else if (eligible)
        {
            statusItem->setText(QStringLiteral("★"));
            statusItem->setForeground(QBrush(QColor(255, 215, 0)));
            statusItem->setToolTip("Beförderungsvoraussetzungen erfüllt");
        }
        else
        {
            statusItem->setText(QString());
            statusItem->setForeground(QBrush(Qt::NoBrush));
            statusItem->setToolTip(QString());
        }
    }
    if (!player.name.isEmpty())
    {
        if (QStandardItem *rankItem = model->item(row, 8))
        {
            rankItem->setData(player.rank, Qt::EditRole);
            rankItem->setData(formatRankDisplay(player, eligible), Qt::DisplayRole);
        }
    }
}

QString MainWindow::playerKeyForRow(int row) const
{
    if (!model || row < 0 || row >= model->rowCount())
        return {};
    for (int c = 0; c < model->columnCount(); ++c)
    {
        if (QStandardItem *item = model->item(row, c))
        {
            QString key = item->data(Qt::UserRole + 1).toString();
            if (!key.isEmpty())
                return key;
        }
    }
    QStandardItem *fallback = model->item(row, 0);
    return fallback ? fallback->text() : QString();
}

MainWindow::AttendanceSummary MainWindow::attendanceSummaryForPlayer(const QString &playerKey, const QDate &referenceDate) const
{
    AttendanceSummary summary;
    if (playerKey.isEmpty())
        return summary;
    for (const Player &player : list.players)
    {
        if (player.name == playerKey)
        {
            Q_UNUSED(referenceDate);
            summary.trainings = player.attendance;
            summary.events = player.events;
            summary.reserve = player.reserve;
            return summary;
        }
    }
    return summary;
}

bool MainWindow::openAttendanceEntryDialog(const QString &playerName, AttendanceDialogResult &result, const QString &forcedType, bool lockType)
{
    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("Eintrag für %1").arg(playerName));
    QFormLayout *form = new QFormLayout(&dlg);

    QComboBox *typeCombo = new QComboBox(&dlg);
    typeCombo->addItems({"Training", "Event", "Reserve"});
    if (!forcedType.isEmpty())
    {
        int idx = typeCombo->findText(forcedType, Qt::MatchFixedString);
        if (idx >= 0)
            typeCombo->setCurrentIndex(idx);
        else
            typeCombo->setCurrentText(forcedType);
        if (lockType)
            typeCombo->setEnabled(false);
    }
    form->addRow("Typ", typeCombo);

    QLineEdit *nameEdit = new QLineEdit(&dlg);
    nameEdit->setPlaceholderText("Name des Trainings/Event");
    form->addRow("Name", nameEdit);

    QComboBox *mapCombo = new QComboBox(&dlg);
    mapCombo->setEditable(true);
    QStringList mapList;
    for (const QString &m : maps)
        mapList << m;
    if (mapList.isEmpty())
        mapList << "Unbekannt";
    mapCombo->addItems(mapList);
    form->addRow("Karte", mapCombo);

    QDateEdit *dateEdit = new QDateEdit(&dlg);
    dateEdit->setCalendarPopup(true);
    dateEdit->setDisplayFormat("yyyy-MM-dd");
    dateEdit->setDate(nowDate());
    form->addRow("Datum", dateEdit);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted)
        return false;

    result.type = typeCombo->currentText().trimmed();
    if (result.type.isEmpty())
        result.type = "Training";
    result.name = nameEdit->text().trimmed();
    result.map = mapCombo->currentText().trimmed();
    result.date = dateEdit->date();
    return true;
}

bool MainWindow::openPlayerEditDialog(Player &player, bool editing)
{
    QDialog dlg(this);
    dlg.setWindowTitle(editing ? "Spieler bearbeiten" : "Spieler anlegen");
    QFormLayout *form = new QFormLayout(&dlg);

    QLineEdit *nameEdit = new QLineEdit(player.name, &dlg);
    nameEdit->setPlaceholderText("Spielername");
    form->addRow("Name", nameEdit);

    QLineEdit *t17Edit = new QLineEdit(player.t17name, &dlg);
    t17Edit->setPlaceholderText("T17 / Discord");
    form->addRow("T17", t17Edit);

    QSpinBox *levelSpin = new QSpinBox(&dlg);
    levelSpin->setRange(0, 9999);
    levelSpin->setValue(player.level);
    form->addRow("Level", levelSpin);

    QComboBox *groupCombo = new QComboBox(&dlg);
    groupCombo->setEditable(true);
    groupCombo->addItems(groups);
    groupCombo->setCurrentText(player.group);
    form->addRow("Gruppe", groupCombo);

    QComboBox *rankCombo = new QComboBox(&dlg);
    const QStringList rankChoices = MainWindow::rankOptions();
    rankCombo->addItems(rankChoices);
    int rankIndex = rankCombo->findText(player.rank);
    if (rankIndex < 0)
        rankIndex = 0;
    rankCombo->setCurrentIndex(rankIndex);
    form->addRow("Dienstrang", rankCombo);

    if (editing && !rankChoices.isEmpty())
    {
        QWidget *rankButtons = new QWidget(&dlg);
        QHBoxLayout *rankButtonsLayout = new QHBoxLayout(rankButtons);
        rankButtonsLayout->setContentsMargins(0, 0, 0, 0);
        QPushButton *promoteBtn = new QPushButton("Befördern", rankButtons);
        QPushButton *demoteBtn = new QPushButton("Degradieren", rankButtons);
        rankButtonsLayout->addWidget(promoteBtn);
        rankButtonsLayout->addWidget(demoteBtn);
        form->addRow("", rankButtons);

        auto moveRank = [rankCombo, rankChoices](int delta)
        {
            int current = rankCombo->currentIndex();
            if (current < 0)
                return;
            int maxIndex = static_cast<int>(rankChoices.size() - 1);
            int next = std::clamp(current + delta, 0, maxIndex);
            if (next != current)
                rankCombo->setCurrentIndex(next);
        };
        connect(promoteBtn, &QPushButton::clicked, rankCombo, [moveRank]()
                { moveRank(1); });
        connect(demoteBtn, &QPushButton::clicked, rankCombo, [moveRank]()
                { moveRank(-1); });
    }

    QSpinBox *trainingTotalSpin = new QSpinBox(&dlg);
    trainingTotalSpin->setRange(0, 5000);
    trainingTotalSpin->setValue(qMax(player.attendance, player.totalAttendance));
    form->addRow("Trainings gesamt", trainingTotalSpin);

    QDateEdit *joinEdit = new QDateEdit(&dlg);
    joinEdit->setDisplayFormat("yyyy-MM-dd");
    joinEdit->setCalendarPopup(true);
    joinEdit->setDate(player.joinDate.isValid() ? player.joinDate : nowDate());
    form->addRow("Beitritt", joinEdit);

    QLineEdit *commentEdit = new QLineEdit(player.comment, &dlg);
    commentEdit->setPlaceholderText("Kommentar");
    form->addRow("Kommentar", commentEdit);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);

    // Löschen-Button nur im Bearbeitungsmodus
    QPushButton *deleteBtn = nullptr;
    if (editing)
    {
        deleteBtn = new QPushButton("Spieler löschen", &dlg);
        deleteBtn->setStyleSheet("QPushButton { background-color: #d32f2f; color: white; font-weight: bold; }");
        buttons->addButton(deleteBtn, QDialogButtonBox::DestructiveRole);

        connect(deleteBtn, &QPushButton::clicked, &dlg, [&]()
                {
            QMessageBox::StandardButton reply = QMessageBox::question(
                &dlg, 
                "Spieler löschen", 
                QStringLiteral("Möchten Sie '%1' wirklich löschen?\nAlle Daten und Teilnahmen werden entfernt.").arg(nameEdit->text()),
                QMessageBox::Yes | QMessageBox::No);
            
            if (reply == QMessageBox::Yes)
            {
                dlg.setProperty("deleteRequested", true);
                dlg.accept();
            } });
    }

    form->addRow(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dlg, [&]()
            {
        if (nameEdit->text().trimmed().isEmpty())
        {
            QMessageBox::warning(&dlg, "Eingabe fehlt", "Bitte einen Spielernamen angeben.");
            return;
        }
        dlg.accept(); });
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted)
        return false;

    player.name = nameEdit->text().trimmed();
    player.t17name = t17Edit->text().trimmed();
    player.level = levelSpin->value();
    player.group = groupCombo->currentText().trimmed();
    player.rank = rankCombo->currentText();
    int newTrainingTotal = trainingTotalSpin->value();
    player.totalAttendance = newTrainingTotal;
    if (editing)
        player.attendance = qMin(player.attendance, newTrainingTotal);
    else
        player.attendance = newTrainingTotal;
    player.joinDate = joinEdit->date();
    player.comment = commentEdit->text().trimmed();

    // Wenn "Spieler löschen" geklickt wurde, markiere dies
    if (dlg.property("deleteRequested").toBool())
    {
        player.name = "__DELETE__"; // Marker für Löschung
        return true;
    }

    return true;
}

void MainWindow::loadRankRequirements()
{
    rankRequirements.clear();
    auto ensureDefaults = [this]()
    {
        if (!rankRequirements.isEmpty())
            return;
        const QStringList ranks = MainWindow::rankOptions();
        for (const QString &rank : ranks)
            rankRequirements.insert(rank, defaultRequirementForRank(rank));
    };
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty())
        dir = QDir::homePath();
    QDir().mkpath(dir);
    QString path = QDir(dir).filePath("clan_rank_requirements.json");
    QFile f(path);
    if (!f.exists())
    {
        // nothing to load
        ensureDefaults();
        return;
    }
    if (!f.open(QIODevice::ReadOnly))
    {
        ensureDefaults();
        return;
    }
    QByteArray data = f.readAll();
    f.close();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject())
    {
        ensureDefaults();
        return;
    }
    QJsonObject obj = doc.object();
    for (auto it = obj.begin(); it != obj.end(); ++it)
    {
        RankRequirement req = defaultRequirementForRank(it.key());
        if (it.value().isObject())
        {
            QJsonObject o = it.value().toObject();
            req.minMonths = o.value("months").toInt(req.minMonths);
            req.minLevel = o.value("level").toInt(req.minLevel);
            req.minCombined = o.value("sessions").toInt(req.minCombined);
            if (req.minCombined == 0)
            {
                int legacy = o.value("trainings").toInt() + o.value("events").toInt() + o.value("reserve").toInt();
                if (legacy > 0)
                    req.minCombined = legacy;
            }
        }
        else if (it.value().isDouble())
        {
            req.minMonths = it.value().toInt();
        }
        rankRequirements.insert(it.key(), req);
    }
    for (const QString &rank : MainWindow::rankOptions())
    {
        if (!rankRequirements.contains(rank))
            rankRequirements.insert(rank, defaultRequirementForRank(rank));
    }
    ensureDefaults();
}
void MainWindow::saveRankRequirements()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty())
        dir = QDir::homePath();
    QDir().mkpath(dir);
    QString path = QDir(dir).filePath("clan_rank_requirements.json");
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly))
        return;
    QJsonObject obj;
    for (auto it = rankRequirements.constBegin(); it != rankRequirements.constEnd(); ++it)
    {
        QJsonObject reqObj;
        reqObj.insert("months", it.value().minMonths);
        reqObj.insert("level", it.value().minLevel);
        reqObj.insert("sessions", it.value().minCombined);
        obj.insert(it.key(), reqObj);
    }
    QJsonDocument doc(obj);
    f.write(doc.toJson(QJsonDocument::Indented));
    f.close();
}
void MainWindow::showSettingsDialog()
{
    QDialog dlg(this);
    dlg.setWindowTitle("Einstellungen");
    dlg.resize(600, 500);

    QVBoxLayout *mainLayout = new QVBoxLayout(&dlg);

    QTabWidget *tabs = new QTabWidget(&dlg);

    // Tab 1: Allgemeine Einstellungen
    QWidget *generalTab = new QWidget;
    QFormLayout *generalForm = new QFormLayout(generalTab);

    QLabel *hinweisLabel = new QLabel("Spaltenname für Hinweis-Spalte:", generalTab);
    QLineEdit *hinweisEdit = new QLineEdit(hintColumnName, generalTab);
    hinweisEdit->setToolTip("Name der Spalte, die Warnungen/Beförderungen anzeigt");
    generalForm->addRow(hinweisLabel, hinweisEdit);

    QLabel *counterThresholdLabel = new QLabel("Schwellenwert für rotes X:", generalTab);
    QSpinBox *counterThresholdSpin = new QSpinBox(generalTab);
    counterThresholdSpin->setRange(1, 50);
    counterThresholdSpin->setValue(noResponseThreshold);
    counterThresholdSpin->setToolTip("Ab wie vielen fehlenden Rückmeldungen erscheint das rote X");
    generalForm->addRow(counterThresholdLabel, counterThresholdSpin);

    QCheckBox *autoCreatePlayersCheck = new QCheckBox("Unbekannte Spieler automatisch anlegen (OCR)", generalTab);
    autoCreatePlayersCheck->setChecked(autoCreatePlayers);
    generalForm->addRow(autoCreatePlayersCheck);

    QCheckBox *fuzzyMatchingCheck = new QCheckBox("Fuzzy-Matching bei OCR aktivieren", generalTab);
    fuzzyMatchingCheck->setChecked(fuzzyMatchingEnabled);
    fuzzyMatchingCheck->setToolTip("Erlaubt kleine Tippfehler bei der Spielererkennung");
    generalForm->addRow(fuzzyMatchingCheck);

    QLabel *fuzzyThresholdLabel = new QLabel("Fuzzy-Matching Toleranz:", generalTab);
    QSpinBox *fuzzyThresholdSpin = new QSpinBox(generalTab);
    fuzzyThresholdSpin->setRange(0, 5);
    fuzzyThresholdSpin->setValue(fuzzyMatchThreshold);
    fuzzyThresholdSpin->setToolTip("Maximale Levenshtein-Distanz für Namensübereinstimmung");
    generalForm->addRow(fuzzyThresholdLabel, fuzzyThresholdSpin);

    tabs->addTab(generalTab, "Allgemein");

    // Tab 2: Gruppenreihenfolge
    QWidget *groupTab = new QWidget;
    QVBoxLayout *groupLayout = new QVBoxLayout(groupTab);

    QLabel *groupHint = new QLabel("Reihenfolge der Gruppen in der Anzeige (eine pro Zeile):", groupTab);
    groupHint->setWordWrap(true);
    groupLayout->addWidget(groupHint);

    QTextEdit *groupOrderEdit = new QTextEdit(groupTab);
    groupOrderEdit->setPlainText(groups.join("\n"));
    groupLayout->addWidget(groupOrderEdit);

    QPushButton *groupRefreshBtn = new QPushButton("Aus aktuellen Spielern aktualisieren", groupTab);
    connect(groupRefreshBtn, &QPushButton::clicked, groupTab, [this, groupOrderEdit]()
            {
        QSet<QString> currentGroups;
        for (const Player &p : list.players) {
            if (!p.group.isEmpty())
                currentGroups.insert(p.group);
        }
        QStringList sorted = currentGroups.values();
        std::sort(sorted.begin(), sorted.end());
        groupOrderEdit->setPlainText(sorted.join("\n")); });
    groupLayout->addWidget(groupRefreshBtn);

    tabs->addTab(groupTab, "Gruppen");

    // Tab 3: OCR-Einstellungen
    QWidget *ocrTab = new QWidget;
    QFormLayout *ocrForm = new QFormLayout(ocrTab);

    QLabel *ocrLangLabel = new QLabel("Tesseract Sprache:", ocrTab);
    QLineEdit *ocrLangEdit = new QLineEdit(ocrLanguage, ocrTab);
    ocrLangEdit->setToolTip("Sprache für OCR (z.B. 'deu', 'eng', 'deu+eng')");
    ocrForm->addRow(ocrLangLabel, ocrLangEdit);

    QLabel *unassignedGroupLabel = new QLabel("Gruppe für neue Spieler:", ocrTab);
    QLineEdit *unassignedGroupEdit = new QLineEdit(unassignedGroupName, ocrTab);
    unassignedGroupEdit->setToolTip("In diese Gruppe werden per OCR erkannte neue Spieler eingetragen");
    ocrForm->addRow(unassignedGroupLabel, unassignedGroupEdit);

    QCheckBox *autoFillMetadataCheck = new QCheckBox("Event-Metadaten automatisch übernehmen", ocrTab);
    autoFillMetadataCheck->setChecked(autoFillMetadata);
    autoFillMetadataCheck->setToolTip("Übernimmt erkannte Daten (Name, Datum, Map) automatisch in Session-Felder");
    ocrForm->addRow(autoFillMetadataCheck);

    tabs->addTab(ocrTab, "OCR");

    // Tab 4: Zähler-Logik
    QWidget *counterTab = new QWidget;
    QFormLayout *counterForm = new QFormLayout(counterTab);

    QLabel *counterHint = new QLabel("Konfiguration für noResponseCounter:", counterTab);
    counterHint->setWordWrap(true);
    counterForm->addRow(counterHint);

    QCheckBox *incrementOnNoResponseCheck = new QCheckBox("Zähler bei fehlender Meldung erhöhen", counterTab);
    incrementOnNoResponseCheck->setChecked(incrementCounterOnNoResponse);
    counterForm->addRow(incrementOnNoResponseCheck);

    QCheckBox *resetOnResponseCheck = new QCheckBox("Zähler bei An-/Abmeldung zurücksetzen", counterTab);
    resetOnResponseCheck->setChecked(resetCounterOnResponse);
    counterForm->addRow(resetOnResponseCheck);

    QCheckBox *showCounterInTableCheck = new QCheckBox("Zähler in Einsätze-Spalte anzeigen", counterTab);
    showCounterInTableCheck->setChecked(showCounterInTable);
    counterForm->addRow(showCounterInTableCheck);

    tabs->addTab(counterTab, "Zähler");

    // Tab 5: Daten (Import/Export)
    QWidget *dataTab = new QWidget;
    QVBoxLayout *dataLayout = new QVBoxLayout(dataTab);
    QLabel *dataHint = new QLabel("Datenimport/-export verwalten:", dataTab);
    dataLayout->addWidget(dataHint);
    QHBoxLayout *dataBtns = new QHBoxLayout;
    QPushButton *importBtnDlg = new QPushButton("Import (CSV/TXT)", dataTab);
    QPushButton *exportBtnDlg = new QPushButton("Export (CSV)", dataTab);
    dataBtns->addWidget(importBtnDlg);
    dataBtns->addWidget(exportBtnDlg);
    dataBtns->addStretch();
    dataLayout->addLayout(dataBtns);
    // Neuer Button: Alle Spieler löschen (mit Sicherheitsabfrage)
    QPushButton *deleteAllPlayersBtn = new QPushButton("Alle Spieler löschen", dataTab);
    deleteAllPlayersBtn->setStyleSheet("QPushButton { background:#b30000; color:white; font-weight:bold; } QPushButton:hover { background:#d60000; }");
    dataLayout->addWidget(deleteAllPlayersBtn);
    QPushButton *showErrorLogBtn = new QPushButton("Fehlerprotokoll anzeigen", dataTab);
    dataLayout->addWidget(showErrorLogBtn);
    connect(importBtnDlg, &QPushButton::clicked, this, &MainWindow::importCsv);
    connect(exportBtnDlg, &QPushButton::clicked, this, &MainWindow::exportCsv);
    connect(showErrorLogBtn, &QPushButton::clicked, this, &MainWindow::showErrorLogDialog);
    connect(deleteAllPlayersBtn, &QPushButton::clicked, this, [this, groupOrderEdit]()
            {
        QMessageBox::StandardButton res = QMessageBox::warning(this, QStringLiteral("Alle Spieler löschen"),
                                                               QStringLiteral("WARNUNG: Diese Aktion löscht ALLE Spieler, Gruppen, Kommentare und Anwesenheitsdaten unwiderruflich. Fortfahren?"),
                                                               QMessageBox::Yes | QMessageBox::No);
        if (res != QMessageBox::Yes)
            return;
        // Listen und Strukturen leeren
        list.players.clear();
        attendanceRecords.clear();
        soldbuchRecords.clear();
        groups.clear();
        groupCategory.clear();
        groupColors.clear();
        sessionSelectedPlayers.clear();
        // Model leeren
        if (model)
            model->removeRows(0, model->rowCount());
        // Persistenz
        savePlayers();
        saveAttendance();
        saveSoldbuch();
        saveGroups();
        saveGroupColors();
        // UI aktualisieren
        refreshGroupFilterCombo();
        updateGroupDecorations();
        validateAllRows();
        if (groupOrderEdit)
            groupOrderEdit->setPlainText(QString());
        updateSessionSummary();
        QMessageBox::information(this, QStringLiteral("Löschung abgeschlossen"), QStringLiteral("Alle Spieler und zugehörige Daten wurden entfernt.")); });
    tabs->addTab(dataTab, "Daten");

    // Tab 5: Rang-Anforderungen
    QWidget *rankTab = new QWidget;
    QVBoxLayout *rankLayout = new QVBoxLayout(rankTab);

    QLabel *rankHint = new QLabel("Monate geben die Mindestzeit seit letzter Beförderung an. Level definiert die benötigte Spieler-Stufe und Einsätze die relevante Anzahl an Trainings für den nächsten Rang.", rankTab);
    rankHint->setWordWrap(true);
    rankLayout->addWidget(rankHint);

    QScrollArea *rankScrollArea = new QScrollArea(rankTab);
    rankScrollArea->setWidgetResizable(true);
    QWidget *rankScrollWidget = new QWidget;
    QGridLayout *rankGrid = new QGridLayout(rankScrollWidget);

    rankGrid->addWidget(new QLabel("Rang", rankScrollWidget), 0, 0);
    rankGrid->addWidget(new QLabel("Monate", rankScrollWidget), 0, 1);
    rankGrid->addWidget(new QLabel("Level", rankScrollWidget), 0, 2);
    rankGrid->addWidget(new QLabel("T+E+R", rankScrollWidget), 0, 3);
    rankGrid->addWidget(new QLabel("Spieler", rankScrollWidget), 0, 4);

    struct RankWidgets
    {
        QSpinBox *months = nullptr;
        QSpinBox *level = nullptr;
        QSpinBox *combined = nullptr;
    };

    QMap<QString, int> rankCounts;
    for (const Player &player : list.players)
    {
        if (player.rank.isEmpty())
            continue;
        rankCounts[player.rank] += 1;
    }

    QMap<QString, RankWidgets> rankWidgets;
    int rowIdx = 1;
    const QStringList ranks = MainWindow::rankOptions();
    for (const QString &r : ranks)
    {
        RankRequirement req = rankRequirements.value(r, defaultRequirementForRank(r));
        rankGrid->addWidget(new QLabel(r, rankScrollWidget), rowIdx, 0);

        RankWidgets set;
        set.months = new QSpinBox(rankScrollWidget);
        set.months->setRange(0, 60);
        set.months->setValue(req.minMonths);
        rankGrid->addWidget(set.months, rowIdx, 1);

        set.level = new QSpinBox(rankScrollWidget);
        set.level->setRange(0, 500);
        set.level->setValue(req.minLevel);
        rankGrid->addWidget(set.level, rowIdx, 2);

        set.combined = new QSpinBox(rankScrollWidget);
        set.combined->setRange(0, 500);
        set.combined->setValue(req.minCombined);
        rankGrid->addWidget(set.combined, rowIdx, 3);

        int count = rankCounts.value(r);
        QLabel *countLabel = new QLabel(QString::number(count), rankScrollWidget);
        countLabel->setAlignment(Qt::AlignCenter);
        rankGrid->addWidget(countLabel, rowIdx, 4);

        rankWidgets.insert(r, set);
        ++rowIdx;
    }

    rankScrollArea->setWidget(rankScrollWidget);
    rankLayout->addWidget(rankScrollArea);

    int rankedCountTotal = 0;
    for (auto it = rankCounts.constBegin(); it != rankCounts.constEnd(); ++it)
        rankedCountTotal += it.value();
    int allPlayersTotal = static_cast<int>(list.players.size());
    QLabel *totalLbl = new QLabel(QStringLiteral("Gesamt Spieler: %1  (mit Rang: %2)")
                                      .arg(allPlayersTotal)
                                      .arg(rankedCountTotal),
                                  rankTab);
    totalLbl->setAlignment(Qt::AlignRight);
    rankLayout->addWidget(totalLbl);

    tabs->addTab(rankTab, "Rang-Anforderungen");

    // Tab 6: Hilfe/Anleitung
    QWidget *helpTab = new QWidget;
    QVBoxLayout *helpLayout = new QVBoxLayout(helpTab);
    QTextBrowser *helpView = new QTextBrowser(helpTab);
    helpView->setOpenExternalLinks(true);
    QString manualHtml;
    manualHtml += "<h2>ClanManager – Nutzer-Anleitung</h2>";
    manualHtml += "<p>Diese Anleitung beschreibt die Hauptfunktionen des ClanManagers und hilft bei Import, Registrierung und Verwaltung von Sessions, Spielern und Gruppen.</p>";

    manualHtml += "<h3>1. Spielerimport</h3>";
    manualHtml += "<ul>";
    manualHtml += "<li><b>CSV/TSV:</b> Trennzeichen und Spalten werden automatisch erkannt (u. a. Name, T17, Gruppe, Datum, Rang). Nach dem Import erhalten Sie eine Auswertung, wie viele Zeilen neu angelegt, zusammengeführt oder übersprungen wurden.</li>";
    manualHtml += "<li><b>Bilddateien (OCR):</b> Über den Button <i>Spielerliste hochladen</i> können Screenshots/Fotos ausgewertet werden. Namen und T17-Namen werden erkannt und markiert. Unbekannte Spieler können automatisch in die konfigurierte Gruppe angelegt werden (optional Fuzzy‑Matching).</li>";
    manualHtml += "<li><b>Feedback:</b> Nach jedem Import wird eine Meldung angezeigt (neu/aktualisiert/übersprungen). Bei OCR zusätzlich erkannte Event‑Daten (Name/Datum/Map) sofern verfügbar.</li>";
    manualHtml += "</ul>";

    manualHtml += "<h3>2. Registrierungs‑Panel (\"Letztes Gefecht / Training\")</h3>";
    manualHtml += "<ul>";
    manualHtml += "<li><b>Felder:</b> Vorlage, Typ (Training/Event/Reserve), Datum, Name, Karte.</li>";
    manualHtml += "<li><b>Checkliste:</b> Spieler sind in einer Liste mit Checkboxen auswählbar; die Zusammenfassung (Anzahl) aktualisiert sich automatisch.</li>";
    manualHtml += "<li><b>Buttons:</b> <i>Aus Liste übernehmen</i> übernimmt die aktuelle Auswahl aus der Tabelle. <i>Session bestätigen</i> schreibt die Anwesenheit für alle markierten Spieler.</li>";
    manualHtml += "</ul>";

    manualHtml += "<h3>3. Session‑Verwaltung</h3>";
    manualHtml += "<ul>";
    manualHtml += "<li><b>Gruppierung:</b> Spieler erscheinen im QTreeWidget nach Gruppen gegliedert (auf-/zuklappbar). Farb‑Akzente der Gruppen bleiben erhalten.</li>";
    manualHtml += "<li><b>Persistenz:</b> Vorlagen (Templates) und Karten (Maps) werden gepflegt und bleiben aktuell.</li>";
    manualHtml += "<li><b>Bestätigen:</b> <i>Session bestätigen</i> bucht die Anwesenheit, erhöht Zähler, validiert Zeilen und speichert die Vorlage bei Bedarf.</li>";
    manualHtml += "</ul>";

    manualHtml += "<h3>4. Zähler‑Logik</h3>";
    manualHtml += "<ul>";
    manualHtml += "<li><b>Einsätze:</b> In der Spalte <i>Einsätze</i> wird die Teilnahme (Training/Event/Reserve) angezeigt; optional mit aktuellem Zähler in Klammern.</li>";
    manualHtml += "<li><b>No‑Response‑Zähler:</b> Erhöht sich automatisch, wenn ein Spieler weder Zu- noch Absage für eine Session hat. Ab dem Schwellenwert (z. B. 10) erscheint ein rotes ✖ in der Spalte <i>Hinweis</i>.</li>";
    manualHtml += "<li><b>Rücksetzen:</b> Bei An-/Abmeldung wird der Zähler wieder auf 0 gesetzt (einstellbar).</li>";
    manualHtml += "</ul>";

    manualHtml += "<h3>5. Gruppen‑ und Spieler‑Verwaltung</h3>";
    manualHtml += "<ul>";
    manualHtml += "<li><b>Gruppen:</b> Neue Gruppen aus Importen/OCR werden automatisch registriert. Farben/Kategorien können konfiguriert werden.</li>";
    manualHtml += "<li><b>Spieler‑Merge:</b> Doppelte Einträge werden zusammengeführt. Totale (Anwesenheit/Events/Reserve) bleiben konsistent.</li>";
    manualHtml += "</ul>";

    manualHtml += "<h3>6. Menüstruktur</h3>";
    manualHtml += "<ul>";
    manualHtml += "<li><b>Aufgeräumt:</b> Buttons zur direkten Training/Event‑Erstellung wurden entfernt.</li>";
    manualHtml += "<li><b>Daten:</b> Import/Export finden Sie jetzt im Tab <i>Daten</i> innerhalb des Einstellungsdialogs.</li>";
    manualHtml += "<li><b>Einstellungen:</b> Über den Button <i>⚙️ Einstellungen</i> konfigurieren Sie Gruppenreihenfolge, Spaltennamen, Zählergrenzen, OCR/Import‑Optionen u. v. m. Änderungen werden gespeichert und sofort angewendet.</li>";
    manualHtml += "</ul>";

    manualHtml += "<h3>Tipps</h3>";
    manualHtml += "<ul>";
    manualHtml += "<li>Mit dem Suchfeld und den Filter‑Combos (Rang/Gruppe) finden Sie Spieler schneller.</li>";
    manualHtml += "<li>OCR funktioniert am besten mit scharfen Screenshots und klaren Namen/Spalten.</li>";
    manualHtml += "<li>Die Schwelle für das rote ✖ und die Anzeige des Zählers können Sie im Tab <i>Zähler</i> einstellen.</li>";
    manualHtml += "</ul>";

    helpView->setHtml(manualHtml);
    helpLayout->addWidget(helpView);
    tabs->addTab(helpTab, "Hilfe");

    mainLayout->addWidget(tabs);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    mainLayout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted)
    {
        // Speichere alle Einstellungen
        noResponseThreshold = counterThresholdSpin->value();
        autoCreatePlayers = autoCreatePlayersCheck->isChecked();
        fuzzyMatchingEnabled = fuzzyMatchingCheck->isChecked();
        fuzzyMatchThreshold = fuzzyThresholdSpin->value();
        ocrLanguage = ocrLangEdit->text().trimmed();
        unassignedGroupName = unassignedGroupEdit->text().trimmed();
        autoFillMetadata = autoFillMetadataCheck->isChecked();
        incrementCounterOnNoResponse = incrementOnNoResponseCheck->isChecked();
        resetCounterOnResponse = resetOnResponseCheck->isChecked();
        showCounterInTable = showCounterInTableCheck->isChecked();

        // Speichere Spaltennamen 'Hinweis'
        hintColumnName = hinweisEdit->text().trimmed().isEmpty() ? QStringLiteral("Hinweis") : hinweisEdit->text().trimmed();

        // Speichere Gruppenreihenfolge
        QString groupText = groupOrderEdit->toPlainText();
        QStringList newGroups = groupText.split("\n", Qt::SkipEmptyParts);
        for (QString &g : newGroups)
            g = g.trimmed();
        groups = newGroups;
        saveGroups();

        // Speichere Rang-Anforderungen
        rankRequirements.clear();
        for (auto it = rankWidgets.constBegin(); it != rankWidgets.constEnd(); ++it)
        {
            RankRequirement req;
            req.minMonths = it.value().months->value();
            req.minLevel = it.value().level->value();
            req.minCombined = it.value().combined->value();
            rankRequirements.insert(it.key(), req);
        }
        saveRankRequirements();

        // Speichere Settings in JSON
        saveSettings();

        // Aktualisiere Anzeige
        applySettingsToUI();

        QMessageBox::information(this, "Einstellungen", "Einstellungen wurden gespeichert und angewendet.");
    }
}

void MainWindow::editRankRequirements()
{
    QDialog dlg(this);
    dlg.setWindowTitle("Rank Requirements");
    QVBoxLayout *layout = new QVBoxLayout(&dlg);
    QLabel *hint = new QLabel("Monate geben die Mindestzeit seit letzter Beförderung an. Level definiert die benötigte Spieler-Stufe und Einsätze die relevante Anzahl an Trainings für den nächsten Rang.", &dlg);
    hint->setWordWrap(true);
    layout->addWidget(hint);

    QGridLayout *grid = new QGridLayout;
    grid->addWidget(new QLabel("Rang", &dlg), 0, 0);
    grid->addWidget(new QLabel("Monate", &dlg), 0, 1);
    grid->addWidget(new QLabel("Level", &dlg), 0, 2);
    grid->addWidget(new QLabel("T+E+R", &dlg), 0, 3);
    grid->addWidget(new QLabel("Spieler", &dlg), 0, 4);

    struct RankWidgets
    {
        QSpinBox *months = nullptr;
        QSpinBox *level = nullptr;
        QSpinBox *combined = nullptr;
    };

    QMap<QString, int> rankCounts;
    for (const Player &player : list.players)
    {
        if (player.rank.isEmpty())
            continue;
        rankCounts[player.rank] += 1;
    }

    QMap<QString, RankWidgets> widgets;
    int rowIdx = 1;
    const QStringList ranks = MainWindow::rankOptions();
    for (const QString &r : ranks)
    {
        RankRequirement req = rankRequirements.value(r, defaultRequirementForRank(r));
        grid->addWidget(new QLabel(r, &dlg), rowIdx, 0);
        RankWidgets set;
        set.months = new QSpinBox(&dlg);
        set.months->setRange(0, 60);
        set.months->setValue(req.minMonths);
        grid->addWidget(set.months, rowIdx, 1);

        auto makeCountBox = [&dlg](int value)
        {
            QSpinBox *sb = new QSpinBox(&dlg);
            sb->setRange(0, 500);
            sb->setValue(value);
            return sb;
        };

        set.level = new QSpinBox(&dlg);
        set.level->setRange(0, 500);
        set.level->setValue(req.minLevel);
        grid->addWidget(set.level, rowIdx, 2);

        set.combined = makeCountBox(req.minCombined);
        grid->addWidget(set.combined, rowIdx, 3);

        int count = rankCounts.value(r);
        QLabel *countLabel = new QLabel(QString::number(count), &dlg);
        countLabel->setAlignment(Qt::AlignCenter);
        grid->addWidget(countLabel, rowIdx, 4);

        widgets.insert(r, set);
        ++rowIdx;
    }

    layout->addLayout(grid);

    int rankedCountTotal = 0;
    for (auto it = rankCounts.constBegin(); it != rankCounts.constEnd(); ++it)
        rankedCountTotal += it.value();
    int allPlayersTotal = static_cast<int>(list.players.size());
    QLabel *totalLbl = new QLabel(QStringLiteral("Gesamt Spieler: %1  (mit Rang: %2)")
                                      .arg(allPlayersTotal)
                                      .arg(rankedCountTotal),
                                  &dlg);
    totalLbl->setAlignment(Qt::AlignRight);
    layout->addWidget(totalLbl);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    if (dlg.exec() == QDialog::Accepted)
    {
        rankRequirements.clear();
        for (auto it = widgets.constBegin(); it != widgets.constEnd(); ++it)
        {
            RankRequirement req;
            req.minMonths = it.value().months->value();
            req.minLevel = it.value().level->value();
            req.minCombined = it.value().combined->value();
            rankRequirements.insert(it.key(), req);
        }
        saveRankRequirements();
        validateAllRows();
    }
}

RankRequirement MainWindow::requirementForRank(const QString &rank) const
{
    if (rankRequirements.contains(rank))
        return rankRequirements.value(rank);
    for (auto it = rankRequirements.constBegin(); it != rankRequirements.constEnd(); ++it)
    {
        if (rank.startsWith(it.key()))
            return it.value();
    }
    return defaultRequirementForRank(rank);
}

RankRequirement MainWindow::defaultRequirementForRank(const QString &rank) const
{
    RankRequirement req;
    if (rank.startsWith("Anwerber"))
        req.minMonths = 3;
    else if (rank.startsWith("Panzergrenadier"))
        req.minMonths = 3;
    else if (rank.startsWith("Obergrenadier"))
        req.minMonths = 3;
    else if (rank.startsWith("Gefreiter"))
        req.minMonths = 3;
    else if (rank.startsWith("Obergefreiter"))
        req.minMonths = 3;
    else if (rank.startsWith("Stabsgefreiter"))
        req.minMonths = 3;
    else if (rank.startsWith("Unteroffizier"))
        req.minMonths = 0;
    else if (rank.startsWith("Stabsunteroffizier"))
        req.minMonths = 4;
    else if (rank.startsWith("Unterfeldwebel"))
        req.minMonths = 0;
    else if (rank.startsWith("Feldwebel"))
        req.minMonths = 3;
    else if (rank.startsWith("Oberfeldwebel"))
        req.minMonths = 3;
    else if (rank.startsWith("Hauptfeldwebel"))
        req.minMonths = 3;
    else if (rank.startsWith("Stabsfeldwebel"))
        req.minMonths = 3;
    else
        req.minMonths = 0;
    return req;
}

void MainWindow::editRanks() {}
void MainWindow::addPlayer()
{
    Player newPlayer;
    newPlayer.joinDate = nowDate();
    if (!MainWindow::rankOptions().isEmpty())
        newPlayer.rank = MainWindow::rankOptions().first();

    if (!openPlayerEditDialog(newPlayer, false))
        return;

    // Prüfe ob Spieler mit diesem Namen bereits existiert
    QString normalizedName = newPlayer.name.trimmed();
    for (const Player &existing : list.players)
    {
        if (existing.name.trimmed().compare(normalizedName, Qt::CaseInsensitive) == 0)
        {
            QMessageBox::warning(this, "Spieler existiert bereits",
                                 QStringLiteral("Ein Spieler mit dem Namen '%1' ist bereits vorhanden.\n"
                                                "Bitte verwenden Sie einen anderen Namen oder bearbeiten Sie den existierenden Spieler.")
                                     .arg(existing.name));
            return;
        }
    }

    newPlayer.totalAttendance = qMax(newPlayer.totalAttendance, newPlayer.attendance);
    newPlayer.totalEvents = qMax(newPlayer.totalEvents, newPlayer.events);
    newPlayer.totalReserve = qMax(newPlayer.totalReserve, newPlayer.reserve);
    list.players.push_back(newPlayer);

    if (ensureGroupRegistered(newPlayer.group))
        saveGroups();

    addPlayerToModel(newPlayer);
    validateRow(model->rowCount() - 1);
    savePlayers();
}
void MainWindow::editGroups()
{
    struct ManagedGroup
    {
        QString originalName;
        QString name;
        QString category;
        QString color;
    };

    QVector<ManagedGroup> data;
    data.reserve(groups.size());
    for (const QString &g : groups)
    {
        ManagedGroup entry;
        entry.originalName = g;
        entry.name = g;
        entry.category = groupCategory.value(g);
        entry.color = groupColors.value(g);
        data.append(entry);
    }

    QDialog dlg(this);
    dlg.setWindowTitle("Gruppen verwalten");
    dlg.resize(420, 360);
    QVBoxLayout *outer = new QVBoxLayout(&dlg);

    QListWidget *listWidget = new QListWidget(&dlg);
    listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    outer->addWidget(listWidget, 1);

    auto rebuildList = [&](int selectRow)
    {
        listWidget->clear();
        for (const ManagedGroup &entry : data)
        {
            QString text = entry.name;
            if (!entry.category.isEmpty())
                text = QStringLiteral("%1 (%2)").arg(entry.name, entry.category);
            QListWidgetItem *item = new QListWidgetItem(text, listWidget);
            if (!entry.color.isEmpty())
            {
                QColor col(entry.color);
                if (col.isValid())
                    item->setBackground(col.lighter(160));
            }
        }
        if (listWidget->count() == 0)
            return;
        if (selectRow < 0 || selectRow >= listWidget->count())
            selectRow = 0;
        listWidget->setCurrentRow(selectRow);
    };
    rebuildList(0);

    auto showEditor = [&](ManagedGroup &entry, int skipIndex, const QString &title) -> bool
    {
        QDialog editor(&dlg);
        editor.setWindowTitle(title);
        QFormLayout *form = new QFormLayout(&editor);

        QLineEdit *nameEdit = new QLineEdit(entry.name, &editor);
        form->addRow("Name", nameEdit);

        QComboBox *categoryCombo = new QComboBox(&editor);
        categoryCombo->setEditable(true);
        QSet<QString> categories;
        for (const ManagedGroup &grp : data)
            if (!grp.category.isEmpty())
                categories.insert(grp.category);
        QStringList categoryList = categories.values();
        std::sort(categoryList.begin(), categoryList.end(), [](const QString &a, const QString &b)
                  { return a.compare(b, Qt::CaseInsensitive) < 0; });
        categoryCombo->addItems(categoryList);
        categoryCombo->setCurrentText(entry.category);
        form->addRow("Kategorie", categoryCombo);

        QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &editor);
        form->addRow(buttons);

        QObject::connect(buttons, &QDialogButtonBox::accepted, &editor, [&]()
                         {
            QString name = nameEdit->text().trimmed();
            if (name.isEmpty())
            {
                QMessageBox::warning(&editor, "Name fehlt", "Bitte einen Gruppennamen angeben.");
                return;
            }
            for (int i = 0; i < data.size(); ++i)
            {
                if (i == skipIndex)
                    continue;
                if (data.at(i).name.compare(name, Qt::CaseInsensitive) == 0)
                {
                    QMessageBox::warning(&editor, "Duplikat", "Es existiert bereits eine Gruppe mit diesem Namen.");
                    return;
                }
            }
            entry.name = name;
            entry.category = categoryCombo->currentText().trimmed();
            editor.accept(); });
        QObject::connect(buttons, &QDialogButtonBox::rejected, &editor, &QDialog::reject);

        return editor.exec() == QDialog::Accepted;
    };

    auto selectedRow = [&]() -> int
    {
        int row = listWidget->currentRow();
        if (row < 0 || row >= data.size())
            return -1;
        return row;
    };

    QHBoxLayout *buttonRow = new QHBoxLayout;
    QPushButton *addBtn = new QPushButton("Neu", &dlg);
    QPushButton *editBtn = new QPushButton("Bearbeiten", &dlg);
    QPushButton *removeBtn = new QPushButton("Löschen", &dlg);
    QPushButton *colorBtn = new QPushButton("Farbe", &dlg);
    QPushButton *clearColorBtn = new QPushButton("Farbe löschen", &dlg);
    buttonRow->addWidget(addBtn);
    buttonRow->addWidget(editBtn);
    buttonRow->addWidget(removeBtn);
    buttonRow->addWidget(colorBtn);
    buttonRow->addWidget(clearColorBtn);
    outer->addLayout(buttonRow);

    connect(addBtn, &QPushButton::clicked, &dlg, [&]()
            {
        ManagedGroup entry;
        if (!showEditor(entry, -1, "Neue Gruppe"))
            return;
        data.append(entry);
        rebuildList(listWidget->count()); });

    auto editSelected = [&]()
    {
        int row = selectedRow();
        if (row < 0)
            return;
        ManagedGroup entry = data.at(row);
        if (!showEditor(entry, row, "Gruppe bearbeiten"))
            return;
        entry.originalName = data.at(row).originalName;
        data[row] = entry;
        rebuildList(row);
    };
    connect(editBtn, &QPushButton::clicked, &dlg, editSelected);
    connect(listWidget, &QListWidget::itemDoubleClicked, &dlg, [editSelected](QListWidgetItem *)
            { editSelected(); });

    connect(removeBtn, &QPushButton::clicked, &dlg, [&]()
            {
        int row = selectedRow();
        if (row < 0)
            return;
        const QString name = data.at(row).name;
        if (QMessageBox::question(&dlg, "Gruppe löschen", QStringLiteral("%1 wirklich entfernen?").arg(name)) != QMessageBox::Yes)
            return;
        data.removeAt(row);
        rebuildList(qMax(0, row - 1)); });

    connect(colorBtn, &QPushButton::clicked, &dlg, [&]()
            {
        int row = selectedRow();
        if (row < 0)
            return;
        QColor initial(data.at(row).color);
        if (!initial.isValid())
            initial = QColor("#2b4c7e");
        QColor chosen = QColorDialog::getColor(initial, &dlg, "Farbe wählen");
        if (!chosen.isValid())
            return;
        data[row].color = chosen.name(QColor::HexRgb);
        rebuildList(row); });

    connect(clearColorBtn, &QPushButton::clicked, &dlg, [&]()
            {
        int row = selectedRow();
        if (row < 0)
            return;
        data[row].color.clear();
        rebuildList(row); });

    QDialogButtonBox *closeButtons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    outer->addWidget(closeButtons);
    connect(closeButtons, &QDialogButtonBox::accepted, &dlg, [&]()
            {
        if (data.isEmpty())
        {
            QMessageBox::warning(&dlg, "Keine Gruppen", "Mindestens eine Gruppe wird benötigt.");
            return;
        }
        dlg.accept(); });
    connect(closeButtons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted)
        return;

    const QStringList previousGroups = groups;
    QMap<QString, QString> renameMap;
    QStringList newGroups;
    QMap<QString, QString> newCategories;
    QMap<QString, QString> newColors;
    for (const ManagedGroup &entry : data)
    {
        QString name = entry.name.trimmed();
        if (name.isEmpty())
            continue;
        newGroups.append(name);
        if (!entry.category.isEmpty())
            newCategories.insert(name, entry.category);
        if (!entry.color.isEmpty())
            newColors.insert(name, entry.color);
        if (!entry.originalName.isEmpty() && entry.originalName != name)
            renameMap.insert(entry.originalName, name);
    }
    std::sort(newGroups.begin(), newGroups.end(), [](const QString &a, const QString &b)
              { return a.compare(b, Qt::CaseInsensitive) < 0; });
    newGroups.erase(std::unique(newGroups.begin(), newGroups.end(), [](const QString &a, const QString &b)
                                { return a.compare(b, Qt::CaseInsensitive) == 0; }),
                    newGroups.end());

    QSet<QString> renamedSources;
    for (auto it = renameMap.constBegin(); it != renameMap.constEnd(); ++it)
        renamedSources.insert(it.key());
    QSet<QString> removedGroups;
    for (const QString &oldName : previousGroups)
    {
        if (renamedSources.contains(oldName))
            continue;
        if (indexOfGroupName(newGroups, oldName) < 0)
            removedGroups.insert(oldName);
    }

    groups = newGroups;
    groupCategory = newCategories;
    groupColors = newColors;

    bool playersChanged = false;
    for (Player &p : list.players)
    {
        if (p.group.isEmpty())
            continue;
        bool applied = false;
        for (auto it = renameMap.constBegin(); it != renameMap.constEnd(); ++it)
        {
            if (p.group.compare(it.key(), Qt::CaseInsensitive) == 0)
            {
                p.group = it.value();
                applied = true;
                playersChanged = true;
                break;
            }
        }
        if (applied)
            continue;
        for (const QString &removed : removedGroups)
        {
            if (p.group.compare(removed, Qt::CaseInsensitive) == 0)
            {
                p.group.clear();
                playersChanged = true;
                break;
            }
        }
    }

    refreshGroupFilterCombo();
    refreshModelFromList();
    updateGroupDecorations();
    saveGroups();
    saveGroupColors();
    if (playersChanged)
        savePlayers();
}
void MainWindow::showOrganization() {}
void MainWindow::showContextMenu(const QPoint &pos)
{
    if (!table || !model)
        return;

    QModelIndex proxyIndex = table->indexAt(pos);
    if (!proxyIndex.isValid())
        return;

    int proxyRow = proxyIndex.row();
    int sourceRow = proxy ? proxy->mapToSource(proxyIndex).row() : proxyRow;
    if (sourceRow < 0 || sourceRow >= model->rowCount())
        return;

    table->selectRow(proxyRow);
    table->setCurrentIndex(proxyIndex);

    QString playerKey = playerKeyForRow(sourceRow);
    if (playerKey.isEmpty())
        return;

    QMenu menu(this);
    QAction *editAction = menu.addAction("Spieler bearbeiten");
    QAction *attendanceAction = menu.addAction("Teilnahmen anzeigen");
    QAction *soldbuchAction = menu.addAction("Soldbuch anzeigen");
    menu.addSeparator();
    QAction *promoteAction = menu.addAction("Befördern");
    QAction *demoteAction = menu.addAction("Degradieren");

    QAction *chosen = menu.exec(table->viewport()->mapToGlobal(pos));
    if (!chosen)
        return;

    if (chosen == editAction)
    {
        editPlayer();
    }
    else if (chosen == attendanceAction)
    {
        showPlayerAttendance();
    }
    else if (chosen == soldbuchAction)
    {
        showPlayerSoldbuch();
    }
    else if (chosen == promoteAction)
    {
        promotePlayerByKey(playerKey);
    }
    else if (chosen == demoteAction)
    {
        demotePlayerByKey(playerKey);
    }
}
void MainWindow::recordAttendanceForPlayer(const QString &playerKey, const QString &type, const QDate &date, const QString &trainingId, const QString &map, const QString &eventName)
{
    Q_UNUSED(playerKey);
    Q_UNUSED(type);
    Q_UNUSED(date);
    Q_UNUSED(trainingId);
    Q_UNUSED(map);
    Q_UNUSED(eventName);
}
void MainWindow::editPlayer()
{
    if (!table || !model)
        return;

    QModelIndex proxyIndex = table->currentIndex();
    if (!proxyIndex.isValid())
    {
        QMessageBox::information(this, "Auswahl fehlt", "Bitte zuerst einen Spieler in der Tabelle markieren.");
        return;
    }

    int sourceRow = proxy ? proxy->mapToSource(proxyIndex).row() : proxyIndex.row();
    if (sourceRow < 0 || sourceRow >= model->rowCount())
        return;

    QString originalKey = playerKeyForRow(sourceRow);
    Player existing = playerFromModelRow(sourceRow);
    if (existing.name.isEmpty())
        return;

    Player edited = existing;
    if (!openPlayerEditDialog(edited, true))
        return;

    // Prüfe ob Spieler gelöscht werden soll
    if (edited.name == "__DELETE__")
    {
        // Entferne aus der Spielerliste
        list.players.erase(
            std::remove_if(list.players.begin(), list.players.end(),
                           [&originalKey](const Player &p)
                           { return p.name == originalKey; }),
            list.players.end());

        // Entferne Teilnahme-Historie
        if (attendanceRecords.contains(originalKey))
            attendanceRecords.remove(originalKey);

        // Entferne Zeile aus der Tabelle
        model->removeRow(sourceRow);

        savePlayers();
        saveAttendance();

        QMessageBox::information(this, "Spieler gelöscht",
                                 QStringLiteral("'%1' wurde erfolgreich gelöscht.").arg(originalKey));
        return;
    }

    auto setItemText = [this, sourceRow](int column, const QString &text)
    {
        if (QStandardItem *item = model->item(sourceRow, column))
            item->setText(text);
    };

    setItemText(0, edited.name);
    setItemText(2, edited.t17name);
    setItemText(3, QString::number(edited.level));
    setItemText(4, edited.group);
    setItemText(5, formatTrainingDisplay(edited));
    if (QStandardItem *trainingItem = model->item(sourceRow, 5))
        trainingItem->setToolTip(trainingTooltip(edited));
    setItemText(6, edited.comment);
    setItemText(7, edited.joinDate.toString(Qt::ISODate));
    setItemText(8, edited.rank);

    const QString newKey = edited.name;
    for (int c = 0; c < model->columnCount(); ++c)
    {
        if (QStandardItem *item = model->item(sourceRow, c))
            item->setData(newKey, Qt::UserRole + 1);
    }

    if (ensureGroupRegistered(edited.group))
        saveGroups();

    bool replaced = false;
    for (Player &p : list.players)
    {
        if (p.name == originalKey)
        {
            p = edited;
            replaced = true;
            break;
        }
    }
    if (!replaced)
        list.addOrMerge(edited);

    validateRow(sourceRow);
    savePlayers();
}
void MainWindow::showPlayerAttendance()
{
    if (!table || !model)
        return;

    auto resolveSelection = [this](int &sourceRow, QString &playerKey, QString &playerName) -> bool
    {
        QModelIndex proxyIndex = table->currentIndex();
        if (!proxyIndex.isValid())
        {
            QMessageBox::information(this, "Keine Auswahl", "Bitte zuerst einen Spieler auswählen.");
            return false;
        }
        sourceRow = proxy ? proxy->mapToSource(proxyIndex).row() : proxyIndex.row();
        if (sourceRow < 0 || sourceRow >= model->rowCount())
            return false;
        playerKey = playerKeyForRow(sourceRow);
        if (playerKey.isEmpty())
            return false;
        if (QStandardItem *nameItem = model->item(sourceRow, 0))
            playerName = nameItem->text();
        else
            playerName = playerKey;
        return true;
    };

    int sourceRow = -1;
    QString playerKey;
    QString playerName;
    if (!resolveSelection(sourceRow, playerKey, playerName))
        return;

    const QList<QJsonObject> entries = attendanceRecords.value(playerKey);

    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("Teilnahmen - %1").arg(playerName));
    QVBoxLayout *layout = new QVBoxLayout(&dlg);
    if (entries.isEmpty())
    {
        layout->addWidget(new QLabel("Keine Einträge vorhanden.", &dlg));
    }
    else
    {
        QTreeWidget *tree = new QTreeWidget(&dlg);
        tree->setColumnCount(4);
        tree->setHeaderLabels({"Datum", "Typ", "Name", "Karte"});
        for (auto it = entries.crbegin(); it != entries.crend(); ++it)
        {
            QString date = it->value("date").toString();
            QString type = it->value("type").toString();
            QString name = it->value("name").toString();
            QString map = it->value("map").toString();
            QTreeWidgetItem *item = new QTreeWidgetItem({date, type, name, map});
            tree->addTopLevelItem(item);
        }
        tree->resizeColumnToContents(0);
        layout->addWidget(tree);
    }

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dlg);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    layout->addWidget(buttons);
    dlg.exec();
}
void MainWindow::showPlayerSoldbuch()
{
    if (!table || !model)
        return;

    auto resolveSelection = [this](int &sourceRow, QString &playerKey, QString &playerName) -> bool
    {
        QModelIndex proxyIndex = table->currentIndex();
        if (!proxyIndex.isValid())
        {
            QMessageBox::information(this, "Keine Auswahl", "Bitte zuerst einen Spieler auswählen.");
            return false;
        }
        sourceRow = proxy ? proxy->mapToSource(proxyIndex).row() : proxyIndex.row();
        if (sourceRow < 0 || sourceRow >= model->rowCount())
            return false;
        playerKey = playerKeyForRow(sourceRow);
        if (playerKey.isEmpty())
            return false;
        if (QStandardItem *nameItem = model->item(sourceRow, 0))
            playerName = nameItem->text();
        else
            playerName = playerKey;
        return true;
    };

    int sourceRow = -1;
    QString playerKey;
    QString playerName;
    if (!resolveSelection(sourceRow, playerKey, playerName))
        return;

    showSoldbuchDialogForPlayer(playerKey, playerName);
}

void MainWindow::showSoldbuchDialogForPlayer(const QString &playerKey, const QString &playerName)
{
    const QList<QJsonObject> entries = soldbuchRecords.value(playerKey);

    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("Soldbuch - %1").arg(playerName));
    QVBoxLayout *layout = new QVBoxLayout(&dlg);
    if (entries.isEmpty())
    {
        layout->addWidget(new QLabel("Keine Einträge vorhanden.", &dlg));
    }
    else
    {
        QTreeWidget *tree = new QTreeWidget(&dlg);
        tree->setColumnCount(3);
        tree->setHeaderLabels({"Datum", "Eintrag", "Details"});
        for (auto it = entries.crbegin(); it != entries.crend(); ++it)
        {
            QString date = it->value("date").toString();
            QString kind = it->value("kind").toString();
            QJsonObject details = *it;
            details.remove("date");
            details.remove("timestamp");
            details.remove("kind");
            QStringList pairs;
            for (auto dt = details.begin(); dt != details.end(); ++dt)
                pairs << QStringLiteral("%1: %2").arg(dt.key(), dt.value().toVariant().toString());
            QString detailText = pairs.join(", ");
            QTreeWidgetItem *item = new QTreeWidgetItem({date, kind, detailText});
            tree->addTopLevelItem(item);
        }
        tree->resizeColumnToContents(0);
        layout->addWidget(tree);
    }

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dlg);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    layout->addWidget(buttons);
    dlg.exec();
}
void MainWindow::promotePlayer() {}
void MainWindow::demotePlayer() {}

void MainWindow::importCsv()
{
    QString fileName = QFileDialog::getOpenFileName(this, QStringLiteral("CSV importieren"), QString(), QStringLiteral("CSV/TSV Dateien (*.csv *.tsv *.txt);;Alle Dateien (*.*)"));
    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, QStringLiteral("Import fehlgeschlagen"), QStringLiteral("Datei konnte nicht geöffnet werden:\n%1").arg(file.errorString()));
        appendErrorLog("importCsv", QStringLiteral("Datei konnte nicht geöffnet werden: %1").arg(file.errorString()));
        return;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    QStringList rawLines;
    while (!stream.atEnd())
        rawLines << stream.readLine();
    file.close();

    auto detectDelimiter = [](const QStringList &lines) -> QChar
    {
        const QList<QChar> candidates = {QChar('\t'), QChar(';'), QChar(','), QChar('|')};
        QMap<QChar, int> scores;
        for (QChar cand : candidates)
            scores.insert(cand, 0);
        int inspected = 0;
        for (const QString &line : lines)
        {
            const QString trimmed = line.trimmed();
            if (trimmed.isEmpty())
                continue;
            for (QChar cand : candidates)
                scores[cand] += trimmed.count(cand);
            if (++inspected >= 12)
                break;
        }
        QChar best = QChar(',');
        int bestScore = -1;
        for (QChar cand : candidates)
        {
            if (scores.value(cand) > bestScore)
            {
                bestScore = scores.value(cand);
                best = cand;
            }
        }
        if (bestScore <= 0)
            return QChar(';');
        return best;
    };

    const QChar delimiter = detectDelimiter(rawLines);

    auto splitLine = [delimiter](const QString &line) -> QStringList
    {
        QString normalized = line;
        normalized.remove('\r');
        QStringList cols;
        QString current;
        bool inQuotes = false;
        for (int i = 0; i < normalized.size(); ++i)
        {
            const QChar ch = normalized.at(i);
            if (ch == '"')
            {
                if (inQuotes && i + 1 < normalized.size() && normalized.at(i + 1) == '"')
                {
                    current.append('"');
                    ++i;
                }
                else
                {
                    inQuotes = !inQuotes;
                }
            }
            else if (ch == delimiter && !inQuotes)
            {
                cols << current;
                current.clear();
            }
            else
            {
                current.append(ch);
            }
        }
        cols << current;
        for (QString &value : cols)
        {
            value = value.trimmed();
            if (value.startsWith('"') && value.endsWith('"') && value.size() >= 2)
                value = value.mid(1, value.size() - 2).trimmed();
            if (!value.isEmpty() && value.at(0) == QChar(0xFEFF))
                value.remove(0, 1);
        }
        return cols;
    };

    QList<QStringList> rows;
    rows.reserve(rawLines.size());
    for (const QString &line : rawLines)
    {
        if (line.trimmed().isEmpty())
            continue;
        rows.append(splitLine(line));
    }

    if (rows.isEmpty())
    {
        QMessageBox::information(this, QStringLiteral("Import"), QStringLiteral("Die Datei enthielt keine verwertbaren Daten."));
        appendErrorLog("importCsv", QStringLiteral("Datei leer oder keine verwertbaren Zeilen"));
        return;
    }

    auto normalizeToken = [](const QString &text) -> QString
    {
        QString lowered = text.normalized(QString::NormalizationForm_D).toLower();
        QString result;
        result.reserve(lowered.size());
        bool lastWasSpace = true;
        for (const QChar ch : lowered)
        {
            const QChar::Category category = ch.category();
            if (category == QChar::Mark_NonSpacing || category == QChar::Mark_SpacingCombining || category == QChar::Mark_Enclosing)
                continue;
            if (ch.isLetterOrNumber())
            {
                result.append(ch);
                lastWasSpace = false;
            }
            else
            {
                if (!lastWasSpace)
                {
                    result.append(' ');
                    lastWasSpace = true;
                }
            }
        }
        return result.simplified();
    };

    auto looksLikeHeader = [&](const QStringList &cols) -> bool
    {
        for (const QString &value : cols)
        {
            QString norm = normalizeToken(value);
            if (norm == "name" || norm == "spielername" || norm.contains(" name"))
                return true;
        }
        return false;
    };

    int headerRow = -1;
    for (int i = 0; i < rows.size(); ++i)
    {
        if (looksLikeHeader(rows.at(i)))
        {
            headerRow = i;
            break;
        }
    }
    if (headerRow < 0)
        headerRow = 0;

    const QStringList headers = rows.at(headerRow);
    QVector<QString> normalizedHeaders;
    normalizedHeaders.reserve(headers.size());
    for (const QString &h : headers)
        normalizedHeaders.append(normalizeToken(h));

    auto findColumn = [&](const QStringList &candidates) -> int
    {
        for (const QString &candidate : candidates)
        {
            QString needle = normalizeToken(candidate);
            if (needle.isEmpty())
                continue;
            for (int idx = 0; idx < normalizedHeaders.size(); ++idx)
            {
                if (normalizedHeaders.at(idx) == needle)
                    return idx;
            }
        }
        for (const QString &candidate : candidates)
        {
            QString needle = normalizeToken(candidate);
            if (needle.isEmpty())
                continue;
            for (int idx = 0; idx < normalizedHeaders.size(); ++idx)
            {
                if (normalizedHeaders.at(idx).contains(needle))
                    return idx;
            }
        }
        return -1;
    };

    auto findColumnByParts = [&](const QStringList &parts) -> int
    {
        for (int idx = 0; idx < normalizedHeaders.size(); ++idx)
        {
            bool matches = true;
            for (const QString &part : parts)
            {
                QString needle = normalizeToken(part);
                if (needle.isEmpty())
                    continue;
                if (!normalizedHeaders.at(idx).contains(needle))
                {
                    matches = false;
                    break;
                }
            }
            if (matches)
                return idx;
        }
        return -1;
    };

    const int nameIdx = findColumn(QStringList() << QStringLiteral("Name") << QStringLiteral("Spielername"));
    if (nameIdx < 0)
    {
        QMessageBox::warning(this, QStringLiteral("Import"), QStringLiteral("Die Datei enthält keine erkennbaren Namensspalten."));
        appendErrorLog("importCsv", QStringLiteral("Keine Namensspalte erkannt"));
        return;
    }
    const int t17Idx = findColumn(QStringList() << QStringLiteral("T17") << QStringLiteral("T17 Name") << QStringLiteral("T17-Name"));
    const int joinIdx = findColumn(QStringList() << QStringLiteral("Datum Vollmitglied") << QStringLiteral("Eintrittsdatum") << QStringLiteral("Beitrittsdatum") << QStringLiteral("Eintritt") << QStringLiteral("Beitritt"));
    const int rankIdx = findColumn(QStringList() << QStringLiteral("Dienstgrad") << QStringLiteral("Dienstgradstufe") << QStringLiteral("Dienstrang") << QStringLiteral("Rang"));
    const int groupIdx = findColumn(QStringList() << QStringLiteral("Gruppe") << QStringLiteral("Trupp") << QStringLiteral("Squad"));
    const int levelIdx = findColumn(QStringList() << QStringLiteral("Level") << QStringLiteral("Dienstgradstufe") << QStringLiteral("Stufe"));
    const int commentIdx = findColumn(QStringList() << QStringLiteral("Kommentar") << QStringLiteral("Notiz"));
    const int nextRankIdx = findColumn(QStringList() << QStringLiteral("Nächster Rang") << QStringLiteral("Naechster Rang") << QStringLiteral("Next Rank"));
    const int lastPromotionIdx = findColumn(QStringList() << QStringLiteral("Datum letzte Beförderung") << QStringLiteral("Letzte Beförderung") << QStringLiteral("Last Promotion"));
    int trainingsTotalIdx = findColumnByParts(QStringList() << QStringLiteral("Training") << QStringLiteral("Gesamt"));
    const int trainingsRankIdxFallback = findColumnByParts(QStringList() << QStringLiteral("Training") << QStringLiteral("Rang"));
    const int trainingsSinceIdx = findColumnByParts(QStringList() << QStringLiteral("Training") << QStringLiteral("seit"));
    const int eventsIdx = findColumnByParts(QStringList() << QStringLiteral("Event"));
    const int reserveIdx = findColumnByParts(QStringList() << QStringLiteral("Reserve"));
    if (trainingsTotalIdx < 0)
        trainingsTotalIdx = findColumn(QStringList() << QStringLiteral("Trainings") << QStringLiteral("Teilnahmen"));
    const int trainingsRankIdx = trainingsSinceIdx >= 0 ? trainingsSinceIdx : (trainingsRankIdxFallback >= 0 ? trainingsRankIdxFallback : trainingsTotalIdx);

    auto valueAt = [](const QStringList &cols, int idx) -> QString
    {
        if (idx < 0 || idx >= cols.size())
            return QString();
        return cols.at(idx).trimmed();
    };

    auto parseIntValue = [](const QString &text) -> int
    {
        QString cleaned;
        cleaned.reserve(text.size());
        for (const QChar ch : text)
        {
            if (ch.isDigit())
                cleaned.append(ch);
            else if (ch == '-' && cleaned.isEmpty())
                cleaned.append(ch);
        }
        bool ok = false;
        int val = cleaned.toInt(&ok);
        return ok ? val : 0;
    };

    auto parseDateValue = [](QString text) -> QDate
    {
        text = text.trimmed();
        if (text.isEmpty())
            return QDate();
        int spaceIdx = text.indexOf(' ');
        if (spaceIdx > 0)
            text = text.left(spaceIdx);
        text.replace(',', '.');
        static const QStringList formats = {QStringLiteral("yyyy-MM-dd"), QStringLiteral("dd.MM.yyyy"), QStringLiteral("dd.MM.yy"), QStringLiteral("dd/MM/yyyy"), QStringLiteral("dd/MM/yy"), QStringLiteral("dd-MM-yyyy"), QStringLiteral("dd-MM-yy"), QStringLiteral("yyyy.MM.dd"), QStringLiteral("yyyy/MM/dd"), QStringLiteral("MM/dd/yyyy"), QStringLiteral("MM/dd/yy")};
        for (const QString &fmt : formats)
        {
            QDate parsed = QDate::fromString(text, fmt);
            if (parsed.isValid())
                return parsed;
        }
        bool ok = false;
        int serial = text.toInt(&ok);
        if (ok && serial > 0)
        {
            QDate base(1899, 12, 30);
            return base.addDays(serial);
        }
        return QDate();
    };

    int imported = 0;
    int merged = 0;
    int skipped = 0;
    bool groupsChanged = false;
    for (int row = headerRow + 1; row < rows.size(); ++row)
    {
        const QStringList &cols = rows.at(row);
        const QString name = valueAt(cols, nameIdx);
        if (name.trimmed().isEmpty())
        {
            ++skipped;
            continue;
        }
        const QString nameNormalized = normalizeToken(name);
        if (nameNormalized == "name" || nameNormalized == "spielername")
        {
            ++skipped;
            continue;
        }

        Player player;
        player.name = name.trimmed();
        player.t17name = valueAt(cols, t17Idx);
        player.group = valueAt(cols, groupIdx);
        player.comment = valueAt(cols, commentIdx);
        player.rank = valueAt(cols, rankIdx);
        player.nextRank = valueAt(cols, nextRankIdx);
        player.level = parseIntValue(valueAt(cols, levelIdx));
        player.joinDate = parseDateValue(valueAt(cols, joinIdx));
        player.lastPromotionDate = parseDateValue(valueAt(cols, lastPromotionIdx));
        player.attendance = parseIntValue(valueAt(cols, trainingsRankIdx));
        player.totalAttendance = parseIntValue(valueAt(cols, trainingsTotalIdx));
        if (player.attendance <= 0 && player.totalAttendance > 0)
            player.attendance = player.totalAttendance;
        if (player.totalAttendance <= 0 && player.attendance > 0)
            player.totalAttendance = player.attendance;
        player.events = parseIntValue(valueAt(cols, eventsIdx));
        player.totalEvents = player.events;
        player.reserve = parseIntValue(valueAt(cols, reserveIdx));
        player.totalReserve = player.reserve;
        if (!player.group.isEmpty())
            groupsChanged = ensureGroupRegistered(player.group) || groupsChanged;

        const int before = static_cast<int>(list.players.size());
        list.addOrMerge(player);
        if (static_cast<int>(list.players.size()) == before)
            ++merged;
        else
            ++imported;
    }

    if (groupsChanged)
        saveGroups();

    refreshModelFromList();
    validateAllRows();
    savePlayers();

    QMessageBox::information(this, QStringLiteral("Import abgeschlossen"), QStringLiteral("%1 Spieler verarbeitet (%2 neu, %3 aktualisiert). %4 Zeilen übersprungen.").arg(imported + merged).arg(imported).arg(merged).arg(skipped));
}

// Test-Hilfsmethode: direkter Import ohne QFileDialog
bool MainWindow::importCsvFile(const QString &filePath, int *outImported, int *outMerged, int *outSkipped)
{
    if (outImported)
        *outImported = 0;
    if (outMerged)
        *outMerged = 0;
    if (outSkipped)
        *outSkipped = 0;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        appendErrorLog("importCsvFile", QStringLiteral("Datei konnte nicht geöffnet werden: %1").arg(file.errorString()));
        return false;
    }
    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    QStringList rawLines;
    while (!stream.atEnd())
        rawLines << stream.readLine();
    file.close();
    if (rawLines.isEmpty())
        return false;
    auto detectDelimiter = [](const QStringList &lines) -> QChar
    {
        const QList<QChar> candidates = {QChar('\t'), QChar(';'), QChar(','), QChar('|')};
        QMap<QChar, int> scores;
        for (QChar c : candidates)
            scores[c] = 0;
        int inspected = 0;
        for (const QString &line : lines)
        {
            QString t = line.trimmed();
            if (t.isEmpty())
                continue;
            for (QChar c : candidates)
                scores[c] += t.count(c);
            if (++inspected >= 12)
                break;
        }
        QChar best = ',';
        int bestScore = -1;
        for (QChar c : candidates)
            if (scores[c] > bestScore)
            {
                bestScore = scores[c];
                best = c;
            }
        return bestScore <= 0 ? QChar(';') : best;
    };
    const QChar delimiter = detectDelimiter(rawLines);
    auto splitLine = [delimiter](const QString &line) -> QStringList
    {
        QString normalized = line; normalized.remove('\r'); QStringList cols; QString current; bool inQuotes=false;
        for (int i=0;i<normalized.size();++i) { QChar ch=normalized.at(i); if (ch=='"') { if (inQuotes && i+1<normalized.size() && normalized.at(i+1)=='"'){ current.append('"'); ++i;} else inQuotes=!inQuotes; }
            else if (ch==delimiter && !inQuotes) { cols<<current; current.clear(); } else current.append(ch); }
        cols<<current; for (QString &v: cols){ v=v.trimmed(); if (v.startsWith('"') && v.endsWith('"') && v.size()>=2) v=v.mid(1,v.size()-2).trimmed(); if(!v.isEmpty() && v.at(0)==QChar(0xFEFF)) v.remove(0,1);} return cols; };
    QList<QStringList> rows;
    for (const QString &line : rawLines)
    {
        if (line.trimmed().isEmpty())
            continue;
        rows << splitLine(line);
    }
    if (rows.isEmpty())
        return false;
    int headerRow = 0;
    QStringList headers = rows.at(headerRow);
    QStringList normalizedHeaders;
    for (const QString &h : headers)
    {
        QString n = h.trimmed().toLower();
        n.replace(' ', '_');
        normalizedHeaders << n;
    }
    auto normalizeToken = [](QString t)
    { t=t.trimmed().toLower(); t.replace(' ','_'); return t; };
    auto findColumn = [&](const QStringList &candidates)
    { for (const QString &cand: candidates){ QString needle=normalizeToken(cand); if (needle.isEmpty()) continue; for (int i=0;i<normalizedHeaders.size();++i){ if (normalizedHeaders.at(i)==needle) return i; } } for (const QString &cand: candidates){ QString needle=normalizeToken(cand); if (needle.isEmpty()) continue; for (int i=0;i<normalizedHeaders.size();++i){ if (normalizedHeaders.at(i).contains(needle)) return i; } } return -1; };
    auto findColumnByParts = [&](const QStringList &parts)
    { for (int i=0;i<normalizedHeaders.size();++i){ bool matches=true; for (const QString &part: parts){ QString needle=normalizeToken(part); if (needle.isEmpty()) continue; if (!normalizedHeaders.at(i).contains(needle)){ matches=false; break; } } if (matches) return i; } return -1; };
    int nameIdx = findColumn(QStringList{QStringLiteral("Name"), QStringLiteral("Spielername")});
    if (nameIdx < 0)
        return false;
    int groupIdx = findColumn(QStringList{QStringLiteral("Gruppe"), QStringLiteral("Trupp")});
    int levelIdx = findColumn(QStringList{QStringLiteral("Level")});
    auto valueAt = [](const QStringList &cols, int idx) -> QString
    { return (idx < 0 || idx >= cols.size()) ? QString() : cols.at(idx).trimmed(); };
    auto parseIntValue = [](const QString &text) -> int
    { QString cleaned; for (QChar ch: text){ if (ch.isDigit()) cleaned.append(ch); else if (ch=='-' && cleaned.isEmpty()) cleaned.append(ch);} bool ok=false; int v=cleaned.toInt(&ok); return ok? v:0; };
    int imported = 0, merged = 0, skipped = 0;
    bool groupsChanged = false;
    for (int r = headerRow + 1; r < rows.size(); ++r)
    {
        const QStringList &cols = rows.at(r);
        QString name = valueAt(cols, nameIdx);
        if (name.trimmed().isEmpty())
        {
            ++skipped;
            continue;
        }
        Player player;
        player.name = name.trimmed();
        player.group = valueAt(cols, groupIdx);
        player.level = parseIntValue(valueAt(cols, levelIdx));
        if (!player.group.isEmpty())
            groupsChanged = ensureGroupRegistered(player.group) || groupsChanged;
        int before = list.players.size();
        list.addOrMerge(player);
        if ((int)list.players.size() == before)
            ++merged;
        else
            ++imported;
    }
    if (groupsChanged)
        saveGroups();
    refreshModelFromList();
    validateAllRows();
    savePlayers();
    if (outImported)
        *outImported = imported;
    if (outMerged)
        *outMerged = merged;
    if (outSkipped)
        *outSkipped = skipped;
    return true;
}

bool MainWindow::isPlayerFlaggedNoResponse(const QString &playerName) const
{
    if (!model)
        return false;
    int row = rowForPlayerKey(playerName);
    if (row < 0)
        return false;
    QStandardItem *statusItem = model->item(row, 1);
    if (!statusItem)
        return false;
    return statusItem->text() == QStringLiteral("❌");
}

QMap<QString, QStringList> MainWindow::groupingSnapshot() const
{
    QMap<QString, QStringList> grouped;
    for (const Player &p : list.players)
    {
        QString g = p.group.trimmed();
        if (g.isEmpty())
            g = QStringLiteral("Ohne Gruppe");
        grouped[g].append(p.name);
    }
    return grouped;
}

QString MainWindow::readErrorLogContents() const
{
    QFile f(errorLogPath());
    if (!f.exists() || !f.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();
    QTextStream ts(&f);
    ts.setEncoding(QStringConverter::Utf8);
    return ts.readAll();
}

void MainWindow::simulateNoResponseIncrement(const QStringList &selectedKeys)
{
    QSet<QString> sel(selectedKeys.begin(), selectedKeys.end());
    for (Player &p : list.players)
    {
        if (!sel.contains(p.name))
        {
            if (incrementCounterOnNoResponse)
                p.noResponseCounter++;
        }
    }
    refreshModelFromList();
    validateAllRows();
}
void MainWindow::testAddPlayer(const Player &p)
{
    Player copy = p;
    list.addOrMerge(copy);
}
void MainWindow::testRebuildModel()
{
    refreshModelFromList();
    validateAllRows();
}
void MainWindow::exportCsv()
{
    QString filter = QStringLiteral("CSV (*.csv);;TSV (*.tsv);;Alle Dateien (*.*)");
    const QString defaultBase = QStringLiteral("ClanManager_Export_%1").arg(nowDate().toString("yyyy-MM-dd"));
    QString fileName = QFileDialog::getSaveFileName(this, QStringLiteral("Spielerliste exportieren"), QDir::homePath() + "/" + defaultBase + ".csv", filter);
    if (fileName.isEmpty())
        return;

    // Bestimme Delimiter anhand der Dateiendung
    QChar delimiter = '\t';
    if (fileName.endsWith(".csv", Qt::CaseInsensitive))
        delimiter = ';';
    else if (fileName.endsWith(".tsv", Qt::CaseInsensitive))
        delimiter = '\t';

    auto quoteField = [delimiter](const QString &value) -> QString
    {
        QString v = value;
        bool needQuote = v.contains('\n') || v.contains('\r') || v.contains('"') || v.contains(delimiter);
        v.replace('"', "\"\"");
        if (needQuote)
            return '"' + v + '"';
        return v;
    };

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
    {
        QMessageBox::warning(this, QStringLiteral("Export fehlgeschlagen"), QStringLiteral("Datei konnte nicht geschrieben werden:\n%1").arg(file.errorString()));
        appendErrorLog("exportCsv", QStringLiteral("Datei konnte nicht geschrieben werden: %1").arg(file.errorString()));
        return;
    }
    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);

    // Kopfzeile (Header) passend zu Player::toCsvLine()-Reihenfolge
    QStringList headers;
    headers << QStringLiteral("Name")
            << QStringLiteral("T17-Name")
            << QStringLiteral("Level")
            << QStringLiteral("Gruppe")
            << QStringLiteral("Training (Rang)")
            << QStringLiteral("Kommentar")
            << QStringLiteral("Beitrittsdatum")
            << QStringLiteral("Dienstrang")
            << QStringLiteral("Datum letzte Beförderung")
            << QStringLiteral("Nächster Rang")
            << QStringLiteral("Trainings (Gesamt)")
            << QStringLiteral("Events")
            << QStringLiteral("Events (Gesamt)")
            << QStringLiteral("Reserve")
            << QStringLiteral("Reserve (Gesamt)")
            << QStringLiteral("NoResponseCounter");

    // Header schreiben
    for (int i = 0; i < headers.size(); ++i)
    {
        if (i)
            stream << delimiter;
        stream << quoteField(headers.at(i));
    }
    stream << '\n';

    // Daten schreiben
    for (const Player &p : list.players)
    {
        QStringList cols;
        cols << p.name
             << p.t17name
             << QString::number(p.level)
             << p.group
             << QString::number(p.attendance)
             << p.comment
             << p.joinDate.toString(Qt::ISODate)
             << p.rank
             << p.lastPromotionDate.toString(Qt::ISODate)
             << p.nextRank
             << QString::number(p.totalAttendance)
             << QString::number(p.events)
             << QString::number(p.totalEvents)
             << QString::number(p.reserve)
             << QString::number(p.totalReserve)
             << QString::number(p.noResponseCounter);

        for (int i = 0; i < cols.size(); ++i)
        {
            if (i)
                stream << delimiter;
            stream << quoteField(cols.at(i));
        }
        stream << '\n';
    }

    stream.flush();
    file.close();
    QMessageBox msg(this);
    msg.setWindowTitle(QStringLiteral("Export abgeschlossen"));
    msg.setText(QStringLiteral("%1 Spieler exportiert nach:\n%2").arg(list.players.size()).arg(fileName));
    QPushButton *openBtn = msg.addButton(QStringLiteral("Im Finder zeigen"), QMessageBox::ActionRole);
    msg.addButton(QMessageBox::Ok);
    msg.exec();
    if (msg.clickedButton() == openBtn)
    {
        QProcess::execute(QStringLiteral("open"), QStringList() << QStringLiteral("-R") << fileName);
    }
}
void MainWindow::addAttendance() {}
void MainWindow::assignAttendanceMulti()
{
    if (list.players.empty())
    {
        QMessageBox::information(this, "Keine Spieler", "Es sind keine Spieler vorhanden.");
        return;
    }

    QDialog dlg(this);
    dlg.setWindowTitle("Training/Event zuweisen");
    dlg.resize(740, 660);
    QVBoxLayout *outer = new QVBoxLayout(&dlg);

    QLabel *playerLbl = new QLabel("Spieler auswählen (nach Gruppen):", &dlg);
    outer->addWidget(playerLbl);

    QTreeWidget *playerTree = new QTreeWidget(&dlg);
    playerTree->setHeaderHidden(true);
    playerTree->setSelectionMode(QAbstractItemView::NoSelection);
    playerTree->setRootIsDecorated(true);
    playerTree->setUniformRowHeights(true);
    // group players
    QMap<QString, QList<Player>> grouped;
    const QString noGroupLabel = QStringLiteral("Ohne Gruppe");
    for (const Player &p : list.players)
    {
        QString grp = p.group.trimmed();
        if (grp.isEmpty())
            grp = noGroupLabel;
        grouped[grp].append(p);
    }
    // render groups in known order, then extras
    QStringList renderOrder;
    QSet<QString> seen;
    for (const QString &g : groups)
    {
        if (grouped.contains(g))
        {
            renderOrder.append(g);
            seen.insert(g);
        }
    }
    QStringList extras;
    for (auto it = grouped.cbegin(); it != grouped.cend(); ++it)
    {
        if (it.key() != noGroupLabel && !seen.contains(it.key()))
            extras.append(it.key());
    }
    std::sort(extras.begin(), extras.end(), [](const QString &a, const QString &b)
              { return QString::localeAwareCompare(a, b) < 0; });
    renderOrder.append(extras);
    if (grouped.contains(noGroupLabel))
        renderOrder.append(noGroupLabel);

    for (const QString &grp : renderOrder)
    {
        QTreeWidgetItem *groupItem = new QTreeWidgetItem(playerTree);
        groupItem->setText(0, QStringLiteral("%1 (%2)").arg(grp).arg(grouped.value(grp).size()));
        groupItem->setData(0, Qt::UserRole, QString());
        groupItem->setFlags(groupItem->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        groupItem->setCheckState(0, Qt::Unchecked);
        for (const Player &p : grouped.value(grp))
        {
            const QString key = p.name;
            if (key.isEmpty())
                continue;
            QString label = p.name;
            if (!p.t17name.isEmpty() && p.t17name != p.name)
                label = QStringLiteral("%1 (%2)").arg(p.name, p.t17name);
            QTreeWidgetItem *it = new QTreeWidgetItem(groupItem);
            it->setText(0, label);
            it->setData(0, Qt::UserRole, key);
            it->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
            it->setCheckState(0, Qt::Unchecked);
        }
        groupItem->setExpanded(false);
    }
    if (!playerTree->topLevelItemCount())
    {
        QMessageBox::information(this, "Keine Spieler", "Es sind keine Spieler vorhanden.");
        return;
    }
    outer->addWidget(playerTree, 1);

    // Importleiste mit OCR-Upload und Auswahl-Zusammenfassung
    QHBoxLayout *importRow = new QHBoxLayout;
    QPushButton *uploadBtn = new QPushButton("Spielerliste hochladen", &dlg);
    uploadBtn->setToolTip("Bild (Screenshot/Foto) mit Namensliste laden und per OCR Spieler markieren");
    QLabel *selectedSummary = new QLabel("Ausgewählt: 0", &dlg);
    importRow->addWidget(uploadBtn);
    importRow->addStretch();
    importRow->addWidget(selectedSummary);
    outer->addLayout(importRow);

    auto updateSelectedCount = [playerTree, selectedSummary]()
    {
        int count = 0;
        for (int i = 0; i < playerTree->topLevelItemCount(); ++i)
        {
            QTreeWidgetItem *groupItem = playerTree->topLevelItem(i);
            for (int j = 0; j < groupItem->childCount(); ++j)
            {
                if (groupItem->child(j)->checkState(0) == Qt::Checked)
                    ++count;
            }
        }
        selectedSummary->setText(QStringLiteral("Ausgewählt: %1").arg(count));
    };

    // Synchronisiere Gruppen- und Kind-Häkchen (tristate Verhalten per Code) und aktualisiere Zähler
    QObject::connect(playerTree, &QTreeWidget::itemChanged, &dlg, [playerTree, updateSelectedCount](QTreeWidgetItem *item, int column)
                     {
        Q_UNUSED(column);
        static bool guard = false;
        if (guard) return;
        guard = true;
        if (!item)
        {
            guard = false;
            return;
        }
        if (!item->parent())
        {
            // Top-Level: setze alle Kinder auf gleichen Zustand
            Qt::CheckState st = item->checkState(0);
            for (int i = 0; i < item->childCount(); ++i)
                item->child(i)->setCheckState(0, st);
        }
        else
        {
            // Kind geändert: aggregiere Zustand zum Parent
            QTreeWidgetItem *parent = item->parent();
            int total = parent->childCount();
            int checked = 0;
            for (int i = 0; i < total; ++i)
            {
                if (parent->child(i)->checkState(0) == Qt::Checked)
                    ++checked;
            }
            if (checked == 0)
                parent->setCheckState(0, Qt::Unchecked);
            else if (checked == total)
                parent->setCheckState(0, Qt::Checked);
            else
                parent->setCheckState(0, Qt::PartiallyChecked);
        }
        updateSelectedCount();
        guard = false; });

    // OCR Upload: Bilddatei -> Tesseract -> Namen erkennen -> Häkchen setzen
    QObject::connect(uploadBtn, &QPushButton::clicked, &dlg, [this, &dlg, playerTree, updateSelectedCount]()
                     {
        QFileDialog::Options opts;
        opts |= QFileDialog::DontUseNativeDialog;
        QString file = QFileDialog::getOpenFileName(&dlg, QStringLiteral("Bild mit Spielerliste wählen"), QDir::homePath(),
                                                    QStringLiteral("Bilder (*.png *.jpg *.jpeg *.bmp *.tif *.tiff);;Alle Dateien (*.*)"), nullptr, opts);
        if (file.isEmpty())
            return;

        QProcess proc;
        QString program = QStringLiteral("tesseract");
        QStringList args;
        // Deutsch als OCR-Sprache verwenden
        args << file << QStringLiteral("stdout") << QStringLiteral("-l") << QStringLiteral("deu");
        proc.start(program, args);
        if (!proc.waitForStarted(5000))
        {
            QMessageBox::warning(&dlg, QStringLiteral("Tesseract fehlt"),
                                 QStringLiteral("Tesseract konnte nicht gestartet werden. Bitte installieren (z.B. via Homebrew: brew install tesseract)."));
            return;
        }
        proc.waitForFinished(-1);
        QByteArray out = proc.readAllStandardOutput();
        QByteArray err = proc.readAllStandardError();
        if (proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0)
        {
            QMessageBox::warning(&dlg, QStringLiteral("OCR-Fehler"),
                                 QStringLiteral("Die Texterkennung ist fehlgeschlagen.\n%1").arg(QString::fromUtf8(err)));
            return;
        }
        QString textRaw = QString::fromUtf8(out);
        QString textLower = textRaw.toLower();

        // Event-Metadaten aus OCR extrahieren (falls vorhanden)
        QString extractedEventName;
        QString extractedDate;
        QString extractedMap;
        bool isTraining = false;

        // Suche nach Event-Name (erste Zeile oder Zeile mit "vs" oder spezifischen Mustern)
        QStringList allLines = textRaw.split(QRegularExpression("[\\r\\n]+"), Qt::SkipEmptyParts);

        // Training vs Event Erkennung
        QStringList trainingKeywords = {"training", "clantraining", "freitagstraining", "montagstraining",
                                        "übung", "practice", "drill"};
        QStringList eventKeywords = {"event", "vs", "versus", "match", "scrim", "scrimmage", "gegen"};

        for (const QString &line : allLines)
        {
            QString lineLower = line.toLower();
            for (const QString &kw : trainingKeywords)
            {
                if (lineLower.contains(kw))
                {
                    isTraining = true;
                    break;
                }
            }
            if (!isTraining)
            {
                for (const QString &kw : eventKeywords)
                {
                    if (lineLower.contains(kw))
                    {
                        isTraining = false;
                        break;
                    }
                }
            }
        }
        if (!allLines.isEmpty())
        {
            // Erste nicht-leere Zeile als möglicher Event-Name
            QString firstLine = allLines.first().trimmed();
            if (firstLine.length() >= 5 && firstLine.length() <= 80)
            {
                extractedEventName = firstLine;
            }
        }

        // Suche nach Datum (Format: DD.MM.YYYY oder ähnlich)
        QRegularExpression dateRx(QStringLiteral(R"(\b(\d{1,2})[\.\-/](\d{1,2})[\.\-/](\d{4})\b)"));
        QRegularExpressionMatch dateMatch = dateRx.match(textRaw);
        if (dateMatch.hasMatch())
        {
            int day = dateMatch.captured(1).toInt();
            int month = dateMatch.captured(2).toInt();
            int year = dateMatch.captured(3).toInt();
            QDate parsedDate(year, month, day);
            if (parsedDate.isValid())
            {
                extractedDate = parsedDate.toString("dd.MM.yyyy");
            }
        }

        // Suche nach Map (häufige Maps: SME, Carentan, Foy, Kursk, etc.)
        QStringList knownMaps = {"SME", "Carentan", "Foy", "Kursk", "Stalingrad", "Omaha", "Utah",
                                 "Purple Heart Lane", "Hill 400", "Hurtgen", "Sainte", "SMDM"};
        for (const QString &map : knownMaps)
        {
            if (textRaw.contains(map, Qt::CaseInsensitive))
            {
                extractedMap = map;
                break;
            }
        }

        // Parse Zusagen/Absagen aus dem Bild (Akzeptiert, Tank, Abgelehnt)
        QSet<QString> acceptedPlayers; // Spieler die zugesagt haben
        QSet<QString> rejectedPlayers; // Spieler die abgesagt haben

        // Suche nach Status-Kategorien im OCR-Text
        QRegularExpression acceptedRx(QStringLiteral(R"(Akzeptiert\s*\((\d+)\))"), QRegularExpression::CaseInsensitiveOption);
        QRegularExpression tankRx(QStringLiteral(R"(Tank\s*\((\d+)\))"), QRegularExpression::CaseInsensitiveOption);
        QRegularExpression rejectedRx(QStringLiteral(R"(Abgelehnt\s*\((\d+)\))"), QRegularExpression::CaseInsensitiveOption);

        int acceptedIdx = -1, tankIdx = -1, rejectedIdx = -1;
        for (int i = 0; i < allLines.size(); ++i)
        {
            QString line = allLines[i];
            if (acceptedRx.match(line).hasMatch())
                acceptedIdx = i;
            if (tankRx.match(line).hasMatch())
                tankIdx = i;
            if (rejectedRx.match(line).hasMatch())
                rejectedIdx = i;
        }

        // Extrahiere Namen unter den jeweiligen Kategorien
        if (acceptedIdx >= 0)
        {
            int nextCategoryIdx = qMin(tankIdx >= 0 ? tankIdx : allLines.size(),
                                       rejectedIdx >= 0 ? rejectedIdx : allLines.size());
            for (int i = acceptedIdx + 1; i < nextCategoryIdx && i < allLines.size(); ++i)
            {
                QString line = allLines[i].trimmed();
                if (!line.isEmpty() && line.length() >= 3 && line.length() <= 48)
                {
                    acceptedPlayers.insert(line);
                }
            }
        }

        if (tankIdx >= 0)
        {
            int nextCategoryIdx = rejectedIdx >= 0 ? rejectedIdx : allLines.size();
            for (int i = tankIdx + 1; i < nextCategoryIdx && i < allLines.size(); ++i)
            {
                QString line = allLines[i].trimmed();
                if (!line.isEmpty() && line.length() >= 3 && line.length() <= 48)
                {
                    acceptedPlayers.insert(line); // Tank ist auch eine Zusage!
                }
            }
        }

        if (rejectedIdx >= 0)
        {
            for (int i = rejectedIdx + 1; i < allLines.size(); ++i)
            {
                QString line = allLines[i].trimmed();
                if (!line.isEmpty() && line.length() >= 3 && line.length() <= 48)
                {
                    rejectedPlayers.insert(line);
                }
            }
        }

        // Levenshtein-Distanz Hilfsfunktion
        auto levenshtein = [](const QString &s1, const QString &s2) -> int
        {
            const int m = s1.size(), n = s2.size();
            QVector<QVector<int>> dp(m + 1, QVector<int>(n + 1));
            for (int i = 0; i <= m; ++i)
                dp[i][0] = i;
            for (int j = 0; j <= n; ++j)
                dp[0][j] = j;
            for (int i = 1; i <= m; ++i)
            {
                for (int j = 1; j <= n; ++j)
                {
                    int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
                    dp[i][j] = std::min({dp[i - 1][j] + 1, dp[i][j - 1] + 1, dp[i - 1][j - 1] + cost});
                }
            }
            return dp[m][n];
        };

        // Bekannte Spieler per Name oder T17-Name im OCR-Text finden (exakt + Fuzzy)
        QSet<QString> recognizedKeys;
        QMap<QString, QString> existingByLower;
        for (const Player &p : list.players)
        {
            if (!p.name.isEmpty())
                existingByLower.insert(p.name.toLower(), p.name);
            if (!p.t17name.isEmpty())
                existingByLower.insert(p.t17name.toLower(), p.name);
            QString name = p.name.toLower();
            QString t17 = p.t17name.toLower();
            if (!name.isEmpty() && textLower.contains(name))
                recognizedKeys.insert(p.name);
            else if (!t17.isEmpty() && textLower.contains(t17))
                recognizedKeys.insert(p.name);
        }

        // Rang-Präfixe zum Trennen von Namen
        QStringList rankPrefixes = MainWindow::rankOptions();

        // Hilfsfunktion: Trenne Namen mit mehreren Rängen oder Leerzeichen + Großbuchstabe
        auto splitNames = [&rankPrefixes](const QString &text) -> QStringList {
            QStringList result;
            QString current = text.trimmed();
            
            // Finde alle Vorkommen von Rängen
            QList<int> rankPositions;
            for (const QString &rank : rankPrefixes) {
                if (rank.isEmpty()) continue;
                QRegularExpression rx(QRegularExpression::escape(rank), 
                                     QRegularExpression::CaseInsensitiveOption);
                QRegularExpressionMatchIterator it = rx.globalMatch(current);
                while (it.hasNext()) {
                    QRegularExpressionMatch match = it.next();
                    rankPositions << match.capturedStart();
                }
            }
            
            // Wenn mehrere Ränge: trenne bei jedem Rang
            if (rankPositions.size() > 1) {
                std::sort(rankPositions.begin(), rankPositions.end());
                for (int i = 0; i < rankPositions.size(); ++i) {
                    int start = rankPositions[i];
                    int end = (i + 1 < rankPositions.size()) ? rankPositions[i + 1] : current.length();
                    QString segment = current.mid(start, end - start).trimmed();
                    if (!segment.isEmpty() && segment.length() >= 3)
                        result << segment;
                }
                return result;
            }
            
            // Fallback: Trenne bei Leerzeichen + Kleinbuchstabe oder Großbuchstabe
            QRegularExpression splitPattern1("\\s+(?=[a-zäöüß])");
            QStringList parts1 = current.split(splitPattern1, Qt::SkipEmptyParts);
            if (parts1.size() > 1) {
                for (const QString &p : parts1) {
                    if (p.trimmed().length() >= 3)
                        result << p.trimmed();
                }
                if (!result.isEmpty()) return result;
            }
            
            QRegularExpression splitPattern2("\\s+(?=[A-ZÄÖÜ])");
            QStringList parts2 = current.split(splitPattern2, Qt::SkipEmptyParts);
            if (parts2.size() > 1) {
                for (const QString &p : parts2) {
                    if (p.trimmed().length() >= 3)
                        result << p.trimmed();
                }
                if (!result.isEmpty()) return result;
            }
            
            if (!current.isEmpty() && current.length() >= 3)
                result << current;
            return result;
        };

        // Alle Spieler aus acceptedPlayers und rejectedPlayers sammeln (mit Trennung)
        QSet<QString> allParsedPlayers;
        for (const QString &player : acceptedPlayers) {
            for (const QString &split : splitNames(player))
                allParsedPlayers.insert(split);
        }
        for (const QString &player : rejectedPlayers) {
            for (const QString &split : splitNames(player))
                allParsedPlayers.insert(split);
        }

        // Hilfsfunktion: Teile String bei mehrfach vorkommenden Rängen
        auto splitByRanks = [&rankPrefixes](const QString &text) -> QStringList
        {
            QStringList result;
            QString current = text.trimmed();
            
            // Finde alle Vorkommen von Rängen im Text
            QList<int> rankPositions;
            QList<QString> foundRanks;
            
            for (const QString &rank : rankPrefixes) {
                if (rank.isEmpty()) continue;
                
                // Suche nach Rang-Vorkommen (case-insensitive)
                QRegularExpression rx(QRegularExpression::escape(rank), 
                                     QRegularExpression::CaseInsensitiveOption);
                QRegularExpressionMatchIterator it = rx.globalMatch(current);
                while (it.hasNext()) {
                    QRegularExpressionMatch match = it.next();
                    rankPositions << match.capturedStart();
                    foundRanks << rank;
                }
            }
            
            // Wenn mehr als ein Rang gefunden wurde, trenne bei jedem Rang
            if (rankPositions.size() > 1) {
                // Sortiere nach Position
                QList<QPair<int, QString>> posRankPairs;
                for (int i = 0; i < rankPositions.size(); ++i) {
                    posRankPairs << qMakePair(rankPositions[i], foundRanks[i]);
                }
                std::sort(posRankPairs.begin(), posRankPairs.end());
                
                for (int i = 0; i < posRankPairs.size(); ++i) {
                    int start = posRankPairs[i].first;
                    int end = (i + 1 < posRankPairs.size()) ? posRankPairs[i + 1].first : current.length();
                    QString segment = current.mid(start, end - start).trimmed();
                    if (!segment.isEmpty())
                        result << segment;
                }
                return result;
            }
            
            // Fallback: Trenne bei Leerzeichen gefolgt von Kleinbuchstabe oder Großbuchstabe
            // Dies erfasst sowohl "GefrBuddy eiben" als auch "Buddy Eiben" (Namen ohne Rang)
            if (!current.isEmpty()) {
                // Versuche zuerst Trennung bei Leerzeichen + Kleinbuchstabe
                QRegularExpression splitPattern1("\\s+(?=[a-zäöüß])");
                QStringList parts1 = current.split(splitPattern1, Qt::SkipEmptyParts);
                if (parts1.size() > 1) {
                    result = parts1;
                    return result;
                }
                
                // Dann versuche Trennung bei Leerzeichen + Großbuchstabe (für Namen ohne Rang)
                // Aber nur wenn kein Rang am Anfang des zweiten Teils steht
                QRegularExpression splitPattern2("\\s+(?=[A-ZÄÖÜ])");
                QStringList parts2 = current.split(splitPattern2, Qt::SkipEmptyParts);
                if (parts2.size() > 1) {
                    // Prüfe ob Teile valide Namen sind (nicht nur einzelne Buchstaben)
                    bool allValid = true;
                    for (const QString &part : parts2) {
                        if (part.trimmed().length() < 2) {
                            allValid = false;
                            break;
                        }
                    }
                    if (allValid) {
                        result = parts2;
                        return result;
                    }
                }
            }
            
            // Kein mehrfacher Rang gefunden, gebe Original zurück
            if (result.isEmpty())
                result << current;
            return result;
        };

        // Kandidaten-Zeilen aus OCR extrahieren (Zeilen/Kommas), säubern und deduplizieren
        QSet<QString> candidates;
        const auto addCandidate = [&candidates](const QString &s)
        {
            QString c = s;
            c.replace('\t', ' ');
            c.replace(QRegularExpression("\\s+"), " ");
            c = c.trimmed();
            if (c.size() >= 3 && c.size() <= 48)
                candidates.insert(c);
        };

        // Falls wir Status-Kategorien gefunden haben, verwende diese als Kandidaten
        if (!allParsedPlayers.isEmpty())
        {
            for (const QString &player : allParsedPlayers)
            {
                QStringList subParts = splitByRanks(player);
                for (const QString &sub : subParts)
                    addCandidate(sub);
            }
        }
        
        // Falls keine Status-Kategorien gefunden wurden, verwende Fallback (alle Zeilen)
        if (candidates.isEmpty()) {
        const QStringList lines = textRaw.split(QRegularExpression("[\\r\\n]+"), Qt::SkipEmptyParts);
        for (const QString &ln : lines)
        {
            const QStringList parts = ln.split(',', Qt::SkipEmptyParts);
            if (parts.size() > 1)
            {
                for (const QString &p : parts)
                {
                    QStringList subParts = splitByRanks(p);
                    for (const QString &sub : subParts)
                        addCandidate(sub);
                }
            }
            else
            {
                QStringList subParts = splitByRanks(ln);
                for (const QString &sub : subParts)
                    addCandidate(sub);
            }
        }
        }

        // Unbekannte Spieler anlegen in Gruppe "Nicht zugewiesen" (mit Fuzzy-Matching)
        const QString unassignedGroup = QStringLiteral("Nicht zugewiesen");
        QSet<QString> createdKeys;
        QSet<QString> acceptedCreatedKeys;  // Neue Spieler die zugesagt haben
        constexpr int fuzzyThreshold = 2;
        if (!candidates.isEmpty())
        {
        for (const QString &cand : std::as_const(candidates))
        {
            const QString lower = cand.toLower();
            // Exakte Übereinstimmung prüfen
            if (existingByLower.contains(lower))
            {
                // Prüfe ob dieser existierende Spieler zugesagt hat
                QString matchedPlayerName = existingByLower.value(lower);
                bool isAccepted = false;
                for (const QString &acc : acceptedPlayers)
                {
                    if (acc.toLower() == lower || levenshtein(acc.toLower(), lower) <= 1)
                    {
                        isAccepted = true;
                        recognizedKeys.insert(matchedPlayerName);
                        break;
                    }
                }
                continue;
            }
            // Fuzzy-Matching gegen alle vorhandenen Namen
            bool fuzzyMatch = false;
            for (auto it = existingByLower.cbegin(); it != existingByLower.cend(); ++it)
            {
                if (levenshtein(lower, it.key()) <= fuzzyThreshold)
                {
                    fuzzyMatch = true;
                    // Prüfe ob dieser Spieler zugesagt hat
                    bool isAccepted = false;
                    for (const QString &acc : acceptedPlayers)
                    {
                        if (levenshtein(acc.toLower(), lower) <= fuzzyThreshold)
                        {
                            isAccepted = true;
                            break;
                        }
                    }
                    if (isAccepted)
                    {
                        recognizedKeys.insert(it.value());
                    }
                    break;
                }
            }
            if (fuzzyMatch)
                continue;

            // Prüfe ob neuer Spieler zugesagt hat
            bool isAccepted = false;
            for (const QString &acc : acceptedPlayers)
            {
                if (levenshtein(acc.toLower(), lower) <= 1)
                {
                    isAccepted = true;
                    break;
                }
            }

            // neuen Spieler anlegen
            Player np;
            np.name = cand;
            np.group = unassignedGroup;
            np.joinDate = nowDate();
            if (!MainWindow::rankOptions().isEmpty())
                np.rank = MainWindow::rankOptions().first();
            np.totalAttendance = qMax(np.totalAttendance, np.attendance);
            np.totalEvents = qMax(np.totalEvents, np.events);
            np.totalReserve = qMax(np.totalReserve, np.reserve);
            list.players.push_back(np);
            if (ensureGroupRegistered(unassignedGroup))
                saveGroups();
            addPlayerToModel(np);
            validateRow(model->rowCount() - 1);
            createdKeys.insert(np.name);
            // Nur bei Zusagen markieren
            if (isAccepted)
            {
                recognizedKeys.insert(np.name);
                acceptedCreatedKeys.insert(np.name);
            }
            existingByLower.insert(lower, np.name);
        }
        if (!createdKeys.isEmpty())
            savePlayers();
        }

        // Häkchen im Baum setzen (inkl. neu erstellte)
        int matched = 0;
        for (int i = 0; i < playerTree->topLevelItemCount(); ++i)
        {
        QTreeWidgetItem *groupItem = playerTree->topLevelItem(i);
        int groupChecked = 0;
        for (int j = 0; j < groupItem->childCount(); ++j)
        {
            QTreeWidgetItem *child = groupItem->child(j);
            QString key = child->data(0, Qt::UserRole).toString();
            if (recognizedKeys.contains(key))
            {
                child->setCheckState(0, Qt::Checked);
                ++groupChecked;
                ++matched;
            }
        }
        if (groupChecked == 0)
            groupItem->setCheckState(0, Qt::Unchecked);
        else if (groupChecked == groupItem->childCount())
            groupItem->setCheckState(0, Qt::Checked);
        else
            groupItem->setCheckState(0, Qt::PartiallyChecked);
        }

        // Neue, bisher nicht im Tree vorhandene Spieler unter "Nicht zugewiesen" anhängen und anhaken
        if (!createdKeys.isEmpty())
        {
        QTreeWidgetItem *unassignedItem = nullptr;
        for (int i = 0; i < playerTree->topLevelItemCount(); ++i)
        {
            QTreeWidgetItem *groupItem = playerTree->topLevelItem(i);
            if (groupItem->text(0).startsWith(unassignedGroup))
            {
                unassignedItem = groupItem;
                break;
            }
        }
        if (!unassignedItem)
        {
            unassignedItem = new QTreeWidgetItem(playerTree);
            unassignedItem->setText(0, QStringLiteral("%1").arg(unassignedGroup));
            unassignedItem->setData(0, Qt::UserRole, QString());
            unassignedItem->setFlags(unassignedItem->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            unassignedItem->setCheckState(0, Qt::PartiallyChecked);
            unassignedItem->setExpanded(true);
        }
        int added = 0;
        for (const QString &k : std::as_const(createdKeys))
        {
            QTreeWidgetItem *it = new QTreeWidgetItem(unassignedItem);
            it->setText(0, k);
            it->setData(0, Qt::UserRole, k);
            it->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
            // Nur Häkchen setzen wenn Spieler zugesagt hat
            bool shouldCheck = acceptedCreatedKeys.contains(k);
            it->setCheckState(0, shouldCheck ? Qt::Checked : Qt::Unchecked);
            ++added;
            if (shouldCheck)
                ++matched;
        }
        if (added > 0)
            unassignedItem->setText(0, QStringLiteral("%1 (%2)").arg(unassignedGroup).arg(unassignedItem->childCount()));
        }

        updateSelectedCount();

        // Zeige Ergebnis-Dialog mit erkannten Metadaten
        int totalParsed = acceptedPlayers.size() + rejectedPlayers.size();
        QString infoMsg = QStringLiteral("%1 Spieler erkannt (%2 zugesagt, %3 abgesagt).\n%4 Spieler markiert, %5 neu angelegt.")
            .arg(totalParsed > 0 ? totalParsed : matched)
            .arg(acceptedPlayers.size())
            .arg(rejectedPlayers.size())
            .arg(matched)
            .arg(createdKeys.size());
        
        if (isTraining) {
        infoMsg += QStringLiteral("\n\n📋 Typ: TRAINING");
        } else {
        infoMsg += QStringLiteral("\n\n🎮 Typ: EVENT");
        }
        if (!extractedEventName.isEmpty() || !extractedDate.isEmpty() || !extractedMap.isEmpty()) {
        infoMsg += QStringLiteral("\n\nErkannte Event-Daten:");
        if (!extractedEventName.isEmpty())
            infoMsg += QStringLiteral("\nEvent: %1").arg(extractedEventName);
        if (!extractedDate.isEmpty())
            infoMsg += QStringLiteral("\nDatum: %1").arg(extractedDate);
        if (!extractedMap.isEmpty())
            infoMsg += QStringLiteral("\nMap: %1").arg(extractedMap);
        infoMsg += QStringLiteral("\n\nMöchten Sie diese Daten für die Session übernehmen?");

        QMessageBox::StandardButton reply = QMessageBox::question(&dlg, QStringLiteral("OCR Import"), infoMsg,
                                                                  QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes)
        {
            // Übernehme erkannte Daten in Session-Felder (falls Multi-Assign-Dialog aus Session-Panel aufgerufen)
            // Hinweis: sessionNameEdit, sessionDateEdit sind in MainWindow, nicht in diesem Dialog
            // Wir müssen die Daten zurückgeben oder direkt setzen
            // Für jetzt: Zeige nur Info, Implementierung folgt
            QMessageBox::information(&dlg, QStringLiteral("Hinweis"),
                                     QStringLiteral("Event-Daten erkannt. Diese können manuell in die Session-Felder übernommen werden:\n\nName: %1\nDatum: %2\nMap (Notiz): %3")
                                         .arg(extractedEventName.isEmpty() ? "-" : extractedEventName)
                                         .arg(extractedDate.isEmpty() ? "-" : extractedDate)
                                         .arg(extractedMap.isEmpty() ? "-" : extractedMap));
        }
        } else {
        QMessageBox::information(&dlg, QStringLiteral("OCR Import"), infoMsg);
        } });

    if (table && table->selectionModel())
    {
        const QModelIndexList proxyRows = table->selectionModel()->selectedRows();
        for (const QModelIndex &proxyIdx : proxyRows)
        {
            QModelIndex srcIdx = proxy ? proxy->mapToSource(proxyIdx) : proxyIdx;
            QString key = playerKeyForRow(srcIdx.row());
            // select matching child items
            for (int i = 0; i < playerTree->topLevelItemCount(); ++i)
            {
                QTreeWidgetItem *groupItem = playerTree->topLevelItem(i);
                for (int j = 0; j < groupItem->childCount(); ++j)
                {
                    QTreeWidgetItem *child = groupItem->child(j);
                    if (child->data(0, Qt::UserRole).toString() == key)
                        child->setCheckState(0, Qt::Checked);
                }
            }
        }
    }
    // ensure at least one selected
    bool anySelected = false;
    for (int i = 0; i < playerTree->topLevelItemCount() && !anySelected; ++i)
    {
        QTreeWidgetItem *groupItem = playerTree->topLevelItem(i);
        for (int j = 0; j < groupItem->childCount(); ++j)
        {
            if (groupItem->child(j)->checkState(0) == Qt::Checked)
            {
                anySelected = true;
                break;
            }
        }
    }
    if (!anySelected && playerTree->topLevelItemCount() > 0 && playerTree->topLevelItem(0)->childCount() > 0)
        playerTree->topLevelItem(0)->child(0)->setCheckState(0, Qt::Checked);

    QFormLayout *sessionForm = new QFormLayout;
    outer->addLayout(sessionForm);

    QComboBox *existingCombo = new QComboBox(&dlg);
    existingCombo->addItem("Neuer Eintrag …", QString());
    const QList<Training> templates = recentTrainings(31);
    for (const Training &t : templates)
    {
        QString dateText = t.date.isValid() ? t.date.toString("yyyy-MM-dd") : QStringLiteral("?");
        QString label = QStringLiteral("%1 · %2 · %3").arg(dateText, t.type, t.title);
        existingCombo->addItem(label, t.id);
    }
    sessionForm->addRow("Vorlage", existingCombo);

    QComboBox *typeCombo = new QComboBox(&dlg);
    typeCombo->addItems({"Training", "Event", "Reserve"});
    sessionForm->addRow("Typ", typeCombo);

    QLineEdit *nameEdit = new QLineEdit(&dlg);
    nameEdit->setPlaceholderText("Name des Trainings/Event");
    sessionForm->addRow("Name", nameEdit);

    QComboBox *mapCombo = new QComboBox(&dlg);
    mapCombo->setEditable(true);
    if (maps.isEmpty())
        mapCombo->addItem("Unbekannt");
    else
        mapCombo->addItems(maps);
    sessionForm->addRow("Karte", mapCombo);

    QDateEdit *dateEdit = new QDateEdit(&dlg);
    dateEdit->setCalendarPopup(true);
    dateEdit->setDisplayFormat("yyyy-MM-dd");
    dateEdit->setDate(nowDate());
    sessionForm->addRow("Datum", dateEdit);

    QCheckBox *rememberCheck = new QCheckBox("Für spätere Auswahl speichern", &dlg);
    rememberCheck->setChecked(true);
    outer->addWidget(rememberCheck);

    auto populateFromExisting = [this, existingCombo, typeCombo, nameEdit, mapCombo, dateEdit, rememberCheck]()
    {
        QString id = existingCombo->currentData().toString();
        const Training *tpl = trainingById(id);
        if (!tpl)
        {
            typeCombo->setCurrentIndex(0);
            nameEdit->clear();
            if (mapCombo->count() > 0)
                mapCombo->setCurrentIndex(0);
            dateEdit->setDate(nowDate());
            rememberCheck->setChecked(true);
            return;
        }
        int typeIdx = typeCombo->findText(tpl->type, Qt::MatchFixedString);
        if (typeIdx >= 0)
            typeCombo->setCurrentIndex(typeIdx);
        else
            typeCombo->setCurrentText(tpl->type);
        nameEdit->setText(tpl->title);
        if (!tpl->maps.isEmpty())
            mapCombo->setCurrentText(tpl->maps.first());
        dateEdit->setDate(tpl->date.isValid() ? tpl->date : nowDate());
        rememberCheck->setChecked(true);
    };
    connect(existingCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), &dlg, [populateFromExisting](int)
            { populateFromExisting(); });
    populateFromExisting();

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    outer->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, [&]()
            {
        // collect selected players from tree
        QList<QString> selectedKeys;
        for (int i = 0; i < playerTree->topLevelItemCount(); ++i)
        {
            QTreeWidgetItem *groupItem = playerTree->topLevelItem(i);
            for (int j = 0; j < groupItem->childCount(); ++j)
            {
                QTreeWidgetItem *child = groupItem->child(j);
                if (child->checkState(0) == Qt::Checked)
                    selectedKeys.append(child->data(0, Qt::UserRole).toString());
            }
        }
        if (selectedKeys.isEmpty())
        {
            QMessageBox::warning(&dlg, "Auswahl fehlt", "Bitte mindestens einen Spieler auswählen.");
            return;
        }
        if (nameEdit->text().trimmed().isEmpty())
        {
            QMessageBox::warning(&dlg, "Name fehlt", "Bitte einen Trainings- oder Eventnamen eingeben.");
            return;
        }
        dlg.accept(); });
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted)
        return;

    QString type = typeCombo->currentText().trimmed();
    if (type.isEmpty())
        type = "Training";
    QString name = nameEdit->text().trimmed();
    QString map = mapCombo->currentText().trimmed();
    QDate date = dateEdit->date();
    QDateTime when(date, QTime::currentTime());

    auto ensureMapKnown = [this](const QString &entered)
    {
        if (entered.trimmed().isEmpty())
            return;
        for (const QString &existing : maps)
        {
            if (existing.compare(entered, Qt::CaseInsensitive) == 0)
                return;
        }
        maps.append(entered);
        saveMaps();
        refreshSessionMapCombo();
    };
    ensureMapKnown(map);

    QString existingId = existingCombo->currentData().toString();
    Training entry;
    entry.id = existingId.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) : existingId;
    entry.title = name;
    entry.type = type;
    entry.date = date;
    if (!map.isEmpty())
        entry.maps = QStringList{map};

    if (rememberCheck->isChecked())
    {
        bool updated = false;
        for (Training &t : trainings)
        {
            if (t.id == entry.id)
            {
                t = entry;
                updated = true;
                break;
            }
        }
        if (!updated)
            trainings.append(entry);
        saveTrainings();
        purgeOldTrainings();
        refreshSessionTemplates();
    }

    QStringList affected;
    // iterate selected keys from tree
    QList<QString> selectedKeys;
    for (int i = 0; i < playerTree->topLevelItemCount(); ++i)
    {
        QTreeWidgetItem *groupItem = playerTree->topLevelItem(i);
        for (int j = 0; j < groupItem->childCount(); ++j)
        {
            QTreeWidgetItem *child = groupItem->child(j);
            if (child->checkState(0) == Qt::Checked)
                selectedKeys.append(child->data(0, Qt::UserRole).toString());
        }
    }

    // Sammle alle Spieler-Keys für noResponseCounter-Erhöhung
    QSet<QString> selectedKeySet(selectedKeys.begin(), selectedKeys.end());
    QStringList allPlayerKeys;
    for (const Player &p : list.players)
        allPlayerKeys.append(p.name);

    // Erhöhe noResponseCounter für alle NICHT ausgewählten Spieler (optional)
    for (const QString &key : allPlayerKeys)
    {
        if (!selectedKeySet.contains(key))
        {
            Player *player = findPlayerByKey(key);
            if (player)
            {
                if (incrementCounterOnNoResponse)
                    player->noResponseCounter++;
                int row = rowForPlayerKey(key);
                if (row >= 0)
                {
                    // Update Einsätze-Anzeige mit neuem Counter
                    if (QStandardItem *trainingItem = model->item(row, 5))
                    {
                        trainingItem->setText(formatTrainingDisplay(*player));
                        trainingItem->setToolTip(trainingTooltip(*player));
                    }
                    // Setze rotes X wenn Counter >= threshold
                    if (QStandardItem *statusItem = model->item(row, 1))
                    {
                        if (player->noResponseCounter >= noResponseThreshold)
                        {
                            statusItem->setText("❌");
                            statusItem->setForeground(QBrush(Qt::red));
                        }
                    }
                }
            }
        }
    }

    for (const QString &key : selectedKeys)
    {
        if (key.isEmpty())
            continue;
        affected << key;
        appendAttendanceLog(key, type.toLower(), when, name, map);
        incrementPlayerCounters(key, type);
        int row = rowForPlayerKey(key);
        if (row >= 0)
            validateRow(row);
    }

    if (affected.isEmpty())
        return;

    savePlayers(); // Speichere geänderte noResponseCounter
    QMessageBox::information(this, "Zugewiesen", QStringLiteral("%1 Spieler erhielten %2 '%3'.").arg(affected.size()).arg(type, name));
}
void MainWindow::loadCommentOptions() {}
void MainWindow::saveCommentOptions() {}
void MainWindow::loadTrainings()
{
    trainings.clear();
    QFile f(dataFilePath("clan_sessions.json"));
    if (!f.exists())
        return;
    if (!f.open(QIODevice::ReadOnly))
        return;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isArray())
        return;
    for (const QJsonValue &val : doc.array())
    {
        if (val.isObject())
            trainings.append(Training::fromJson(val.toObject()));
    }
    purgeOldTrainings();
    refreshSessionTemplates();
}
void MainWindow::saveTrainings()
{
    QJsonArray arr;
    for (const Training &t : trainings)
        arr.append(t.toJson());
    QFile f(dataFilePath("clan_sessions.json"));
    if (!f.open(QIODevice::WriteOnly))
        return;
    QJsonDocument doc(arr);
    f.write(doc.toJson(QJsonDocument::Indented));
}

void MainWindow::purgeOldTrainings(int days)
{
    if (days <= 0)
        days = 31;
    QDate cutoff = nowDate().addDays(-days);
    bool changed = false;
    auto it = trainings.begin();
    while (it != trainings.end())
    {
        if (it->date.isValid() && it->date < cutoff)
        {
            it = trainings.erase(it);
            changed = true;
        }
        else
        {
            ++it;
        }
    }
    if (changed)
        saveTrainings();
}
void MainWindow::loadMaps()
{
    maps.clear();
    QFile f(dataFilePath("clan_maps.json"));
    if (f.exists() && f.open(QIODevice::ReadOnly))
    {
        QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
        if (doc.isArray())
        {
            for (const QJsonValue &val : doc.array())
            {
                if (val.isString())
                    maps.append(val.toString());
            }
        }
    }
    if (maps.isEmpty())
        resetMapsToDefaults();
}
void MainWindow::saveMaps()
{
    QJsonArray arr;
    for (const QString &map : maps)
        arr.append(map);
    QFile f(dataFilePath("clan_maps.json"));
    if (!f.open(QIODevice::WriteOnly))
        return;
    QJsonDocument doc(arr);
    f.write(doc.toJson(QJsonDocument::Indented));
}
void MainWindow::resetMapsToDefaults()
{
    maps = {
        "Omaha Beach",
        "Utah Beach",
        "Sainte-Mère-Église",
        "Carentan",
        "Foy",
        "Hill 400",
        "Hurtgen Forest",
        "Kursk",
        "Stalingrad",
        "Kharkov",
        "Smolensk",
        "El Alamein",
        "Remagen",
        "Purple Heart Lane",
        "St. Marie du Mont"};
}
QList<struct Training> MainWindow::recentTrainings(int days) const
{
    QList<Training> recent;
    if (days <= 0)
        days = 21;
    const_cast<MainWindow *>(this)->purgeOldTrainings(qMax(days, 31));
    QDate cutoff = nowDate().addDays(-days);
    for (const Training &t : trainings)
    {
        if (!t.date.isValid() || t.date >= cutoff)
            recent.append(t);
    }
    std::sort(recent.begin(), recent.end(), [](const Training &a, const Training &b)
              { return a.date > b.date; });
    return recent;
}

const Training *MainWindow::trainingById(const QString &id) const
{
    if (id.isEmpty())
        return nullptr;
    for (const Training &t : trainings)
    {
        if (t.id == id)
            return &t;
    }
    return nullptr;
}
void MainWindow::showSelectTrainingDialogForPlayer(const QString &playerKey) { Q_UNUSED(playerKey); }
void MainWindow::showSelectEventDialogForPlayer(const QString &playerKey) { Q_UNUSED(playerKey); }
void MainWindow::showCreateSessionDialog(const QStringList &preselect, bool forceTraining, bool forceEvent)
{
    QDialog dlg(this);
    dlg.setWindowTitle("Training/Event anlegen");
    QFormLayout *form = new QFormLayout(&dlg);

    QComboBox *typeCombo = new QComboBox(&dlg);
    typeCombo->addItems({"Training", "Event", "Reserve"});
    if (forceTraining)
        typeCombo->setCurrentText("Training");
    else if (forceEvent)
        typeCombo->setCurrentText("Event");
    if (forceTraining != forceEvent && (forceTraining || forceEvent))
        typeCombo->setEnabled(false);
    form->addRow("Typ", typeCombo);

    QLineEdit *nameEdit = new QLineEdit(&dlg);
    nameEdit->setPlaceholderText("Name des Trainings/Event");
    if (!preselect.isEmpty())
        nameEdit->setText(preselect.join(", "));
    form->addRow("Name", nameEdit);

    QComboBox *mapCombo = new QComboBox(&dlg);
    mapCombo->setEditable(true);
    if (maps.isEmpty())
        mapCombo->addItem("Unbekannt");
    else
        mapCombo->addItems(maps);
    form->addRow("Karte", mapCombo);

    QDateEdit *dateEdit = new QDateEdit(&dlg);
    dateEdit->setCalendarPopup(true);
    dateEdit->setDisplayFormat("yyyy-MM-dd");
    dateEdit->setDate(nowDate());
    form->addRow("Datum", dateEdit);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    form->addRow(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dlg, [&]()
            {
        if (nameEdit->text().trimmed().isEmpty())
        {
            QMessageBox::warning(&dlg, "Eingabe fehlt", "Bitte einen Namen angeben.");
            return;
        }
        dlg.accept(); });
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted)
        return;

    Training entry;
    entry.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    entry.date = dateEdit->date();
    entry.title = nameEdit->text().trimmed();
    entry.type = typeCombo->currentText();
    const QString mapText = mapCombo->currentText().trimmed();
    if (!mapText.isEmpty())
        entry.maps = QStringList{mapText};

    trainings.append(entry);
    saveTrainings();
    refreshSessionTemplates();

    auto mapExists = [this](const QString &candidate)
    {
        for (const QString &existing : maps)
        {
            if (existing.compare(candidate, Qt::CaseInsensitive) == 0)
                return true;
        }
        return false;
    };
    if (!mapText.isEmpty() && !mapExists(mapText))
    {
        maps.append(mapText);
        saveMaps();
        refreshSessionMapCombo();
    }

    QMessageBox::information(this, "Gespeichert", QStringLiteral("%1 am %2 wurde angelegt.").arg(entry.type, entry.date.toString("yyyy-MM-dd")));
}
void MainWindow::refreshSessionTemplates()
{
    if (!sessionTemplateCombo)
        return;
    QString previousId = sessionTemplateCombo->currentData().toString();
    sessionTemplateCombo->blockSignals(true);
    sessionTemplateCombo->clear();
    sessionTemplateCombo->addItem("Neue Session …", QString());
    const QList<Training> templates = recentTrainings(60);
    for (const Training &t : templates)
    {
        QString dateText = t.date.isValid() ? t.date.toString("yyyy-MM-dd") : QStringLiteral("?");
        QString label = QStringLiteral("%1 · %2 · %3").arg(dateText, t.type, t.title);
        sessionTemplateCombo->addItem(label, t.id);
    }
    int idx = previousId.isEmpty() ? 0 : sessionTemplateCombo->findData(previousId);
    sessionTemplateCombo->setCurrentIndex(idx >= 0 ? idx : 0);
    sessionTemplateCombo->blockSignals(false);
    applySessionTemplateSelection();
}

void MainWindow::refreshSessionMapCombo()
{
    if (!sessionMapCombo)
        return;
    const QString currentText = sessionMapCombo->currentText();
    sessionMapCombo->blockSignals(true);
    sessionMapCombo->clear();
    if (maps.isEmpty())
    {
        sessionMapCombo->addItem("Unbekannt");
    }
    else
    {
        for (const QString &map : maps)
            sessionMapCombo->addItem(map);
    }
    int idx = sessionMapCombo->findText(currentText, Qt::MatchFixedString);
    if (idx >= 0)
        sessionMapCombo->setCurrentIndex(idx);
    sessionMapCombo->blockSignals(false);
}

void MainWindow::applySessionTemplateSelection()
{
    if (!sessionTemplateCombo)
        return;
    QString id = sessionTemplateCombo->currentData().toString();
    const Training *tpl = trainingById(id);
    if (!tpl)
    {
        if (sessionTypeCombo)
            sessionTypeCombo->setCurrentIndex(0);
        if (sessionNameEdit)
            sessionNameEdit->clear();
        if (sessionMapCombo && sessionMapCombo->count() > 0)
            sessionMapCombo->setCurrentIndex(0);
        if (sessionDateEdit)
            sessionDateEdit->setDate(nowDate());
        if (sessionRememberCheck)
            sessionRememberCheck->setChecked(true);
        updateSessionSummary();
        return;
    }

    if (sessionTypeCombo)
    {
        int typeIdx = sessionTypeCombo->findText(tpl->type, Qt::MatchFixedString);
        if (typeIdx >= 0)
            sessionTypeCombo->setCurrentIndex(typeIdx);
        else
            sessionTypeCombo->setCurrentText(tpl->type);
    }
    if (sessionNameEdit)
        sessionNameEdit->setText(tpl->title);
    if (sessionMapCombo && !tpl->maps.isEmpty())
        sessionMapCombo->setCurrentText(tpl->maps.first());
    if (sessionDateEdit)
        sessionDateEdit->setDate(tpl->date.isValid() ? tpl->date : nowDate());
    if (sessionRememberCheck)
        sessionRememberCheck->setChecked(true);
    // Übernehme gespeicherte Teilnehmerlisten aus der Vorlage
    sessionPlayerStatus.clear();
    for (const QString &n : tpl->confirmedPlayers)
        sessionPlayerStatus.insert(n, ResponseStatus::Confirmed);
    for (const QString &n : tpl->declinedPlayers)
        sessionPlayerStatus.insert(n, ResponseStatus::Declined);
    for (const QString &n : tpl->noResponsePlayers)
        sessionPlayerStatus.insert(n, ResponseStatus::NoResponse);
    refreshSessionPlayerLists();
    updateSessionSummary();
}

void MainWindow::refreshSessionPlayerTable()
{
    if (!sessionPlayerTree)
        return;

    std::vector<Player> sorted = list.players;
    std::sort(sorted.begin(), sorted.end(), [](const Player &a, const Player &b)
              { return a.name.toLower() < b.name.toLower(); });

    const QString noGroupLabel = QStringLiteral("Ohne Gruppe");
    QMap<QString, QList<Player>> groupedPlayers;
    QSet<QString> validKeys;
    for (const Player &player : sorted)
    {
        QString grp = player.group.trimmed();
        if (grp.isEmpty())
            grp = noGroupLabel;
        groupedPlayers[grp].append(player);
        validKeys.insert(player.name);
    }
    sessionSelectedPlayers &= validKeys;

    QStringList renderOrder;
    QSet<QString> seen;
    for (const QString &grp : groups)
    {
        if (groupedPlayers.contains(grp))
        {
            renderOrder.append(grp);
            seen.insert(grp);
        }
    }
    QStringList extras;
    for (auto it = groupedPlayers.cbegin(); it != groupedPlayers.cend(); ++it)
    {
        if (it.key() == noGroupLabel)
            continue;
        if (!seen.contains(it.key()))
            extras.append(it.key());
    }
    std::sort(extras.begin(), extras.end(), [](const QString &a, const QString &b)
              { return QString::localeAwareCompare(a, b) < 0; });
    renderOrder.append(extras);
    if (groupedPlayers.contains(noGroupLabel))
        renderOrder.append(noGroupLabel);

    sessionPlayerTree->blockSignals(true);
    sessionPlayerTree->clear();

    for (const QString &grp : renderOrder)
    {
        const QList<Player> players = groupedPlayers.value(grp);
        if (players.isEmpty())
            continue;

        QTreeWidgetItem *groupItem = new QTreeWidgetItem(sessionPlayerTree);
        groupItem->setFlags(Qt::ItemIsEnabled);
        groupItem->setData(0, Qt::UserRole, QString());
        QString displayName = grp;
        if (grp == noGroupLabel)
            displayName = noGroupLabel;
        groupItem->setText(1, QStringLiteral("%1 (%2)").arg(displayName).arg(players.size()));

        for (const Player &player : players)
        {
            QTreeWidgetItem *playerItem = new QTreeWidgetItem(groupItem);
            playerItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);
            playerItem->setText(1, player.name);
            playerItem->setData(0, Qt::UserRole, player.name);
            playerItem->setCheckState(0, sessionSelectedPlayers.contains(player.name) ? Qt::Checked : Qt::Unchecked);
        }
        groupItem->setExpanded(false);
    }

    sessionPlayerTree->blockSignals(false);
    updateSessionSummary();
}

void MainWindow::applySessionSelectionFromTable()
{
    if (!table || !sessionPlayerTree)
        return;
    if (!table->selectionModel())
    {
        QMessageBox::information(this, "Auswahl", "Bitte zuerst Spieler in der Haupttabelle markieren.");
        return;
    }

    const QModelIndexList proxyRows = table->selectionModel()->selectedRows();
    if (proxyRows.isEmpty())
    {
        QMessageBox::information(this, "Auswahl", "Keine Spieler ausgewählt.");
        return;
    }

    sessionSelectedPlayers.clear();
    for (const QModelIndex &proxyIndex : proxyRows)
    {
        QModelIndex srcIndex = proxy ? proxy->mapToSource(proxyIndex) : proxyIndex;
        QString key = playerKeyForRow(srcIndex.row());
        if (!key.isEmpty())
            sessionSelectedPlayers.insert(key);
    }

    refreshSessionPlayerTable();
}

void MainWindow::commitSessionAssignment()
{
    if (!sessionTypeCombo || !sessionNameEdit || !sessionDateEdit)
        return;
    if (sessionSelectedPlayers.isEmpty())
    {
        QMessageBox::information(this, "Session", "Bitte mindestens einen Spieler markieren.");
        return;
    }

    QString type = sessionTypeCombo->currentText().trimmed();
    if (type.isEmpty())
        type = "Training";
    QString name = sessionNameEdit->text().trimmed();
    if (name.isEmpty())
    {
        QMessageBox::warning(this, "Session", "Bitte einen Namen für Training/Event angeben.");
        return;
    }
    QString map = sessionMapCombo ? sessionMapCombo->currentText().trimmed() : QString();
    QDate date = sessionDateEdit->date().isValid() ? sessionDateEdit->date() : nowDate();
    QDateTime when(date, QTime::currentTime());

    auto ensureMapKnown = [this](const QString &entered)
    {
        if (entered.trimmed().isEmpty())
            return;
        for (const QString &existing : maps)
        {
            if (existing.compare(entered, Qt::CaseInsensitive) == 0)
                return;
        }
        maps.append(entered);
        saveMaps();
        refreshSessionMapCombo();
    };
    ensureMapKnown(map);

    bool remember = sessionRememberCheck && sessionRememberCheck->isChecked();
    QString templateId = sessionTemplateCombo ? sessionTemplateCombo->currentData().toString() : QString();
    if (templateId.isEmpty())
        templateId = QUuid::createUuid().toString(QUuid::WithoutBraces);

    if (remember)
    {
        Training entry;
        entry.id = templateId;
        entry.type = type;
        entry.title = name;
        entry.date = date;
        if (!map.isEmpty())
            entry.maps = QStringList{map};
        // Teilnehmer aus den drei Listen übernehmen
        entry.confirmedPlayers.clear();
        entry.declinedPlayers.clear();
        entry.noResponsePlayers.clear();
        for (auto it = sessionPlayerStatus.constBegin(); it != sessionPlayerStatus.constEnd(); ++it)
        {
            const QString playerName = it.key();
            switch (it.value())
            {
            case ResponseStatus::Confirmed:
                entry.confirmedPlayers << playerName;
                break;
            case ResponseStatus::Declined:
                entry.declinedPlayers << playerName;
                break;
            case ResponseStatus::NoResponse:
                entry.noResponsePlayers << playerName;
                break;
            }
        }

        bool updated = false;
        for (Training &t : trainings)
        {
            if (t.id == entry.id)
            {
                t = entry;
                updated = true;
                break;
            }
        }
        if (!updated)
            trainings.append(entry);
        saveTrainings();
        purgeOldTrainings();
        refreshSessionTemplates();
    }

    const QString normalizedType = type.toLower();
    QStringList affected;
    QStringList duplicates;

    // Iteriere über sessionPlayerStatus statt sessionSelectedPlayers
    for (auto it = sessionPlayerStatus.constBegin(); it != sessionPlayerStatus.constEnd(); ++it)
    {
        const QString &key = it.key();
        ResponseStatus status = it.value();

        if (playerHasSessionRecord(key, normalizedType, name, date))
        {
            duplicates << key;
            continue;
        }

        // Nur für Confirmed und Declined: Attendance logging
        if (status == ResponseStatus::Confirmed || status == ResponseStatus::Declined)
        {
            appendAttendanceLog(key, normalizedType, when, name, map);
        }

        // Counter-Logik basierend auf Status
        if (status == ResponseStatus::Confirmed || status == ResponseStatus::Declined)
        {
            // Zugesagt oder Abgesagt: Counter auf 0 setzen
            incrementPlayerCounters(key, type);

            // noResponseCounter zurücksetzen
            for (Player &p : list.players)
            {
                if (p.name.compare(key, Qt::CaseInsensitive) == 0)
                {
                    p.noResponseCounter = 0;
                    break;
                }
            }
        }
        else if (status == ResponseStatus::NoResponse)
        {
            // Keine Antwort: noResponseCounter erhöhen
            for (Player &p : list.players)
            {
                if (p.name.compare(key, Qt::CaseInsensitive) == 0)
                {
                    p.noResponseCounter++;
                    break;
                }
            }
        }

        int row = rowForPlayerKey(key);
        if (row >= 0)
            validateRow(row);
        affected << key;
    }

    if (!duplicates.isEmpty())
    {
        QMessageBox::information(this, "Session",
                                 QStringLiteral("Folgende Spieler hatten bereits '%1' am %2 eingetragen:\n%3")
                                     .arg(name, date.toString("yyyy-MM-dd"), duplicates.join(", ")));
    }

    // Spieler speichern (für noResponseCounter Updates)
    savePlayers();

    sessionPlayerStatus.clear();
    refreshSessionPlayerLists();

    if (affected.isEmpty())
        return;

    QMessageBox::information(this, "Session", QStringLiteral("%1 Spieler erhielten %2 '%3'.").arg(affected.size()).arg(type, name));
}

bool MainWindow::playerHasSessionRecord(const QString &playerKey, const QString &type, const QString &name, const QDate &date) const
{
    if (playerKey.isEmpty() || !date.isValid())
        return false;

    auto it = attendanceRecords.constFind(playerKey);
    if (it == attendanceRecords.constEnd())
        return false;

    const QList<QJsonObject> &records = it.value();
    for (const QJsonObject &entry : records)
    {
        const QString existingType = entry.value("type").toString();
        if (existingType.compare(type, Qt::CaseInsensitive) != 0)
            continue;

        const QString existingName = entry.value("name").toString();
        if (!name.isEmpty() && existingName.compare(name, Qt::CaseInsensitive) != 0)
            continue;

        const QDate existingDate = QDate::fromString(entry.value("date").toString(), Qt::ISODate);
        if (!existingDate.isValid())
            continue;

        if (existingDate == date)
            return true;
    }
    return false;
}

void MainWindow::updateSessionSummary()
{
    if (!sessionSummaryLabel)
        return;

    // Zähle Spieler nach Status
    int confirmedCount = 0;
    int declinedCount = 0;
    int noResponseCount = 0;

    for (auto it = sessionPlayerStatus.constBegin(); it != sessionPlayerStatus.constEnd(); ++it)
    {
        switch (it.value())
        {
        case ResponseStatus::Confirmed:
            confirmedCount++;
            break;
        case ResponseStatus::Declined:
            declinedCount++;
            break;
        case ResponseStatus::NoResponse:
            noResponseCount++;
            break;
        }
    }

    int totalCount = confirmedCount + declinedCount + noResponseCount;
    QStringList parts;
    parts << QStringLiteral("%1 zugesagt, %2 abgesagt, %3 keine Antwort").arg(confirmedCount).arg(declinedCount).arg(noResponseCount);

    if (sessionNameEdit)
    {
        QString title = sessionNameEdit->text().trimmed();
        if (!title.isEmpty())
            parts << title;
    }
    if (sessionDateEdit && sessionDateEdit->date().isValid())
        parts << sessionDateEdit->date().toString("yyyy-MM-dd");
    sessionSummaryLabel->setText(parts.join(QStringLiteral(" · ")));
}
void MainWindow::updateTrainingsSummary() {}
void MainWindow::showCreateLineupDialog()
{
    LineupDialog dlg(this);
    // build player list and optional group mapping
    QStringList players;
    QMap<QString, QString> playerToGroup;
    for (const Player &p : list.players)
    {
        players << p.name;
        playerToGroup.insert(p.name, p.group);
    }
    dlg.setPlayerList(players, playerToGroup);
    if (dlg.exec() != QDialog::Accepted)
        return;

    QVector<LineupTrupp> trupps = dlg.getLineup();
    // commander: try to find the commander line edit inside the dialog
    QLineEdit *commEd = dlg.findChild<QLineEdit *>();
    QString commander;
    if (commEd)
        commander = commEd->text();

    QString fileName = QFileDialog::getSaveFileName(this, "Export Aufstellung (XLSX)", QString(), "Excel Workbook (*.xlsx)");
    if (fileName.isEmpty())
        return;

    bool ok = LineupExporter::writeXlsx(fileName, commander, trupps);
    if (ok)
        QMessageBox::information(this, "Export", "Aufstellung erfolgreich exportiert.");
    else
        QMessageBox::warning(this, "Export", "Fehler beim Export der Aufstellung.");
}
void MainWindow::loadPlayers()
{
    list.players.clear();
    QFile f(dataFilePath("clan_players.json"));
    if (f.exists())
    {
        if (!f.open(QIODevice::ReadOnly))
        {
            appendErrorLog("loadPlayers", QStringLiteral("Datei konnte nicht geöffnet werden: %1").arg(f.errorString()));
        }
        else
        {
            QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
            if (doc.isArray())
            {
                for (const QJsonValue &val : doc.array())
                {
                    if (!val.isObject())
                        continue;
                    QJsonObject obj = val.toObject();
                    Player p;
                    p.name = obj.value("name").toString();
                    p.t17name = obj.value("t17").toString();
                    p.level = obj.value("level").toInt();
                    p.group = obj.value("group").toString();
                    p.attendance = obj.value("attendance").toInt();
                    p.totalAttendance = obj.value("totalAttendance").toInt(p.attendance);
                    p.events = obj.value("events").toInt();
                    p.totalEvents = obj.value("totalEvents").toInt(p.events);
                    p.reserve = obj.value("reserve").toInt();
                    p.totalReserve = obj.value("totalReserve").toInt(p.reserve);
                    p.comment = obj.value("comment").toString();
                    p.joinDate = QDate::fromString(obj.value("joinDate").toString(), Qt::ISODate);
                    p.rank = obj.value("rank").toString();
                    p.lastPromotionDate = QDate::fromString(obj.value("lastPromotion").toString(), Qt::ISODate);
                    p.nextRank = obj.value("nextRank").toString();
                    list.players.push_back(p);
                }
            }
            else
            {
                appendErrorLog("loadPlayers", QStringLiteral("Unerwartetes JSON-Format"));
            }
        }
    }

    bool addedGroupsFromPlayers = false;
    QSet<QString> knownGroups;
    for (const QString &existing : groups)
        knownGroups.insert(existing.toLower());
    for (const Player &p : list.players)
    {
        if (p.group.isEmpty())
            continue;
        const QString lower = p.group.toLower();
        if (knownGroups.contains(lower))
            continue;
        groups.append(p.group);
        knownGroups.insert(lower);
        addedGroupsFromPlayers = true;
    }
    if (addedGroupsFromPlayers)
    {
        std::sort(groups.begin(), groups.end(), [](const QString &a, const QString &b)
                  { return a.compare(b, Qt::CaseInsensitive) < 0; });
        groups.erase(std::unique(groups.begin(), groups.end(), [](const QString &a, const QString &b)
                                 { return a.compare(b, Qt::CaseInsensitive) == 0; }),
                     groups.end());
        saveGroups();
    }
    refreshGroupFilterCombo();

    refreshModelFromList();
    updateGroupDecorations();
    validateAllRows();
}
void MainWindow::savePlayers()
{
    QJsonArray arr;
    for (const Player &p : list.players)
    {
        QJsonObject obj;
        obj.insert("name", p.name);
        obj.insert("t17", p.t17name);
        obj.insert("level", p.level);
        obj.insert("group", p.group);
        obj.insert("attendance", p.attendance);
        obj.insert("totalAttendance", p.totalAttendance);
        obj.insert("events", p.events);
        obj.insert("totalEvents", p.totalEvents);
        obj.insert("reserve", p.reserve);
        obj.insert("totalReserve", p.totalReserve);
        obj.insert("comment", p.comment);
        obj.insert("joinDate", p.joinDate.toString(Qt::ISODate));
        obj.insert("rank", p.rank);
        obj.insert("lastPromotion", p.lastPromotionDate.toString(Qt::ISODate));
        obj.insert("nextRank", p.nextRank);
        arr.append(obj);
    }
    QFile f(dataFilePath("clan_players.json"));
    if (!f.open(QIODevice::WriteOnly))
    {
        appendErrorLog("savePlayers", QStringLiteral("Datei konnte nicht geschrieben werden: %1").arg(f.errorString()));
        return;
    }
    QJsonDocument doc(arr);
    f.write(doc.toJson(QJsonDocument::Indented));
}

void MainWindow::loadSettings()
{
    QFile f(dataFilePath("clan_settings.json"));
    if (!f.exists())
        return;
    if (!f.open(QIODevice::ReadOnly))
    {
        appendErrorLog("loadSettings", QStringLiteral("Datei konnte nicht geöffnet werden: %1").arg(f.errorString()));
        return;
    }
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject())
    {
        appendErrorLog("loadSettings", QStringLiteral("Unerwartetes JSON-Format"));
        return;
    }
    QJsonObject obj = doc.object();

    noResponseThreshold = obj.value("noResponseThreshold").toInt(10);
    fuzzyMatchThreshold = obj.value("fuzzyMatchThreshold").toInt(2);
    autoCreatePlayers = obj.value("autoCreatePlayers").toBool(true);
    fuzzyMatchingEnabled = obj.value("fuzzyMatchingEnabled").toBool(true);
    autoFillMetadata = obj.value("autoFillMetadata").toBool(true);
    ocrLanguage = obj.value("ocrLanguage").toString("deu");
    unassignedGroupName = obj.value("unassignedGroupName").toString("Nicht zugewiesen");
    incrementCounterOnNoResponse = obj.value("incrementCounterOnNoResponse").toBool(true);
    resetCounterOnResponse = obj.value("resetCounterOnResponse").toBool(true);
    showCounterInTable = obj.value("showCounterInTable").toBool(true);
    hintColumnName = obj.value("hintColumnName").toString("Hinweis");
}

void MainWindow::saveSettings()
{
    QJsonObject obj;
    obj.insert("noResponseThreshold", noResponseThreshold);
    obj.insert("fuzzyMatchThreshold", fuzzyMatchThreshold);
    obj.insert("autoCreatePlayers", autoCreatePlayers);
    obj.insert("fuzzyMatchingEnabled", fuzzyMatchingEnabled);
    obj.insert("autoFillMetadata", autoFillMetadata);
    obj.insert("ocrLanguage", ocrLanguage);
    obj.insert("unassignedGroupName", unassignedGroupName);
    obj.insert("incrementCounterOnNoResponse", incrementCounterOnNoResponse);
    obj.insert("resetCounterOnResponse", resetCounterOnResponse);
    obj.insert("showCounterInTable", showCounterInTable);
    obj.insert("hintColumnName", hintColumnName);

    QFile f(dataFilePath("clan_settings.json"));
    if (!f.open(QIODevice::WriteOnly))
    {
        appendErrorLog("saveSettings", QStringLiteral("Datei konnte nicht geschrieben werden: %1").arg(f.errorString()));
        return;
    }
    QJsonDocument doc(obj);
    f.write(doc.toJson(QJsonDocument::Indented));
}

void MainWindow::applySettingsToUI()
{
    if (model)
    {
        model->setHeaderData(1, Qt::Horizontal, hintColumnName);
        // Aktualisiere Einsätze-Text je nach Anzeige der Klammer
        for (int row = 0; row < model->rowCount(); ++row)
        {
            QString key = playerKeyForRow(row);
            if (key.isEmpty())
                continue;
            const Player *p = findPlayerByKey(key);
            if (!p)
                continue;
            if (QStandardItem *trainingItem = model->item(row, 5))
            {
                trainingItem->setText(formatTrainingDisplay(*p));
                trainingItem->setToolTip(trainingTooltip(*p));
            }
            validateRow(row);
        }
    }
}

void MainWindow::loadGroupColors()
{
    QFile f(dataFilePath("clan_group_colors.json"));
    if (!f.exists())
        return;
    if (!f.open(QIODevice::ReadOnly))
        return;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject())
        return;
    QJsonObject obj = doc.object();
    for (auto it = obj.begin(); it != obj.end(); ++it)
    {
        const QString color = it.value().toString();
        if (!color.isEmpty())
            groupColors.insert(it.key(), color);
    }
}
void MainWindow::saveGroupColors()
{
    QJsonObject obj;
    for (auto it = groupColors.constBegin(); it != groupColors.constEnd(); ++it)
    {
        if (it.value().isEmpty())
            continue;
        obj.insert(it.key(), it.value());
    }
    QFile f(dataFilePath("clan_group_colors.json"));
    if (!f.open(QIODevice::WriteOnly))
        return;
    f.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
}
void MainWindow::updateGroupDecorations()
{
    // Null check added to prevent crash
    if (!model || !table) {
        qWarning() << "updateGroupDecorations: model or table is null, skipping";
        return;
    }
    
    if (!model)
        return;
    for (int row = 0; row < model->rowCount(); ++row)
    {
        QStandardItem *groupItem = model->item(row, 4);
        if (!groupItem)
            continue;
        const QString groupName = groupItem->text();
        QColor base = colorForGroup(groupName);
        if (!base.isValid())
        {
            groupItem->setBackground(Qt::NoBrush);
            groupItem->setForeground(QBrush());
            continue;
        }
        QColor bg = base;
        // Mild normalization so very dark/very light colors are shifted toward middle
        if (bg.lightness() < 85)
            bg = bg.lighter(140);
        else if (bg.lightness() > 235)
            bg = bg.darker(120);

        auto srgbToLinear = [](double c)
        {
            return (c <= 0.04045) ? c / 12.92 : pow((c + 0.055) / 1.055, 2.4);
        };
        double r = srgbToLinear(bg.red() / 255.0);
        double g = srgbToLinear(bg.green() / 255.0);
        double b = srgbToLinear(bg.blue() / 255.0);
        // Relative luminance (WCAG)
        double L = 0.2126 * r + 0.7152 * g + 0.0722 * b;
        // Contrast ratios with white and black
        double contrastWhite = (1.0 + 0.05) / (L + 0.05);
        double contrastBlack = (L + 0.05) / (0.0 + 0.05);
        QColor fg = (contrastWhite >= contrastBlack) ? Qt::white : Qt::black;
        groupItem->setBackground(bg);
        groupItem->setForeground(QBrush(fg));
    }
    if (table)
        table->viewport()->update();
}

QColor MainWindow::colorForGroup(const QString &groupName) const
{
    if (groupName.isEmpty())
        return QColor();

    const QString directHex = groupColors.value(groupName);
    if (!directHex.isEmpty())
    {
        QColor direct(directHex);
        if (direct.isValid())
            return direct;
    }

    const QString category = groupCategory.value(groupName);
    if (!category.isEmpty())
    {
        static const QMap<QString, QString> categoryPalette = {
            {QStringLiteral("Angriff"), QStringLiteral("#c0392b")},
            {QStringLiteral("Verteidigung"), QStringLiteral("#2980b9")},
            {QStringLiteral("Panzerwaffe"), QStringLiteral("#16a085")},
            {QStringLiteral("Artillerie"), QStringLiteral("#8e44ad")},
            {QStringLiteral("Sonstiges"), QStringLiteral("#7f8c8d")}};
        const QString catHex = categoryPalette.value(category);
        if (!catHex.isEmpty())
        {
            QColor catColor(catHex);
            if (catColor.isValid())
                return catColor;
        }
    }

    quint32 hash = qHash(groupName.toLower());
    int hue = static_cast<int>(hash % 360);
    QColor fallback = QColor::fromHsl(hue, 130, 160);
    return fallback;
}

void MainWindow::applySortSettings()
{
    if (!proxy || !table || !sortFieldCombo || !sortOrderCombo)
        return;
    int column = sortFieldCombo->currentData().toInt();
    if (column < 0)
        column = 0;
    Qt::SortOrder order = static_cast<Qt::SortOrder>(sortOrderCombo->currentData().toInt());
    proxy->sort(column, order);
    if (QHeaderView *hdr = table->horizontalHeader())
        hdr->setSortIndicator(column, order);
}
void MainWindow::loadAttendance()
{
    attendanceRecords.clear();
    QFile f(dataFilePath("clan_attendance_log.json"));
    if (!f.exists())
        return;
    if (!f.open(QIODevice::ReadOnly))
        return;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject())
        return;
    QJsonObject root = doc.object();
    for (auto it = root.begin(); it != root.end(); ++it)
    {
        if (!it.value().isArray())
            continue;
        QList<QJsonObject> entries;
        for (const QJsonValue &val : it.value().toArray())
        {
            if (val.isObject())
                entries.append(val.toObject());
        }
        attendanceRecords.insert(it.key(), entries);
    }
}
void MainWindow::saveAttendance()
{
    QJsonObject root;
    for (auto it = attendanceRecords.constBegin(); it != attendanceRecords.constEnd(); ++it)
    {
        QJsonArray arr;
        for (const QJsonObject &obj : it.value())
            arr.append(obj);
        root.insert(it.key(), arr);
    }
    QFile f(dataFilePath("clan_attendance_log.json"));
    if (!f.open(QIODevice::WriteOnly))
        return;
    QJsonDocument doc(root);
    f.write(doc.toJson(QJsonDocument::Indented));
}
void MainWindow::recordAttendance() {}
void MainWindow::appendAttendanceLog(const QString &playerKey, const QString &type, const QDateTime &when, const QString &trainingId, const QString &map)
{
    if (playerKey.isEmpty())
        return;
    QJsonObject entry;
    entry.insert("type", type);
    entry.insert("date", when.date().toString(Qt::ISODate));
    entry.insert("timestamp", when.toString(Qt::ISODate));
    if (!trainingId.isEmpty())
        entry.insert("name", trainingId);
    if (!map.isEmpty())
        entry.insert("map", map);
    attendanceRecords[playerKey].append(entry);
    saveAttendance();
}
void MainWindow::appendSoldbuchEntry(const QString &playerKey, const QString &kind, const QJsonObject &data, const QDateTime &when)
{
    if (playerKey.isEmpty())
        return;
    QJsonObject entry = data;
    entry.insert("kind", kind);
    entry.insert("date", when.date().toString(Qt::ISODate));
    entry.insert("timestamp", when.toString(Qt::ISODate));
    soldbuchRecords[playerKey].append(entry);
    saveSoldbuch();
}
void MainWindow::loadSoldbuch()
{
    soldbuchRecords.clear();
    QFile f(dataFilePath("clan_soldbuch_log.json"));
    if (!f.exists())
        return;
    if (!f.open(QIODevice::ReadOnly))
        return;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject())
        return;
    QJsonObject root = doc.object();
    for (auto it = root.begin(); it != root.end(); ++it)
    {
        if (!it.value().isArray())
            continue;
        QList<QJsonObject> entries;
        for (const QJsonValue &val : it.value().toArray())
        {
            if (val.isObject())
                entries.append(val.toObject());
        }
        soldbuchRecords.insert(it.key(), entries);
    }
}
void MainWindow::saveSoldbuch()
{
    QJsonObject root;
    for (auto it = soldbuchRecords.constBegin(); it != soldbuchRecords.constEnd(); ++it)
    {
        QJsonArray arr;
        for (const QJsonObject &obj : it.value())
            arr.append(obj);
        root.insert(it.key(), arr);
    }
    QFile f(dataFilePath("clan_soldbuch_log.json"));
    if (!f.open(QIODevice::WriteOnly))
        return;
    QJsonDocument doc(root);
    f.write(doc.toJson(QJsonDocument::Indented));
}
void MainWindow::saveAttendancePercentToSoldbuch(const QString &playerKey, int percent, const QDateTime &when)
{
    if (playerKey.isEmpty())
        return;
    QJsonObject payload;
    payload.insert("percent", percent);
    appendSoldbuchEntry(playerKey, "attendance-percent", payload, when);
}

void MainWindow::updateEligibilityForRow(int sourceRow) { Q_UNUSED(sourceRow); }
void MainWindow::updateEligibilityForPlayerKey(const QString &playerKey) { Q_UNUSED(playerKey); }
void MainWindow::updateAttendancePercentForPlayerKey(const QString &playerKey) { Q_UNUSED(playerKey); }
void MainWindow::showPromotionDialogForRow(int sourceRow) { Q_UNUSED(sourceRow); }
void MainWindow::promotePlayerAtRow(int sourceRow) { Q_UNUSED(sourceRow); }
void MainWindow::demotePlayerAtRow(int sourceRow) { Q_UNUSED(sourceRow); }
void MainWindow::promotePlayerByKey(const QString &playerKey)
{
    Player *player = findPlayerByKey(playerKey);
    if (!player)
        return;
    const QStringList ranks = MainWindow::rankOptions();
    int idx = ranks.indexOf(player->rank);
    if (idx < 0 || idx + 1 >= ranks.size())
    {
        QMessageBox::information(this, "Beförderung", "Kein höherer Rang verfügbar.");
        return;
    }

    QString targetRank = ranks.at(idx + 1);
    QString question = QStringLiteral("%1 von %2 zu %3 befördern?")
                           .arg(player->name.isEmpty() ? playerKey : player->name)
                           .arg(player->rank.isEmpty() ? QStringLiteral("-") : player->rank)
                           .arg(targetRank);
    if (QMessageBox::question(this, "Beförderung bestätigen", question, QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;

    player->rank = targetRank;
    player->nextRank = (idx + 2 < ranks.size()) ? ranks.at(idx + 2) : QString();
    player->attendance = 0;
    player->events = 0;
    player->reserve = 0;
    player->lastPromotionDate = nowDate();

    int row = rowForPlayerKey(playerKey);
    if (row >= 0)
    {
        if (QStandardItem *rankItem = model->item(row, 8))
        {
            rankItem->setData(player->rank, Qt::EditRole);
            rankItem->setData(formatRankDisplay(*player, true), Qt::DisplayRole);
        }
        if (QStandardItem *trainingItem = model->item(row, 5))
        {
            trainingItem->setText(formatTrainingDisplay(*player));
            trainingItem->setToolTip(trainingTooltip(*player));
        }
    }

    savePlayers();
    if (row >= 0)
        validateRow(row);
}
void MainWindow::demotePlayerByKey(const QString &playerKey)
{
    Player *player = findPlayerByKey(playerKey);
    if (!player)
        return;
    const QStringList ranks = MainWindow::rankOptions();
    int idx = ranks.indexOf(player->rank);
    if (idx <= 0)
    {
        QMessageBox::information(this, "Degradierung", "Kein niedrigerer Rang vorhanden.");
        return;
    }
    QString targetRank = ranks.at(idx - 1);
    QString question = QStringLiteral("%1 von %2 zu %3 degradieren?")
                           .arg(player->name.isEmpty() ? playerKey : player->name)
                           .arg(player->rank.isEmpty() ? QStringLiteral("-") : player->rank)
                           .arg(targetRank);
    if (QMessageBox::question(this, "Degradierung bestätigen", question, QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;

    player->rank = targetRank;
    player->nextRank = ranks.at(idx);
    int row = rowForPlayerKey(playerKey);
    if (row >= 0)
    {
        if (QStandardItem *rankItem = model->item(row, 8))
        {
            rankItem->setData(player->rank, Qt::EditRole);
            rankItem->setData(formatRankDisplay(*player, false), Qt::DisplayRole);
        }
    }
    savePlayers();
    if (row >= 0)
        validateRow(row);
}
QDate MainWindow::nowDate() const
{
    if (useTestDate && useTestDate->isChecked() && testDateEdit)
        return testDateEdit->date();
    return QDate::currentDate();
}
void MainWindow::handleCreateSessionButtonForRow(int sourceRow)
{
    if (!model || sourceRow < 0 || sourceRow >= model->rowCount())
        return;
    QStringList preselect;
    QString playerKey;
    QString playerName;
    if (playerContextForRow(sourceRow, playerKey, playerName))
        preselect << playerName;
    showCreateSessionDialog(preselect, false, false);
}

void MainWindow::handleSoldbuchButtonForRow(int sourceRow)
{
    QString playerKey;
    QString playerName;
    if (!playerContextForRow(sourceRow, playerKey, playerName))
        return;
    showSoldbuchDialogForPlayer(playerKey, playerName);
}

void MainWindow::handleTrainingButtonForRow(int sourceRow)
{
    handleAttendanceButtonForRow(sourceRow, QStringLiteral("Training"), true);
}

void MainWindow::handleEventButtonForRow(int sourceRow)
{
    handleAttendanceButtonForRow(sourceRow, QStringLiteral("Event"), true);
}

void MainWindow::handleReserveButtonForRow(int sourceRow)
{
    handleAttendanceButtonForRow(sourceRow, QStringLiteral("Reserve"), true);
}
void MainWindow::loadGroups()
{
    groups.clear();
    groupCategory.clear();
    QFile f(dataFilePath("clan_groups.json"));
    if (f.exists() && f.open(QIODevice::ReadOnly))
    {
        QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
        QJsonArray arr;
        if (doc.isArray())
            arr = doc.array();
        else if (doc.isObject())
            arr = doc.object().value("groups").toArray();
        for (const QJsonValue &val : arr)
        {
            if (!val.isObject())
                continue;
            QJsonObject obj = val.toObject();
            QString name = obj.value("name").toString().trimmed();
            if (name.isEmpty())
                continue;
            groups.append(name);
            QString category = obj.value("category").toString().trimmed();
            if (!category.isEmpty())
                groupCategory.insert(name, category);
            QString color = obj.value("color").toString().trimmed();
            if (!color.isEmpty())
                groupColors.insert(name, color);
        }
    }

    if (groups.isEmpty())
    {
        appendDefaultGroups(groups, groupCategory, groupColors);
    }
    std::sort(groups.begin(), groups.end(), [](const QString &a, const QString &b)
              { return a.compare(b, Qt::CaseInsensitive) < 0; });
    groups.erase(std::unique(groups.begin(), groups.end(), [](const QString &a, const QString &b)
                             { return a.compare(b, Qt::CaseInsensitive) == 0; }),
                 groups.end());
    const QMap<QString, QString> oldCategories = groupCategory;
    const QMap<QString, QString> oldColors = groupColors;
    groupCategory.clear();
    groupColors.clear();
    for (const QString &name : groups)
    {
        for (auto it = oldCategories.constBegin(); it != oldCategories.constEnd(); ++it)
        {
            if (it.key().compare(name, Qt::CaseInsensitive) == 0)
            {
                groupCategory.insert(name, it.value());
                break;
            }
        }
        for (auto it = oldColors.constBegin(); it != oldColors.constEnd(); ++it)
        {
            if (it.key().compare(name, Qt::CaseInsensitive) == 0)
            {
                groupColors.insert(name, it.value());
                break;
            }
        }
    }

    refreshGroupFilterCombo();
}
void MainWindow::saveGroups()
{
    QJsonArray arr;
    for (const QString &name : groups)
    {
        QJsonObject obj;
        obj.insert("name", name);
        const QString category = groupCategory.value(name);
        if (!category.isEmpty())
            obj.insert("category", category);
        const QString color = groupColors.value(name);
        if (!color.isEmpty())
            obj.insert("color", color);
        arr.append(obj);
    }
    QFile f(dataFilePath("clan_groups.json"));
    if (!f.open(QIODevice::WriteOnly))
        return;
    f.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
}
void MainWindow::loadOrganization() {}
void MainWindow::saveOrganization() {}
void MainWindow::setTableWatermark() {}
bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched);
    Q_UNUSED(event);
    return false;
}
void MainWindow::paintEvent(QPaintEvent *event)
{
    QMainWindow::paintEvent(event);
    Q_UNUSED(event);
}
void MainWindow::updateRankFilterCombo() {}
void MainWindow::loadSoldbuchSettings() {}
void MainWindow::saveSoldbuchSettings() {}

QString MainWindow::errorLogPath() const
{
    return dataFilePath("error.log");
}

void MainWindow::appendErrorLog(const QString &source, const QString &message)
{
    QFile f(errorLogPath());
    if (!f.open(QIODevice::Append | QIODevice::Text))
        return;
    QTextStream ts(&f);
    ts.setEncoding(QStringConverter::Utf8);
    const QString stamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    ts << stamp << " [" << source << "] " << message << '\n';
    ts.flush();
}

void MainWindow::showErrorLogDialog()
{
    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("Fehlerprotokoll"));
    QVBoxLayout *lay = new QVBoxLayout(&dlg);
    QTextEdit *edit = new QTextEdit(&dlg);
    edit->setReadOnly(true);
    QFile f(errorLogPath());
    QString content;
    if (f.exists() && f.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream ts(&f);
        ts.setEncoding(QStringConverter::Utf8);
        content = ts.readAll();
    }
    else
    {
        content = QStringLiteral("Keine Einträge vorhanden.");
    }
    edit->setPlainText(content);
    lay->addWidget(edit);
    QHBoxLayout *btnRow = new QHBoxLayout;
    QPushButton *clearBtn = new QPushButton(QStringLiteral("Leeren"), &dlg);
    QPushButton *closeBtn = new QPushButton(QStringLiteral("Schließen"), &dlg);
    btnRow->addWidget(clearBtn);
    btnRow->addStretch();
    btnRow->addWidget(closeBtn);
    lay->addLayout(btnRow);
    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
    connect(clearBtn, &QPushButton::clicked, this, [this, edit]()
            {
        QFile f(errorLogPath());
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            f.close();
            edit->setPlainText(QString());
        } });
    dlg.resize(700, 500);
    dlg.exec();
}

// ==================== Session Management Methods ====================

void MainWindow::refreshSessionPlayerLists()
{
    sessionConfirmedList->clear();
    sessionDeclinedList->clear();
    sessionNoResponseList->clear();

    for (auto it = sessionPlayerStatus.constBegin(); it != sessionPlayerStatus.constEnd(); ++it)
    {
        const QString &playerName = it.key();
        ResponseStatus status = it.value();

        switch (status)
        {
        case ResponseStatus::Confirmed:
            sessionConfirmedList->addItem(playerName);
            break;
        case ResponseStatus::Declined:
            sessionDeclinedList->addItem(playerName);
            break;
        case ResponseStatus::NoResponse:
        {
            int cnt = 0;
            for (const Player &p : list.players)
            {
                if (p.name == playerName)
                {
                    cnt = p.noResponseCounter;
                    break;
                }
            }
            sessionNoResponseList->addItem(QStringLiteral("%1 (NR: %2)").arg(playerName).arg(cnt));
        }
        break;
        }
    }
}

void MainWindow::addPlayerToSession()
{
    // Mehrfachauswahl-Dialog mit Gruppen zum Aufklappen
    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("Spieler zur Session hinzufügen (Mehrfachauswahl)"));
    dlg.resize(600, 500);
    QVBoxLayout *v = new QVBoxLayout(&dlg);

    QLabel *hint = new QLabel(QStringLiteral("Wähle Spieler per Checkbox aus. Gruppen sind aufklappbar."), &dlg);
    v->addWidget(hint);

    // Ziel-Liste (Status) für hinzugefügte Spieler
    QHBoxLayout *statusRow = new QHBoxLayout;
    QLabel *statusLbl = new QLabel(QStringLiteral("Status:"), &dlg);
    QComboBox *statusCombo = new QComboBox(&dlg);
    statusCombo->addItems({QStringLiteral("Zugesagt"), QStringLiteral("Abgesagt"), QStringLiteral("Keine Antwort")});
    statusRow->addWidget(statusLbl);
    statusRow->addWidget(statusCombo, 1);
    v->addLayout(statusRow);

    QTreeWidget *tree = new QTreeWidget(&dlg);
    tree->setHeaderHidden(true);
    tree->setRootIsDecorated(true);
    v->addWidget(tree, 1);

    // Baue Gruppen -> Spieler Struktur
    QMap<QString, QTreeWidgetItem *> groupItems;
    for (const QString &grp : groups)
    {
        auto *gitem = new QTreeWidgetItem(QStringList{grp});
        gitem->setFlags(gitem->flags() | Qt::ItemIsUserCheckable);
        gitem->setCheckState(0, Qt::Unchecked);
        tree->addTopLevelItem(gitem);
        groupItems.insert(grp, gitem);
    }
    // Fallback-Gruppe, falls Spieler eine unbekannte Gruppe haben
    if (!groupItems.contains(unassignedGroupName))
    {
        auto *gitem = new QTreeWidgetItem(QStringList{unassignedGroupName});
        gitem->setFlags(gitem->flags() | Qt::ItemIsUserCheckable);
        gitem->setCheckState(0, Qt::Unchecked);
        tree->addTopLevelItem(gitem);
        groupItems.insert(unassignedGroupName, gitem);
    }

    // Spieler als Kinder mit Checkboxen
    for (const Player &p : list.players)
    {
        QString grp = p.group.isEmpty() ? unassignedGroupName : p.group;
        QTreeWidgetItem *parent = groupItems.value(grp, groupItems.value(unassignedGroupName));
        auto *pitem = new QTreeWidgetItem(QStringList{p.name});
        pitem->setFlags(pitem->flags() | Qt::ItemIsUserCheckable);
        pitem->setCheckState(0, Qt::Unchecked);
        parent->addChild(pitem);
    }
    tree->expandAll();

    // Gruppen-Checkbox beeinflusst Kinder
    connect(tree, &QTreeWidget::itemChanged, &dlg, [tree](QTreeWidgetItem *item, int col)
            {
        if (!item || item->parent()) return; // nur Gruppe
        Qt::CheckState st = item->checkState(col);
        for (int i=0;i<item->childCount();++i)
            item->child(i)->setCheckState(0, st); });

    // Buttons
    QHBoxLayout *btns = new QHBoxLayout;
    QPushButton *ok = new QPushButton(QStringLiteral("Hinzufügen"), &dlg);
    QPushButton *cancel = new QPushButton(QStringLiteral("Abbrechen"), &dlg);
    btns->addStretch();
    btns->addWidget(ok);
    btns->addWidget(cancel);
    v->addLayout(btns);
    connect(ok, &QPushButton::clicked, &dlg, &QDialog::accept);
    connect(cancel, &QPushButton::clicked, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted)
        return;

    // Sammle ausgewählte Spieler
    QStringList selectedPlayers;
    for (int i = 0; i < tree->topLevelItemCount(); ++i)
    {
        QTreeWidgetItem *grpItem = tree->topLevelItem(i);
        for (int c = 0; c < grpItem->childCount(); ++c)
        {
            QTreeWidgetItem *pitem = grpItem->child(c);
            if (pitem->checkState(0) == Qt::Checked)
                selectedPlayers << pitem->text(0);
        }
    }
    if (selectedPlayers.isEmpty())
    {
        QMessageBox::information(this, QStringLiteral("Session"), QStringLiteral("Keine Spieler ausgewählt."));
        return;
    }

    // Füge alle ausgewählten zur Session mit gewähltem Status hinzu
    int added = 0;
    ResponseStatus targetStatus = ResponseStatus::NoResponse;
    const QString chosen = statusCombo->currentText();
    if (chosen == QStringLiteral("Zugesagt"))
        targetStatus = ResponseStatus::Confirmed;
    else if (chosen == QStringLiteral("Abgesagt"))
        targetStatus = ResponseStatus::Declined;
    else
        targetStatus = ResponseStatus::NoResponse;
    for (const QString &name : selectedPlayers)
    {
        if (sessionPlayerStatus.contains(name))
            continue;
        sessionPlayerStatus.insert(name, targetStatus);
        added++;
    }
    refreshSessionPlayerLists();
    updateSessionSummary();
    QMessageBox::information(this, QStringLiteral("Session"), QStringLiteral("%1 Spieler zur Session hinzugefügt.").arg(added));
}

void MainWindow::removePlayerFromSession()
{
    // Finde ausgewählten Spieler in einer der drei Listen
    QListWidget *activeList = nullptr;
    QString selectedPlayer;

    if (sessionConfirmedList->currentItem())
    {
        activeList = sessionConfirmedList;
        selectedPlayer = sessionConfirmedList->currentItem()->text();
    }
    else if (sessionDeclinedList->currentItem())
    {
        activeList = sessionDeclinedList;
        selectedPlayer = sessionDeclinedList->currentItem()->text();
    }
    else if (sessionNoResponseList->currentItem())
    {
        activeList = sessionNoResponseList;
        selectedPlayer = sessionNoResponseList->currentItem()->text();
    }

    if (selectedPlayer.isEmpty())
    {
        QMessageBox::information(this, QStringLiteral("Info"),
                                 QStringLiteral("Bitte wähle einen Spieler aus einer der Listen aus."));
        return;
    }

    // Aus Status-Map entfernen
    sessionPlayerStatus.remove(selectedPlayer);

    // Listen aktualisieren
    refreshSessionPlayerLists();
    updateSessionSummary();
}

void MainWindow::movePlayerToConfirmed()
{
    QString selectedPlayer;

    // Prüfe zuerst Abgelehnt-Liste
    if (sessionDeclinedList->currentItem())
    {
        selectedPlayer = sessionDeclinedList->currentItem()->text();
    }
    // Dann Keine Antwort-Liste
    else if (sessionNoResponseList->currentItem())
    {
        selectedPlayer = sessionNoResponseList->currentItem()->text();
    }

    if (selectedPlayer.isEmpty())
    {
        QMessageBox::information(this, QStringLiteral("Info"),
                                 QStringLiteral("Bitte wähle einen Spieler aus der 'Abgesagt' oder 'Keine Antwort' Liste aus."));
        return;
    }

    // Status auf Confirmed setzen
    sessionPlayerStatus[selectedPlayer] = ResponseStatus::Confirmed;

    // Listen aktualisieren
    refreshSessionPlayerLists();
    updateSessionSummary();
}

void MainWindow::movePlayerToDeclined()
{
    QString selectedPlayer;

    // Prüfe zuerst Zugesagt-Liste
    if (sessionConfirmedList->currentItem())
    {
        selectedPlayer = sessionConfirmedList->currentItem()->text();
    }
    // Dann Keine Antwort-Liste
    else if (sessionNoResponseList->currentItem())
    {
        selectedPlayer = sessionNoResponseList->currentItem()->text();
    }

    if (selectedPlayer.isEmpty())
    {
        QMessageBox::information(this, QStringLiteral("Info"),
                                 QStringLiteral("Bitte wähle einen Spieler aus der 'Zugesagt' oder 'Keine Antwort' Liste aus."));
        return;
    }

    // Status auf Declined setzen
    sessionPlayerStatus[selectedPlayer] = ResponseStatus::Declined;

    // Listen aktualisieren
    refreshSessionPlayerLists();
    updateSessionSummary();
}
