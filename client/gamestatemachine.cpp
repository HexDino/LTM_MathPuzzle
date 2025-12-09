#include "gamestatemachine.h"
#include <QDebug>

GameStateMachine::GameStateMachine(QObject *parent)
    : QObject(parent)
    , m_currentState(GameState::Login)
{
}

void GameStateMachine::setState(GameState newState)
{
    if (m_currentState != newState) {
        qDebug() << "State transition:" << (int)m_currentState << "->" << (int)newState;
        m_currentState = newState;
        emit stateChanged(newState);
    }
}

void GameStateMachine::transitionToLogin()
{
    setState(GameState::Login);
    emit showLoginScreen();
}

void GameStateMachine::transitionToLobby()
{
    setState(GameState::Lobby);
    emit showLobbyScreen();
}

void GameStateMachine::transitionToRoom()
{
    setState(GameState::Room);
    emit showRoomScreen();
}

void GameStateMachine::transitionToGame()
{
    setState(GameState::InGame);
    emit showGameScreen();
}

void GameStateMachine::transitionToResult()
{
    setState(GameState::Result);
    emit showResultScreen();
}


