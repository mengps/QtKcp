#include "qkcpsocket_p.h"

#include <QDateTime>
#include <QNetworkDatagram>
#include <QBasicTimer>

static quint32 get_new_conv()
{
    static uint32_t static_cur_conv = 1000;
    return ++static_cur_conv;
}

QKcpSocket::QKcpSocket(Mode mode, QObject *parent)
    : QIODevice(parent)
    , d_ptr(new QKcpSocketPrivate(this))
{
    Q_D(QKcpSocket);

    d->m_mode = mode;
    d->m_kcpConv = get_new_conv();
    d->m_udpSocket = new QUdpSocket(this);
}

QKcpSocket::~QKcpSocket()
{
    Q_D(QKcpSocket);

    d->m_udpSocket->abort();
    d->_release_kcp();
}

void QKcpSocket::connectToHost(const QHostAddress &address, qint16 port)
{
    Q_D(QKcpSocket);

    disconnectToHost();

    d->m_hostAddress = d->m_connectionAddress = address;
    d->m_hostPort = d->m_connectionPort = port;
    d->m_firstChange = true;

    d->m_isBinded = d->m_udpSocket->bind();

    if (d->m_isBinded) {
        d->m_kcpContext = ikcp_create(d->m_kcpConv, d);
        d->m_kcpContext->output = QKcpSocketPrivate::udp_output;
        /*
        d->m_kcpContext->writelog = kcp_log;
        d->m_kcpContext->logmask = IKCP_LOG_OUTPUT
                | IKCP_LOG_INPUT
                | IKCP_LOG_SEND
                | IKCP_LOG_RECV
                | IKCP_LOG_IN_DATA
                | IKCP_LOG_IN_ACK
                | IKCP_LOG_IN_PROBE
                | IKCP_LOG_IN_WINS
                | IKCP_LOG_OUT_DATA
                | IKCP_LOG_OUT_ACK
                | IKCP_LOG_OUT_PROBE
                | IKCP_LOG_OUT_WINS;
        */
        ikcp_wndsize(d->m_kcpContext, 128, 128);
        ikcp_setmtu(d->m_kcpContext, 1400);
        d->m_kcpMaxSendSize = d->m_kcpContext->mtu * (d->m_kcpContext->snd_wnd - 4);

        if (d->m_mode == Mode::Default) {
            ikcp_nodelay(d->m_kcpContext, 0, 10, 0, 0);
        } else if (d->m_mode == Mode::Normal) {
            ikcp_nodelay(d->m_kcpContext, 0, 10, 0, 1);
        } else {
            /**
             * Fast mode
             * 第二个参数 nodelay 启用以后若干常规加速将启动
             * 第三个参数 interval 为内部处理时钟，默认设置为 10ms
             * 第四个参数 resend 为快速重传指标，设置为2
             * 第五个参数 nc 为是否禁用常规流控，这里禁止
             */
            ikcp_nodelay(d->m_kcpContext, 0, 10, 2, 1);
            d->m_kcpContext->rx_minrto = 10;
            d->m_kcpContext->fastresend = 1;
        }
        d->m_lastRecvTime = QDateTime::currentMSecsSinceEpoch();

        connect(d->m_udpSocket, &QUdpSocket::readyRead, this, [this]{
            Q_D(QKcpSocket);
            while (d->m_udpSocket->hasPendingDatagrams()) {
                const auto datagram = d->m_udpSocket->receiveDatagram();
                if (!datagram.isValid()) return;

                /*! 连接后需更改源地址端口 */
                if (d->m_firstChange) {
                    d->m_connectionAddress = datagram.senderAddress();
                    d->m_connectionPort = datagram.senderPort();
                    d->m_firstChange = false;
                    d->m_isConnceted = true;
                    emit connected();
                }
                const auto data = datagram.data();
                d->m_lastRecvTime = QDateTime::currentMSecsSinceEpoch();
                d->_kcp_input(data.constData(), data.size());
            }
        });

        QIODevice::open(QIODevice::ReadWrite);
        d->m_udpSocket->open(QIODevice::ReadWrite);
        d->m_updateTimer.start(10, this);
    }
}

void QKcpSocket::disconnectToHost()
{
    Q_D(QKcpSocket);

    if (d->m_isBinded) {
        QIODevice::close();
        d->m_udpSocket->abort();
        d->_release_kcp();
        d->m_updateTimer.stop();
        disconnect(d->m_udpSocket, &QUdpSocket::readyRead, nullptr, nullptr);

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
