#ifndef LOBBYSCREEN_H
#define LOBBYSCREEN_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include "networkmanager.h"

class LobbyScreen : public QWidget
{
    Q_OBJECT

public:
    explicit LobbyScreen(NetworkManager *network, QWidget *parent = nullptr);
    
    void refresh();

private slots:
    void onRefreshClicked();
    void onCreateRoomClicked();
    void onJoinRoomClicked();
    void onRoomListReceived(const QVector<RoomInfo> &rooms);
    void onRoomCreated(int roomId, const QString &roomName);
    void onError(const QString &error);

private:
    NetworkManager *networkManager;
    
    QLabel *welcomeLabel;
    QTableWidget *roomTable;
    QPushButton *refreshButton;
    QPushButton *createButton;
    QPushButton *joinButton;
    QLineEdit *roomNameEdit;
    
    void setupUI();
    void updateRoomList(const QVector<RoomInfo> &rooms);
};

#endif // LOBBYSCREEN_H


