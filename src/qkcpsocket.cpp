#include "qkcpsocket_p.h"

QKcpSocket::QKcpSocket(Mode mode, quint32 kcpConv, QObject *parent)
    : QIODevice(parent)
    , d_ptr(new QKcpSocketPrivate(this))
{
    Q_D(QKcpSocket);

    d->m_mode = mode;
    d->m_kcpConv = kcpConv;
    d->m_udpSocket = new QUdpSocket(this);

    d->_create_kcp();

    connect(d->m_udpSocket, SIGNAL(readyRead()), this, SLOT(_read_udp_data()));

    d->m_updateTimer.start(10, this);
}

/*! QKcpServer 使用 */
QKcpSocket::QKcpSocket(quint32 kcpConv, const QHostAddress &address, quint16 port)
    : QIODevice(nullptr)
    , d_ptr(new QKcpSocketPrivate(this))
{
    Q_D(QKcpSocket);

    d->m_mode = Mode::Fast;
    d->m_kcpConv = kcpConv;
    d->m_udpSocket = new QUdpSocket(this);
    d->_create_kcp();

    connect(d->m_udpSocket, &QUdpSocket::readyRead, this, [d]{
        while (d->m_udpSocket->hasPendingDatagrams()) {
            const auto datagram = d->m_udpSocket->receiveDatagram();
            if (!datagram.isValid()) return;
            const auto data = datagram.data();
            d->m_lastRecvTime = QDateTime::currentMSecsSinceEpoch();
            d->_kcp_input(data.constData(), data.size());
        }
    });

    connectToHost(address, port);

    d->m_isConnceted = true;
}

QKcpSocket::~QKcpSocket()
{
    Q_D(QKcpSocket);

    if (d->m_updateTimer.isActive())  d->m_updateTimer.stop();

    disconnectFromHost();

    d->_release_kcp();

    emit deleteFromHost(d->m_connectionAddress, d->m_connectionPort, QPrivateSignal{});
}

void QKcpSocket::connectToHost(const QHostAddress &address, quint16 port)
{
    Q_D(QKcpSocket);

    QIODevice::close();
    d->m_udpSocket->close();

    d->_release_kcp();

    d->m_hostAddress = d->m_connectionAddress = address;
    d->m_hostPort = d->m_connectionPort = port;
    d->m_firstChange = true;
    d->m_isConnceted = false;

    d->m_isBinded = d->m_udpSocket->bind(QHostAddress::Any, 0, QAbstractSocket::ShareAddress | QAbstractSocket::ReuseAddressHint);

    if (d->m_isBinded) {
        QIODevice::open(QIODevice::ReadWrite);
        d->m_lastRecvTime = QDateTime::currentMSecsSinceEpoch();

        d->_create_kcp();
    }
}

void QKcpSocket::disconnectFromHost()
{
    Q_D(QKcpSocket);

    if (d->m_isBinded) {
        d->m_isBinded = false;
        QIODevice::close();
        d->m_udpSocket->close();
        d->_release_kcp();

        if (d->m_isConnceted) {
            d->m_isConnceted = false;
            emit disconnected();
        }
    }
}

QHostAddress QKcpSocket::peerAddress() const
{
    Q_D(const QKcpSocket);

    return d->m_connectionAddress;
}

quint16 QKcpSocket::peerPort() const
{
    Q_D(const QKcpSocket);

    return d->m_connectionPort;
}

bool QKcpSocket::isConnected() const
{
    Q_D(const QKcpSocket);

    return d->m_isConnceted;
}

void *QKcpSocket::kcpHandle() const
{
    Q_D(const QKcpSocket);

    return d->m_kcpContext;
}

int QKcpSocket::waitSentCount() const
{
    Q_D(const QKcpSocket);

    if (d->m_kcpContext)
        return ikcp_waitsnd(d->m_kcpContext);
    else return 0;
}

qint64 QKcpSocket::bytesAvailable() const
{
    Q_D(const QKcpSocket);

    return d->m_bytesAvailable;
}

qint64 QKcpSocket::readData(char *data, qint64 maxlen)
{
    Q_D(QKcpSocket);

    int ret = ikcp_recv(d->m_kcpContext, data, maxlen);
    if (ret > 0)
        d->m_bytesAvailable -= ret;

    return ret;
}

qint64 QKcpSocket::writeData(const char *data, qint64 len)
{
    Q_D(QKcpSocket);

    int ret;
    if (len > d->m_kcpMaxSendSize) {
        /*! KCP 内部限制一次了可发送最大大小, 需要分包 */
        int count = len / d->m_kcpMaxSendSize;
        for (int i = 0; i < count; i++) {
            ret = d->_kcp_send(data + i * d->m_kcpMaxSendSize, d->m_kcpMaxSendSize);
            if (ret != 0)
                return ret;
        }
        ret = d->_kcp_send(data + count * d->m_kcpMaxSendSize, len - count * d->m_kcpMaxSendSize);
    } else {
        ret = d->_kcp_send(data, len);
    }

    return ret == 0 ? len : ret;
}

void QKcpSocket::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event);
    Q_D(QKcpSocket);

    d->_kcp_update();
}

#include "moc_qkcpsocket.cpp"
