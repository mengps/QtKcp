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

    explicit QKcpSocket(Mode mode = Mode::Default, quint32 kcpConv = 1000, QObject *parent = nullptr);
    ~QKcpSocket();

    void connectToHost(const QHostAddress &address, quint16 port);

    void disconnectFromHost();

    QHostAddress peerAddress() const;

    quint16 peerPort() const;

    bool isConnected() const;

    void *kcpHandle() const;

    int waitSentCount() const;

    virtual qint64 bytesAvailable() const override;

protected:
    virtual qint64 readData(char *data, qint64 maxlen) override;

    virtual qint64 writeData(const char *data, qint64 len) override;

    void timerEvent(QTimerEvent *event) override;

signals:
    void connected();
    void disconnected();
    void deleteFromHost(const QHostAddress &address, quint16 port, QPrivateSignal);

private:
    QKcpSocket(quint32 kcpConv, const QHostAddress &address, quint16 port);

    Q_DISABLE_COPY(QKcpSocket)
    Q_PRIVATE_SLOT(d_func(), void _kcp_update());
    Q_PRIVATE_SLOT(d_func(), void _read_udp_data());

    QScopedPointer<QKcpSocketPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QKcpSocket);

    friend class QKcpServer;
};


#endif // QKCPSOCKET_H
