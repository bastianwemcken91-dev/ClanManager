#include "Player.h"
#include <QStringList>

QString Player::toCsvLine() const
{
    QStringList parts;
    parts << name
          << t17name
          << QString::number(level)
          << group
          << QString::number(attendance)
          << comment
          << joinDate.toString(Qt::ISODate)
          << rank
          << lastPromotionDate.toString(Qt::ISODate)
          << nextRank;
    // append counters so exports retain per-rank tracking
    parts << QString::number(totalAttendance)
          << QString::number(events)
          << QString::number(totalEvents)
          << QString::number(reserve)
          << QString::number(totalReserve)
          << QString::number(noResponseCounter);
    return parts.join('\t');
}

Player Player::fromCsvLine(const QString &line)
{
    Player p;
    QStringList parts = line.split('\t');
    if (parts.size() >= 1)
        p.name = parts.value(0).trimmed();
    if (parts.size() >= 2)
        p.t17name = parts.value(1).trimmed();
    if (parts.size() >= 3)
        p.level = parts.value(2).toInt();
    if (parts.size() >= 4)
        p.group = parts.value(3).trimmed();
    if (parts.size() >= 5)
        p.attendance = parts.value(4).toInt();
    if (parts.size() >= 6)
        p.comment = parts.value(5).trimmed();
    if (parts.size() >= 7)
        p.joinDate = QDate::fromString(parts.value(6).trimmed(), Qt::ISODate);
    if (parts.size() >= 8)
        p.rank = parts.value(7).trimmed();
    if (parts.size() >= 9)
        p.lastPromotionDate = QDate::fromString(parts.value(8).trimmed(), Qt::ISODate);
    if (parts.size() >= 10)
        p.nextRank = parts.value(9).trimmed();
    if (parts.size() >= 11)
        p.totalAttendance = parts.value(10).toInt();
    else
        p.totalAttendance = p.attendance;
    if (parts.size() >= 12)
        p.events = parts.value(11).toInt();
    if (parts.size() >= 13)
        p.totalEvents = parts.value(12).toInt();
    else
        p.totalEvents = p.events;
    if (parts.size() >= 14)
        p.reserve = parts.value(13).toInt();
    if (parts.size() >= 15)
        p.totalReserve = parts.value(14).toInt();
    else
        p.totalReserve = p.reserve;
    if (parts.size() >= 16)
        p.noResponseCounter = parts.value(15).toInt();
    return p;
}
