#ifndef QKCPSOCKET_P_H
#define QKCPSOCKET_P_H

#include "ikcp.h"
#include "qkcpsocket.h"

#include <QDateTime>
#include <QUdpSocket>
#include <QBasicTimer>

class QKcpSocketPrivate
{
public:
    Q_DECLARE_PUBLIC(QKcpSocket);

    QKcpSocketPrivate(QKcpSocket *q) : q_ptr(q) { }

    static int udp_output(const char *buf, int len, struct IKCPCB *kcp, void *user)
    {
        QKcpSocketPrivate *_this = reinterpret_cast<QKcpSocketPrivate *>(user);

        int ret = _this->m_udpSocket->writeDatagram(buf, len, _this->m_connectionAddress, _this->m_connectionPort);

        if (ret < 0) {
            if (_this->m_isConnceted) {
                _this->m_isConnceted = false;
                emit _this->q_ptr->disconnected();
            }
        }

        return ret;
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

    QKcpSocket *q_ptr = nullptr;

    quint64 m_bytesAvailable = 0;
    bool m_firstChange = true;
    bool m_isBinded = false;
    bool m_isConnceted = false;
    QHostAddress m_hostAddress, m_connectionAddress;
    quint16 m_hostPort, m_connectionPort;
    ikcpcb *m_kcpContext = nullptr;
    quint32 m_kcpConv = 0;
    quint32 m_kcpMaxSendSize = 0;
    QUdpSocket *m_udpSocket = nullptr;
    QBasicTimer m_updateTimer;
    quint64 m_lastRecvTime;
    QKcpSocket::Mode m_mode = QKcpSocket::Mode::Default;
};

#endif // QKCPSOCKET_P_H
