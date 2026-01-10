#pragma once

#include <QString>
#include <QDate>

struct Player
{
    QString name;
    QString t17name;
    int level = 0;
    QString group;
    int attendance = 0;      // trainings seit letzter Beförderung
    int totalAttendance = 0; // trainings gesamt
    int events = 0;
    int totalEvents = 0;
    int reserve = 0;
    int totalReserve = 0;
    QString comment;
    QDate joinDate;
    QString rank; // Dienstrang, chosen from combo
    QDate lastPromotionDate;
    QString nextRank;
    int noResponseCounter = 0; // Zählt fehlende An-/Abmeldungen

    QString toCsvLine() const;
    static Player fromCsvLine(const QString &line);
};
