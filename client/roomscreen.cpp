#include "roomscreen.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>

RoomScreen::RoomScreen(NetworkManager *network, QWidget *parent)
    : QWidget(parent)
    , networkManager(network)
{
    setupUI();
    
    // Connect signals
    connect(networkManager, &NetworkManager::playerJoined, 
            this, &RoomScreen::onPlayerJoined);
    connect(networkManager, &NetworkManager::roomStatusUpdated, 
            this, &RoomScreen::onRoomStatusUpdated);
    connect(networkManager, &NetworkManager::errorReceived, 
            this, &RoomScreen::onError);
}

void RoomScreen::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Room title
    roomTitleLabel = new QLabel("Room", this);
    QFont titleFont = roomTitleLabel->font();
    titleFont.setPointSize(18);
    titleFont.setBold(true);
    roomTitleLabel->setFont(titleFont);
    roomTitleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(roomTitleLabel);
    
    // Instructions
    instructionLabel = new QLabel(
        "Wait for 4 players to join. Host can start the game when ready!", 
        this);
    instructionLabel->setWordWrap(true);
    instructionLabel->setAlignment(Qt::AlignCenter);
    QFont instructionFont = instructionLabel->font();
    instructionFont.setPointSize(11);
    instructionLabel->setFont(instructionFont);
    mainLayout->addWidget(instructionLabel);
    
    mainLayout->addSpacing(20);
    
    // Player list group
    QGroupBox *playerGroup = new QGroupBox("Players in Room", this);
    QVBoxLayout *playerLayout = new QVBoxLayout(playerGroup);
    
    playerTable = new QTableWidget(4, 3, this);
    playerTable->setHorizontalHeaderLabels({"Slot", "Username", "Status"});
    playerTable->horizontalHeader()->setStretchLastSection(true);
    playerTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    playerTable->setSelectionMode(QAbstractItemView::NoSelection);
    
    // Initialize empty slots
    for (int i = 0; i < 4; i++) {
        playerTable->setItem(i, 0, new QTableWidgetItem(QString("P%1").arg(i + 1)));
        playerTable->setItem(i, 1, new QTableWidgetItem("Waiting..."));
        playerTable->setItem(i, 2, new QTableWidgetItem(""));
        
        // Gray out empty slots
        for (int j = 0; j < 3; j++) {
            playerTable->item(i, j)->setForeground(Qt::gray);
        }
    }
    
    playerLayout->addWidget(playerTable);
    mainLayout->addWidget(playerGroup);
    
    mainLayout->addSpacing(20);
    
    // Status label
    statusLabel = new QLabel("Waiting for players to join...", this);
    statusLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(statusLabel);
    
    // Button layout
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    // Ready button
    readyButton = new QPushButton("Ready", this);
    readyButton->setMinimumHeight(50);
    QFont buttonFont = readyButton->font();
    buttonFont.setPointSize(14);
    buttonFont.setBold(true);
    readyButton->setFont(buttonFont);
    
    // Start Game button (only for host)
    startGameButton = new QPushButton("Start Game", this);
    startGameButton->setMinimumHeight(50);
    startGameButton->setFont(buttonFont);
    startGameButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; }");
    startGameButton->setVisible(false);  // Hidden by default
    
    buttonLayout->addStretch();
    buttonLayout->addWidget(readyButton);
    buttonLayout->addWidget(startGameButton);
    buttonLayout->addStretch();
    
    mainLayout->addLayout(buttonLayout);
    
    mainLayout->addStretch();
    
    // Connect buttons
    connect(readyButton, &QPushButton::clicked, this, &RoomScreen::onReadyClicked);
    connect(startGameButton, &QPushButton::clicked, this, &RoomScreen::onStartGameClicked);
}

void RoomScreen::onReadyClicked()
{
    networkManager->sendReady();
}

void RoomScreen::onStartGameClicked()
{
    networkManager->sendStartGame();
}

void RoomScreen::onPlayerJoined(int /* index */, const QString &username)
{
    statusLabel->setText(username + " joined the room!");
}

void RoomScreen::onRoomStatusUpdated(const QVector<PlayerInfo> &players)
{
    updatePlayerList(players);
}

void RoomScreen::onError(const QString &error)
{
    QMessageBox::warning(this, "Error", error);
}

void RoomScreen::updatePlayerList(const QVector<PlayerInfo> &players)
{
    // Reset all slots
    for (int i = 0; i < 4; i++) {
        playerTable->item(i, 1)->setText("Waiting...");
        playerTable->item(i, 2)->setText("");
        for (int j = 0; j < 3; j++) {
            playerTable->item(i, j)->setForeground(Qt::gray);
        }
    }
    
    // Update with actual players
    int readyCount = 0;
    for (const PlayerInfo &player : players) {
        if (player.index >= 0 && player.index < 4) {
            QString displayName = player.username;
            if (player.isHost) {
                displayName += " (Host)";
            }
            
            playerTable->item(player.index, 1)->setText(displayName);
            playerTable->item(player.index, 2)->setText(player.ready ? "✓ Ready" : "Not Ready");
            
            // Set colors
            QColor color = player.ready ? Qt::darkGreen : Qt::black;
            for (int j = 0; j < 3; j++) {
                playerTable->item(player.index, j)->setForeground(color);
            }
            
            if (player.ready) readyCount++;
        }
    }
    
    // Show/hide Start Game button based on whether current player is host
    bool isHost = networkManager->isCurrentPlayerHost();
    startGameButton->setVisible(isHost);
    
    // TODO: FOR TESTING - Enable Start Game button with any number of players
    // PRODUCTION: Change to: startGameButton->setEnabled(players.size() == 4);
    if (isHost) {
        startGameButton->setEnabled(players.size() >= 1);
    }
    
    // Update status
    // TODO: FOR TESTING - Allow starting with less than 4 players
    if (isHost) {
        statusLabel->setText(QString("You are the host. Click 'Start Game' to begin! (%1 player(s), %2 ready)").arg(players.size()).arg(readyCount));
    } else if (players.size() == 4) {
        if (readyCount == 4) {
            statusLabel->setText("All players ready! Starting game...");
        } else {
            statusLabel->setText(QString("Waiting for host to start game... (%1/4 ready)").arg(readyCount));
        }
    } else {
        statusLabel->setText(QString("Waiting for players... (%1/4)").arg(players.size()));
    }
}


