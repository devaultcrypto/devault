//
// Created by Kolby on 6/19/2019.
//


#include <startoptionssort.h>
#include <ui_startoptionssort.h>
#include <QtWidgets>
#include <QTableWidget>
#include <QListView>
#include <QStringListModel>
#include <QLineEdit>
#include <QListWidget>
#include <dvtui.h>

CustomRectItem::CustomRectItem(QGraphicsItem *parent):
        QGraphicsRectItem(parent)
{
    setAcceptDrops(true);
    setFlags(QGraphicsItem::ItemIsSelectable);
}

void CustomRectItem::setText(const QString &text){
    this->m_text = text;
    update();
}

void CustomRectItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                           QWidget *widget){
    QGraphicsRectItem::paint(painter, option, widget);
    painter->drawText(boundingRect(),m_text,
                      QTextOption(Qt::AlignCenter));
}

void CustomRectItem::dropEvent(QGraphicsSceneDragDropEvent *event){
    if (event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist")){
        QByteArray itemData = event->mimeData()->data(
                "application/x-qabstractitemmodeldatalist");
        QDataStream stream(&itemData, QIODevice::ReadOnly);
        int row, col;
        QMap<int, QVariant> valueMap;
        stream >> row >> col >> valueMap;
        if(!valueMap.isEmpty())
            setText(valueMap.value(0).toString());
        event->accept();
    }
    else
        event->ignore();
}



static QLabel *createDragLabel(const QString &text, QWidget *parent)
{
    QLabel *label = new QLabel(text, parent);
    label->setStyleSheet("QLabel{background-color:#e3e3e3;color : black;}");
    return label;
}

static QString hotSpotMimeDataKey() { return QStringLiteral("application/x-hotspot"); }

StartOptionsSort::StartOptionsSort(std::vector<std::string> Words, int rows, QWidget *parent)
        : QWidget(parent), ui(new Ui::StartOptionsSort) {
    ui->setupUi(this);
    if(DVTUI::customThemeIsSet()) {
        QString appstyle = "fusion";
        QApplication::setStyle(appstyle);
        setStyleSheet(DVTUI::styleSheetString);
    } 

    scene = new QGraphicsScene(this);
    if(rows == 4) {
        scene->setSceneRect(0,0,500,189);
        view = new QGraphicsView;
        view->setScene(scene);
        view->setMinimumSize(QSize(1,190));
    } else {
        scene->setSceneRect(0,0,500,94);
        view = new QGraphicsView;
        view->setScene(scene);
        view->setMinimumSize(QSize(1,95));
    }
    QRectF rect(0,0,70,40);
    for(int i=0; i<4; i++){
        for(int k=0; k<6; k++){
            int ki;
            if (k < 1){
                ki = 20;
            } else {
                ki = 20 + 80 * k;
            }
            int ii;
            if (i < 1){
                ii = 1;
            } else {
                ii = 1 + 50 * i;
            }
            CustomRectItem *listView = new CustomRectItem;
            scene->addItem(listView);
            listView->setRect(rect);
            listView->setPos(ki,ii);
            listView->setBrush(QColor("#e3e3e3"));
            graphicsList.push_back(listView);
        }
    }
    ui->gridLayoutRevealed->addWidget(view, 0, 0, 1, 6, Qt::AlignCenter);

        for(int k=0; k<6; k++){

            QListWidget* itemListWidget = new QListWidget;
            QStringList itemList;
            if(rows == 4){
                if(k<0){
                    itemList.append(QString::fromStdString(Words[k]));
                    itemList.append(QString::fromStdString(Words[k + 1]));
                    itemList.append(QString::fromStdString(Words[k + 2]));
                    itemList.append(QString::fromStdString(Words[k + 3]));
                } else if(k<6) {
                    itemList.append(QString::fromStdString(Words[k * 4]));
                    itemList.append(QString::fromStdString(Words[k * 4 + 3]));
                    itemList.append(QString::fromStdString(Words[k * 4 + 1]));
                    itemList.append(QString::fromStdString(Words[k * 4 + 2]));
                }
            } else {
                if(k<0){
                    itemList.append(QString::fromStdString(Words[k]));
                    itemList.append(QString::fromStdString(Words[k + 1]));
                } else if(k<6) {
                    itemList.append(QString::fromStdString(Words[k * 2 + 1]));
                    itemList.append(QString::fromStdString(Words[k * 2]));
                }
            }

            itemListWidget->addItems(itemList);
            itemListWidget->setFixedWidth(80);
            if(rows == 4){
                itemListWidget->setMinimumSize(QSize(80,80));
                itemListWidget->setMaximumSize(QSize(80,80));
            } else {
                itemListWidget->setMaximumSize(QSize(80,40));
            }
            itemListWidget->setDragEnabled(true);
            labelsList.push_back(itemListWidget);
            ui->gridLayoutRevealed->addWidget(itemListWidget, 2, k, Qt::AlignCenter);
        }


    setAcceptDrops(true);
    setWindowTitle(tr("Draggable Text"));

}

std::list<QString> StartOptionsSort::getOrderedStrings(){
    std::list<QString> list;
    foreach(QGraphicsItem *item, scene->items())
    if(CustomRectItem *rItem = qgraphicsitem_cast<CustomRectItem*> (item)) {
        list.push_back(rItem->text());
    }
    return list;
}

StartOptionsSort::~StartOptionsSort() {
    delete ui;
}

