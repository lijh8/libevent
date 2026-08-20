#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <arpa/inet.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/listener.h>

#define PORT 8888

static void sigint_cb(evutil_socket_t fd, short what, void *arg)
{
    struct event_base *base = (struct event_base *)arg;
    printf("Quitting on SIGINT ...\n");
    event_base_loopbreak(base);
}

static void read_cb(struct bufferevent *bev, void *ctx)
{
    struct evbuffer *input = bufferevent_get_input(bev);
    char *line;
    size_t len;

    while ((line = evbuffer_readln(input, &len, EVBUFFER_EOL_LF)))
    {
        printf("Received: %.*s\n", (int)len, line);

        const char *reply = "hello from server\n";
        bufferevent_write(bev, reply, strlen(reply));

        free(line);
    }
}

static void event_cb(struct bufferevent *bev, short events, void *ctx)
{
    int fd = bufferevent_getfd(bev);

    if (events & BEV_EVENT_ERROR)
    {
        int err = EVUTIL_SOCKET_ERROR();
        fprintf(stderr, "Connection error, fd=%d: %s\n", fd,
                evutil_socket_error_to_string(err));
        bufferevent_free(bev);
    }
    else if (events & BEV_EVENT_EOF)
    {
        printf("Connection closed, fd=%d\n", fd);
        bufferevent_free(bev);
    }
    else if (events & BEV_EVENT_TIMEOUT)
    {
        printf("Connection timeout, fd=%d\n", fd);
        bufferevent_free(bev);
    }
}

static void accept_cb(struct evconnlistener *listener,
                      evutil_socket_t fd,
                      struct sockaddr *addr,
                      int socklen,
                      void *ctx)
{
    struct event_base *base = (struct event_base *)ctx;
    struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);

    bufferevent_setcb(bev, read_cb, NULL, event_cb, NULL);
    bufferevent_enable(bev, EV_READ);

    struct sockaddr_in *peer_addr = (struct sockaddr_in *)addr;
    char peer_ip[INET_ADDRSTRLEN];
    evutil_inet_ntop(AF_INET, &(peer_addr->sin_addr), peer_ip, INET_ADDRSTRLEN);
    printf("New client connected: %s:%d\n",
           peer_ip, ntohs(peer_addr->sin_port));
}

int main(int argc, char **argv)
{

    struct sockaddr_in sin = {0};
    sin.sin_family = AF_INET;
    sin.sin_port = htons(PORT);
    sin.sin_addr.s_addr = htonl(INADDR_ANY);

    struct event_base *base = event_base_new();
    struct evconnlistener *listener = evconnlistener_new_bind(
        base, accept_cb, base,
        LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE,
        -1, (struct sockaddr *)&sin, sizeof(sin));

    if (!listener)
    {
        fprintf(stderr, "Failed to create listener on port %d\n", PORT);
        event_base_free(base);
        return 1;
    }

    struct event *signal_ev = evsignal_new(base, SIGINT, sigint_cb, base);
    event_add(signal_ev, NULL);

    printf("Listening on port %d ...\n", PORT);
    event_base_dispatch(base);

    event_free(signal_ev);
    evconnlistener_free(listener);
    event_base_free(base);

    return 0;
}
