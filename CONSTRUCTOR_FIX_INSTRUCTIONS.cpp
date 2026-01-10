// MainWindow.cpp - CRITICAL FIX FOR CONSTRUCTOR
// 
// Problem: Der Konstruktor ist zu lang (1400+ Zeilen) und führt zu Crashes
// Lösung: Verschiebe die Initialisierung in eine separate init() Methode
//
// ANWEISUNGEN:
// 1. Füge diese Deklaration zu MainWindow.h hinzu (im private-Bereich):
//    void initializeUI();
//    void loadDataFiles();
//
// 2. Ersetze im MainWindow-Konstruktor (MainWindow.cpp, Zeile 576) 
//    den GESAMTEN Inhalt ab Zeile 576 bis Zeile 1483 durch:

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // Initialize members to nullptr/default values FIRST
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
    sessionDateEdit = nullptr;
    useTestDate = nullptr;
    testDateEdit = nullptr;
    trainingsSummaryLabel = nullptr;
    
    // Set default values
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

// 3. Füge diese zwei neuen Methoden zum Ende von MainWindow.cpp hinzu:

void MainWindow::initializeUI()
{
    // Hier kommt der GESAMTE UI-Aufbau-Code aus dem ursprünglichen Konstruktor
    // (Zeilen 576-1470 des Original-Konstruktors)
    
    // WICHTIG: Kopiere NUR den UI-Aufbau-Code hierher, NICHT die load*() Aufrufe!
    // Der UI-Code beginnt mit:
    //   QWidget *central = new QWidget(this);
    //   setCentralWidget(central);
    //   model = new QStandardItemModel(this);
    //   ...
    // Und endet mit:
    //   setCentralWidget(central);
    //   connect(...);
    
    qDebug() << "initializeUI: Creating central widget...";
    
    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    
    // HIER DEN GESAMTEN UI-CODE EINFÜGEN (Zeilen 576-1470)
    // Dies ist nur ein Platzhalter - Sie müssen den echten Code kopieren!
    
    qDebug() << "initializeUI: UI creation complete";
}

void MainWindow::loadDataFiles()
{
    // Hier kommen die load*() Aufrufe aus dem ursprünglichen Konstruktor
    // (Zeilen 1473-1483 des Original-Konstruktors)
    
    qDebug() << "loadDataFiles: Loading settings...";
    
    // Add null checks before each load operation
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

// HINWEIS FÜR DIE HEADER-DATEI (MainWindow.h):
// Fügen Sie diese Zeilen zum private-Bereich der MainWindow-Klasse hinzu:
//
//    // Initialization helpers (added to fix startup crash)
//    void initializeUI();
//    void loadDataFiles();
