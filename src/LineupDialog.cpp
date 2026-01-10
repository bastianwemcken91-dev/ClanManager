#include "LineupDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QColorDialog>
#include <QGroupBox>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QStandardPaths>
#include <QDir>
#include <QMessageBox>
#include <QScrollArea>
#include <QInputDialog>

struct LineupDialog::TruppWidget
{
    QGroupBox *box = nullptr;
    QLineEdit *name = nullptr;
    QPushButton *colorBtn = nullptr;
    QVector<QComboBox *> playerChoosers; // up to m_maxPerTrupp
    QString colorHex;
};

static QString colorToHex(const QColor &c)
{
    return c.isValid() ? c.name() : QString();
}

LineupDialog::LineupDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Aufstellung bearbeiten");
    createUi();
}

void LineupDialog::createUi()
{
    m_mainLayout = new QVBoxLayout(this);
    QLabel *commLbl = new QLabel("Kommandant:", this);
    m_commander = new QLineEdit(this);
    QHBoxLayout *h = new QHBoxLayout;
    h->addWidget(commLbl);
    h->addWidget(m_commander);
    m_mainLayout->addLayout(h);

    // scroll area for variable trupps
    QScrollArea *sa = new QScrollArea(this);
    sa->setWidgetResizable(true);
    QWidget *cont = new QWidget(sa);
    QVBoxLayout *trLay = new QVBoxLayout(cont);
    cont->setLayout(trLay);
    sa->setWidget(cont);
    m_mainLayout->addWidget(sa);

    // initially create default trupps
    QStringList defaults = {"Angriff", "Verteidigung", "Panzerwaffe", "Artillerie"};
    for (const QString &d : defaults)
    {
        TruppWidget *tw = new TruppWidget;
        tw->box = new QGroupBox(d, cont);
        tw->name = new QLineEdit(d, tw->box);
        tw->colorBtn = new QPushButton("Farbe", tw->box);
        tw->colorHex = QString();
        QHBoxLayout *bLay = new QHBoxLayout(tw->box);
        bLay->addWidget(new QLabel("Name:"));
        bLay->addWidget(tw->name);
        bLay->addWidget(tw->colorBtn);
        // player choosers
        for (int i = 0; i < m_maxPerTrupp; ++i)
        {
            QComboBox *cb = new QComboBox(tw->box);
            cb->addItem("");
            tw->playerChoosers.append(cb);
            bLay->addWidget(cb);
        }
        trLay->addWidget(tw->box);
        m_trupps.append(tw);

        connect(tw->colorBtn, &QPushButton::clicked, this, [this, tw]()
                {
            QColor c = QColorDialog::getColor(QColor(tw->colorHex), this, "Trupp-Farbe wählen");
            if (c.isValid())
            {
                tw->colorHex = colorToHex(c);
                tw->colorBtn->setStyleSheet(QString("background:%1").arg(tw->colorHex));
            } });
    }

    // Controls: add/remove trupp, save/load template, ok/cancel
    QHBoxLayout *ctl = new QHBoxLayout;
    QPushButton *add = new QPushButton("Trupp hinzufügen", this);
    QPushButton *rem = new QPushButton("Letzten entfernen", this);
    QPushButton *saveT = new QPushButton("Template speichern", this);
    QPushButton *loadT = new QPushButton("Template laden", this);
    ctl->addWidget(add);
    ctl->addWidget(rem);
    ctl->addWidget(saveT);
    ctl->addWidget(loadT);
    m_mainLayout->addLayout(ctl);

    QHBoxLayout *okLay = new QHBoxLayout;
    QPushButton *ok = new QPushButton("OK", this);
    QPushButton *cancel = new QPushButton("Abbrechen", this);
    okLay->addStretch();
    okLay->addWidget(ok);
    okLay->addWidget(cancel);
    m_mainLayout->addLayout(okLay);

    connect(add, &QPushButton::clicked, this, &LineupDialog::onAddTrupp);
    connect(rem, &QPushButton::clicked, this, &LineupDialog::onRemoveTrupp);
    connect(saveT, &QPushButton::clicked, this, &LineupDialog::onSaveTemplate);
    connect(loadT, &QPushButton::clicked, this, &LineupDialog::onLoadTemplate);
    connect(ok, &QPushButton::clicked, this, [this]()
            {
        QString reason;
        if (!validate(reason))
        {
            QMessageBox::warning(this, "Ungültige Aufstellung", reason);
            return;
        }
        accept(); });
    connect(cancel, &QPushButton::clicked, this, &LineupDialog::reject);
}

void LineupDialog::setPlayerList(const QStringList &players, const QMap<QString, QString> &playerToGroup)
{
    m_players = players;
    m_playerToGroup = playerToGroup;
    // populate all player choosers
    for (TruppWidget *tw : m_trupps)
    {
        for (QComboBox *cb : tw->playerChoosers)
        {
            cb->clear();
            cb->addItem("");
            cb->addItems(m_players);
        }
    }
}

QVector<LineupTrupp> LineupDialog::getLineup() const
{
    QVector<LineupTrupp> out;
    for (const TruppWidget *tw : m_trupps)
    {
        LineupTrupp lt;
        lt.name = tw->name->text();
        lt.color = tw->colorHex.isEmpty() ? QColor() : QColor(tw->colorHex);
        for (const QComboBox *cb : tw->playerChoosers)
        {
            QString p = cb->currentText().trimmed();
            if (!p.isEmpty())
                lt.players.append(p);
        }
        out.append(lt);
    }
    return out;
}

bool LineupDialog::validate(QString &outReason) const
{
    // commander required
    if (m_commander->text().trimmed().isEmpty())
    {
        outReason = "Kommandant muss angegeben werden.";
        return false;
    }
    // unique selection
    QSet<QString> chosen;
    for (const TruppWidget *tw : m_trupps)
    {
        int cnt = 0;
        for (const QComboBox *cb : tw->playerChoosers)
        {
            QString p = cb->currentText().trimmed();
            if (p.isEmpty())
                continue;
            cnt++;
            if (chosen.contains(p))
            {
                outReason = QString("Spieler '%1' ist mehrfach ausgewählt.").arg(p);
                return false;
            }
            chosen.insert(p);
            if (cnt > m_maxPerTrupp)
            {
                outReason = QString("Trupp '%1' hat mehr als %2 Spieler.").arg(tw->name->text()).arg(m_maxPerTrupp);
                return false;
            }
        }
    }
    outReason.clear();
    return true;
}

void LineupDialog::onAddTrupp()
{
    QWidget *parent = this;
    TruppWidget *tw = new TruppWidget;
    tw->box = new QGroupBox("Neuer Trupp", parent);
    tw->name = new QLineEdit("Neuer Trupp", tw->box);
    tw->colorBtn = new QPushButton("Farbe", tw->box);
    QHBoxLayout *bLay = new QHBoxLayout(tw->box);
    bLay->addWidget(new QLabel("Name:"));
    bLay->addWidget(tw->name);
    bLay->addWidget(tw->colorBtn);
    for (int i = 0; i < m_maxPerTrupp; ++i)
    {
        QComboBox *cb = new QComboBox(tw->box);
        cb->addItem("");
        cb->addItems(m_players);
        tw->playerChoosers.append(cb);
        bLay->addWidget(cb);
    }
    // append to layout (scroll area's content is the second widget of main layout)
    QScrollArea *sa = findChild<QScrollArea *>();
    if (sa && sa->widget())
    {
        QWidget *cont = sa->widget();
        QVBoxLayout *vl = qobject_cast<QVBoxLayout *>(cont->layout());
        if (vl)
            vl->addWidget(tw->box);
    }
    m_trupps.append(tw);
    connect(tw->colorBtn, &QPushButton::clicked, this, [this, tw]()
            {
        QColor c = QColorDialog::getColor(QColor(tw->colorHex), this, "Trupp-Farbe wählen");
        if (c.isValid())
        {
            tw->colorHex = colorToHex(c);
            tw->colorBtn->setStyleSheet(QString("background:%1").arg(tw->colorHex));
        } });
    emit lineupChanged();
}

void LineupDialog::onRemoveTrupp()
{
    if (m_trupps.isEmpty())
        return;
    TruppWidget *tw = m_trupps.takeLast();
    if (tw->box)
        delete tw->box;
    delete tw;
    emit lineupChanged();
}

QString LineupDialog::templatesDir() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty())
        dir = QDir::homePath();
    QDir d(dir);
    d.mkpath("lineup_templates");
    return d.filePath("lineup_templates");
}

bool LineupDialog::saveTemplate(const QString &name, QString &outError) const
{
    if (name.trimmed().isEmpty())
    {
        outError = "Template-Name darf nicht leer sein.";
        return false;
    }
    QJsonObject root;
    root.insert("commander", m_commander->text());
    QJsonArray trA;
    for (const TruppWidget *tw : m_trupps)
    {
        QJsonObject t;
        t.insert("name", tw->name->text());
        t.insert("color", tw->colorHex);
        QJsonArray players;
        for (const QComboBox *cb : tw->playerChoosers)
        {
            QString p = cb->currentText().trimmed();
            if (!p.isEmpty())
                players.append(p);
        }
        t.insert("players", players);
        trA.append(t);
    }
    root.insert("trupps", trA);
    QJsonDocument doc(root);
    QString path = QDir(templatesDir()).filePath(name + ".json");
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
    {
        outError = QString("Template konnte nicht geschrieben werden: %1").arg(path);
        return false;
    }
    f.write(doc.toJson(QJsonDocument::Indented));
    f.close();
    return true;
}

bool LineupDialog::loadTemplate(const QString &name, QString &outError)
{
    QString path = QDir(templatesDir()).filePath(name + ".json");
    QFile f(path);
    if (!f.exists())
    {
        outError = "Template nicht gefunden.";
        return false;
    }
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        outError = "Template konnte nicht geöffnet werden.";
        return false;
    }
    QByteArray in = f.readAll();
    f.close();
    QJsonDocument doc = QJsonDocument::fromJson(in);
    if (!doc.isObject())
    {
        outError = "Ungültiges Template-Format.";
        return false;
    }
    QJsonObject root = doc.object();
    m_commander->setText(root.value("commander").toString());
    QJsonArray trA = root.value("trupps").toArray();
    // remove existing trupps
    for (TruppWidget *tw : m_trupps)
        if (tw->box)
            delete tw->box;
    qDeleteAll(m_trupps);
    m_trupps.clear();
    // Build from template
    QScrollArea *sa = findChild<QScrollArea *>();
    QWidget *cont = sa ? sa->widget() : nullptr;
    QVBoxLayout *vl = cont ? qobject_cast<QVBoxLayout *>(cont->layout()) : nullptr;
    if (!vl)
    {
        outError = "Interner Fehler beim Laden des Templates.";
        return false;
    }
    for (const QJsonValue &v : trA)
    {
        if (!v.isObject())
            continue;
        QJsonObject to = v.toObject();
        TruppWidget *tw = new TruppWidget;
        tw->box = new QGroupBox(to.value("name").toString(), cont);
        tw->name = new QLineEdit(to.value("name").toString(), tw->box);
        tw->colorBtn = new QPushButton("Farbe", tw->box);
        tw->colorHex = to.value("color").toString();
        if (!tw->colorHex.isEmpty())
            tw->colorBtn->setStyleSheet(QString("background:%1").arg(tw->colorHex));
        QHBoxLayout *bLay = new QHBoxLayout(tw->box);
        bLay->addWidget(new QLabel("Name:"));
        bLay->addWidget(tw->name);
        bLay->addWidget(tw->colorBtn);
        QJsonArray pa = to.value("players").toArray();
        for (int i = 0; i < m_maxPerTrupp; ++i)
        {
            QComboBox *cb = new QComboBox(tw->box);
            cb->addItem("");
            cb->addItems(m_players);
            if (i < pa.size())
            {
                QString p = pa.at(i).toString();
                int idx = cb->findText(p);
                if (idx >= 0)
                    cb->setCurrentIndex(idx);
            }
            tw->playerChoosers.append(cb);
            bLay->addWidget(cb);
        }
        vl->addWidget(tw->box);
        connect(tw->colorBtn, &QPushButton::clicked, this, [this, tw]()
                {
            QColor c = QColorDialog::getColor(QColor(tw->colorHex), this, "Trupp-Farbe wählen");
            if (c.isValid())
            {
                tw->colorHex = colorToHex(c);
                tw->colorBtn->setStyleSheet(QString("background:%1").arg(tw->colorHex));
            } });
        m_trupps.append(tw);
    }
    return true;
}

QStringList LineupDialog::availableTemplates() const
{
    QString dir = templatesDir();
    QDir d(dir);
    QStringList list = d.entryList(QStringList() << "*.json", QDir::Files);
    for (QString &s : list)
        s = s.left(s.length() - 5); // strip .json
    return list;
}

void LineupDialog::onSaveTemplate()
{
    bool ok = false;
    QString name = QInputDialog::getText(this, "Template speichern", "Name:", QLineEdit::Normal, QString(), &ok);
    if (!ok || name.isEmpty())
        return;
    QString err;
    if (!saveTemplate(name, err))
    {
        QMessageBox::warning(this, "Speichern fehlgeschlagen", err);
        return;
    }
    QMessageBox::information(this, "Gespeichert", QString("Template '%1' gespeichert.").arg(name));
}

void LineupDialog::onLoadTemplate()
{
    QStringList templ = availableTemplates();
    if (templ.isEmpty())
    {
        QMessageBox::information(this, "Keine Templates", "Keine Templates vorhanden.");
        return;
    }
    bool ok = false;
    QString sel = QInputDialog::getItem(this, "Template laden", "Template:", templ, 0, false, &ok);
    if (!ok || sel.isEmpty())
        return;
    QString err;
    if (!loadTemplate(sel, err))
    {
        QMessageBox::warning(this, "Laden fehlgeschlagen", err);
        return;
    }
}
