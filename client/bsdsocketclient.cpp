#include "bsdsocketclient.h"

BSDSocketClient::BSDSocketClient(QObject *parent)
    : QObject{parent},
    m_sockfd(-1),
    m_notifier(nullptr)
{
// Windows bắt buộc phải khởi động Winsock trước khi dùng bất cứ lệnh socket nào
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        qDebug() << "WSAStartup failed";
    }
#endif

}
BSDSocketClient::~BSDSocketClient()
{
    closeConnection();
#ifdef _WIN32
    WSACleanup();
#endif
}

void BSDSocketClient::setNonBlocking(int fd)
{
#ifdef _WIN32
    // Cách set Non-blocking trên Windows
    u_long mode = 1;
    ioctlsocket(fd, FIONBIO, &mode);
#else
    // Cách set Non-blocking trên Linux
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return;
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#endif
}

bool BSDSocketClient::connectServer(const QString &host, int port)
{
    // Tạo socket
    m_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (IS_INVALID_SOCKET(m_sockfd)) {
        qDebug() << "Không thể tạo socket";
        return false;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // Xử lý Hostname / DNS (Code dùng chung cho cả 2 OS)
    std::string hostStr = host.toStdString();
    struct hostent *server = gethostbyname(hostStr.c_str());

    if (server == NULL) {
        if (inet_pton(AF_INET, hostStr.c_str(), &server_addr.sin_addr) <= 0) {
            qDebug() << "Địa chỉ không hợp lệ";
            CLOSE_SOCKET(m_sockfd);
            return false;
        }
    } else {
        memcpy((char *)&server_addr.sin_addr.s_addr, (char *)server->h_addr, server->h_length);
    }

    // Connect
    if (::connect(m_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        // Trên Windows connect non-blocking có thể trả về lỗi ngay lập tức, cần kiểm tra kỹ hơn
        // Nhưng để đơn giản, ta chỉ báo lỗi nếu thực sự fail
#ifdef _WIN32
        int err = WSAGetLastError();
        if (err != WSAEWOULDBLOCK) { // Nếu không phải lỗi "đang xử lý"
            qDebug() << "Kết nối thất bại. Error Code:" << err;
            CLOSE_SOCKET(m_sockfd);
            return false;
        }
#else
        qDebug() << "Kết nối thất bại: " << strerror(errno);
        CLOSE_SOCKET(m_sockfd);
        return false;
#endif
    }

    setNonBlocking(m_sockfd);

    // QSocketNotifier hoạt động tốt với cả SOCKET của Windows
    m_notifier = new QSocketNotifier(m_sockfd, QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, &BSDSocketClient::onSocketReadActivated);

    emit connected();
    return true;
}


void BSDSocketClient::onSocketReadActivated(int socket)
{
    char buffer[4096];
    // recv trả về int trên cả 2 OS
    int bytesRead = recv(m_sockfd, buffer, sizeof(buffer), 0);

    if (bytesRead > 0) {
        QByteArray data(buffer, bytesRead);
        emit dataReceived(data);
    } else if (bytesRead == 0) {
        qDebug() << "Server ngắt kết nối";
        closeConnection();
        emit disconnected();
    } else {
        // Xử lý lỗi đọc
#ifdef _WIN32
        int err = WSAGetLastError();
        if (err != WSAEWOULDBLOCK) {
            closeConnection();
        }
#else
        if (errno != EWOULDBLOCK && errno != EAGAIN) {
            closeConnection();
        }
#endif
    }
}

void BSDSocketClient::sendData(const QByteArray &data)
{
    if (IS_INVALID_SOCKET(m_sockfd)) return;

    int bytesSent = ::send(m_sockfd, data.constData(), data.size(), 0);
    if (bytesSent < 0) {
        qDebug() << "Gửi lỗi";
    }
}

void BSDSocketClient::closeConnection()
{
    if (m_notifier) {
        m_notifier->setEnabled(false);
        delete m_notifier;
        m_notifier = nullptr;
    }
    if (!IS_INVALID_SOCKET(m_sockfd)) {
        CLOSE_SOCKET(m_sockfd);
        m_sockfd = INVALID_SOCKET;
    }
}

