#pragma once

#include "Player.h"
#include <vector>
#include <QString>

class PlayerList
{
public:
    std::vector<Player> players;

    void clear();
    void addOrMerge(const Player &p);
    QString toCsv() const;
    void fromCsv(const QString &text);
};
