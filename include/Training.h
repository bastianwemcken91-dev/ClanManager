#pragma once

#include <QString>
#include <QDate>
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>

struct Training
{
    QString id; // arbitrary unique id (e.g. uuid or timestamp-based)
    QDate date;
    QString title;
    QString type;     // Training, Event, Reserve, ...
    QStringList maps; // rollout list of maps played
    // Optional: stored player lists for templates
    QStringList confirmedPlayers;
    QStringList declinedPlayers;
    QStringList noResponsePlayers;

    QJsonObject toJson() const
    {
        QJsonObject obj;
        obj.insert("id", id);
        obj.insert("date", date.toString(Qt::ISODate));
        obj.insert("title", title);
        obj.insert("type", type);
        QJsonArray a;
        for (const QString &m : maps)
            a.append(m);
        obj.insert("maps", a);
        // participants
        QJsonArray conf;
        for (const QString &n : confirmedPlayers)
            conf.append(n);
        obj.insert("confirmedPlayers", conf);
        QJsonArray decl;
        for (const QString &n : declinedPlayers)
            decl.append(n);
        obj.insert("declinedPlayers", decl);
        QJsonArray nor;
        for (const QString &n : noResponsePlayers)
            nor.append(n);
        obj.insert("noResponsePlayers", nor);
        return obj;
    }

    static Training fromJson(const QJsonObject &obj)
    {
        Training t;
        t.id = obj.value("id").toString();
        t.date = QDate::fromString(obj.value("date").toString(), Qt::ISODate);
        t.title = obj.value("title").toString();
        t.type = obj.value("type").toString("Training");
        QJsonArray a = obj.value("maps").toArray();
        for (const QJsonValue &v : a)
            if (v.isString())
                t.maps << v.toString();
        // participants
        QJsonArray conf = obj.value("confirmedPlayers").toArray();
        for (const QJsonValue &v : conf)
            if (v.isString())
                t.confirmedPlayers << v.toString();
        QJsonArray decl = obj.value("declinedPlayers").toArray();
        for (const QJsonValue &v : decl)
            if (v.isString())
                t.declinedPlayers << v.toString();
        QJsonArray nor = obj.value("noResponsePlayers").toArray();
        for (const QJsonValue &v : nor)
            if (v.isString())
                t.noResponsePlayers << v.toString();
        return t;
    }
};
