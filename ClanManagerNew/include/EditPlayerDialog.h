#pragma once

#include <QDialog>
#include "Player.h"

class QLineEdit;
class QSpinBox;
class QDateEdit;
class QComboBox;
class QTextEdit;

class EditPlayerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit EditPlayerDialog(QWidget *parent = nullptr);
    void setPlayer(const Player &p);
    Player player() const;

private:
    QLineEdit *m_name = nullptr;
    QSpinBox *m_level = nullptr;
    QLineEdit *m_group = nullptr;
    QDateEdit *m_joinDate = nullptr;
    QLineEdit *m_rank = nullptr;
    QTextEdit *m_comment = nullptr;
};
