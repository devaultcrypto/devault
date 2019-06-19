#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <startoptionsdialog.h>
#include <ui_startoptionsdialog.h>

#include <guiconstants.h>
#include <dvtui.h>

#include <support/allocators/secure.h>

#include <QKeyEvent>
#include <QMessageBox>
#include <QPushButton>

StartOptionsDialog::StartOptionsDialog(const QString error_words, QWidget* parent)
    : QDialog(parent), ui(new Ui::StartOptionsDialog) {
    ui->setupUi(this);
    if(DVTUI::customThemeIsSet()) {
        QString appstyle = "fusion";
        QApplication::setStyle(appstyle);
        setStyleSheet(DVTUI::styleSheetString);
    } 

    ui->ErrorLable->setText(error_words);
}

StartOptionsDialog::~StartOptionsDialog() {
    delete ui;
}

void StartOptionsDialog::accept() {
    close();
}
