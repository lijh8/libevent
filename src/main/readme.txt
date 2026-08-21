Install:

$ sudo apt-get install libevent-dev


Makefile:

LDLIBS += -levent


---


libevent 采用的是 Reactor (反应器) 模式. 在这种模式下, 当 I/O 事件 (可读、可写、超时) 发生时,
libevent 通过回调函数通知用户"状态已就绪 (readiness) ", 即"现在可以读了"或"现在可以写了".
用户收到通知后, 需要在自己的回调函数中主动调用读写函数
 (如 bufferevent_read, bufferevent_write, evbuffer_readln 等) 来完成实际的数据传输.

这与 Proactor (前摄器) 模式 不同. 以 C++ 的 boost::asio 为例,
Proactor 模式通知的是"读写操作已完成 (completion) ", 回调触发时,
系统已经把数据读到了指定缓冲区, 或者已经把数据写出去了.


---


服务端流程:
1.  创建事件循环: event_base_new();
2.  创建监听器: evconnlistener_new_bind(base, accept_cb, ...);
3.  在 accept_cb 中 (fd 已由 libevent accept) :
    -  3.1  创建 bufferevent: bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    -  3.2  设置回调: bufferevent_setcb(bev, read_cb, NULL, event_cb, arg);
    -  3.3  启用事件: bufferevent_enable(bev, EV_READ);
4.  启动事件循环: event_base_dispatch(base);
5.  退出后释放资源: evconnlistener_free(listener); event_base_free(base);

客户端流程:
1.  创建事件循环: event_base_new();
2.  创建 bufferevent (fd 传 -1) : bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
3.  设置回调: bufferevent_setcb(bev, read_cb, NULL, event_cb, arg);
4.  启用事件: bufferevent_enable(bev, EV_READ);
5.  发起连接: bufferevent_socket_connect(bev, addr, addrlen);
6.  启动事件循环: event_base_dispatch(base);
7.  退出后释放资源: bufferevent_free(bev); event_base_free(base);

数据读写:
写数据: 可以随时调用 bufferevent_write(), 数据写入 output buffer, libevent 自动在 socket 可写时发送.
读数据: 通常在 read_cb 中调用 bufferevent_read() 或 evbuffer_readln() 消费 input buffer.


---


只要服务端和客户端已连接，事件循环已启动,  bufferevent 对象有效,
就可以在任意代码位置进行灵活的数据读写操作，比如：
 - 回调函数内部, 比如 read_cb, event_cb 回调;
 - evtimer_new 或 event_new 注册的 timer_cb 回调;
 - 任意业务代码中;


---


通常不注册 write_cb:

bufferevent_setcb(bev, read_cb, NULL, event_cb, NULL);
bufferevent_setcb 通常不注册第3个参数 write_cb;
因为 write_cb 在输出缓冲区可写时就会触发, 在这里写业务数据可能会过于频繁.
只要事件循环已启动, bufferevent 有效, 我们就能在业务需要的地方,
主动调用 bufferevent_write 来写数据.
write_cb 通常只用于做背压 backpressure 测试.


---
