#include "lobbyscreen.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QGroupBox>

LobbyScreen::LobbyScreen(NetworkManager *network, QWidget *parent)
    : QWidget(parent)
    , networkManager(network)
{
    setupUI();
    
    // Connect signals
    connect(networkManager, &NetworkManager::roomListReceived, 
            this, &LobbyScreen::onRoomListReceived);
    connect(networkManager, &NetworkManager::roomCreated, 
            this, &LobbyScreen::onRoomCreated);
    // Note: errorReceived is handled by LoginScreen only to avoid duplicate message boxes
}

void LobbyScreen::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Welcome header
    welcomeLabel = new QLabel(this);
    QFont headerFont = welcomeLabel->font();
    headerFont.setPointSize(16);
    headerFont.setBold(true);
    welcomeLabel->setFont(headerFont);
    welcomeLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(welcomeLabel);
    
    mainLayout->addSpacing(15);
    
    // Create room group (MOVED TO TOP - more prominent)
    QGroupBox *createGroup = new QGroupBox("Create New Room", this);
    createGroup->setStyleSheet("QGroupBox { font-size: 12pt; font-weight: bold; }");
    
    QVBoxLayout *createMainLayout = new QVBoxLayout(createGroup);
    QHBoxLayout *createLayout = new QHBoxLayout();
    
    roomNameEdit = new QLineEdit(this);
    roomNameEdit->setPlaceholderText("Enter room name...");
    roomNameEdit->setMinimumHeight(35);
    
    createButton = new QPushButton("Create Room", this);
    createButton->setMinimumHeight(35);
    createButton->setMinimumWidth(120);
    
    createLayout->addWidget(roomNameEdit, 2);
    createLayout->addWidget(createButton, 1);
    createMainLayout->addLayout(createLayout);
    
    mainLayout->addWidget(createGroup);
    mainLayout->addSpacing(20);
    
    // Room list group
    QGroupBox *roomListGroup = new QGroupBox("Available Rooms", this);
    roomListGroup->setStyleSheet("QGroupBox { font-size: 12pt; font-weight: bold; }");
    QVBoxLayout *listLayout = new QVBoxLayout(roomListGroup);
    
    // Room table
    roomTable = new QTableWidget(0, 3, this);
    roomTable->setHorizontalHeaderLabels({"Room ID", "Room Name", "Players"});
    roomTable->horizontalHeader()->setStretchLastSection(true);
    roomTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    roomTable->setSelectionMode(QAbstractItemView::SingleSelection);
    roomTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    
    listLayout->addWidget(roomTable);
    
    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    refreshButton = new QPushButton("Refresh", this);
    joinButton = new QPushButton("Join Selected Room", this);
    buttonLayout->addWidget(refreshButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(joinButton);
    listLayout->addLayout(buttonLayout);
    
    mainLayout->addWidget(roomListGroup);
    
    // Connect signals
    connect(refreshButton, &QPushButton::clicked, this, &LobbyScreen::onRefreshClicked);
    connect(createButton, &QPushButton::clicked, this, &LobbyScreen::onCreateRoomClicked);
    connect(joinButton, &QPushButton::clicked, this, &LobbyScreen::onJoinRoomClicked);
    connect(roomNameEdit, &QLineEdit::returnPressed, this, &LobbyScreen::onCreateRoomClicked);
    connect(roomTable, &QTableWidget::cellDoubleClicked, this, &LobbyScreen::onRoomDoubleClicked);
}

void LobbyScreen::refresh()
{
    welcomeLabel->setText("Welcome, " + networkManager->getCurrentUsername() + "!");
    networkManager->sendListRooms();
}

void LobbyScreen::onRefreshClicked()
{
    networkManager->sendListRooms();
}

void LobbyScreen::onCreateRoomClicked()
{
    QString roomName = roomNameEdit->text().trimmed();
    
    if (roomName.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please enter a room name");
        return;
    }
    
    networkManager->sendCreateRoom(roomName);
    roomNameEdit->clear();
}

void LobbyScreen::onJoinRoomClicked()
{
    int selectedRow = roomTable->currentRow();
    
    if (selectedRow < 0) {
        QMessageBox::warning(this, "Error", "Please select a room to join");
        return;
    }
    
    int roomId = roomTable->item(selectedRow, 0)->text().toInt();
    networkManager->sendJoinRoom(roomId);
}

void LobbyScreen::onRoomDoubleClicked(int row, int /* column */)
{
    if (row >= 0 && row < roomTable->rowCount()) {
        int roomId = roomTable->item(row, 0)->text().toInt();
        networkManager->sendJoinRoom(roomId);
    }
}

void LobbyScreen::onRoomListReceived(const QVector<RoomInfo> &rooms)
{
    updateRoomList(rooms);
}

void LobbyScreen::onRoomCreated(int /* roomId */, const QString & /* roomName */)
{
    // Room created, will automatically transition to room screen
}

void LobbyScreen::onError(const QString &error)
{
    QMessageBox::warning(this, "Error", error);
}

void LobbyScreen::updateRoomList(const QVector<RoomInfo> &rooms)
{
    roomTable->setRowCount(0);
    
    for (const RoomInfo &room : rooms) {
        int row = roomTable->rowCount();
        roomTable->insertRow(row);
        
        roomTable->setItem(row, 0, new QTableWidgetItem(QString::number(room.id)));
        roomTable->setItem(row, 1, new QTableWidgetItem(room.name));
        roomTable->setItem(row, 2, new QTableWidgetItem(QString("%1/4").arg(room.playerCount)));
    }
}


