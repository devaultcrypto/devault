//
// Created by Kolby on 6/19/2019.
//


#include <startoptionsrevealed.h>
#include <ui_startoptionsrevealed.h>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <dvtui.h>


StartOptionsRevealed::StartOptionsRevealed(std::vector<std::string> Words, int rows, QWidget *parent)
        : QWidget(parent), ui(new Ui::StartOptionsRevealed)
        {
    ui->setupUi(this);
    if(DVTUI::customThemeIsSet()) {
        QString appstyle = "fusion";
        QApplication::setStyle(appstyle);
        setStyleSheet(DVTUI::styleSheetString);
    } 
    ui->seedLabel->setText(
        tr("The words below are your recovery phrase (mnemonic seed). Please write them down and/or securely save them. "));
            for(int i=0; i<rows; i++){
                for(int k=0; k<6; k++){

                    QLabel* label = new QLabel(this);
                    label->setStyleSheet("QLabel{background-color: " + DVTUI::s_highlight_light_midgrey + "; border: 1px solid " + DVTUI::s_Dark + "; font-size: 19px; font-weight: thin; color: " + DVTUI::s_white + ";}");
                    label->setMinimumSize(90,36);
                    label->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
                    label->setContentsMargins(8,12,8,12);
                    label->setAlignment(Qt::AlignCenter);
                    labelsList.push_back(label);
                    ui->gridLayoutRevealed->addWidget(label, i, k, Qt::AlignCenter);
                }
            }
            int i = 0;
            for (QLabel* label : labelsList) {
                label->setContentsMargins(8,12,8,12);
                label->setText(QString::fromStdString(Words[i]));
                i++;
            }


}

StartOptionsRevealed::~StartOptionsRevealed() {
    delete ui;
}

