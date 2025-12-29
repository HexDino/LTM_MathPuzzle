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
    void onRoundEnded(int currentRound, int totalRounds, const QString &message);
    void onWaitingForContinue();
    void onBackToRoomClicked();
    void onContinueClicked();

private:
    NetworkManager *networkManager;
    
    QLabel *resultLabel;
    QLabel *reasonLabel;
    QLabel *solutionTitleLabel;
    QLabel *solutionLabel;
    QLabel *messageLabel;
    QPushButton *backButton;
    QPushButton *continueButton;  // Button for continuing to next round
    
    bool isRoundEndScreen;  // Track if showing round end or game end
    
    void setupUI();
    void displayResult(bool won, const QString &message);
    void displayRoundEnd(int currentRound, int totalRounds, const QString &message);
};

#endif // RESULTSCREEN_H


