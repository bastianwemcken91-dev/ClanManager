#pragma once

#include <QString>
#include <QDate>

struct Player
{
    int id = 0;
    QString name;
    int level = 0;
    QString group;
    QDate joinDate;
    QString rank;
    int attendance = 0;
    QString comment;

    static Player fromCsvLine(const QString &line);
    QString toCsvLine() const;
};
