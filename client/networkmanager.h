#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QTcpSocket>
#include <QString>
#include <QStringList>

// Data structures matching server protocol
struct RoomInfo {
    int id;
    QString name;
    int playerCount;
};

struct PlayerInfo {
    int index;
    QString username;
    bool ready;
    bool isHost;
    int ping;  // Ping in milliseconds
};

struct GameData {
    QString equation;
    QVector<QVector<int>> matrices[4];  // 4 matrices, each 4x4
    bool matrixHidden[4];               // Which matrix is hidden
    int currentRound = 1;
    int totalRounds = 5;
};

class NetworkManager : public QObject
{
    Q_OBJECT

public:
    explicit NetworkManager(QObject *parent = nullptr);
    ~NetworkManager();
    
    // Connection methods
    void connectToServer(const QString &host, quint16 port);
    void disconnectFromServer();
    bool isConnected() const;
    
    // Protocol commands
    void sendRegister(const QString &username, const QString &password);
    void sendLogin(const QString &username, const QString &password);
    void sendListRooms();
    void sendCreateRoom(const QString &roomName);
    void sendJoinRoom(int roomId);
    void sendLeaveRoom();
    void sendReady();
    void sendStartGame();
    void sendChat(const QString &message);
    void sendSubmit(int row, int col);
    void sendPong();
    
    // Getters for current state
    QString getCurrentUsername() const { return currentUsername; }
    int getCurrentRoomId() const { return currentRoomId; }
    int getCurrentHostIndex() const { return currentHostIndex; }
    int getCurrentPlayerIndex() const { return currentPlayerIndex; }
    bool isCurrentPlayerHost() const { return currentPlayerIndex == currentHostIndex; }
    const QVector<RoomInfo>& getRooms() const { return rooms; }
    const QVector<PlayerInfo>& getPlayers() const { return players; }
    const GameData& getGameData() const { return gameData; }
    int getTimeRemaining() const { return timeRemaining; }

signals:
    // Connection signals
    void connected();
    void disconnected();
    void connectionError(const QString &error);
    
    // Authentication signals
    void welcomeReceived(const QString &message);
    void loginSuccessful(const QString &username);
    void reconnectSuccessful(const QString &username);  // For reconnect after disconnect
    void registerSuccessful();
    void authError(const QString &error);
    
    // Lobby signals
    void roomListReceived(const QVector<RoomInfo> &rooms);
    
    // Room signals
    void roomCreated(int roomId, const QString &roomName);
    void roomJoined(int roomId);
    void leftRoom();
    void playerJoined(int index, const QString &username);
    void playerLeft(const QString &username);
    void playerDisconnected(const QString &username);  // Temporary disconnect
    void playerReconnected(const QString &username);   // Reconnected successfully
    void roomStatusUpdated(const QVector<PlayerInfo> &players);
    
    // Game signals
    void gameStarted(const GameData &data);
    void timerUpdated(int secondsRemaining);
    void playerSubmitted(int playerIndex, const QString &username);
    void chatReceived(const QString &username, const QString &message);
    
    // Result signals
    void gameEnded(bool won, const QString &message);
    void gameAborted(const QString &reason);
    
    // System signals
    void pingReceived();
    void errorReceived(const QString &error);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError socketError);

private:
    QTcpSocket *socket;
    QString receiveBuffer;
    
    // Current state
    QString currentUsername;
    int currentRoomId;
    int currentHostIndex;
    int currentPlayerIndex;
    QVector<RoomInfo> rooms;
    QVector<PlayerInfo> players;
    GameData gameData;
    int timeRemaining;
    
    // Protocol parsing
    void handleMessage(const QString &message);
    void parseGameStart(const QStringList &parts);
    void parseRoomList(const QStringList &parts);
    void parseRoomStatus(const QStringList &parts);
    
    // Utility
    void sendCommand(const QString &command);
};

#endif // NETWORKMANAGER_H


