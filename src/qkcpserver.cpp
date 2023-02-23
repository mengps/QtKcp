#include "qkcpserver.h"
#include "qkcpsocket.h"
#include "qkcpsocket_p.h"

#include <QDebug>
#include <QQueue>
#include <QMutex>

static void registerAllType()
{
    static bool registered = false;
    if (!registered) {
        qRegisterMetaType<QHostAddress>("QHostAddress");
        registered = true;
    }
}

class QKcpServerPrivate
{
public:
    bool m_isListening = false;
    QUdpSocket *m_udpSocket = nullptr;
    QQueue<QKcpSocket *> m_pendingQueue;
    QMutex m_clientsMutex;
    QHash<QPair<QHostAddress, quint16>, QKcpSocket *> m_clients;
    QBasicTimer m_updateTimer;
};

QKcpServer::QKcpServer(QObject *parent)
    : QObject{parent}
    , d_ptr(new QKcpServerPrivate)
{
    Q_D(QKcpServer);

    registerAllType();

    d->m_udpSocket = new QUdpSocket(this);
    d->m_updateTimer.start(10, this);
    connect(d->m_udpSocket, &QUdpSocket::readyRead, this, [this]{
        Q_D(QKcpServer);
        while (d->m_udpSocket->hasPendingDatagrams()) {
            auto datagram = d->m_udpSocket->receiveDatagram();
            if (!datagram.isValid()) return;

            auto data = datagram.data();
            auto address = datagram.senderAddress();
            auto port = datagram.senderPort();
            auto kcpConv = ikcp_getconv(data.constData());
            auto key = qMakePair<QHostAddress, quint16>(datagram.senderAddress(), datagram.senderPort());

            if (d->m_clients.contains(key)) {
                /*! TODO */
            } else {
                QKcpSocket *socket = new QKcpSocket(kcpConv, address, port);
                connect(socket, &QKcpSocket::deleteFromHost, this, [d](const QHostAddress &address, quint16 port){
                    QMutexLocker locker(&d->m_clientsMutex);
                    d->m_clients.remove(qMakePair<QHostAddress, quint16>(address, port));
                    qDebug() << "deleteFromHost" << qMakePair<QHostAddress, quint16>(address, port)
                             << Qt::endl << d->m_clients.keys();
                }, Qt::DirectConnection);
                if (socket->d_func()->m_kcpContext) {
                    d->m_clientsMutex.lock();
                    d->m_clients[key] = socket;
                    d->m_clientsMutex.unlock();
                    d->m_pendingQueue.enqueue(socket);
                    ikcp_input(socket->d_func()->m_kcpContext, data.constData(), data.size());

                    emit newConnection();
                } else {
                    qWarning() << "The client connection failed:" << socket->errorString();
                    socket->deleteLater();
                }
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

void QKcpServer::timerEvent(QTimerEvent *)
{
    Q_D(QKcpServer);

    /*! 统一更新 KCP */
    QMutexLocker locker(&d->m_clientsMutex);
    for (auto &client: d->m_clients) {
        QMetaObject::invokeMethod(client, "_kcp_update", Qt::QueuedConnection);
    }
}
