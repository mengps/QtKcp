#ifndef QKCPSOCKET_P_H
#define QKCPSOCKET_P_H

#include "ikcp.h"
#include "qkcpsocket.h"

#include <QBasicTimer>
#include <QDateTime>
#include <QNetworkDatagram>
#include <QUdpSocket>

class QKcpSocketPrivate
{
public:
    Q_DECLARE_PUBLIC(QKcpSocket);

    QKcpSocketPrivate(QKcpSocket *q) : q_ptr(q) { }

    static int udp_output(const char *buf, int len, struct IKCPCB *kcp, void *user)
    {
        Q_UNUSED(kcp);

        QKcpSocketPrivate *_this = reinterpret_cast<QKcpSocketPrivate *>(user);

        int ret = _this->m_udpSocket->writeDatagram(buf, len, _this->m_connectionAddress, _this->m_connectionPort);

        /*! Windows ret 有时会为 -1, 但不能作为错误处理 */
        if (ret < 0) {
            /*if (_this->m_isConnceted) {
                _this->m_isConnceted = false;
                emit _this->q_ptr->disconnected();
            }*/
        }

        return ret;
    }

    void _create_kcp()
    {
        m_kcpContext = ikcp_create(m_kcpConv, this);
        m_kcpContext->output = QKcpSocketPrivate::udp_output;
        //m_kcpContext->writelog = kcp_log;
        m_kcpContext->logmask = IKCP_LOG_OUTPUT
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

        ikcp_wndsize(m_kcpContext, 128, 128);

        ikcp_setmtu(m_kcpContext, 1400);

        m_kcpMaxSendSize = m_kcpContext->mtu * (m_kcpContext->snd_wnd - 4);

        if (m_mode == QKcpSocket::Mode::Default) {
            ikcp_nodelay(m_kcpContext, 0, 10, 0, 0);
        } else if (m_mode == QKcpSocket::Mode::Normal) {
            ikcp_nodelay(m_kcpContext, 0, 10, 0, 1);
        } else {
            /**
             * Fast mode
             * 第二个参数 nodelay 启用以后若干常规加速将启动
             * 第三个参数 interval 为内部处理时钟，默认设置为 10ms
             * 第四个参数 resend 为快速重传指标，设置为2
             * 第五个参数 nc 为是否禁用常规流控，这里禁止
             */
            ikcp_nodelay(m_kcpContext, 0, 10, 2, 1);
            m_kcpContext->rx_minrto = 10;
            m_kcpContext->fastresend = 1;
        }
    }

    void _release_kcp()
    {
        if (m_kcpContext) {
            auto releaseCtx = m_kcpContext;
            m_kcpContext = nullptr;
            ikcp_release(releaseCtx);
        }
    }

    int _kcp_input(const char *data, long size)
    {
        int ret = ikcp_input(m_kcpContext, data, size);
        _kcp_update();

        return ret;
    }

    int _kcp_send(const char *data, long size)
    {
        int ret = ikcp_send(m_kcpContext, data, size);
        _kcp_update();

        return ret;
    }

    void _kcp_update()
    {
        Q_Q(QKcpSocket);

        if (!m_kcpContext) return;

        auto cur_time = QDateTime::currentMSecsSinceEpoch();
        ikcp_update(m_kcpContext, cur_time);
        int peek_size = ikcp_peeksize(m_kcpContext);
        if (peek_size > 0) {
            m_bytesAvailable = peek_size;
            emit q->QIODevice::readyRead();
        }

        if ((cur_time - m_lastRecvTime) > KCP_CONNECTION_TIMEOUT_TIME) {
            if (m_isConnceted) {
                m_isConnceted = false;
                emit q->disconnected();
            }
        }
    }

    void _read_udp_data()
    {
        Q_Q(QKcpSocket);

        while (m_udpSocket->hasPendingDatagrams()) {
            const auto datagram = m_udpSocket->receiveDatagram();
            if (!datagram.isValid()) return;

            /*! 连接后需更改源地址端口 */
            if (m_firstChange) {
                m_connectionAddress = datagram.senderAddress();
                m_connectionPort = datagram.senderPort();
                m_firstChange = false;
                m_isConnceted = true;
                emit q->connected();
            }
            const auto data = datagram.data();
            m_lastRecvTime = QDateTime::currentMSecsSinceEpoch();
            _kcp_input(data.constData(), data.size());
        }
    }

    QKcpSocket *q_ptr = nullptr;

    quint64 m_bytesAvailable = 0;
    bool m_firstChange = true;
    bool m_isBinded = false;
    bool m_isConnceted = false;
    QHostAddress m_hostAddress, m_connectionAddress;
    quint16 m_hostPort, m_connectionPort;
    ikcpcb *m_kcpContext = nullptr;
    quint32 m_kcpMaxSendSize = 0;
    quint32 m_kcpConv = 1000;
    QUdpSocket *m_udpSocket = nullptr;
    QBasicTimer m_updateTimer;
    quint64 m_lastRecvTime;
    QKcpSocket::Mode m_mode = QKcpSocket::Mode::Default;
};

#endif // QKCPSOCKET_P_H
