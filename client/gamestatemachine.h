#ifndef GAMESTATEMACHINE_H
#define GAMESTATEMACHINE_H

#include <QObject>

// Game states matching the 5 screens
enum class GameState {
    Login,
    Lobby,
    Room,
    InGame,
    Result
};

class GameStateMachine : public QObject
{
    Q_OBJECT

public:
    explicit GameStateMachine(QObject *parent = nullptr);
    
    GameState currentState() const { return m_currentState; }
    
    // State transitions
    void transitionToLogin();
    void transitionToLobby();
    void transitionToRoom();
    void transitionToGame();
    void transitionToResult();

signals:
    void stateChanged(GameState newState);
    
    // Screen signals
    void showLoginScreen();
    void showLobbyScreen();
    void showRoomScreen();
    void showGameScreen();
    void showResultScreen();

private:
    GameState m_currentState;
    void setState(GameState newState);
};

#endif // GAMESTATEMACHINE_H


