/*
  This example program provides a trivial server program that listens for TCP
  connections on port 9995.  When they arrive, it writes a short message to
  each client connection, and closes each connection once it is flushed.

  Where possible, it exits cleanly in response to a SIGINT (ctrl-c).
*/


#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#ifndef _WIN32
#include <netinet/in.h>
# ifdef _XOPEN_SOURCE_EXTENDED
#  include <arpa/inet.h>
# endif
#include <sys/socket.h>
#endif

#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>

static const char MESSAGE[] = "Hello, World! ";
const char *message2;

static int PORT = 9995;

static void listener_cb(struct evconnlistener *, evutil_socket_t,
    struct sockaddr *, int socklen, void *);
static void conn_readcb(struct bufferevent *, void *);
static void conn_writecb(struct bufferevent *, void *);
static void conn_eventcb(struct bufferevent *, short, void *);
static void signal_cb(evutil_socket_t, short, void *);

int
main(int argc, char **argv)
{
    struct event_base *base;
    // struct evconnlistener *listener;
    struct event *signal_event;

    struct sockaddr_in sin = {0};
#ifdef _WIN32
    WSADATA wsa_data;
    WSAStartup(0x0201, &wsa_data);
#endif

    if (argc != 4) {
        printf("Usage: client <ip> <port> <msg>\n");
        return 1;
    }

    base = event_base_new();
    if (!base) {
        fprintf(stderr, "Could not initialize libevent!\n");
        return 1;
    }

    const char *host = argv[1];
    const int port = atoi(argv[2]);
    message2 = argv[3];

    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    inet_aton(host, (struct in_addr*)&sin.sin_addr.s_addr);

    // listener = evconnlistener_new_bind(base, listener_cb, (void *)base,
    //     LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE, -1,
    //     (struct sockaddr*)&sin,
    //     sizeof(sin));

    // if (!listener) {
    //     fprintf(stderr, "Could not create a listener!\n");
    //     return 1;
    // }

    struct bufferevent *    bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
    if (!bev) {
        fprintf(stderr, "Error constructing bufferevent!\n");
        event_base_loopbreak(base);
        return 1;
    }

    bufferevent_setcb(bev, conn_readcb, conn_writecb, conn_eventcb, NULL);

    if (bufferevent_socket_connect(
        bev, (struct sockaddr *)&sin, sizeof(sin)) < 0)
    {
        bufferevent_free(bev);
        printf("bufferevent_socket_connect failed\n");
        return 1;
    }

    bufferevent_enable(bev, EV_WRITE | EV_READ);

    // signal_event = evsignal_new(base, SIGINT, signal_cb, (void *)base);

    // if (!signal_event || event_add(signal_event, NULL)<0) {
    //     fprintf(stderr, "Could not create/add a signal event!\n");
    //     return 1;
    // }

    event_base_dispatch(base);

    // evconnlistener_free(listener);
    // event_free(signal_event);
    event_base_free(base);

    printf("done\n");
    return 0;
}

static void
listener_cb(struct evconnlistener *listener, evutil_socket_t fd,
    struct sockaddr *sa, int socklen, void *user_data)
{
    struct sockaddr_in *in = (struct sockaddr_in *)sa;
    printf("accept connection: %s:%u \n",
        inet_ntoa(in->sin_addr), in->sin_port);

    struct event_base *base = (event_base *)user_data;
    struct bufferevent *bev;

    bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    if (!bev) {
        fprintf(stderr, "Error constructing bufferevent!\n");
        event_base_loopbreak(base);
        return;
    }
    bufferevent_setcb(bev, conn_readcb, conn_writecb, conn_eventcb, NULL);
    bufferevent_enable(bev, EV_WRITE | EV_READ);
    bufferevent_write(bev, MESSAGE, strlen(MESSAGE)); //
}

static void
conn_readcb(struct bufferevent *bev, void *user_data)
{
    char buf[1024] = {'\0'};
    struct evbuffer *input = bufferevent_get_input(bev);
    size_t length = evbuffer_get_length(input);

    if (length > 0) {
        bufferevent_read(bev, buf, sizeof(buf) - 1);
        printf("%s\n", buf);
    }
}

static void
conn_writecb(struct bufferevent *bev, void *user_data)
{
    // struct evbuffer *output = bufferevent_get_output(bev);
    // size_t length = evbuffer_get_length(output);
    // if (length == 0) {
    //     printf("flushed answer\n");
    //     // bufferevent_free(bev);
    // }

    sleep(1); //test
    static int cnt = 1;
    char buf[1000] = {'\0'};
    snprintf(buf, sizeof(buf),"hello from client %s %d ", message2, cnt++);
    bufferevent_write(bev, buf, strlen(buf));
}

static void
conn_eventcb(struct bufferevent *bev, short events, void *user_data)
{
    int finished = 0;

    if (events & BEV_EVENT_EOF) {
        printf("Connection closed.\n");
        finished = 1;
    } else if (events & BEV_EVENT_ERROR) {
        printf("Got an error on the connection: %s\n",
            // strerror(errno));/*XXX win32*/
            evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()));
        finished = 1;
    } else if (events & BEV_EVENT_CONNECTED) {
        //send first message to trigger event loop
        const char *buf = "Hello, World! ";
        bufferevent_write(bev, buf, strlen(buf)); //
    }

    /* None of the other events can happen here, since we haven't enabled
     * timeouts */

    if (finished) {
        struct timeval delay = { 2, 0 };
        free(user_data);
        bufferevent_free(bev);
        event_base_loopexit((event_base *)user_data, &delay);
    }
}

static void
signal_cb(evutil_socket_t sig, short events, void *user_data)
{
    struct event_base *base = (event_base *)user_data;
    struct timeval delay = { 2, 0 };

    printf("Caught an interrupt signal; exiting cleanly in two seconds.\n");

    event_base_loopexit(base, &delay);
}
