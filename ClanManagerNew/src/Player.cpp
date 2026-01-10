#include "Player.h"
#include <QStringList>

Player Player::fromCsvLine(const QString &line)
{
    // simple comma-separated parser, tolerant to missing fields
    QStringList parts = line.split(',');
    Player p;
    if (parts.size() > 0)
        p.id = parts[0].trimmed().toInt();
    if (parts.size() > 1)
        p.name = parts[1].trimmed();
    if (parts.size() > 2)
        p.level = parts[2].trimmed().toInt();
    if (parts.size() > 3)
        p.group = parts[3].trimmed();
    // flexible parsing: support formats with or without attendance/comment
    if (parts.size() > 4)
    {
        // try to detect if parts[4] is attendance (integer)
        QString p4 = parts[4].trimmed();
        bool ok = false;
        int att = p4.toInt(&ok);
        if (ok)
        {
            p.attendance = att;
            if (parts.size() > 5)
                p.comment = parts[5].trimmed();
            if (parts.size() > 6)
                p.joinDate = QDate::fromString(parts[6].trimmed(), Qt::ISODate);
            if (!p.joinDate.isValid() && parts.size() > 6)
                p.joinDate = QDate::fromString(parts[6].trimmed(), "yyyy-MM-dd");
            if (parts.size() > 7)
                p.rank = parts[7].trimmed();
        }
        else
        {
            // assume parts[4] is joinDate
            p.joinDate = QDate::fromString(parts[4].trimmed(), Qt::ISODate);
            if (!p.joinDate.isValid())
                p.joinDate = QDate::fromString(parts[4].trimmed(), "yyyy-MM-dd");
            if (parts.size() > 5)
                p.rank = parts[5].trimmed();
        }
    }
    return p;
}

QString Player::toCsvLine() const
{
    // prefer extended format: id,name,level,group,attendance,comment,joinDate,rank
    QStringList out;
    out << QString::number(id);
    out << name;
    out << QString::number(level);
    out << group;
    out << QString::number(attendance);
    out << comment;
    out << joinDate.toString(Qt::ISODate);
    out << rank;
    return out.join(',');
}
