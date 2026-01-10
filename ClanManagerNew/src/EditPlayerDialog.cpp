#include "EditPlayerDialog.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QDateEdit>
#include <QComboBox>
#include <QTextEdit>
#include <QDialogButtonBox>

EditPlayerDialog::EditPlayerDialog(QWidget *parent) : QDialog(parent)
{
    m_name = new QLineEdit(this);
    m_level = new QSpinBox(this);
    m_level->setRange(0, 100);
    m_group = new QLineEdit(this);
    m_joinDate = new QDateEdit(QDate::currentDate(), this);
    m_joinDate->setCalendarPopup(true);
    m_rank = new QLineEdit(this);
    m_comment = new QTextEdit(this);

    QFormLayout *form = new QFormLayout;
    form->addRow(tr("Name:"), m_name);
    form->addRow(tr("Level:"), m_level);
    form->addRow(tr("Gruppe:"), m_group);
    form->addRow(tr("Beitrittsdatum:"), m_joinDate);
    form->addRow(tr("Rang:"), m_rank);
    form->addRow(tr("Kommentar:"), m_comment);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);

    setWindowTitle(tr("Spieler bearbeiten"));
    setMinimumWidth(400);
}

void EditPlayerDialog::setPlayer(const Player &p)
{
    m_name->setText(p.name);
    m_level->setValue(p.level);
    m_group->setText(p.group);
    if (p.joinDate.isValid())
        m_joinDate->setDate(p.joinDate);
    m_rank->setText(p.rank);
    // No comment field in Player currently, keep as free-form
}

Player EditPlayerDialog::player() const
{
    Player p;
    p.name = m_name->text().trimmed();
    p.level = m_level->value();
    p.group = m_group->text().trimmed();
    p.joinDate = m_joinDate->date();
    p.rank = m_rank->text().trimmed();
    return p;
}
