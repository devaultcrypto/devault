//
//
#pragma once

#include <QDialog>

namespace Ui {
    class Sweep;
}

/** Dialog to grap Private Key */
class Sweep : public QDialog {
    Q_OBJECT

public:
    explicit Sweep(QWidget *parent = nullptr);
    ~Sweep();
    void accept() override;
    void reject() override;
    std::string getSecretAddress() { return secret; }

private:
    Ui::Sweep *ui;
    std::string secret;

private Q_SLOTS:
    void on_okButton_accepted();

};
