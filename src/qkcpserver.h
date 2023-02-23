#ifndef QKCPSERVER_H
#define QKCPSERVER_H

#include "qkcpsocket_global.h"

#include <QObject>
#include <QHostAddress>

QT_FORWARD_DECLARE_CLASS(QKcpSocket);
QT_FORWARD_DECLARE_CLASS(QKcpServerPrivate);

class QTKCP_EXPORT QKcpServer : public QObject
{
    Q_OBJECT

public:
    explicit QKcpServer(QObject *parent = nullptr);
    ~QKcpServer();

    bool hasPendingConnections() const;

    QKcpSocket *nextPendingConnection();

    void listen(const QHostAddress &address, quint16 port);

    bool isListening() const;

    void close();

signals:
    void newConnection();

protected:
    void timerEvent(QTimerEvent *) override;

private:
    Q_DISABLE_COPY(QKcpServer);

    QScopedPointer<QKcpServerPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QKcpServer);
};

#endif // QKCPSERVER_H
