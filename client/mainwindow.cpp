#include "mainwindow.h"
#include <QVBoxLayout>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QIcon>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , isReconnecting(false)
{
    setWindowTitle("Math Puzzle Game - Multiplayer");
    resize(1000, 700);
    
    // Set window icon
    setWindowIcon(QIcon(":/resources/logo.png"));
    
    // Initialize network manager
    networkManager = new NetworkManager(this);
    
    // Initialize state machine
    stateMachine = new GameStateMachine(this);
    
    setupUI();
    connectSignals();
    
    // Start with login screen
    stateMachine->transitionToLogin();
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    // Create central widget with stacked layout
    stackedWidget = new QStackedWidget(this);
    setCentralWidget(stackedWidget);
    
    // Create all screens
    loginScreen = new LoginScreen(networkManager, this);
    lobbyScreen = new LobbyScreen(networkManager, this);
    roomScreen = new RoomScreen(networkManager, this);
    gameScreen = new GameScreen(networkManager, this);
    resultScreen = new ResultScreen(networkManager, this);
    
    // Add screens to stacked widget
    stackedWidget->addWidget(loginScreen);    // Index 0
    stackedWidget->addWidget(lobbyScreen);    // Index 1
    stackedWidget->addWidget(roomScreen);     // Index 2
    stackedWidget->addWidget(gameScreen);     // Index 3
    stackedWidget->addWidget(resultScreen);   // Index 4
    
    // Create menu bar
    QMenu *fileMenu = menuBar()->addMenu("&File");
    QAction *exitAction = fileMenu->addAction("E&xit");
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);
    
    QMenu *helpMenu = menuBar()->addMenu("&Help");
    QAction *aboutAction = helpMenu->addAction("&About");
    connect(aboutAction, &QAction::triggered, [this]() {
        QMessageBox::about(this, "About Math Puzzle Game",
            "Math Puzzle Game - Multiplayer\n\n"
            "A cooperative 4-player puzzle game with asymmetric information.\n"
            "Work together to solve math equations!\n\n"
            "Version 1.0\n"
            "Network Programming Project");
    });
}

void MainWindow::connectSignals()
{
    // Connect state machine to screen changes
    connect(stateMachine, &GameStateMachine::showLoginScreen, [this]() {
        stackedWidget->setCurrentWidget(loginScreen);
    });
    
    connect(stateMachine, &GameStateMachine::showLobbyScreen, [this]() {
        stackedWidget->setCurrentWidget(lobbyScreen);
        lobbyScreen->refresh();
    });
    
    connect(stateMachine, &GameStateMachine::showRoomScreen, [this]() {
        stackedWidget->setCurrentWidget(roomScreen);
    });
    
    connect(stateMachine, &GameStateMachine::showGameScreen, [this]() {
        stackedWidget->setCurrentWidget(gameScreen);
    });
    
    connect(stateMachine, &GameStateMachine::showResultScreen, [this]() {
        stackedWidget->setCurrentWidget(resultScreen);
    });
    
    // Connect network events to state machine
    connect(networkManager, &NetworkManager::loginSuccessful, 
            stateMachine, &GameStateMachine::transitionToLobby);
    
    // Handle reconnect separately - don't auto-transition, wait for server data
    connect(networkManager, &NetworkManager::reconnectSuccessful, [this](const QString &username) {
        qDebug() << "Reconnected as" << username << "- waiting for server data...";
        isReconnecting = true;
        // Server will send GAME_START, ROOM_STATUS, or ROOM_LIST
        // which will trigger the appropriate screen transition
    });
    
    connect(networkManager, &NetworkManager::roomJoined, 
            stateMachine, &GameStateMachine::transitionToRoom);
    
    connect(networkManager, &NetworkManager::leftRoom, 
            stateMachine, &GameStateMachine::transitionToLobby);
    
    // Handle room status updates - if reconnecting, transition to room screen
    connect(networkManager, &NetworkManager::roomStatusUpdated, [this]() {
        if (isReconnecting) {
            qDebug() << "Reconnected to room - transitioning to room screen";
            isReconnecting = false;
            stateMachine->transitionToRoom();
        }
    });
    
    // Handle room list - if reconnecting, transition to lobby screen
    connect(networkManager, &NetworkManager::roomListReceived, [this]() {
        if (isReconnecting) {
            qDebug() << "Reconnected to lobby - transitioning to lobby screen";
            isReconnecting = false;
            stateMachine->transitionToLobby();
        }
    });
    
    connect(networkManager, &NetworkManager::gameStarted, [this]() {
        if (isReconnecting) {
            qDebug() << "Reconnected to game - transitioning to game screen";
            isReconnecting = false;
        }
        stateMachine->transitionToGame();
    });
    
    connect(networkManager, &NetworkManager::gameEnded, 
            stateMachine, &GameStateMachine::transitionToResult);
    
    connect(networkManager, &NetworkManager::gameAborted, [this](const QString &reason) {
        QMessageBox::warning(this, "Game Aborted", reason);
        stateMachine->transitionToLobby();
    });
    
    // Handle server disconnect - go back to login screen
    connect(networkManager, &NetworkManager::disconnected, [this]() {
        QMessageBox::critical(this, "Connection Lost", 
            "Lost connection to server!\n\n"
            "The server may have stopped or you may have lost network connection.\n"
            "Please reconnect to continue playing.");
        stateMachine->transitionToLogin();
    });
    
    // Connect back to room button
    connect(resultScreen, &ResultScreen::backToRoomRequested,
            stateMachine, &GameStateMachine::transitionToRoom);
}


