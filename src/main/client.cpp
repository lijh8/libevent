#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <arpa/inet.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>

#define SERVER_IP "192.168.1.16"
#define SERVER_PORT 8888

static void sigint_cb(evutil_socket_t fd, short what, void *arg)
{
    struct event_base *base = (struct event_base *)arg;
    printf("Quitting on SIGINT ...\n");
    event_base_loopbreak(base);
}

static void read_cb(struct bufferevent *bev, void *ctx)
{
    struct event_base *base = bufferevent_get_base(bev);
    struct evbuffer *input = bufferevent_get_input(bev);
    char *line;
    size_t len;

    while ((line = evbuffer_readln(input, &len, EVBUFFER_EOL_LF)) != NULL)
    {
        printf("Received: %.*s\n", (int)len, line);
        free(line);
        event_base_loopbreak(base); // break here for test only
    }
}

static void event_cb(struct bufferevent *bev, short events, void *ctx)
{
    struct event_base *base = bufferevent_get_base(bev);
    int fd = bufferevent_getfd(bev);

    if (events & BEV_EVENT_CONNECTED)
    {
        printf("Connected to server\n");
        const char *msg = "hello from client\n";
        bufferevent_write(bev, msg, strlen(msg));
    }
    else if (events & BEV_EVENT_ERROR)
    {
        int err = EVUTIL_SOCKET_ERROR();
        fprintf(stderr, "Connection error, fd=%d: %s\n", fd, evutil_socket_error_to_string(err));
        event_base_loopbreak(base);
    }
    else if (events & BEV_EVENT_EOF)
    {
        printf("Connection closed, fd=%d\n", fd);
        event_base_loopbreak(base);
    }
    else if (events & BEV_EVENT_TIMEOUT)
    {
        printf("Connection timeout, fd=%d\n", fd);
        event_base_loopbreak(base);
    }
}

int main(int argc, char **argv)
{
    struct sockaddr_in sin = {0};
    sin.sin_family = AF_INET;
    sin.sin_port = htons(SERVER_PORT);

    if (evutil_inet_pton(AF_INET, SERVER_IP, &sin.sin_addr) <= 0)
    {
        fprintf(stderr, "Invalid server IP address: %s\n", SERVER_IP);
        return 1;
    }

    struct event_base *base = event_base_new();
    struct bufferevent *bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);

    bufferevent_setcb(bev, read_cb, NULL, event_cb, NULL);
    bufferevent_enable(bev, EV_READ);

    if (bufferevent_socket_connect(bev, (struct sockaddr *)&sin, sizeof(sin)) < 0)
    {
        int err = EVUTIL_SOCKET_ERROR();
        fprintf(stderr, "Connect error: %s\n", evutil_socket_error_to_string(err));
        bufferevent_free(bev);
        event_base_free(base);
        return 1;
    }
    printf("Connecting to server %s:%d ...\n", SERVER_IP, SERVER_PORT);

    struct event *signal_ev = evsignal_new(base, SIGINT, sigint_cb, base);
    event_add(signal_ev, NULL);

    event_base_dispatch(base);
    event_free(signal_ev);
    bufferevent_free(bev);
    event_base_free(base);

    return 0;
}
