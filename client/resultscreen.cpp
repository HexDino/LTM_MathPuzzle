#include "resultscreen.h"
#include <QVBoxLayout>
#include <QPixmap>
#include <QRegularExpression>

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
    
    mainLayout->addSpacing(20);
    
    // Reason label (Time's up / Wrong answer)
    reasonLabel = new QLabel("", this);
    reasonLabel->setWordWrap(true);
    reasonLabel->setAlignment(Qt::AlignCenter);
    QFont reasonFont = reasonLabel->font();
    reasonFont.setPointSize(16);
    reasonFont.setBold(true);
    reasonLabel->setFont(reasonFont);
    mainLayout->addWidget(reasonLabel);
    
    mainLayout->addSpacing(30);
    
    // Solution title
    solutionTitleLabel = new QLabel("Solution:", this);
    solutionTitleLabel->setAlignment(Qt::AlignCenter);
    QFont solutionTitleFont = solutionTitleLabel->font();
    solutionTitleFont.setPointSize(13);
    solutionTitleFont.setBold(true);
    solutionTitleLabel->setFont(solutionTitleFont);
    solutionTitleLabel->setStyleSheet("QLabel { color: #666; }");
    mainLayout->addWidget(solutionTitleLabel);
    
    mainLayout->addSpacing(10);
    
    // Solution label (equation)
    solutionLabel = new QLabel("", this);
    solutionLabel->setWordWrap(false);
    solutionLabel->setAlignment(Qt::AlignCenter);
    QFont solutionFont = solutionLabel->font();
    solutionFont.setPointSize(16);
    solutionFont.setBold(true);
    solutionLabel->setFont(solutionFont);
    solutionLabel->setStyleSheet("QLabel { color: #D9534F; }");
    mainLayout->addWidget(solutionLabel);
    
    mainLayout->addSpacing(20);
    
    // Message label (additional info)
    messageLabel = new QLabel("", this);
    messageLabel->setWordWrap(true);
    messageLabel->setAlignment(Qt::AlignCenter);
    QFont messageFont = messageLabel->font();
    messageFont.setPointSize(12);
    messageLabel->setFont(messageFont);
    messageLabel->setMinimumWidth(600);
    mainLayout->addWidget(messageLabel);
    
    mainLayout->addSpacing(30);
    
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
        reasonLabel->setText("Congratulations! Your team solved the puzzle!");
        reasonLabel->setStyleSheet("QLabel { color: green; }");
        solutionTitleLabel->hide();
        solutionLabel->hide();
        messageLabel->setText("");
    } else {
        resultLabel->setText("❌ GAME OVER ❌");
        resultLabel->setStyleSheet("QLabel { color: red; }");
        
        // Parse message: format is "reason|solution" or just "solution"
        QStringList parts = message.split('|');
        
        QString reason = "Your team didn't solve the puzzle.";
        QString solution = message;
        
        if (parts.size() >= 2) {
            reason = parts[0].trimmed();
            solution = parts[1].trimmed();
        }
        
        // Clean up: remove "Solution:" prefix if it exists in the message
        solution = solution.replace(QRegularExpression("^Solution:\\s*", QRegularExpression::CaseInsensitiveOption), "");
        reason = reason.replace(QRegularExpression("^Solution:\\s*", QRegularExpression::CaseInsensitiveOption), "");
        
        // Display reason
        reasonLabel->setText(reason);
        if (reason.contains("Time's up", Qt::CaseInsensitive)) {
            reasonLabel->setStyleSheet("QLabel { color: #FF6B6B; }");
            messageLabel->setText("The time ran out before your team could solve the puzzle.");
        } else if (reason.contains("Wrong answer", Qt::CaseInsensitive)) {
            reasonLabel->setStyleSheet("QLabel { color: #FFA500; }");
            messageLabel->setText("Your team selected the wrong cells. Better luck next time!");
        } else {
            reasonLabel->setStyleSheet("QLabel { color: red; }");
            messageLabel->setText("");
        }
        
        // Display solution on separate line
        solutionTitleLabel->show();
        solutionLabel->setText(solution);
        solutionLabel->show();
        
        messageLabel->setStyleSheet("QLabel { color: #555; }");
    }
}

void ResultScreen::onBackToRoomClicked()
{
    // Emit signal to go back to room screen
    emit backToRoomRequested();
}


