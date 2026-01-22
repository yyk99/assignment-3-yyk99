#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <syslog.h>
#include <unistd.h> // close(), etc.
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#include <assert.h>

#define OUTPUT_FILE "/var/tmp/aesdsocketdata"
#define SERVICE "9000"
#define BACK_LOG 10

#include <arpa/inet.h>
#include <netinet/in.h>

#include <pthread.h>

#include "queue.h"


struct param_t {
    int cli_sock;
    char buf[80];
};

static char* ip2str(struct sockaddr *sa, char *s, size_t maxlen)
{
    switch(sa->sa_family) {
    case AF_INET:
        inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr),
                  s, maxlen);
        break;
    case AF_INET6:
        inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr),
                  s, maxlen);
        break;
    default:
        strncpy(s, "[n/a]", maxlen);
    }
    return s;
}

static FILE *dump;

static void on_signal(int)
{
    syslog(LOG_INFO, "Caught signal, exiting");
    if (dump) {
        fclose(dump);
    }
    exit(0);
}

static void return_dump(int client)
{
    /* fprintf(stderr, "send it back\n"); */

    fflush(dump);
    FILE *f = fopen(OUTPUT_FILE, "r");
    if (f) {
        char buf[512];
        int s;

        while ((s = fread(buf, 1, sizeof(buf), f)) > 0) {
            /* fprintf(stderr, "fread(...) == %d\n", (int)s); */
            send(client, buf, s, 0);
        }
        fclose(f);
    }
}

static void *server_function(void *p)
{
    struct param_t *ctx = (struct param_t *)p;
    if(!p)
        return p;
    char buf[80];
    do {
        ssize_t s = recv(ctx->cli_sock, buf, sizeof(buf), 0);
        if (s <= 0) {
            syslog(LOG_INFO, "Close connection from %s", ctx->buf);
            fflush(dump);
            break;
        }
        /* fprintf(stderr, "buf[s-1] == %d\n", buf[s-1] & 255); */
        fwrite(buf, 1, s, dump);
        if (buf[s-1] == '\n'){
            return_dump(ctx->cli_sock);
        }
    }while(1);

    return p;
}

int main(int argc, char **argv)
{
    int listen_sock;
    int daemon_flag = 0;
    char *me = argv[0];
    int opt;

    while ((opt = getopt(argc, argv, "dvh")) != -1) {
        switch (opt) {
        case 'd':
            daemon_flag = 1;
            break;
        case 'v':
            /* TODO: verbose */
            break;
        case 'h':
        default:
            /* usage */
            fprintf(stderr, "Usage: %s [-d][-h]\n", me);
            return -1;
        }
    }

    (void)openlog("aesdsocket", LOG_PID|LOG_PERROR, LOG_USER);

    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == -1) {
        syslog(LOG_ERR, "socket(...) failed: %d", errno);
        return -1;
    }

    struct addrinfo hints, *info = 0;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if(getaddrinfo(NULL, SERVICE, &hints, &info)) {
        /* TODO: find out the error code */
        syslog(LOG_ERR, "getaddrinfo(...) failed");
        return -1;
    }

    if (bind(listen_sock, info->ai_addr, info->ai_addrlen)) {
        syslog(LOG_ERR, "bind(...) failed: %d", errno);
        freeaddrinfo(info);
        return -1;
    }
    freeaddrinfo(info);

    dump = fopen(OUTPUT_FILE, "w");
    if (!dump) {
        syslog(LOG_ERR, "Cannot create dump file: %s", strerror(errno));
        return -1;
    }
    if (listen(listen_sock, BACK_LOG)) {
        syslog(LOG_ERR, "listen(...) failed: %d", errno);
        return -1;
    }

    if (daemon_flag) {
        int rc = fork();
        if (rc) {
            return 0;
        }
        setsid();
        syslog(LOG_INFO, "Daemon started");
    }

    do {
        struct sockaddr addr;
        socklen_t addrlen = sizeof(addr);
        memset(&addr, 0, sizeof(addr));
        int cli_sock = accept(listen_sock, &addr, &addrlen);
        if (cli_sock == -1){
            syslog(LOG_ERR, "accept(...) failed: %d", errno);
            return -1;
        }

        struct param_t *p = calloc(sizeof(struct param_t), 1);
        if (!p) {
            syslog(LOG_ERR, "Cannot allocate memory for thread param_t");
            return -1;
        }
        syslog(LOG_INFO, "Accepted connection from %s",
               ip2str(&addr, p->buf, sizeof(p->buf)));
        p->cli_sock = cli_sock;
        pthread_t t;
        if (pthread_create(&t, NULL, server_function, p)){
            syslog(LOG_ERR, "Cannot create thread");
            return -1;
        }
        void *thread_return = NULL;
        int err = pthread_join(t, &thread_return);
        if(err) {
            syslog(LOG_ERR, "pthread_join failed (%d)", err);
            return -1;
        }
        free(thread_return);
    } while(1);
    /* Hmm, how can we get here? */
    fclose(dump);
    return 0;
}
