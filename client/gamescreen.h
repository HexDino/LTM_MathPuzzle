#ifndef GAMESCREEN_H
#define GAMESCREEN_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QTimer>
#include "networkmanager.h"

class MatrixWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MatrixWidget(int matrixIndex, QWidget *parent = nullptr);
    
    void setMatrix(const QVector<QVector<int>> &data);
    void setHidden();
    void clearSelection();
    int getSelectedRow() const { return selectedRow; }
    int getSelectedCol() const { return selectedCol; }
    bool hasSelection() const { return selectedRow >= 0 && selectedCol >= 0; }

signals:
    void cellSelected(int row, int col);

private:
    int matrixIndex;
    QTableWidget *table;
    QLabel *titleLabel;
    int selectedRow;
    int selectedCol;
    
    void setupUI();
    void onCellClicked(int row, int col);
};

class GameScreen : public QWidget
{
    Q_OBJECT

public:
    explicit GameScreen(NetworkManager *network, QWidget *parent = nullptr);

private slots:
    void onGameStarted(const GameData &data);
    void onTimerUpdated(int secondsRemaining);
    void onPlayerSubmitted(int playerIndex, const QString &username);
    void onChatReceived(const QString &username, const QString &message);
    void onSendChatClicked();
    void onSubmitClicked();
    void onMatrixCellSelected(int row, int col);

private:
    NetworkManager *networkManager;
    
    // UI components
    QLabel *roundLabel;
    QLabel *equationLabel;
    QLabel *timerLabel;
    MatrixWidget *matrixWidgets[4];
    QTextEdit *chatDisplay;
    QLineEdit *chatInput;
    QPushButton *sendChatButton;
    QPushButton *submitButton;
    QLabel *statusLabel;
    QLabel *instructionLabel;
    
    // Game state
    int selectedMatrixIndex;
    
    void setupUI();
    void updateSubmitButton();
};

#endif // GAMESCREEN_H


