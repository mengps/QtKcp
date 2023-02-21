#include "qkcpserver.h"
#include "qkcpsocket.h"
#include "qkcpsocket_p.h"

#include <QDebug>
#include <QQueue>
#include <QNetworkDatagram>
#include <QUdpSocket>

class QKcpServerPrivate
{
public:
    bool m_isListening = false;
    QUdpSocket *m_udpSocket = nullptr;
    QQueue<QKcpSocket *> m_pendingQueue;
};

QKcpServer::QKcpServer(QObject *parent)
    : QObject{parent}
    , d_ptr(new QKcpServerPrivate)
{
    Q_D(QKcpServer);

    d->m_udpSocket = new QUdpSocket(this);
    connect(d->m_udpSocket, &QUdpSocket::readyRead, this, [this]{
        Q_D(QKcpServer);
        while (d->m_udpSocket->hasPendingDatagrams()) {
            auto datagram = d->m_udpSocket->receiveDatagram();
            if (!datagram.isValid()) return;

            auto data = datagram.data();
            auto kcpConv = ikcp_getconv(data.constData());
            auto socket = new QKcpSocket;
            socket->d_func()->m_kcpConv = kcpConv;
            socket->connectToHost(datagram.senderAddress(), datagram.senderPort());
            if (socket->d_func()->m_kcpContext) {
                ikcp_input(socket->d_func()->m_kcpContext, data.constData(), data.size());
                d->m_pendingQueue.enqueue(socket);
                emit newConnection();
            } else {
                qWarning() << "The client connection failed:" << socket->errorString();
                socket->deleteLater();
            }
        }
    });
}

QKcpServer::~QKcpServer()
{

}

bool QKcpServer::hasPendingConnections() const
{
    Q_D(const QKcpServer);

    return d->m_pendingQueue.size() > 0;
}

QKcpSocket *QKcpServer::nextPendingConnection()
{
    Q_D(QKcpServer);

    if (d->m_pendingQueue.size() > 0)
        return d->m_pendingQueue.dequeue();
    return nullptr;
}

void QKcpServer::listen(const QHostAddress &address, quint16 port)
{
    Q_D(QKcpServer);

    d->m_isListening = d->m_udpSocket->bind(address, port, QAbstractSocket::ShareAddress | QAbstractSocket::ReuseAddressHint);
    if (d->m_isListening) {
        d->m_udpSocket->open(QIODevice::ReadWrite);
    }
}

bool QKcpServer::isListening() const
{
    Q_D(const QKcpServer);

    return d->m_isListening;
}

void QKcpServer::close()
{
    Q_D(QKcpServer);

    d->m_udpSocket->close();
    d->m_isListening = false;
}
