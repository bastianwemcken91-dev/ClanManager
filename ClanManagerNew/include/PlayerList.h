#pragma once

#include "Player.h"
#include <QVector>
#include <QString>

class PlayerList
{
public:
    PlayerList() = default;
    bool loadFromCsv(const QString &filePath);
    bool saveToCsv(const QString &filePath) const;
    void addPlayer(const Player &p);
    const QVector<Player> &players() const { return m_players; }
    QVector<Player> &playersMutable() { return m_players; }
    Player &at(int idx) { return m_players[idx]; }
    int size() const { return m_players.size(); }
    void clear() { m_players.clear(); }

private:
    QVector<Player> m_players;
};
