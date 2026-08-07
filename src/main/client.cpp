// $ sudo apt-get install libevent-dev
// $ g++ -g client.cpp -o client -levent -std=c++11
// $ ./client 192.168.1.16 12345 tom
// $ ./client 192.168.1.16 12345 jerry # another terminal

#include <iostream>
#include <string>
#include <cstring>
#include <cerrno>
#include <stdexcept>
#include <signal.h>
#include <arpa/inet.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/util.h>

struct event *g_timer_ev = nullptr;
int g_seq = 0;
std::string g_tag;

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
        printf("Received from server: %.*s\n", (int)len, line);
        free(line);
    }
}

void timer_cb(evutil_socket_t fd, short event, void *arg)
{
    struct bufferevent *bev = static_cast<struct bufferevent *>(arg);
    g_seq++;
    std::string msg = "hello from client " + g_tag + " " + std::to_string(g_seq) + "\n";
    bufferevent_write(bev, msg.c_str(), msg.length());
}

void event_cb(struct bufferevent *bev, short events, void *ctx)
{
    int conn_fd = bufferevent_getfd(bev);

    if (events & BEV_EVENT_CONNECTED)
    {
        std::cout << "Connected to server (fd=" << conn_fd << ")" << std::endl;
        g_timer_ev = event_new(bufferevent_get_base(bev), -1, EV_PERSIST, timer_cb, bev);
        struct timeval tv = {1, 0};
        event_add(g_timer_ev, &tv);
        return;
    }

    if (events & BEV_EVENT_EOF)
    {
        std::cout << "Server connection closed (fd=" << conn_fd << ")" << std::endl;
    }
    else if (events & BEV_EVENT_ERROR)
    {
        int err = EVUTIL_SOCKET_ERROR();
        std::cerr << "Server connection error (fd=" << conn_fd << "): "
                  << evutil_socket_error_to_string(err) << std::endl;
    }
    else if (events & BEV_EVENT_TIMEOUT)
    {
        std::cout << "Server connection timeout (fd=" << conn_fd << ")" << std::endl;
    }
    else
    {
        return;
    }

    struct event_base *base = bufferevent_get_base(bev);
    event_base_loopexit(base, NULL);
}

int main(int argc, char **argv)
{
    if (argc < 4)
    {
        std::cerr << "Usage: " << argv[0] << " <server_ip> <server_port> <client_tag>" << std::endl;
        return 1;
    }

    char *server_ip = argv[1];
    int server_port = std::stoi(argv[2]);
    g_tag = argv[3];

    struct event_base *base = event_base_new();

    struct bufferevent *bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
    if (!bev)
    {
        std::cerr << "Failed to create bufferevent!" << std::endl;
        event_base_free(base);
        return 1;
    }

    bufferevent_setcb(bev, read_cb, nullptr, event_cb, bev);
    bufferevent_enable(bev, EV_READ);

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(server_port);

    if (evutil_inet_pton(AF_INET, server_ip, &sin.sin_addr) <= 0)
    {
        std::cerr << "Invalid server IP address: " << server_ip << std::endl;
        bufferevent_free(bev);
        event_base_free(base);
        return 1;
    }

    if (bufferevent_socket_connect(bev, (struct sockaddr *)&sin, sizeof(sin)) < 0)
    {
        std::cerr << "Failed to initiate connection: " << strerror(errno) << std::endl;
        bufferevent_free(bev);
        event_base_free(base);
        return 1;
    }

    struct event *signal_ev = evsignal_new(base, SIGINT, sigint_cb, base);
    event_add(signal_ev, NULL);

    std::cout << "Attempting to connect to server "
              << server_ip << ":" << server_port << "..." << std::endl;

    event_base_dispatch(base);

    event_free(signal_ev);

    if (g_timer_ev)
    {
        event_free(g_timer_ev);
    }

    bufferevent_free(bev);
    event_base_free(base);
    return 0;
}
