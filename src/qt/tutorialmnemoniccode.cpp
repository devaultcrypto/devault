#include "tutorialmnemoniccode.h"
#include "ui_tutorialmnemoniccode.h"
#include <QLineEdit>
#include <QDebug>
#include "dvtui.h"

TutorialMnemonicCode::TutorialMnemonicCode(std::vector<std::string> vWords, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TutorialMnemonicCode)
{
    ui->setupUi(this);
    this->vWords = vWords;

    // right side

    for(int i=0; i<3; i++){
        for(int j=0; j<4; j++){
            QLabel* label = new QLabel(this);
            label->setAttribute(Qt::WA_MacShowFocusRect, 0);

            label->setStyleSheet("QLabel{background-color:#e3e3e3;padding-left:8px;padding-right:8px;padding-top:2px;padding-bottom:2px;border-radius:4px;}");
            label->setMinimumSize(80,36);
            label->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
            label->setContentsMargins(8,12,8,12);
            label->setAlignment(Qt::AlignCenter);
            labelsList.push_back(label);
            ui->gridLayoutMnemonic->addWidget(label, i, j, Qt::AlignCenter);
        }
    }
    
    int i = 0;
    for (QLabel* label : labelsList) {
        label->setStyleSheet("QLabel{background-color:#e3e3e3;padding-left:8px;padding-right:8px;padding-top:2px;padding-bottom:2px;border-radius:4px;}");
        label->setContentsMargins(8,12,8,12);
        label->setText(QString::fromStdString(vWords[i]));
        i++;
    }
}

TutorialMnemonicCode::~TutorialMnemonicCode()
{
    delete ui;
}
