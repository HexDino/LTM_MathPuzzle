#ifndef ROOMSCREEN_H
#define ROOMSCREEN_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include "networkmanager.h"

class RoomScreen : public QWidget
{
    Q_OBJECT

public:
    explicit RoomScreen(NetworkManager *network, QWidget *parent = nullptr);

private slots:
    void onReadyClicked();
    void onStartGameClicked();
    void onLeaveRoomClicked();
    void onPlayerJoined(int index, const QString &username);
    void onPlayerLeft(const QString &username);
    void onPlayerDisconnected(const QString &username);
    void onPlayerReconnected(const QString &username);
    void onRoomStatusUpdated(const QVector<PlayerInfo> &players);
    void onError(const QString &error);

private:
    NetworkManager *networkManager;
    
    QLabel *roomTitleLabel;
    QLabel *instructionLabel;
    QTableWidget *playerTable;
    QPushButton *readyButton;
    QPushButton *startGameButton;
    QPushButton *leaveRoomButton;
    QLabel *statusLabel;
    
    void setupUI();
    void updatePlayerList(const QVector<PlayerInfo> &players);
};

#endif // ROOMSCREEN_H


