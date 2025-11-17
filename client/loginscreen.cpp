#include "loginscreen.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QFormLayout>

LoginScreen::LoginScreen(NetworkManager *network, QWidget *parent)
    : QWidget(parent)
    , networkManager(network)
{
    setupUI();
    
    // Connect network signals
    connect(networkManager, &NetworkManager::connected, this, &LoginScreen::onConnected);
    connect(networkManager, &NetworkManager::disconnected, this, &LoginScreen::onDisconnected);
    connect(networkManager, &NetworkManager::welcomeReceived, this, &LoginScreen::onWelcomeReceived);
    connect(networkManager, &NetworkManager::loginSuccessful, this, &LoginScreen::onLoginSuccessful);
    connect(networkManager, &NetworkManager::registerSuccessful, this, &LoginScreen::onRegisterSuccessful);
    connect(networkManager, &NetworkManager::connectionError, this, &LoginScreen::onError);
    connect(networkManager, &NetworkManager::authError, this, &LoginScreen::onError);
    connect(networkManager, &NetworkManager::errorReceived, this, &LoginScreen::onError);
    
    updateUIState();
}

void LoginScreen::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);
    
    // Title
    QLabel *titleLabel = new QLabel("Math Puzzle Game", this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(24);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    
    QLabel *subtitleLabel = new QLabel("Cooperative 4-Player Puzzle Game", this);
    subtitleLabel->setAlignment(Qt::AlignCenter);
    
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(subtitleLabel);
    mainLayout->addSpacing(30);
    
    // Connection group
    QGroupBox *connectionGroup = new QGroupBox("Server Connection", this);
    QFormLayout *connLayout = new QFormLayout(connectionGroup);
    
    serverEdit = new QLineEdit("localhost", this);
    portEdit = new QLineEdit("8888", this);
    connectButton = new QPushButton("Connect", this);
    statusLabel = new QLabel("Not connected", this);
    
    connLayout->addRow("Server:", serverEdit);
    connLayout->addRow("Port:", portEdit);
    connLayout->addRow(connectButton);
    connLayout->addRow("Status:", statusLabel);
    
    connectionWidget = connectionGroup;
    mainLayout->addWidget(connectionGroup);
    mainLayout->addSpacing(20);
    
    // Authentication group
    QGroupBox *authGroup = new QGroupBox("Login / Register", this);
    QFormLayout *authLayout = new QFormLayout(authGroup);
    
    usernameEdit = new QLineEdit(this);
    passwordEdit = new QLineEdit(this);
    passwordEdit->setEchoMode(QLineEdit::Password);
    
    loginButton = new QPushButton("Login", this);
    registerButton = new QPushButton("Register", this);
    
    authLayout->addRow("Username:", usernameEdit);
    authLayout->addRow("Password:", passwordEdit);
    
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(loginButton);
    buttonLayout->addWidget(registerButton);
    authLayout->addRow(buttonLayout);
    
    authWidget = authGroup;
    mainLayout->addWidget(authGroup);
    
    mainLayout->addStretch();
    
    // Connect button signals
    connect(connectButton, &QPushButton::clicked, this, &LoginScreen::onConnectClicked);
    connect(loginButton, &QPushButton::clicked, this, &LoginScreen::onLoginClicked);
    connect(registerButton, &QPushButton::clicked, this, &LoginScreen::onRegisterClicked);
    
    // Enable return key
    connect(passwordEdit, &QLineEdit::returnPressed, this, &LoginScreen::onLoginClicked);
}

void LoginScreen::updateUIState()
{
    bool connected = networkManager->isConnected();
    
    serverEdit->setEnabled(!connected);
    portEdit->setEnabled(!connected);
    connectButton->setEnabled(!connected);
    
    authWidget->setEnabled(connected);
    
    if (connected) {
        statusLabel->setText("<font color='green'>Connected</font>");
        connectButton->setText("Connected");
    } else {
        statusLabel->setText("<font color='red'>Not connected</font>");
        connectButton->setText("Connect");
    }
}

void LoginScreen::onConnectClicked()
{
    QString server = serverEdit->text().trimmed();
    quint16 port = portEdit->text().toUInt();
    
    if (server.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please enter server address");
        return;
    }
    
    if (port == 0) {
        QMessageBox::warning(this, "Error", "Please enter valid port");
        return;
    }
    
    networkManager->connectToServer(server, port);
}

void LoginScreen::onLoginClicked()
{
    QString username = usernameEdit->text().trimmed();
    QString password = passwordEdit->text();
    
    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please enter username and password");
        return;
    }
    
    networkManager->sendLogin(username, password);
}

void LoginScreen::onRegisterClicked()
{
    QString username = usernameEdit->text().trimmed();
    QString password = passwordEdit->text();
    
    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please enter username and password");
        return;
    }
    
    networkManager->sendRegister(username, password);
}

void LoginScreen::onConnected()
{
    updateUIState();
}

void LoginScreen::onDisconnected()
{
    updateUIState();
    QMessageBox::information(this, "Disconnected", "Disconnected from server");
}

void LoginScreen::onWelcomeReceived(const QString &message)
{
    statusLabel->setText("<font color='green'>" + message + "</font>");
}

void LoginScreen::onLoginSuccessful(const QString &username)
{
    QMessageBox::information(this, "Success", "Logged in as " + username);
}

void LoginScreen::onRegisterSuccessful()
{
    QMessageBox::information(this, "Success", 
        "Registration successful! You can now login.");
}

void LoginScreen::onError(const QString &error)
{
    QMessageBox::warning(this, "Error", error);
}


