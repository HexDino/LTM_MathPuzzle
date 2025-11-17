#ifndef RESULTSCREEN_H
#define RESULTSCREEN_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include "networkmanager.h"

class ResultScreen : public QWidget
{
    Q_OBJECT

public:
    explicit ResultScreen(NetworkManager *network, QWidget *parent = nullptr);

signals:
    void backToRoomRequested();

private slots:
    void onGameEnded(bool won, const QString &message);
    void onBackToRoomClicked();

private:
    NetworkManager *networkManager;
    
    QLabel *resultLabel;
    QLabel *messageLabel;
    QPushButton *backButton;
    
    void setupUI();
    void displayResult(bool won, const QString &message);
};

#endif // RESULTSCREEN_H


