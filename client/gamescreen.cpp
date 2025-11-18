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
        table->setColumnWidth(i, 50);
        table->setRowHeight(i, 50);
    }
    
    // Set exact size to fit 4x4 cells (50*4 = 200 + borders)
    int tableSize = 50 * 4 + 2;  // 4 cells + border
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
            QString numStr = QString::number(data[i][j]);
            QTableWidgetItem *item = new QTableWidgetItem(numStr);
            item->setTextAlignment(Qt::AlignCenter);
            
            QFont font = item->font();
            int numLength = numStr.length();
            if (numLength <= 4) {
                font.setPointSize(12);  
            } else if (numLength == 5) {
                font.setPointSize(10);  
            } else if (numLength == 6) {
                font.setPointSize(8);  
            } else {
                font.setPointSize(7);  
            }
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
            font.setPointSize(18);  // Smaller font for 50px cells
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
    mainLayout->setContentsMargins(10, 10, 10, 10);  // Reduce margins to save space
    mainLayout->setSpacing(5);  // Reduce default spacing
    
    // Timer and Round info on same line
    QHBoxLayout *topInfoLayout = new QHBoxLayout();
    
    // Round info (left)
    roundLabel = new QLabel("Round 1/5", this);
    QFont roundFont = roundLabel->font();
    roundFont.setPointSize(13);
    roundFont.setBold(true);
    roundLabel->setFont(roundFont);
    roundLabel->setStyleSheet("QLabel { color: #FF9800; }");
    topInfoLayout->addWidget(roundLabel);
    
    topInfoLayout->addStretch();
    
    // Timer (right)
    timerLabel = new QLabel("Time: 3:00", this);
    QFont timerFont = timerLabel->font();
    timerFont.setPointSize(14);
    timerFont.setBold(true);
    timerLabel->setFont(timerFont);
    timerLabel->setStyleSheet("QLabel { color: green; }");
    topInfoLayout->addWidget(timerLabel);
    
    mainLayout->addLayout(topInfoLayout);
    mainLayout->addSpacing(5);
    
    // Equation (center, compact)
    equationLabel = new QLabel("Equation: ", this);
    QFont eqFont = equationLabel->font();
    eqFont.setPointSize(22);  // Reduced from 28 to 22
    eqFont.setBold(true);
    equationLabel->setFont(eqFont);
    equationLabel->setAlignment(Qt::AlignCenter);
    equationLabel->setStyleSheet("QLabel { color: #2196F3; background-color: #E3F2FD; padding: 8px 15px; border-radius: 6px; }");
    equationLabel->setMaximumHeight(60);  // Limit height to save space
    mainLayout->addWidget(equationLabel);
    
    mainLayout->addSpacing(8);
    
    
    // Main content: Matrices and Chat side by side
    QHBoxLayout *contentLayout = new QHBoxLayout();
    
    // Left: 4 Matrices in 2x2 grid
    QGroupBox *matricesGroup = new QGroupBox("Matrices", this);
    QGridLayout *matricesLayout = new QGridLayout(matricesGroup);
    matricesLayout->setSpacing(8);  // Reduced from 10 to 8
    matricesLayout->setContentsMargins(8, 8, 8, 8);  // Reduced from 10 to 8
    
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
    
    mainLayout->addLayout(contentLayout, 1);  // Give content layout stretch factor
    
    mainLayout->addSpacing(5);
    
    // Bottom: Submit section (compact)
    statusLabel = new QLabel("Select a cell from YOUR matrix to submit!", this);
    statusLabel->setAlignment(Qt::AlignCenter);
    QFont statusFont = statusLabel->font();
    statusFont.setPointSize(10);
    statusLabel->setFont(statusFont);
    mainLayout->addWidget(statusLabel);
    
    submitButton = new QPushButton("Submit Answer", this);
    submitButton->setEnabled(false);
    submitButton->setMinimumHeight(40);  // Reduced from 50 to 40
    submitButton->setMaximumHeight(40);
    QFont submitFont = submitButton->font();
    submitFont.setPointSize(12);  // Reduced from 14 to 12
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
    // Update round info
    roundLabel->setText(QString("Round %1/%2").arg(data.currentRound).arg(data.totalRounds));
    
    // Update equation (center, no "Equation:" prefix)
    equationLabel->setText(data.equation);
    
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
    chatDisplay->append(QString("<b>Round %1/%2 started! Work together to solve the puzzle!</b>")
                        .arg(data.currentRound).arg(data.totalRounds));
    
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
    
    // Confirm submission (with option to change)
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Confirm Submission",
        QString("Submit cell [%1, %2] from your matrix?\n\nYou can change your answer anytime before all players submit.").arg(row).arg(col),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        networkManager->sendSubmit(row, col);
        // Keep submit button enabled so player can change their answer
        statusLabel->setText(QString("Answer submitted: Cell [%1, %2]. You can still change your selection!").arg(row).arg(col));
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


