#ifndef LOGINSCREEN_H
#define LOGINSCREEN_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include "networkmanager.h"

class LoginScreen : public QWidget
{
    Q_OBJECT

public:
    explicit LoginScreen(NetworkManager *network, QWidget *parent = nullptr);

private slots:
    void onConnectClicked();
    void onLoginClicked();
    void onRegisterClicked();
    void onConnected();
    void onDisconnected();
    void onWelcomeReceived(const QString &message);
    void onLoginSuccessful(const QString &username);
    void onReconnectSuccessful(const QString &username);
    void onRegisterSuccessful();
    void onError(const QString &error);

private:
    NetworkManager *networkManager;
    
    // Logo
    QLabel *logoLabel;
    
    // Connection UI
    QLineEdit *serverEdit;
    QLineEdit *portEdit;
    QPushButton *connectButton;
    QLabel *statusLabel;
    
    // Authentication UI
    QLineEdit *usernameEdit;
    QLineEdit *passwordEdit;
    QPushButton *loginButton;
    QPushButton *registerButton;
    
    QWidget *connectionWidget;
    QWidget *authWidget;
    
    void setupUI();
    void updateUIState();
};

#endif // LOGINSCREEN_H


