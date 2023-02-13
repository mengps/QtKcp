#ifndef QKCPSOCKET_H
#define QKCPSOCKET_H

#include "qkcpsocket_global.h"

#include <QIODevice>
#include <QHostAddress>

QT_FORWARD_DECLARE_CLASS(QKcpSocketPrivate);

class QTKCP_EXPORT QKcpSocket : public QIODevice
{
    Q_OBJECT

public:
    enum class Mode
    {
        Default,
        Normal,
        Fast
    };

    explicit QKcpSocket(Mode mode = Mode::Default, QObject *parent = nullptr);
    ~QKcpSocket();

    void connectToHost(const QHostAddress &address, qint16 port);

    void disconnectToHost();

    bool isConnected() const;

    void *kcpHandle() const;

    virtual qint64 bytesAvailable() const override;

    virtual qint64 readData(char *data, qint64 maxlen) override;

    virtual qint64 writeData(const char *data, qint64 len) override;

signals:
    void connected();
    void disconnected();

private:
    Q_DISABLE_COPY(QKcpSocket)
    Q_PRIVATE_SLOT(d_func(), void _kcp_update());

    QScopedPointer<QKcpSocketPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QKcpSocket);

    friend class QKcpServer;
};


#endif // QKCPSOCKET_H
