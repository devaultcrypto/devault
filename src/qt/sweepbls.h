//
//
#pragma once

#include <QDialog>

namespace Ui {
    class SweepBLS;
}

/** Dialog to grap Private Key */
class SweepBLS : public QDialog {
    Q_OBJECT

public:
    explicit SweepBLS(QWidget *parent = nullptr);
    ~SweepBLS(); 
    void accept() override;
    void reject() override;
    std::string getSecretAddress() { return secret; }

private:
    Ui::SweepBLS *ui;
    std::string secret;

private Q_SLOTS:
    void on_okButton_accepted();

};
