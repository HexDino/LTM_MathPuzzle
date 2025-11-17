#include "resultscreen.h"
#include <QVBoxLayout>
#include <QPixmap>

ResultScreen::ResultScreen(NetworkManager *network, QWidget *parent)
    : QWidget(parent)
    , networkManager(network)
{
    setupUI();
    
    // Connect signal
    connect(networkManager, &NetworkManager::gameEnded, 
            this, &ResultScreen::onGameEnded);
}

void ResultScreen::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);
    
    // Result label (WIN/LOSE)
    resultLabel = new QLabel("Game Result", this);
    QFont resultFont = resultLabel->font();
    resultFont.setPointSize(36);
    resultFont.setBold(true);
    resultLabel->setFont(resultFont);
    resultLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(resultLabel);
    
    mainLayout->addSpacing(30);
    
    // Message label
    messageLabel = new QLabel("", this);
    messageLabel->setWordWrap(true);
    messageLabel->setAlignment(Qt::AlignCenter);
    QFont messageFont = messageLabel->font();
    messageFont.setPointSize(14);
    messageLabel->setFont(messageFont);
    messageLabel->setMinimumWidth(600);
    mainLayout->addWidget(messageLabel);
    
    mainLayout->addSpacing(50);
    
    // Back button
    backButton = new QPushButton("Back to Room", this);
    backButton->setMinimumHeight(50);
    backButton->setMinimumWidth(200);
    QFont buttonFont = backButton->font();
    buttonFont.setPointSize(14);
    backButton->setFont(buttonFont);
    mainLayout->addWidget(backButton, 0, Qt::AlignCenter);
    
    // Connect button
    connect(backButton, &QPushButton::clicked, this, &ResultScreen::onBackToRoomClicked);
}

void ResultScreen::onGameEnded(bool won, const QString &message)
{
    displayResult(won, message);
}

void ResultScreen::displayResult(bool won, const QString &message)
{
    if (won) {
        resultLabel->setText("🎉 VICTORY! 🎉");
        resultLabel->setStyleSheet("QLabel { color: green; }");
        messageLabel->setText("Congratulations! Your team solved the puzzle!\n\n" + message);
        messageLabel->setStyleSheet("QLabel { color: green; }");
    } else {
        resultLabel->setText("❌ GAME OVER ❌");
        resultLabel->setStyleSheet("QLabel { color: red; }");
        messageLabel->setText("Your team didn't solve the puzzle in time.\n\n" + message);
        messageLabel->setStyleSheet("QLabel { color: red; }");
    }
}

void ResultScreen::onBackToRoomClicked()
{
    // Emit signal to go back to room screen
    emit backToRoomRequested();
}


