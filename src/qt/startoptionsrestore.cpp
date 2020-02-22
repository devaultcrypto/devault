//
// Created by Kolby on 6/19/2019.
//

#include <QLabel>
#include <QLineEdit>
#include <dvtui.h>
#include <startoptionsrestore.h>
#include <ui_startoptionsrestore.h>

QString editLineCorrectCss = "QLineEdit{border-bottom:1px solid #00aeff;}";
QString editLineInvalidCss = "QLineEdit{border-bottom:1px solid red;}";

StartOptionsRestore::StartOptionsRestore(QStringList _wordList, int rows, bool _bls,
                                         QWidget *parent)
    : QWidget(parent), ui(new Ui::StartOptionsRestore), wordList(_wordList), bls(_bls) {
    ui->setupUi(this);
    if (DVTUI::customThemeIsSet()) {
        QString appstyle = "fusion";
        QApplication::setStyle(appstyle);
        setStyleSheet(DVTUI::styleSheetString);
    }

    for (int i = 0; i < rows; i++) {
        for (int k = 0; k < 6; k++) {

            QLineEdit *label = new QLineEdit(this);
            label->setStyleSheet(
                "QLabel{background-color: " + DVTUI::s_highlight_light_midgrey +
                "; border: 1px solid " + DVTUI::s_Dark +
                "; font-size: 18px; font-weight: thin; color: " +
                DVTUI::s_LBlue + ";}");
            label->setMinimumSize(80, 36);
            label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            label->setContentsMargins(8, 12, 8, 12);
            label->setAlignment(Qt::AlignCenter);
            editList.push_back(label);
            ui->restoreLabel->setText(
                tr("Restore your wallet using a recovery seed phrase below. "
                   ""));
            connect(label, SIGNAL(textChanged(const QString &)), this,
                    SLOT(textChanged(const QString &)));
            ui->gridLayoutRevealed->addWidget(label, i, k, Qt::AlignCenter);
        }
    }
}

void StartOptionsRestore::textChanged(const QString &text) {

    QObject *senderObj = sender();
    QLineEdit *label = static_cast<QLineEdit *>(senderObj);
    if (wordList.contains(text)) {
        label->setStyleSheet(editLineCorrectCss);
    } else {
        label->setStyleSheet(editLineInvalidCss);
    }
}

std::vector<std::string> StartOptionsRestore::getOrderedStrings() {
    std::vector<std::string> list;
    for (QLineEdit *label : editList) {
        list.push_back(label->text().toStdString());
    }
    return list;
}

StartOptionsRestore::~StartOptionsRestore() {
    delete ui;
}
