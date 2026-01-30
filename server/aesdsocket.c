#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <syslog.h>
#include <unistd.h> // close(), etc.
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <poll.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <assert.h>

#define OUTPUT_FILE "/dev/aesdchar"
#define SERVICE "9000"
#define BACK_LOG 10

#define AESD_TAG "AESDCHAR_IOCSEEKTO:"

#include <arpa/inet.h>
#include <netinet/in.h>

#include <pthread.h>

#include "queue.h"

#include "../aesd-char-driver/aesd_ioctl.h"

static void verbose(const char *format, ...);


/* */
struct param_t {
    int cli_sock;
    int done;          /* non-zero if thread is ready to join */
    char *cli_addr;    /* ip address as text "X.X.X.X" */
    char *record;       /* current record */
    size_t record_len;
    char buf[512];      /* socket I/O buffer */
};

static void release_param(void *thread_return)
{
    /* TODO: check for THREAD_CANCEL */
    struct param_t *p = (struct param_t *)thread_return;
    if (p) {
        if (p->cli_addr)
            free(p->cli_addr);
        free(p);
    }
}

/* a "simple" implementation. Just realloc() the record */
static void append_record(struct param_t *ctx, char *data, size_t data_len)
{
    assert(data_len != 0); // DEBUG
    ctx->record = realloc(ctx->record, ctx->record_len + data_len);
    if(!ctx->record) {
        syslog(LOG_ERR, "Cannot allocate memory for a record");
        exit (-1);
    }
    memcpy(ctx->record+ctx->record_len, data, data_len);
    ctx->record_len += data_len;

    verbose("append_record: ctx->record_len = %lu, data_len = %lu", ctx->record_len, data_len);
}

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

// LIST.
typedef struct list_data_s list_data_t;
struct list_data_s {
    pthread_t tid;
    struct param_t *ctx;
    LIST_ENTRY(list_data_s) entries;
};

volatile sig_atomic_t done = 0;
static pthread_mutex_t dump_mutex = PTHREAD_MUTEX_INITIALIZER;
static int dump = -1;

int daemon_flag = 0;
int verbose_flag = 0;

static void on_signal(int)
{
    syslog(LOG_INFO, "Caught signal, exiting");
    done = 1;
}

static void return_dump(struct param_t *ctx)
{
    struct aesd_seekto req;
    bool aesd_seekto_set = false;

    assert(ctx->record);
    assert(ctx->record[ctx->record_len-1] == '\n');

    pthread_mutex_lock(&dump_mutex);
    if (ctx->record_len > strlen(AESD_TAG) && strncmp(ctx->record, AESD_TAG, strlen(AESD_TAG)) == 0) {
        char *cp;
        /* AESDCHAR_IOCSEEKTO:X,Y */
        ctx->record[ctx->record_len - 1] = 0; /* replace '\n'; just in case */
        req.write_cmd = strtoul(ctx->record + strlen(AESD_TAG), &cp, 10);
        if (*cp++ != ',') {
            syslog(LOG_ERR, "Bad format: %s", ctx->record);
        } else {
            req.write_cmd_offset = strtoul(cp, NULL, 10);
            aesd_seekto_set = true;
        }
    } else {
        if(write(dump, ctx->record, ctx->record_len) != ctx->record_len) {
            syslog(LOG_ERR, "write error: %s", strerror(errno));
            exit(-1);
        }
    }
    int fd = open(OUTPUT_FILE, O_RDONLY);
    if (fd >= 0) {
        int s;

        if (aesd_seekto_set) {
            syslog(LOG_DEBUG, "AESDCHAR_IOCSEEKTO: (%u, %u)", req.write_cmd, req.write_cmd_offset);
            if (ioctl(fd, AESDCHAR_IOCSEEKTO, &req) == -1) {
                syslog(LOG_ERR, "AESDCHAR_IOCSEEKTO failed: %s", strerror(errno));
            }
        }

        while ((s = read(fd, ctx->buf, sizeof(ctx->buf))) > 0) {
            send(ctx->cli_sock, ctx->buf, s, 0);
        }
        close(fd);
    }
    free(ctx->record);
    ctx->record_len = 0;
    ctx->record = NULL;
    pthread_mutex_unlock(&dump_mutex);
}

static void *server_function(void *p)
{
    struct param_t *ctx = (struct param_t *)p;
    if(!p)
        return p;
    struct pollfd conn_request = { ctx->cli_sock, POLLIN, 0 };
    int tmo = 1 * 1000; /* in msec */
    while(!done) {
        int err = poll(&conn_request, 1, tmo);
        if (err < 0)
            break;
        if (err == 0)
            continue;
        ssize_t s = recv(ctx->cli_sock, ctx->buf, sizeof(ctx->buf), 0);
        if (s <= 0) {
            syslog(LOG_INFO, "Close connection from %s", ctx->cli_addr);
            break;
        }

        append_record(ctx, ctx->buf, s);
        if (ctx->buf[s-1] == '\n') {
            return_dump(ctx);
        }
    }

    close(ctx->cli_sock);

    if (ctx->record) {
        free(ctx->record);
        ctx->record = NULL;
    }
    ctx->done = 1;
    return p;
}

#ifdef WITH_TIMESTAMPS
/*
  man strftime(3), RFC 2822-compliant date format:
  "%a, %d %b %Y %T %z"
*/
static void *timestamp_function(void *p)
{
    char outstr[200];
    time_t t;
    struct tm *tmp;
    size_t cnt = 0;
    while(!done) {
        usleep(1 * 1000 * 1000);
        ++cnt;
        if(cnt % 10)
            continue;
        t = time(NULL);
        tmp = localtime(&t);
        strftime(outstr, sizeof(outstr), "%a, %d %b %Y %T %z", tmp);

        pthread_mutex_lock(&dump_mutex);
        fprintf(dump, "timestamp:%s\n", outstr);
        pthread_mutex_unlock(&dump_mutex);
    }
    return p;
}
#endif

static void verbose(const char *format, ...)
{
    va_list ap;
    if(verbose_flag) {
        va_start(ap, format);
        vsyslog(LOG_DEBUG, format, ap);
        va_end(ap);
    }
}

int main(int argc, char **argv)
{
    int listen_sock;
    char *me = argv[0];
    int opt;

    while ((opt = getopt(argc, argv, "dvh")) != -1) {
        switch (opt) {
        case 'd':
            daemon_flag = 1;
            break;
        case 'v':
            verbose_flag = 1;
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

    int optval = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)); // ignore err

    if (bind(listen_sock, info->ai_addr, info->ai_addrlen)) {
        syslog(LOG_ERR, "bind(...) failed: %s (%d)", strerror(errno), errno);
        freeaddrinfo(info);
        return -1;
    }
    freeaddrinfo(info);

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
#ifdef WITH_TIMESTAMPS
    pthread_t timestamp_tid;

    if (pthread_create(&timestamp_tid, NULL, timestamp_function, NULL)){
        syslog(LOG_ERR, "Cannot create timestamp thread");
        return -1;
    }
#endif
    LIST_HEAD(listhead, list_data_s) head;
    LIST_INIT(&head);

    while(!done) {
        /* implement polling for two event types:
           - connection request
           - working thread complete
        */

        struct pollfd conn_request = { listen_sock, POLLIN, 0 };
        int tmo = 1 * 1000; /* in msec */
        int err = poll(&conn_request, 1, tmo);
        if (err == 0) {
            /* timeout - scan the LIST for complete threads */
            list_data_t *datap = NULL;
            LIST_FOREACH(datap, &head, entries) {
                if (datap->ctx && datap->ctx->done) {
                    verbose("joining %lx", datap->tid);
                    void *thread_return = NULL;
                    err = pthread_join(datap->tid, &thread_return);
                    if(err) {
                        syslog(LOG_ERR, "pthread_join failed (%d)", err);
                        return -1;
                    }
                    release_param(thread_return);
                    datap->ctx = NULL;
                    /* TODO: remove */
                    /* LIST_REMOVE(datap, entries); */
                    /* free(datap); */
                }
            }
        } else if (err > 0) {
            /* a connection request. accept */
            struct sockaddr addr;
            socklen_t addrlen = sizeof(addr);
            memset(&addr, 0, sizeof(addr));

            int cli_sock = accept(listen_sock, &addr, &addrlen);
            if (cli_sock == -1){
                syslog(LOG_ERR, "accept(...) failed: %d", errno);
                return -1;
            }

            pthread_mutex_lock(&dump_mutex);
            if (dump == -1) {
                dump = open(OUTPUT_FILE, O_WRONLY);
                if (dump < 0) {
                    syslog(LOG_ERR, "Cannot open %s file: %s", OUTPUT_FILE, strerror(errno));
                    return -1;
                }
            }
            pthread_mutex_unlock(&dump_mutex);

            struct param_t *p = calloc(sizeof(struct param_t), 1);
            if (!p) {
                syslog(LOG_ERR, "Cannot allocate memory for thread param_t");
                return -1;
            }
            syslog(LOG_INFO, "Accepted connection from %s",
                   ip2str(&addr, p->buf, sizeof(p->buf)));
            p->cli_sock = cli_sock;
            p->cli_addr = strdup(p->buf); /* Can be NULL */
            pthread_t t;
            if (pthread_create(&t, NULL, server_function, p)){
                syslog(LOG_ERR, "Cannot create thread");
                return -1;
            }
            /* append to the SLIST */

            list_data_t *datap = calloc(sizeof(list_data_t), 1);
            datap->tid = t;
            datap->ctx = p;
            verbose("Insert TID: %lx", datap->tid);
            LIST_INSERT_HEAD(&head, datap, entries);
        }
    }

    verbose("joining the list...");
    list_data_t *datap = NULL;
    while (!LIST_EMPTY(&head)) {
        datap = LIST_FIRST(&head);
        LIST_REMOVE(datap, entries);
        if (datap->ctx) {
            pthread_join(datap->tid, NULL);
            release_param(datap->ctx);
        }
        free(datap);
    }
#ifdef WITH_TIMESTAMPS
    pthread_join(timestamp_tid, NULL);
#endif
    close(dump);
    return 0;
}
