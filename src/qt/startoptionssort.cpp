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
        QString origText = m_text;
        if(!valueMap.isEmpty())
            setText(valueMap.value(0).toString());

        event->setDropAction(Qt::MoveAction);
        event->accept();

        if(!origText.isEmpty()){
            if(QListWidget* lWidget = qobject_cast<QListWidget*> (event->source()))
                lWidget->addItem(origText);
        }
    }
    else
        event->ignore();
}

StartOptionsSort::StartOptionsSort(std::vector<std::string> Words, int rows, QWidget *parent)
        : QWidget(parent), ui(new Ui::StartOptionsSort) {
    ui->setupUi(this);
    if(DVTUI::customThemeIsSet()) {
        QString appstyle = "fusion";
        QApplication::setStyle(appstyle);
        setStyleSheet(DVTUI::styleSheetString);
    } 

    ui->dragdropLabel->setText(
        tr("Please <b>drag</b> and <b>drop</b> your seed words into the correct order to confirm your recovery phrase. "));

    scene = new QGraphicsScene(this);
    if(rows == 4) {
        scene->setSceneRect(0,0,620,229);
        view = new QGraphicsView;
        view->setScene(scene);
        view->setMinimumSize(QSize(1,230));
    } else {
        scene->setSceneRect(0,0,620,154);
        view = new QGraphicsView;
        view->setScene(scene);
        view->setMinimumSize(QSize(1,155));
    }
    
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QRectF rect(0,0,90,70);
    QRectF rect2(0,0,90,2);
    
    for(int i=0; i<rows; i++){
        for(int k=0; k<6; k++){
            int ki;
            if (k < 1){
                ki = 10;
            } else {
                ki = 10 + 100 * k;
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
            
            CustomRectItem *listViewBorder = new CustomRectItem;
            scene->addItem(listViewBorder);
            listViewBorder->setRect(rect2);
            listViewBorder->setBrush(QColor(35,136,237));
            listViewBorder->setPos(ki,ii + 50);
            listViewBorder->setPen(Qt::NoPen);

            listView->setPos(ki,ii);
            QPen myPen(QColor(255,255,255), 2, Qt::MPenStyle);
            listView->setPen(myPen);
            
            graphicsList.push_back(listView);
            graphicsList.push_back(listViewBorder);
            
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
            itemListWidget->setFixedWidth(120);
            itemListWidget->setMinimumSize(QSize(120,180));
            itemListWidget->setMaximumSize(QSize(120,180));
            itemListWidget->setDragEnabled(true);
            itemListWidget->setFocusPolicy(Qt::NoFocus);
            itemListWidget->setStyleSheet("QWidget{font-size:18px; font-style:bold; color:#fff;}");
            itemListWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
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

