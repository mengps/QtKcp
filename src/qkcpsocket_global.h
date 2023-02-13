#ifndef QKCPSOCKET_GLOBAL_H
#define QKCPSOCKET_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(QTKCP_LIBRARY)
#  define QTKCP_EXPORT Q_DECL_EXPORT
#else
#  define QTKCP_EXPORT Q_DECL_IMPORT
#endif

#define KCP_CONNECTION_TIMEOUT_TIME 5 * 1000

#endif // QKCPSOCKET_GLOBAL_H
