// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rewardcontroltreewidget.h>
#include <rewardcontroldialog.h>

RewardControlTreeWidget::RewardControlTreeWidget(QWidget *parent)
    : QTreeWidget(parent) {}

void RewardControlTreeWidget::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Space) // press spacebar -> select checkbox
    {
        event->ignore();
        if (this->currentItem()) {
            int COLUMN_CHECKBOX = 0;
            this->currentItem()->setCheckState(
                COLUMN_CHECKBOX, ((this->currentItem()->checkState(
                                       COLUMN_CHECKBOX) == Qt::Checked)
                                      ? Qt::Unchecked
                                      : Qt::Checked));
        }
    } else if (event->key() == Qt::Key_Escape) // press esc -> close dialog
    {
        event->ignore();
        RewardControlDialog *rewardControlDialog =
            static_cast<RewardControlDialog *>(this->parentWidget());
        rewardControlDialog->done(QDialog::Accepted);
    } else {
        this->QTreeWidget::keyPressEvent(event);
    }
}
