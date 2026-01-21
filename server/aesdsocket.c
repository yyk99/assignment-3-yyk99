/*



 */

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

#define OUTPUT_FILE "/var/tmp/aesdsocketdata"
#define SERVICE "9000"
#define BACK_LOG 10

#include <arpa/inet.h>
#include <netinet/in.h>

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

void on_signal(int)
{
    syslog(LOG_INFO, "Caught signal, exiting");
    if (dump) {
        fclose(dump);
    }
    exit(0);
}

int main(int argc, char **argv)
{
    int listen_sock;

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

    do {
        if (listen(listen_sock, BACK_LOG)) {
            syslog(LOG_ERR, "listen(...) failed: %d", errno);
            return -1;
        }
        struct sockaddr addr;
        socklen_t addrlen = sizeof(addr);
        char buf[80];
        memset(&addr, 0, sizeof(addr));
        int cli_sock = accept(listen_sock, &addr, &addrlen);
        if (cli_sock == -1){
            syslog(LOG_ERR, "accept(...) failed: %d", errno);
            return -1;
        }

        syslog(LOG_INFO, "Accepted connection from %s",
               ip2str(&addr, buf, sizeof(buf)));

        dump = fopen(OUTPUT_FILE, "w");
        if (!dump) {
            syslog(LOG_ERR, "Cannot create dump file: %s", strerror(errno));
            return -1;
        }
        do {
            ssize_t s = recv(cli_sock, buf, sizeof(buf), 0);
            if (s <= 0) {
                syslog(LOG_INFO, "Close connection from %s",
                       ip2str(&addr, buf, sizeof(buf)));
                fclose(dump);
                dump = 0;
                break;
            }
            fprintf (stderr, "s=%d\n", (int)s);
            fwrite(buf, s, 1, dump);
         }while(1);
    } while(1);

    return 0;
}
