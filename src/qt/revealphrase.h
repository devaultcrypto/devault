//
//
#pragma once

#include <QDialog>
#include <support/allocators/secure.h>

namespace Ui {
    class RevealPhrase;
}

/** Dialog to show 12/24 secret word phrase only */
class RevealPhrase : public QDialog {
    Q_OBJECT

public:
    explicit RevealPhrase(const SecureVector& Words, QWidget *parent = nullptr);
    ~RevealPhrase();

private:
    Ui::RevealPhrase *ui;

};
