#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include "networkmanager.h"
#include "gamestatemachine.h"
#include "loginscreen.h"
#include "lobbyscreen.h"
#include "roomscreen.h"
#include "gamescreen.h"
#include "resultscreen.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    // Network and state management
    NetworkManager *networkManager;
    GameStateMachine *stateMachine;
    
    // UI components
    QStackedWidget *stackedWidget;
    LoginScreen *loginScreen;
    LobbyScreen *lobbyScreen;
    RoomScreen *roomScreen;
    GameScreen *gameScreen;
    ResultScreen *resultScreen;
    
    // Reconnect state
    bool isReconnecting;
    
    void setupUI();
    void connectSignals();
};

#endif // MAINWINDOW_H


