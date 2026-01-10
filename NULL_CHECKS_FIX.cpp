// ZUSÄTZLICHER CRITICAL FIX:
// Füge Null-Checks zu kritischen Funktionen hinzu

// In MainWindow.cpp - Finde die Funktion updateGroupDecorations() und füge am Anfang hinzu:

void MainWindow::updateGroupDecorations()
{
    // ADD THIS NULL CHECK AT THE START:
    if (!model || !table) {
        qWarning() << "updateGroupDecorations: model or table is null, skipping";
        return;
    }
    
    // ... rest of the function ...
}

// In MainWindow.cpp - Finde die Funktion validateAllRows() und füge am Anfang hinzu:

void MainWindow::validateAllRows()
{
    // ADD THIS NULL CHECK AT THE START:
    if (!model || !table) {
        qWarning() << "validateAllRows: model or table is null, skipping";
        return;
    }
    
    // ... rest of the function ...
}

// In MainWindow.cpp - Finde die Funktion refreshModelFromList() und füge am Anfang hinzu:

void MainWindow::refreshModelFromList()
{
    // ADD THIS NULL CHECK AT THE START:
    if (!model) {
        qWarning() << "refreshModelFromList: model is null, skipping";
        return;
    }
    
    // ... rest of the function ...
}

// In MainWindow.cpp - Finde die Funktion refreshSessionMapCombo() und füge am Anfang hinzu:

void MainWindow::refreshSessionMapCombo()
{
    // ADD THIS NULL CHECK AT THE START:
    if (!sessionMapCombo) {
        qWarning() << "refreshSessionMapCombo: sessionMapCombo is null, skipping";
        return;
    }
    
    // ... rest of the function ...
}

// In MainWindow.cpp - Finde die Funktion refreshSessionTemplates() und füge am Anfang hinzu:

void MainWindow::refreshSessionTemplates()
{
    // ADD THIS NULL CHECK AT THE START:
    if (!sessionTemplateCombo) {
        qWarning() << "refreshSessionTemplates: sessionTemplateCombo is null, skipping";
        return;
    }
    
    // ... rest of the function ...
}

// In MainWindow.cpp - Finde die Funktion refreshSessionPlayerTable() und füge am Anfang hinzu:

void MainWindow::refreshSessionPlayerTable()
{
    // ADD THIS NULL CHECK AT THE START:
    if (!sessionConfirmedList || !sessionDeclinedList || !sessionNoResponseList) {
        qWarning() << "refreshSessionPlayerTable: session lists are null, skipping";
        return;
    }
    
    // ... rest of the function ...
}

// In MainWindow.cpp - Finde die Funktion applySettingsToUI() und füge am Anfang hinzu:

void MainWindow::applySettingsToUI()
{
    // ADD THIS NULL CHECK AT THE START:
    if (!model || !table) {
        qWarning() << "applySettingsToUI: model or table is null, skipping";
        return;
    }
    
    // ... rest of the function ...
}
