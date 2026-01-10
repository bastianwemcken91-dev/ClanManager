#include "PlayerList.h"
#include "Player.h"
#include "PlayerList.h"
#include <QStringList>

void PlayerList::clear()
{
    players.clear();
}

void PlayerList::addOrMerge(const Player &p)
{
    // Merge by name + t17name if possible
    for (auto &existing : players)
    {
        if (!p.t17name.isEmpty() && existing.t17name == p.t17name)
        {
            // merge fields conservatively
            existing.level = qMax(existing.level, p.level);
            existing.attendance += p.attendance;
            existing.totalAttendance += p.totalAttendance;
            existing.events += p.events;
            existing.totalEvents += p.totalEvents;
            existing.reserve += p.reserve;
            existing.totalReserve += p.totalReserve;
            if (existing.joinDate.isNull() && !p.joinDate.isNull())
                existing.joinDate = p.joinDate;
            if (existing.comment.isEmpty())
                existing.comment = p.comment;
            if (existing.rank.isEmpty())
                existing.rank = p.rank;
            return;
        }
        if (existing.name == p.name && p.t17name.isEmpty())
        {
            existing.level = qMax(existing.level, p.level);
            existing.attendance += p.attendance;
            existing.totalAttendance += p.totalAttendance;
            existing.events += p.events;
            existing.totalEvents += p.totalEvents;
            existing.reserve += p.reserve;
            existing.totalReserve += p.totalReserve;
            if (existing.joinDate.isNull() && !p.joinDate.isNull())
                existing.joinDate = p.joinDate;
            if (existing.comment.isEmpty())
                existing.comment = p.comment;
            if (existing.rank.isEmpty())
                existing.rank = p.rank;
            return;
        }
    }
    players.push_back(p);
}

QString PlayerList::toCsv() const
{
    QStringList lines;
    for (const auto &p : players)
        lines << p.toCsvLine();
    return lines.join('\n');
}

void PlayerList::fromCsv(const QString &text)
{
    players.clear();
    QStringList lines = text.split('\n', Qt::SkipEmptyParts);
    for (const auto &ln : lines)
    {
        players.push_back(Player::fromCsvLine(ln));
    }
}
