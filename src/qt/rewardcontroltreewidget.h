// Copyright (c) 2011-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QKeyEvent>
#include <QTreeWidget>

class RewardControlTreeWidget : public QTreeWidget {
    Q_OBJECT

public:
    explicit RewardControlTreeWidget(QWidget *parent = nullptr);

protected:
    virtual void keyPressEvent(QKeyEvent *event) override;
};
