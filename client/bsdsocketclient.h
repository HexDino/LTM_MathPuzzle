#ifndef BSDSOCKETCLIENT_H
#define BSDSOCKETCLIENT_H

#include <QObject>
#include <QSocketNotifier>
#include <QDebug>

// --- PHẦN SỬA ĐỔI ĐỂ HỖ TRỢ ĐA NỀN TẢNG ---
#ifdef _WIN32 // Nếu là Windows
#include <winsock2.h>
#include <ws2tcpip.h>
#define CLOSE_SOCKET closesocket
#define IS_INVALID_SOCKET(s) ((s) == INVALID_SOCKET)
typedef int socklen_t; // Windows dùng int cho độ dài addr
#else // Nếu là Linux/macOS
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>
#define CLOSE_SOCKET close
#define IS_INVALID_SOCKET(s) ((s) < 0)
#define SOCKET int
#define INVALID_SOCKET -1
#endif
// ------------------------------------------

class BSDSocketClient : public QObject
{
    Q_OBJECT
public:
    explicit BSDSocketClient(QObject *parent = nullptr);
    ~BSDSocketClient();

    //Ham ket noi thay the connectToHost
    bool connectServer(const QString &ip, int port);

    // Hàm gửi dữ liệu thay thế cho write()
    void sendData(const QByteArray &data);

    // Đóng kết nối
    void closeConnection();

private:
    SOCKET m_sockfd; // File descriptor của C Socket
    QSocketNotifier *m_notifier; // Cầu nối giữa Qt và C Socket

    // Hàm hỗ trợ set non-blocking
    void setNonBlocking(SOCKET fd);


signals:

    // Signal để báo cho UI biết có dữ liệu (thay cho readyRead)
    void dataReceived(QByteArray data);

    // Signal báo kết nối thành công/thất bại
    void connected();
    void disconnected();


private slots:
    // Slot nội bộ để xử lý khi socket có tín hiệu từ OS
    void onSocketReadActivated(int socket);

};

#endif // BSDSOCKETCLIENT_H
