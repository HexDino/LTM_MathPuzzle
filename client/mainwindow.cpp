#include "mainwindow.h"
#include <QVBoxLayout>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Math Puzzle Game - Multiplayer");
    resize(1000, 700);
    
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
    
    connect(networkManager, &NetworkManager::roomJoined, 
            stateMachine, &GameStateMachine::transitionToRoom);
    
    connect(networkManager, &NetworkManager::gameStarted, 
            stateMachine, &GameStateMachine::transitionToGame);
    
    connect(networkManager, &NetworkManager::gameEnded, 
            stateMachine, &GameStateMachine::transitionToResult);
    
    // Connect back to room button
    connect(resultScreen, &ResultScreen::backToRoomRequested,
            stateMachine, &GameStateMachine::transitionToRoom);
}


