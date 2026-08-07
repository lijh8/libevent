// $ sudo apt-get install libevent-dev
// $ g++ -g server.cpp -o server -levent -std=c++11
// $ ./server 12345

#include <iostream>
#include <string>
#include <cstring>
#include <cerrno>
#include <unordered_map>
#include <stdexcept>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <event2/listener.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/util.h>

std::unordered_map<int, struct event *> g_timers;
std::unordered_map<int, int> g_seq;
std::unordered_map<int, struct bufferevent *> g_bevs;

void sigint_cb(evutil_socket_t fd, short what, void *arg)
{
    struct event_base *base = static_cast<struct event_base *>(arg);
    event_base_loopexit(base, NULL);
}

void read_cb(struct bufferevent *bev, void *ctx)
{
    struct evbuffer *input = bufferevent_get_input(bev);
    char *line;
    size_t len;

    while ((line = evbuffer_readln(input, &len, EVBUFFER_EOL_LF)) != NULL)
    {
        printf("Received from client: %.*s\n", (int)len, line);
        free(line);
    }
}

void timer_cb(evutil_socket_t fd, short event, void *arg)
{
    struct bufferevent *bev = static_cast<struct bufferevent *>(arg);
    int conn_fd = bufferevent_getfd(bev);
    if (conn_fd < 0)
    {
        return;
    }

    g_seq[conn_fd]++;
    std::string msg = "hello from server " + std::to_string(g_seq[conn_fd]) + "\n";
    bufferevent_write(bev, msg.c_str(), msg.length());
}

void event_cb(struct bufferevent *bev, short events, void *ctx)
{
    int conn_fd = bufferevent_getfd(bev);

    if (events & BEV_EVENT_EOF)
    {
        std::cout << "Client connection closed (fd=" << conn_fd << ")" << std::endl;
    }
    else if (events & BEV_EVENT_ERROR)
    {
        int err = EVUTIL_SOCKET_ERROR();
        std::cerr << "Client connection error (fd=" << conn_fd << "): "
                  << evutil_socket_error_to_string(err) << std::endl;
    }
    else if (events & BEV_EVENT_TIMEOUT)
    {
        std::cout << "Client connection timeout (fd=" << conn_fd << ")" << std::endl;
    }
    else
    {
        return;
    }

    auto it = g_timers.find(conn_fd);
    if (it != g_timers.end())
    {
        event_free(it->second);
        g_timers.erase(it);
    }

    bufferevent_free(bev);
    g_seq.erase(conn_fd);
    g_bevs.erase(conn_fd);
}

void accept_cb(struct evconnlistener *listener, evutil_socket_t fd,
               struct sockaddr *addr, int socklen, void *ctx)
{
    struct event_base *base = static_cast<struct event_base *>(ctx);

    struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    if (!bev)
    {
        std::cerr << "Failed to create bufferevent!" << std::endl;
        close(fd);
        return;
    }

    bufferevent_setcb(bev, read_cb, nullptr, event_cb, bev);
    bufferevent_enable(bev, EV_READ);

    struct event *timer_ev = event_new(base, -1, EV_PERSIST, timer_cb, bev);
    struct timeval tv = {1, 0};
    event_add(timer_ev, &tv);

    g_timers[fd] = timer_ev;
    g_seq[fd] = 0;
    g_bevs[fd] = bev;

    struct sockaddr_in *peer_addr = (struct sockaddr_in *)addr;
    int peer_port = ntohs(peer_addr->sin_port);
    char peer_ip[INET_ADDRSTRLEN];
    evutil_inet_ntop(AF_INET, &(peer_addr->sin_addr), peer_ip, INET_ADDRSTRLEN);

    std::cout << "New client connected, fd=" << fd
              << ", peer=" << peer_ip << ":" << peer_port << std::endl;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return 1;
    }

    int port = std::stoi(argv[1]);

    struct event_base *base = event_base_new();

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = htonl(INADDR_ANY);

    struct evconnlistener *listener = evconnlistener_new_bind(
        base, accept_cb, base,
        LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE,
        -1, (struct sockaddr *)&sin, sizeof(sin));

    if (!listener)
    {
        std::cerr << "Failed to create listener: " << strerror(errno) << std::endl;
        event_base_free(base);
        return 1;
    }

    struct event *signal_ev = evsignal_new(base, SIGINT, sigint_cb, base);
    event_add(signal_ev, NULL);

    std::cout << "Server started, listening on port " << port << "..." << std::endl;

    event_base_dispatch(base);

    event_free(signal_ev);

    for (auto &p : g_timers)
    {
        event_free(p.second);
    }

    for (auto &p : g_bevs)
    {
        bufferevent_free(p.second);
    }

    evconnlistener_free(listener);
    event_base_free(base);
    return 0;
}
