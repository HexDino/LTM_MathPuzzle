#include "gamescreen.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>

// ========== MatrixWidget Implementation ==========

MatrixWidget::MatrixWidget(int index, QWidget *parent)
    : QWidget(parent)
    , matrixIndex(index)
    , selectedRow(-1)
    , selectedCol(-1)
{
    setupUI();
}

void MatrixWidget::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(5);
    
    // Title
    titleLabel = new QLabel(QString("Matrix P%1").arg(matrixIndex + 1), this);
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(12);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);
    
    // Table
    table = new QTableWidget(4, 4, this);
    table->horizontalHeader()->hide();
    table->verticalHeader()->hide();
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setShowGrid(true);
    table->setStyleSheet("QTableWidget { border: 1px solid #ccc; }");
    
    // Make cells square and remove extra spacing
    for (int i = 0; i < 4; i++) {
        table->setColumnWidth(i, 55);
        table->setRowHeight(i, 55);
    }
    
    // Set exact size to fit 4x4 cells (55*4 = 220 + borders)
    int tableSize = 55 * 4 + 2;  // 4 cells + border
    table->setMaximumSize(tableSize, tableSize);
    table->setMinimumSize(tableSize, tableSize);
    table->setFixedSize(tableSize, tableSize);
    
    layout->addWidget(table);
    
    // Connect cell click
    connect(table, &QTableWidget::cellClicked, this, &MatrixWidget::onCellClicked);
}

void MatrixWidget::setMatrix(const QVector<QVector<int>> &data)
{
    if (data.size() != 4) return;
    
    for (int i = 0; i < 4; i++) {
        if (data[i].size() != 4) continue;
        for (int j = 0; j < 4; j++) {
            QTableWidgetItem *item = new QTableWidgetItem(QString::number(data[i][j]));
            item->setTextAlignment(Qt::AlignCenter);
            QFont font = item->font();
            font.setPointSize(14);
            font.setBold(true);
            item->setFont(font);
            table->setItem(i, j, item);
        }
    }
    
    table->setEnabled(true);
    titleLabel->setStyleSheet("");
}

void MatrixWidget::setHidden()
{
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            QTableWidgetItem *item = new QTableWidgetItem("?");
            item->setTextAlignment(Qt::AlignCenter);
            item->setBackground(Qt::lightGray);
            QFont font = item->font();
            font.setPointSize(20);
            font.setBold(true);
            item->setFont(font);
            table->setItem(i, j, item);
        }
    }
    
    table->setEnabled(true);
    titleLabel->setStyleSheet("QLabel { color: red; }");
    titleLabel->setText(QString("Matrix P%1 (YOUR MATRIX)").arg(matrixIndex + 1));
}

void MatrixWidget::clearSelection()
{
    selectedRow = -1;
    selectedCol = -1;
    table->clearSelection();
}

void MatrixWidget::onCellClicked(int row, int col)
{
    selectedRow = row;
    selectedCol = col;
    emit cellSelected(row, col);
}

// ========== GameScreen Implementation ==========

GameScreen::GameScreen(NetworkManager *network, QWidget *parent)
    : QWidget(parent)
    , networkManager(network)
    , selectedMatrixIndex(-1)
{
    setupUI();
    
    // Connect network signals
    connect(networkManager, &NetworkManager::gameStarted, 
            this, &GameScreen::onGameStarted);
    connect(networkManager, &NetworkManager::timerUpdated, 
            this, &GameScreen::onTimerUpdated);
    connect(networkManager, &NetworkManager::playerSubmitted, 
            this, &GameScreen::onPlayerSubmitted);
    connect(networkManager, &NetworkManager::chatReceived, 
            this, &GameScreen::onChatReceived);
}

void GameScreen::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Top bar: Equation and Timer
    QHBoxLayout *topLayout = new QHBoxLayout();
    
    equationLabel = new QLabel("Equation: ", this);
    QFont eqFont = equationLabel->font();
    eqFont.setPointSize(18);
    eqFont.setBold(true);
    equationLabel->setFont(eqFont);
    
    timerLabel = new QLabel("Time: 3:00", this);
    QFont timerFont = timerLabel->font();
    timerFont.setPointSize(18);
    timerFont.setBold(true);
    timerLabel->setFont(timerFont);
    timerLabel->setStyleSheet("QLabel { color: green; }");
    
    topLayout->addWidget(equationLabel);
    topLayout->addStretch();
    topLayout->addWidget(timerLabel);
    
    mainLayout->addLayout(topLayout);
    
    // Instructions
    instructionLabel = new QLabel(
        "Work together! Share your matrix values via chat. Select a cell from YOUR matrix and submit!",
        this);
    instructionLabel->setWordWrap(true);
    instructionLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(instructionLabel);
    
    mainLayout->addSpacing(10);
    
    // Main content: Matrices and Chat side by side
    QHBoxLayout *contentLayout = new QHBoxLayout();
    
    // Left: 4 Matrices in 2x2 grid
    QGroupBox *matricesGroup = new QGroupBox("Matrices", this);
    QGridLayout *matricesLayout = new QGridLayout(matricesGroup);
    matricesLayout->setSpacing(10);
    matricesLayout->setContentsMargins(10, 10, 10, 10);
    
    for (int i = 0; i < 4; i++) {
        matrixWidgets[i] = new MatrixWidget(i, this);
        int row = i / 2;
        int col = i % 2;
        matricesLayout->addWidget(matrixWidgets[i], row, col, Qt::AlignTop | Qt::AlignLeft);
        
        // Connect cell selection
        connect(matrixWidgets[i], &MatrixWidget::cellSelected, 
                this, &GameScreen::onMatrixCellSelected);
    }
    
    // Set size policy to prevent stretching
    matricesGroup->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    
    contentLayout->addWidget(matricesGroup, 0);
    
    // Right: Chat
    QGroupBox *chatGroup = new QGroupBox("Team Chat", this);
    QVBoxLayout *chatLayout = new QVBoxLayout(chatGroup);
    
    chatDisplay = new QTextEdit(this);
    chatDisplay->setReadOnly(true);
    chatDisplay->setMinimumWidth(300);
    
    QHBoxLayout *chatInputLayout = new QHBoxLayout();
    chatInput = new QLineEdit(this);
    chatInput->setPlaceholderText("Type your message...");
    sendChatButton = new QPushButton("Send", this);
    
    chatInputLayout->addWidget(chatInput);
    chatInputLayout->addWidget(sendChatButton);
    
    chatLayout->addWidget(chatDisplay);
    chatLayout->addLayout(chatInputLayout);
    
    contentLayout->addWidget(chatGroup, 1);
    
    mainLayout->addLayout(contentLayout);
    
    // Bottom: Submit section
    statusLabel = new QLabel("Select a cell from YOUR matrix to submit!", this);
    statusLabel->setAlignment(Qt::AlignCenter);
    QFont statusFont = statusLabel->font();
    statusFont.setPointSize(11);
    statusLabel->setFont(statusFont);
    mainLayout->addWidget(statusLabel);
    
    submitButton = new QPushButton("Submit Answer", this);
    submitButton->setEnabled(false);
    submitButton->setMinimumHeight(50);
    QFont submitFont = submitButton->font();
    submitFont.setPointSize(14);
    submitFont.setBold(true);
    submitButton->setFont(submitFont);
    submitButton->setStyleSheet("QPushButton:enabled { background-color: #4CAF50; color: white; }");
    
    mainLayout->addWidget(submitButton);
    
    // Connect signals
    connect(sendChatButton, &QPushButton::clicked, this, &GameScreen::onSendChatClicked);
    connect(chatInput, &QLineEdit::returnPressed, this, &GameScreen::onSendChatClicked);
    connect(submitButton, &QPushButton::clicked, this, &GameScreen::onSubmitClicked);
}

void GameScreen::onGameStarted(const GameData &data)
{
    // Update equation
    equationLabel->setText("Equation: " + data.equation);
    
    // Update matrices
    for (int i = 0; i < 4; i++) {
        if (data.matrixHidden[i]) {
            matrixWidgets[i]->setHidden();
            selectedMatrixIndex = i;  // This is the player's matrix
        } else {
            matrixWidgets[i]->setMatrix(data.matrices[i]);
        }
    }
    
    // Clear chat
    chatDisplay->clear();
    chatDisplay->append("<b>Game started! Work together to solve the puzzle!</b>");
    
    // Reset status
    statusLabel->setText(QString("Select a cell from Matrix P%1 (YOUR matrix) to submit!").arg(selectedMatrixIndex + 1));
    updateSubmitButton();
}

void GameScreen::onTimerUpdated(int secondsRemaining)
{
    int minutes = secondsRemaining / 60;
    int seconds = secondsRemaining % 60;
    timerLabel->setText(QString("Time: %1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0')));
    
    // Change color based on time
    if (secondsRemaining <= 30) {
        timerLabel->setStyleSheet("QLabel { color: red; }");
    } else if (secondsRemaining <= 60) {
        timerLabel->setStyleSheet("QLabel { color: orange; }");
    } else {
        timerLabel->setStyleSheet("QLabel { color: green; }");
    }
}

void GameScreen::onPlayerSubmitted(int /* playerIndex */, const QString &username)
{
    statusLabel->setText(username + " has submitted their answer!");
    chatDisplay->append(QString("<font color='blue'><b>%1 submitted!</b></font>").arg(username));
}

void GameScreen::onChatReceived(const QString &username, const QString &message)
{
    chatDisplay->append(QString("<b>%1:</b> %2").arg(username, message));
}

void GameScreen::onSendChatClicked()
{
    QString message = chatInput->text().trimmed();
    
    if (message.isEmpty()) return;
    
    networkManager->sendChat(message);
    chatInput->clear();
}

void GameScreen::onSubmitClicked()
{
    if (selectedMatrixIndex < 0 || selectedMatrixIndex >= 4) {
        QMessageBox::warning(this, "Error", "No matrix selected");
        return;
    }
    
    MatrixWidget *widget = matrixWidgets[selectedMatrixIndex];
    
    if (!widget->hasSelection()) {
        QMessageBox::warning(this, "Error", "Please select a cell first");
        return;
    }
    
    int row = widget->getSelectedRow();
    int col = widget->getSelectedCol();
    
    // Confirm submission
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Confirm Submission",
        QString("Submit cell [%1, %2] from your matrix?\n\nYou cannot change after submission!").arg(row).arg(col),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        networkManager->sendSubmit(row, col);
        submitButton->setEnabled(false);
        statusLabel->setText("Answer submitted! Waiting for other players...");
    }
}

void GameScreen::onMatrixCellSelected(int row, int col)
{
    // Clear selection from other matrices
    for (int i = 0; i < 4; i++) {
        if (matrixWidgets[i] != sender()) {
            matrixWidgets[i]->clearSelection();
        }
    }
    
    // Update status
    MatrixWidget *widget = qobject_cast<MatrixWidget*>(sender());
    for (int i = 0; i < 4; i++) {
        if (matrixWidgets[i] == widget) {
            statusLabel->setText(QString("Selected: Matrix P%1, Cell [%2, %3]").arg(i + 1).arg(row).arg(col));
            break;
        }
    }
    
    updateSubmitButton();
}

void GameScreen::updateSubmitButton()
{
    // Enable submit button only if a cell from the hidden matrix is selected
    if (selectedMatrixIndex >= 0 && selectedMatrixIndex < 4) {
        submitButton->setEnabled(matrixWidgets[selectedMatrixIndex]->hasSelection());
    } else {
        submitButton->setEnabled(false);
    }
}


