//
// Created by Kolby on 6/19/2019.
//
#pragma once

#include <QLineEdit>
#include <QWidget>

namespace Ui {
    class StartOptionsRestore;
}

/** Dialog to ask for passphrases. Used for encryption only
 */
class StartOptionsRestore : public QWidget {
    Q_OBJECT

public:
    explicit StartOptionsRestore(int rows, QWidget *parent = nullptr);
    ~StartOptionsRestore();
    std::vector<std::string> getOrderedStrings();


private:
    Ui::StartOptionsRestore *ui;
    std::list<QLineEdit*> editList;

};
