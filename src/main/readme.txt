Install:

$ sudo apt-get install libevent-dev

Makefile:

LDLIBS += -levent

Usage:

使用 bufferevent_read、evbuffer_readln、bufferevent_write 等函数进行数据读写操作。

bufferevent_setcb(bufev, readcb, writecb, eventcb, cbarg);
读写操作可以在 bufferevent_setcb 注册的回调函数（如 readcb 或 writecb）中进行。
也可以在其它位置进行，例如在 evtimer_add、event_new、event_add 注册的回调函数中。

readcb：当 libevent 已将底层 socket fd 中的数据读到 input buffer 时，该回调被触发。
    上层应用可以从 input buffer 中读数据。

writecb：当 libevent 将 output buffer 中的数据写入底层 socket fd 后，该回调被触发。
    此时 output buffer 中有可用空间，上层应用可以向其写入新数据。
    libevent 负责将数据从 output buffer 实际写入系统 socket fd。

Reactor 模式 (e.g., libevent):
    通知 I/O 状态已就绪 (ready)，可以开始读写。

Proactor 模式 (e.g., C++ Boost.Asio):
    通知 I/O 操作已完成 (completion)，读写已经完成。
