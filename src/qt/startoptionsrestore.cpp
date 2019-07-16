//
// Created by Kolby on 6/19/2019.
//


#include <startoptionsrestore.h>
#include <ui_startoptionsrestore.h>
#include <QLineEdit>
#include <dvtui.h>


StartOptionsRestore::StartOptionsRestore(int rows, QWidget *parent)
        : QWidget(parent), ui(new Ui::StartOptionsRestore)
        {
    ui->setupUi(this);
    if(DVTUI::customThemeIsSet()) {
        QString appstyle = "fusion";
        QApplication::setStyle(appstyle);
        setStyleSheet(DVTUI::styleSheetString);
    } 

            for(int i=0; i<rows; i++){
                for(int k=0; k<6; k++){

                    QLineEdit* label = new QLineEdit(this);
                    label->setStyleSheet("QLabel{background-color:#e3e3e3;padding-left:8px;padding-right:8px;padding-top:2px;padding-bottom:2px;border-radius:4px;}");
                    label->setMinimumSize(80,36);
                    label->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
                    label->setContentsMargins(8,12,8,12);
                    label->setAlignment(Qt::AlignCenter);
                    editList.push_back(label);
                    ui->gridLayoutRevealed->addWidget(label, i, k, Qt::AlignCenter);
                }
            }


}

std::vector<std::string> StartOptionsRestore::getOrderedStrings(){
    std::vector<std::string> list;
    for(QLineEdit* label : editList){
        list.push_back(label->text().toStdString());
    }
    return list;
}

StartOptionsRestore::~StartOptionsRestore() {
    delete ui;
}

