//
// Created by Kolby on 6/19/2019.
//


#include <startoptionsrevealed.h>
#include <ui_startoptionsrevealed.h>
#include <QLineEdit>
#include <dvtui.h>


StartOptionsRevealed::StartOptionsRevealed(std::vector<std::string> Words, int rows, QWidget *parent)
        : QWidget(parent), ui(new Ui::StartOptionsRevealed)
        {
    ui->setupUi(this);

            for(int i=0; i<rows; i++){
                for(int k=0; k<6; k++){

                    QLabel* label = new QLabel(this);
                    label->setStyleSheet("QLabel{background-color:#e3e3e3;padding-left:8px;padding-right:8px;padding-top:2px;padding-bottom:2px;border-radius:4px;}");
                    label->setMinimumSize(80,36);
                    label->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
                    label->setContentsMargins(8,12,8,12);
                    label->setAlignment(Qt::AlignCenter);
                    labelsList.push_back(label);
                    ui->gridLayoutRevealed->addWidget(label, i, k, Qt::AlignCenter);
                }
            }
            int i = 0;
            for (QLabel* label : labelsList) {
                label->setStyleSheet("QLabel{background-color:#e3e3e3;padding-left:8px;padding-right:8px;padding-top:2px;padding-bottom:2px;border-radius:4px;color : black;}");
                label->setContentsMargins(8,12,8,12);
                label->setText(QString::fromStdString(Words[i]));
                i++;
            }


}

StartOptionsRevealed::~StartOptionsRevealed() {
    delete ui;
}

