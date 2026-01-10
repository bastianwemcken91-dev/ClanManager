#pragma once

#include <QMainWindow>
#include <QStandardItemModel>
#include "PlayerList.h"

#include <QStringList>
#include <QJsonObject>
#include "Training.h"
#include <QLabel>
#include <QColor>
#include <QPixmap>
#include <QSet>
#include <QListWidget>

struct RankRequirement
{
    int minMonths = 0;
    int minLevel = 0;
    int minCombined = 0;
};

class QTableView;
class QPushButton;
class QSortFilterProxyModel;
class QLineEdit;
class QComboBox;
class QCheckBox;
class QDateEdit;
class QLabel;
class TrainingButtonDelegate;
class QGroupBox;
class QTreeWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    // Allow local delegate classes in MainWindow.cpp to access private helpers/state
    friend class RankDelegate;
    friend class GroupDelegate;
    friend class DateDelegate;
    friend class NextRankDelegate;
    friend class CommentDelegate;
    friend class PromotionDelegate;
    friend class ActionButtonDelegate;

public:
    MainWindow(QWidget *parent = nullptr);

    // Accessor helpers for UI delegates
    QStringList getGroups() const { return groups; }
    QMap<QString, QString> getGroupCategory() const { return groupCategory; }

    static QStringList rankOptions();

    // Test-Hilfen
    bool importCsvFile(const QString &filePath, int *outImported = nullptr, int *outMerged = nullptr, int *outSkipped = nullptr); // nicht interaktiv
    bool isPlayerFlaggedNoResponse(const QString &playerName) const;                                                              // rotes X im Status
    QMap<QString, QStringList> groupingSnapshot() const;                                                                          // Gruppe -> Spielernamen
    QString readErrorLogContents() const;                                                                                         // gesamter Log-Inhalt
    void simulateNoResponseIncrement(const QStringList &selectedKeys);                                                            // erhöht Counter für nicht ausgewählte
    // Test Utilities
    void testAddPlayer(const Player &p);
    int noResponseThresholdValue() const { return noResponseThreshold; }
    void testRebuildModel();
    void testAppendErrorLog(const QString &source, const QString &message) { appendErrorLog(source, message); }

    // Validation helpers
    int monthsRequiredForRank(const QString &rank);
    int monthsSinceJoin(const QDate &joinDate);
    void validateAllRows();
    bool validateRow(int row, QString *outReason = nullptr);

private slots:
    void showSettingsDialog();
    void editRankRequirements();
    void editRanks();
    void addPlayer();
    void editGroups();
    void showOrganization();
    void showContextMenu(const QPoint &pos);
    void recordAttendanceForPlayer(const QString &playerKey, const QString &type, const QDate &date = QDate::currentDate(), const QString &trainingId = QString(), const QString &map = QString(), const QString &eventName = QString());
    void editPlayer();
    void showPlayerAttendance();
    void showPlayerSoldbuch();
    void promotePlayer();
    void demotePlayer();

private:
    QMap<QString, RankRequirement> rankRequirements; // configurable requirements per rank
    void loadRankRequirements();
    void saveRankRequirements();
    RankRequirement requirementForRank(const QString &rank) const;
    RankRequirement defaultRequirementForRank(const QString &rank) const;
private slots:
    void importCsv();
    void exportCsv();
    void addAttendance();
    void assignAttendanceMulti();
    void showErrorLogDialog();

private:
    QTableView *table = nullptr;
    QStandardItemModel *model = nullptr;
    QSortFilterProxyModel *proxy = nullptr;
    QLineEdit *searchEdit = nullptr;
    QComboBox *rankFilterCombo = nullptr;
    QComboBox *filterModeCombo = nullptr;
    QComboBox *groupFilterCombo = nullptr;
    QComboBox *sortFieldCombo = nullptr;
    QComboBox *sortOrderCombo = nullptr;
    QCheckBox *useTestDate = nullptr;
    QDateEdit *testDateEdit = nullptr;
    PlayerList list;
    QGroupBox *sessionBox = nullptr;
    QComboBox *sessionTemplateCombo = nullptr;
    QComboBox *sessionTypeCombo = nullptr;
    QLineEdit *sessionNameEdit = nullptr;
    QComboBox *sessionMapCombo = nullptr;
    QDateEdit *sessionDateEdit = nullptr;
    QCheckBox *sessionRememberCheck = nullptr;
    QTreeWidget *sessionPlayerTree = nullptr;
    QListWidget *sessionConfirmedList = nullptr;
    QListWidget *sessionDeclinedList = nullptr;
    QListWidget *sessionNoResponseList = nullptr;
    QLabel *sessionSummaryLabel = nullptr;
    enum ResponseStatus
    {
        Confirmed,
        Declined,
        NoResponse
    };
    QMap<QString, ResponseStatus> sessionPlayerStatus; // playerKey -> response status
    QStringList groups;                                // list of group names
    QMap<QString, QString> groupCategory;              // group -> category
    QMap<QString, QString> groupColors;                // group -> hex color
    QString organizationHtml;
    QString backgroundImagePath;
    QSet<QString> sessionSelectedPlayers;
    // structured attendance records: each entry is an object with at least { date, type, trainingId? }
    QMap<QString, QList<QJsonObject>> attendanceRecords; // playerKey -> list of attendance objects
    QStringList commentOptions;                          // selectable comment entries saved to attendance log
    QMap<QString, QList<QJsonObject>> soldbuchRecords;   // playerKey -> list of soldbuch entries

    // App Settings
    int noResponseThreshold = 10;
    int fuzzyMatchThreshold = 2;
    bool autoCreatePlayers = true;
    bool fuzzyMatchingEnabled = true;
    bool autoFillMetadata = true;
    QString ocrLanguage = "deu";
    QString unassignedGroupName = "Nicht zugewiesen";
    bool incrementCounterOnNoResponse = true;
    bool resetCounterOnResponse = true;
    bool showCounterInTable = true;
    QString hintColumnName = "Hinweis";

    void loadSettings();
    void saveSettings();
    void applySettingsToUI();

    void loadCommentOptions();
    void saveCommentOptions();

    // Trainings (master list) and summary UI
    QList<struct Training> trainings;
    QLabel *trainingsSummaryLabel = nullptr;
    QList<QString> maps; // master list of known maps

    void loadTrainings();
    void saveTrainings();
    void purgeOldTrainings(int days = 31);
    void loadMaps();
    void saveMaps();
    void resetMapsToDefaults();
    QList<struct Training> recentTrainings(int days = 21) const;
    const Training *trainingById(const QString &id) const;
    void showSelectTrainingDialogForPlayer(const QString &playerKey);
    void showSelectEventDialogForPlayer(const QString &playerKey);
    void showCreateSessionDialog(const QStringList &preselect = QStringList(), bool forceTraining = false, bool forceEvent = false);
    void updateTrainingsSummary();
    void showCreateLineupDialog();

    void loadPlayers();
    void savePlayers();
    void loadGroupColors();
    void saveGroupColors();
    void updateGroupDecorations();
    QColor colorForGroup(const QString &groupName) const;
    void applySortSettings();
    void refreshSessionTemplates();
    void applySessionTemplateSelection();
    void refreshSessionPlayerTable();
    void refreshSessionPlayerLists();
    void refreshSessionMapCombo();
    void applySessionSelectionFromTable();
    void commitSessionAssignment();
    void addPlayerToSession();
    void removePlayerFromSession();
    void movePlayerToConfirmed();
    void movePlayerToDeclined();
    void updateSessionSummary();
    bool playerHasSessionRecord(const QString &playerKey, const QString &type, const QString &name, const QDate &date) const;
    void refreshGroupFilterCombo();
    bool ensureGroupRegistered(const QString &groupName, const QString &category = QString());
    void loadAttendance();
    void saveAttendance();
    void recordAttendance();
    void appendAttendanceLog(const QString &playerKey, const QString &type, const QDateTime &when, const QString &trainingId = QString(), const QString &map = QString());
    void appendSoldbuchEntry(const QString &playerKey, const QString &kind, const QJsonObject &data, const QDateTime &when);
    void loadSoldbuch();
    void saveSoldbuch();
    void saveAttendancePercentToSoldbuch(const QString &playerKey, int percent, const QDateTime &when);
    void showSoldbuchDialogForPlayer(const QString &playerKey, const QString &playerName);
    bool playerContextForRow(int sourceRow, QString &playerKey, QString &playerName) const;

    void refreshModelFromList();
    Player playerFromModelRow(int row) const;
    void addPlayerToModel(const Player &p);
    QString formatTrainingDisplay(const Player &player) const;
    QString trainingTooltip(const Player &player) const;
    QString formatRankDisplay(const Player &player, bool eligible) const;
    void updatePromotionIndicatorForRow(int row, bool eligible);
    Player *findPlayerByKey(const QString &playerKey);
    int rowForPlayerKey(const QString &playerKey) const;
    void incrementPlayerCounters(const QString &playerKey, const QString &type);
    void updateEligibilityForRow(int sourceRow);
    void updateEligibilityForPlayerKey(const QString &playerKey);
    void updateAttendancePercentForPlayerKey(const QString &playerKey);
    QString playerKeyForRow(int row) const;
    struct AttendanceSummary
    {
        int trainings = 0;
        int events = 0;
        int reserve = 0;
    };
    AttendanceSummary attendanceSummaryForPlayer(const QString &playerKey, const QDate &referenceDate) const;
    struct AttendanceDialogResult
    {
        QString type;
        QString name;
        QString map;
        QDate date;
    };
    bool openAttendanceEntryDialog(const QString &playerName, AttendanceDialogResult &result, const QString &forcedType = QString(), bool lockType = true);
    void handleCreateSessionButtonForRow(int sourceRow);
    void handleSoldbuchButtonForRow(int sourceRow);
    void handleTrainingButtonForRow(int sourceRow);
    void handleEventButtonForRow(int sourceRow);
    void handleReserveButtonForRow(int sourceRow);
    void handleAttendanceButtonForRow(int sourceRow, const QString &forcedType, bool quickAdd = false);
    void ensureMapKnown(const QString &map);
    void batchAssignAttendance(const QStringList &playerKeys, const QString &type, const QString &name, const QString &map, const QDate &date, bool rememberTemplate, const QString &templateId, bool showSummary);
    bool openPlayerEditDialog(Player &player, bool editing = false);
    void showPromotionDialogForRow(int sourceRow);
    void promotePlayerAtRow(int sourceRow);
    void demotePlayerAtRow(int sourceRow);
    void promotePlayerByKey(const QString &playerKey);
    void demotePlayerByKey(const QString &playerKey);
    QDate nowDate() const;
    void loadGroups();
    void saveGroups();
    void loadOrganization();
    void saveOrganization();
    // watermark helpers
    void setTableWatermark();
    bool eventFilter(QObject *watched, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void updateRankFilterCombo();

    // Soldbuch UI settings
    void loadSoldbuchSettings();
    void saveSoldbuchSettings();

    bool soldbuchColoringEnabled = true;
    QMap<QString, QColor> soldbuchColors; // kind -> color
    int soldbuchFontPointSize = 10;

    // watermark configuration
    double watermarkOpacity = 0.4;              // default requested by user
    QColor watermarkColor = QColor(64, 64, 64); // dark gray tint
    QPixmap watermarkPixmap;

    // Fehler-Logging
    QString errorLogPath() const;                                       // Pfad zu error.log
    void appendErrorLog(const QString &source, const QString &message); // schreibt einen Eintrag
    
    // Initialization helpers (added to fix startup crash)
    void initializeUI();
    void loadDataFiles();
};
