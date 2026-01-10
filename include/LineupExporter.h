#pragma once

#include <QString>
#include <QColor>
#include <QVector>

struct LineupTrupp
{
    QString name;
    QColor color;
    QVector<QString> players; // player display names
};

class LineupExporter
{
public:
    // Write a simple XLSX file containing commander and trupp columns.
    // Returns true on success, false on failure.
    static bool writeXlsx(const QString &filePath, const QString &commander, const QVector<LineupTrupp> &trupps);
};
