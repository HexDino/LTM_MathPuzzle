#include "networkmanager.h"
#include <QDebug>
#include <QRandomGenerator>

NetworkManager::NetworkManager(QObject *parent)
    : QObject(parent)
    , socket(new QTcpSocket(this))
    , currentRoomId(-1)
    , currentHostIndex(-1)
    , currentPlayerIndex(-1)
    , timeRemaining(0)
{
    // Initialize game data
    for (int i = 0; i < 4; i++) {
        gameData.matrixHidden[i] = false;
    }
    
    // Connect socket signals
    connect(socket, &QTcpSocket::connected, this, &NetworkManager::onConnected);
    connect(socket, &QTcpSocket::disconnected, this, &NetworkManager::onDisconnected);
    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    // Qt 5.15+ and Qt 6 use errorOccurred, older Qt 5 uses error
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(socket, &QAbstractSocket::errorOccurred, this, &NetworkManager::onError);
#else
    connect(socket, static_cast<void(QAbstractSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error),
            this, &NetworkManager::onError);
#endif
}

NetworkManager::~NetworkManager()
{
    disconnectFromServer();
}

void NetworkManager::connectToServer(const QString &host, quint16 port)
{
    qDebug() << "Connecting to" << host << ":" << port;
    socket->connectToHost(host, port);
}

void NetworkManager::disconnectFromServer()
{
    if (socket->state() == QAbstractSocket::ConnectedState) {
        socket->disconnectFromHost();
    }
}

bool NetworkManager::isConnected() const
{
    return socket->state() == QAbstractSocket::ConnectedState;
}

void NetworkManager::sendCommand(const QString &command)
{
    if (!isConnected()) {
        qWarning() << "Not connected, cannot send:" << command;
        return;
    }
    
    QString message = command;
    if (!message.endsWith('\n')) {
        message += '\n';
    }
    
    qDebug() << "→ " << command;
    socket->write(message.toUtf8());
    socket->flush();
}

void NetworkManager::sendRegister(const QString &username, const QString &password)
{
    sendCommand(QString("REGISTER|%1|%2").arg(username, password));
}

void NetworkManager::sendLogin(const QString &username, const QString &password)
{
    sendCommand(QString("LOGIN|%1|%2").arg(username, password));
}

void NetworkManager::sendListRooms()
{
    sendCommand("LIST_ROOMS");
}

void NetworkManager::sendCreateRoom(const QString &roomName)
{
    sendCommand(QString("CREATE_ROOM|%1").arg(roomName));
}

void NetworkManager::sendJoinRoom(int roomId)
{
    sendCommand(QString("JOIN_ROOM|%1").arg(roomId));
}

void NetworkManager::sendLeaveRoom()
{
    sendCommand("LEAVE_ROOM");
}

void NetworkManager::sendReady()
{
    sendCommand("READY");
}

void NetworkManager::sendStartGame()
{
    sendCommand("START_GAME");
}

void NetworkManager::sendChat(const QString &message)
{
    sendCommand(QString("CHAT|%1").arg(message));
}

void NetworkManager::sendSubmit(int row, int col)
{
    sendCommand(QString("SUBMIT|%1|%2").arg(row).arg(col));
}

void NetworkManager::sendPong()
{
    sendCommand("PONG");
}

void NetworkManager::onConnected()
{
    qDebug() << "Connected to server";
    receiveBuffer.clear();
    emit connected();
}

void NetworkManager::onDisconnected()
{
    qDebug() << "Disconnected from server";
    
    // Clear all state when disconnected
    currentUsername.clear();
    currentRoomId = -1;
    currentHostIndex = -1;
    currentPlayerIndex = -1;
    rooms.clear();
    players.clear();
    timeRemaining = 0;
    receiveBuffer.clear();
    
    // Clear game data
    for (int i = 0; i < 4; i++) {
        gameData.matrixHidden[i] = false;
        gameData.matrices[i].clear();
    }
    gameData.equation.clear();
    gameData.currentRound = 0;
    gameData.totalRounds = 0;
    
    emit disconnected();
}

void NetworkManager::onReadyRead()
{
    // Read all available data
    QByteArray data = socket->readAll();
    receiveBuffer += QString::fromUtf8(data);
    
    // Process complete messages (delimited by \n)
    while (receiveBuffer.contains('\n')) {
        int newlinePos = receiveBuffer.indexOf('\n');
        QString message = receiveBuffer.left(newlinePos).trimmed();
        receiveBuffer = receiveBuffer.mid(newlinePos + 1);
        
        if (!message.isEmpty()) {
            qDebug() << "← " << message;
            handleMessage(message);
        }
    }
}

void NetworkManager::onError(QAbstractSocket::SocketError socketError)
{
    QString errorMsg = socket->errorString();
    qWarning() << "Socket error:" << socketError << "-" << errorMsg;
    emit connectionError(errorMsg);
}

void NetworkManager::handleMessage(const QString &message)
{
    QStringList parts = message.split('|');
    if (parts.isEmpty()) return;
    
    QString command = parts[0];
    
    // Auto-respond to PING
    if (command == "PING") {
        sendPong();
        emit pingReceived();
        return;
    }
    
    // Handle other commands
    if (command == "WELCOME") {
        if (parts.size() > 1) {
            emit welcomeReceived(parts[1]);
        }
    }
    else if (command == "LOGIN_OK") {
        if (parts.size() > 1) {
            currentUsername = parts[1];
            emit loginSuccessful(currentUsername);
        }
    }
    else if (command == "RECONNECT_OK") {
        if (parts.size() > 1) {
            currentUsername = parts[1];
            emit reconnectSuccessful(currentUsername);
            // Server will follow up with GAME_START if game is in progress
            // or ROOM_STATUS if in room, or ROOM_LIST if in lobby
            // Don't auto-transition to lobby - wait for server data
        }
    }
    else if (command == "REGISTER_OK") {
        emit registerSuccessful();
    }
    else if (command == "ROOM_LIST") {
        parseRoomList(parts);
    }
    else if (command == "ROOM_CREATED") {
        if (parts.size() >= 3) {
            int roomId = parts[1].toInt();
            QString roomName = parts[2];
            currentRoomId = roomId;
            emit roomCreated(roomId, roomName);
        }
    }
    else if (command == "ROOM_JOINED") {
        if (parts.size() > 1) {
            currentRoomId = parts[1].toInt();
            emit roomJoined(currentRoomId);
        }
    }
    else if (command == "LEFT_ROOM") {
        currentRoomId = -1;
        currentPlayerIndex = -1;
        currentHostIndex = -1;
        players.clear();
        emit leftRoom();
    }
    else if (command == "PLAYER_JOINED") {
        if (parts.size() >= 3) {
            int index = parts[1].toInt();
            QString username = parts[2];
            emit playerJoined(index, username);
        }
    }
    else if (command == "PLAYER_LEFT") {
        if (parts.size() >= 2) {
            QString username = parts[1];
            emit playerLeft(username);
        }
    }
    else if (command == "PLAYER_DISCONNECTED") {
        if (parts.size() >= 2) {
            QString username = parts[1];
            emit playerDisconnected(username);
        }
    }
    else if (command == "PLAYER_RECONNECTED") {
        if (parts.size() >= 2) {
            QString username = parts[1];
            emit playerReconnected(username);
        }
    }
    else if (command == "ROOM_STATUS") {
        parseRoomStatus(parts);
    }
    else if (command == "GAME_START") {
        parseGameStart(parts);
    }
    else if (command == "TIMER") {
        if (parts.size() > 1) {
            timeRemaining = parts[1].toInt();
            emit timerUpdated(timeRemaining);
        }
    }
    else if (command == "PLAYER_SUBMITTED") {
        if (parts.size() >= 3) {
            int index = parts[1].toInt();
            QString username = parts[2];
            emit playerSubmitted(index, username);
        }
    }
    else if (command == "CHAT") {
        if (parts.size() >= 3) {
            QString username = parts[1];
            QString msg = parts[2];
            emit chatReceived(username, msg);
        }
    }
    else if (command == "GAME_END") {
        if (parts.size() >= 3) {
            bool won = (parts[1] == "WIN");
            QString msg;
            if (parts.size() >= 4) {
                // Format: GAME_END|LOSE|reason|solution
                msg = parts[2] + "|" + parts[3];
            } else {
                // Format: GAME_END|WIN|message
                msg = parts[2];
            }
            emit gameEnded(won, msg);
        }
    }
    else if (command == "GAME_ABORTED") {
        if (parts.size() > 1) {
            emit gameAborted(parts[1]);
        }
    }
    else if (command == "ERROR") {
        if (parts.size() > 1) {
            emit errorReceived(parts[1]);
        }
    }
}

void NetworkManager::parseRoomList(const QStringList &parts)
{
    rooms.clear();
    
    // Format: ROOM_LIST|id:name:count|id:name:count|...
    for (int i = 1; i < parts.size(); i++) {
        QStringList roomParts = parts[i].split(':');
        if (roomParts.size() >= 3) {
            RoomInfo room;
            room.id = roomParts[0].toInt();
            room.name = roomParts[1];
            room.playerCount = roomParts[2].toInt();
            rooms.append(room);
        }
    }
    
    emit roomListReceived(rooms);
}

void NetworkManager::parseRoomStatus(const QStringList &parts)
{
    players.clear();
    
    // Format: ROOM_STATUS|count|host_index|idx:name:ready|idx:name:ready|...
    if (parts.size() >= 3) {
        currentHostIndex = parts[2].toInt();
        
        for (int i = 3; i < parts.size(); i++) {
            QStringList playerParts = parts[i].split(':');
            if (playerParts.size() >= 3) {
                PlayerInfo player;
                player.index = playerParts[0].toInt();
                player.username = playerParts[1];
                player.ready = (playerParts[2] == "1");
                player.isHost = (player.index == currentHostIndex);
                
                // Parse ping if available (format: idx:name:ready:ping)
                if (playerParts.size() >= 4) {
                    player.ping = playerParts[3].toInt();
                } else {
                    // No ping data available
                    player.ping = -1;
                }
                
                // Track current player's index
                if (player.username == currentUsername) {
                    currentPlayerIndex = player.index;
                }
                
                players.append(player);
            }
        }
    }
    
    emit roomStatusUpdated(players);
}

void NetworkManager::parseGameStart(const QStringList &parts)
{
    // Format: GAME_START|equation|matrix0|matrix1|matrix2|matrix3|currentRound|totalRounds
    // equation: P1+P2*P3=P4
    // matrix: 16 numbers separated by commas, or HIDDEN
    
    if (parts.size() < 6) {
        qWarning() << "Invalid GAME_START format";
        return;
    }
    
    gameData.equation = parts[1];
    
    // Parse round info if available (backward compatibility)
    if (parts.size() >= 8) {
        gameData.currentRound = parts[6].toInt();
        gameData.totalRounds = parts[7].toInt();
    } else {
        gameData.currentRound = 1;
        gameData.totalRounds = 5;
    }
    
    // Parse 4 matrices
    for (int m = 0; m < 4; m++) {
        QString matrixData = parts[2 + m];
        
        if (matrixData == "HIDDEN") {
            gameData.matrixHidden[m] = true;
            gameData.matrices[m].clear();
        } else {
            gameData.matrixHidden[m] = false;
            
            QStringList numbers = matrixData.split(',');
            if (numbers.size() != 16) {
                qWarning() << "Invalid matrix size:" << numbers.size();
                continue;
            }
            
            // Convert to 4x4 matrix
            gameData.matrices[m].clear();
            for (int i = 0; i < 4; i++) {
                QVector<int> row;
                for (int j = 0; j < 4; j++) {
                    row.append(numbers[i * 4 + j].toInt());
                }
                gameData.matrices[m].append(row);
            }
        }
    }
    
    emit gameStarted(gameData);
}

