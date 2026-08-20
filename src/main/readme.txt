Install:

$ sudo apt-get install libevent-dev


Makefile:

LDLIBS += -levent


---


libevent 采用的是 Reactor（反应器）模式。在这种模式下，当 I/O 事件（可读、可写、超时）发生时，
libevent 通过回调函数通知用户"状态已就绪（readiness）"——即"现在可以读了"或"现在可以写了"。
用户收到通知后，需要在自己的回调函数中主动调用读写函数
（如 bufferevent_read, bufferevent_write, evbuffer_readln 等）来完成实际的数据传输。

这与 Proactor（前摄器）模式 不同。以 C++ 的 boost::asio 为例，
Proactor 模式通知的是"读写操作已完成（completion）"——回调触发时，
系统已经把数据读到了指定缓冲区，或者已经把数据写出去了。


---


服务端和客服端创建事件循环调度器 event_base;
服务端监听并在回调中接受(accept)连接请求 evconnlistener_new_bind,
客服端发起连接请求 bufferevent_socket_connect;
双方调用 bufferevent_socket_new 创建 bufferevent;
双方启动事件循环 event_base_dispatch;


---


数据读写操作：
只要事件循环已启动， bufferevent 对象有效，就可以在的任何位置进行数据读写：
 - 回调函数内部，比如 read_cb, event_cb 回调；
 - evtimer_new 或 event_new 注册的 timer_cb 回调；
 - 任意业务代码中。


---


write_cb:

bufferevent_setcb(bev, read_cb, NULL, event_cb, NULL);
bufferevent_setcb 通常不注册第3个参数 write_cb;
因为 write_cb 在输出缓冲区可写时就会触发，在这里写业务数据可能会过于频繁。
只要事件循环已启动，bufferevent 有效，我们就能在业务需要的地方，
主动调用 bufferevent_write 来写数据。
write_cb 通常只用于做背压 backpressure 测试。


---
