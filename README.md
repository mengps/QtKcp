# QtKcp

  `QtKcp` 是基于 Qt 的 Kcp 包装库，底层使用 UDP 传输。

  它简化了 Kcp 的使用。

  `Kcp` 地址：https://github.com/skywind3000/kcp

----

### 如何构建

克隆仓库

```bash
  git clone https://github.com/mengps/QtKcp && cd QtKcp
  git submodule update --init
```
`cmake` 构建:

```cmake
  mkdir build && cd build
  cmake ..
  cmake --build .
  cmake --install
```

`qmake` 构建:

使用 `QtKcp.pro`

---

### 如何使用

Server 端：

```c++
	QKcpServer server;
	server.listen(QHostAddress::Any, 12345);
```

Client 端：

```c++
	QKcpSocket client;
	client->connectToHost(QHostAddress("127.0.0.1"), 12345);
```

其行为和接口基本与 QTcpServer / QTcpSocket 一致。

当然还远不够完善，将会根据需要添加。

----

### 注意

  为了完成 UDP -> 类 TCP 的转变，实现会稍微有点奇怪：连接的端口会在连接成功时改变。

  这是因为主机 (Server) 只负责监听连接，真正的连接将使用新的端口进行。

---

### 许可证

   使用 `MIT LICENSE`

---

### 开发环境

  Windows 11，Qt 5.15.2 / Qt 6.7.3

