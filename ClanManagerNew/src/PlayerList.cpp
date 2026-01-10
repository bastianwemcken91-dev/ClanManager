#include "PlayerList.h"
#include <QFile>
#include <QTextStream>

bool PlayerList::loadFromCsv(const QString &filePath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    QTextStream in(&f);
    m_players.clear();
    while (!in.atEnd())
    {
        QString line = in.readLine().trimmed();
        if (line.isEmpty())
            continue;
        Player p = Player::fromCsvLine(line);
        m_players.append(p);
    }
    return true;
}

bool PlayerList::saveToCsv(const QString &filePath) const
{
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    QTextStream out(&f);
    for (const Player &p : m_players)
    {
        out << p.toCsvLine() << "\n";
    }
    return true;
}

void PlayerList::addPlayer(const Player &p)
{
    m_players.append(p);
}
