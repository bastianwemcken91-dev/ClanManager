#pragma once

#include "LineupExporter.h"
#include <QDialog>
#include <QVector>
#include <QStringList>
#include <QMap>

class QPushButton;
class QLineEdit;
class QVBoxLayout;
class QComboBox;

// Simple dialog to create/modify a lineup (trupps). Keeps UI lightweight and stores templates.
class LineupDialog : public QDialog
{
    Q_OBJECT
public:
    explicit LineupDialog(QWidget *parent = nullptr);

    // Set available players and optional mapping player->group for filtering
    void setPlayerList(const QStringList &players, const QMap<QString, QString> &playerToGroup = {});

    // Return the assembled lineup (commander + trupps)
    QVector<LineupTrupp> getLineup() const;

    // Validate current dialog state. Returns true if valid, false otherwise with reason in outReason.
    bool validate(QString &outReason) const;

    // Templates: save/load template by name (stored in AppDataLocation/lineup_templates)
    bool saveTemplate(const QString &name, QString &outError) const;
    bool loadTemplate(const QString &name, QString &outError);
    QStringList availableTemplates() const;

signals:
    void lineupChanged();

private slots:
    void onAddTrupp();
    void onRemoveTrupp();
    void onSaveTemplate();
    void onLoadTemplate();

private:
    struct TruppWidget;

    void createUi();
    void rebuildTruppLayout();
    QString templatesDir() const;

    QVBoxLayout *m_mainLayout = nullptr;
    QLineEdit *m_commander = nullptr;
    QList<TruppWidget *> m_trupps;
    QStringList m_players;
    QMap<QString, QString> m_playerToGroup;
    int m_maxPerTrupp = 5;
};
