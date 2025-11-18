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
    connect(networkManager, &NetworkManager::playerLeft, 
            this, &RoomScreen::onPlayerLeft);
    connect(networkManager, &NetworkManager::playerDisconnected, 
            this, &RoomScreen::onPlayerDisconnected);
    connect(networkManager, &NetworkManager::playerReconnected, 
            this, &RoomScreen::onPlayerReconnected);
    connect(networkManager, &NetworkManager::roomStatusUpdated, 
            this, &RoomScreen::onRoomStatusUpdated);
    // Note: errorReceived is handled by LoginScreen only to avoid duplicate message boxes
}

void RoomScreen::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Top bar with Leave Room button (top-right corner)
    QHBoxLayout *topBarLayout = new QHBoxLayout();
    topBarLayout->addStretch();
    
    leaveRoomButton = new QPushButton("Leave Room", this);
    leaveRoomButton->setMaximumWidth(120);
    leaveRoomButton->setMaximumHeight(35);
    QFont leaveButtonFont = leaveRoomButton->font();
    leaveButtonFont.setPointSize(10);
    leaveRoomButton->setFont(leaveButtonFont);
    leaveRoomButton->setStyleSheet("QPushButton { background-color: #f44336; color: white; border-radius: 4px; padding: 5px 10px; }");
    topBarLayout->addWidget(leaveRoomButton);
    
    mainLayout->addLayout(topBarLayout);
    mainLayout->addSpacing(10);
    
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
    playerGroup->setStyleSheet("QGroupBox { font-size: 12pt; font-weight: bold; }");
    QVBoxLayout *playerLayout = new QVBoxLayout(playerGroup);
    
    playerTable = new QTableWidget(4, 4, this);
    playerTable->setHorizontalHeaderLabels({"Slot", "Username", "Status", "Ping"});
    playerTable->horizontalHeader()->setStretchLastSection(true);
    playerTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    playerTable->setSelectionMode(QAbstractItemView::NoSelection);
    
    // Initialize empty slots
    for (int i = 0; i < 4; i++) {
        playerTable->setItem(i, 0, new QTableWidgetItem(QString("P%1").arg(i + 1)));
        playerTable->setItem(i, 1, new QTableWidgetItem("Waiting..."));
        playerTable->setItem(i, 2, new QTableWidgetItem(""));
        playerTable->setItem(i, 3, new QTableWidgetItem(""));
        
        // Gray out empty slots
        for (int j = 0; j < 4; j++) {
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
    
    // Button layout (Ready and Start Game)
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    // Ready button
    readyButton = new QPushButton("Ready", this);
    readyButton->setMinimumHeight(50);
    
    // Start Game button (only for host)
    startGameButton = new QPushButton("Start Game", this);
    startGameButton->setMinimumHeight(50);
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
    connect(leaveRoomButton, &QPushButton::clicked, this, &RoomScreen::onLeaveRoomClicked);
}

void RoomScreen::onReadyClicked()
{
    networkManager->sendReady();
}

void RoomScreen::onStartGameClicked()
{
    networkManager->sendStartGame();
}

void RoomScreen::onLeaveRoomClicked()
{
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Leave Room",
        "Are you sure you want to leave this room?",
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        networkManager->sendLeaveRoom();
    }
}

void RoomScreen::onPlayerJoined(int /* index */, const QString &username)
{
    statusLabel->setText(username + " joined the room!");
}

void RoomScreen::onPlayerLeft(const QString &username)
{
    statusLabel->setText(username + " left the room!");
    QMessageBox::information(this, "Player Left", username + " has disconnected from the room.");
}

void RoomScreen::onPlayerDisconnected(const QString &username)
{
    statusLabel->setText(username + " lost connection! Waiting for reconnect...");
    
    // Show dialog with options
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Player Disconnected");
    msgBox.setText(QString("%1 has lost connection!").arg(username));
    msgBox.setInformativeText("The player has 60 seconds to reconnect.\n\nWhat would you like to do?");
    msgBox.setIcon(QMessageBox::Warning);
    
    QPushButton *waitButton = msgBox.addButton("Wait for Reconnect", QMessageBox::AcceptRole);
    QPushButton *leaveButton = msgBox.addButton("Leave to Lobby", QMessageBox::RejectRole);
    
    msgBox.setDefaultButton(waitButton);
    msgBox.exec();
    
    if (msgBox.clickedButton() == leaveButton) {
        // User chose to leave
        networkManager->sendLeaveRoom();
    }
    // Otherwise, just wait (do nothing)
}

void RoomScreen::onPlayerReconnected(const QString &username)
{
    statusLabel->setText(username + " reconnected successfully!");
    QMessageBox::information(this, "Player Reconnected", 
                            QString("%1 has reconnected to the room!").arg(username));
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
        playerTable->item(i, 3)->setText("");
        for (int j = 0; j < 4; j++) {
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
            
            // Display ping
            QString pingText = player.ping >= 0 ? QString("%1 ms").arg(player.ping) : "-";
            playerTable->item(player.index, 3)->setText(pingText);
            
            // Color code ping
            QColor pingColor = Qt::black;
            if (player.ping >= 0) {
                if (player.ping < 50) {
                    pingColor = Qt::darkGreen;  // Good
                } else if (player.ping < 100) {
                    pingColor = QColor(200, 120, 0);  // Okay (orange)
                } else {
                    pingColor = Qt::red;  // Bad
                }
            }
            playerTable->item(player.index, 3)->setForeground(pingColor);
            
            // Set colors for other columns
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


