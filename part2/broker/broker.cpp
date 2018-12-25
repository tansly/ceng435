#include "util.hpp"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <csignal>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define BACKLOG 10

namespace {

struct {
    int curr_clients;   // current number of clients 
    int max_clients;    // max number of clients

    /*
     * XXX: Why do I use dynamic allocation for these?
     */
    char *bind_addr;    // address to bind
    char *bind_port;    // port to bind
    int backlog;
    int listen_sock;    // socket to listen

    const char *dr1_addr;
    const char *dr1_port;
    int dr1_sock;

    const char *dr2_addr;
    const char *dr2_port;
    int dr2_sock;
} server = {
    .curr_clients = 0,
    .max_clients = 0,
    .bind_addr = NULL,
    .bind_port = NULL,
    .backlog = BACKLOG,
    .listen_sock = -1,

    .dr1_addr = "10.10.3.2",
    .dr1_port = "26599",
    .dr1_sock = -1,

    .dr2_addr = "10.10.5.2",
    .dr2_port = "26599",
    .dr2_sock = -1
};

void sigchld_handler(int sig)
{
    int old_errno = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0) {
        server.curr_clients--;
    }
    errno = old_errno;
}

/*
 * listen() the server socket and return the fd.
 * Return -1 on failure.
 */
int listen_server_socket(void)
{
    int status;
    int sockfd;
    struct addrinfo *servinfo, *rp;
    struct addrinfo hints;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    status = getaddrinfo(server.bind_addr, server.bind_port,
            &hints, &servinfo);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo() error: %s\n", gai_strerror(status));
        return -1;
    }
    for (rp = servinfo; rp; rp = rp->ai_next) {
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd == -1) { // could not get socket
            continue; // try the next one
        }
        if (bind(sockfd, rp->ai_addr, rp->ai_addrlen) == 0) { // we are done
            break;
        }
        close(sockfd); // could not bind, close and try the next one
    }
    freeaddrinfo(servinfo);
    if (!rp) { // no address succeeded
        fprintf(stderr, "Could not bind()\n");
        return -1;
    }
    // Since bind() has succeeded, we now listen() the socket.
    status = listen(sockfd, server.backlog);
    if (status == -1) {
        perror("listen() error");
        return -1;
    }

    return sockfd;
}

/*
 * Create and connect a socket to addr and port.
 * Return -1 on failure.
 */
int connect_dest_socket(const char *addr, const char *port)
{
    int status;
    int sockfd;
    struct addrinfo *servinfo, *rp;
    struct addrinfo hints;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    status = getaddrinfo(addr, port, &hints, &servinfo);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo() error: %s\n", gai_strerror(status));
        return -1;
    }
    for (rp = servinfo; rp; rp = rp->ai_next) {
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd == -1) { // could not get socket
            continue; // try the next one
        }
        if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) == 0) { // we are done
            break;
        }
        close(sockfd); // could not connect, close and try the next one
    }
    freeaddrinfo(servinfo);
    if (!rp) { // no address succeeded
        fprintf(stderr, "Could not connect() socket\n");
        return -1;
    }

    return sockfd;
}

/*
 * Assume that the server struct is filled.
 * Setup all sockets.
 * * The socket that the server listens on,
 * * The sockets that sends to destination.
 * Return 0 on success, non-zero on error.
 */
int setup_sockets(void)
{
    int sockfd;

    sockfd = listen_server_socket();
    if (sockfd == -1) {
        return 1;
    }
    server.listen_sock = sockfd;

    sockfd = connect_dest_socket(server.dr1_addr, server.dr2_port);
    if (sockfd == -1) {
        return 1;
    }
    server.dr1_sock = sockfd;

    sockfd = connect_dest_socket(server.dr2_addr, server.dr2_port);
    if (sockfd == -1) {
        return 1;
    }
    server.dr2_sock = sockfd;

    return 0;
}

/*
 * Main routine for fork()ed child
 */
void child_main(int recv_sock)
{
    ssize_t recved;
    // Child will not need the signal handlers of the parent.
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = SIG_IGN;
    sigaction(SIGCHLD, &sa, NULL);
    sa.sa_handler = SIG_DFL;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    free(server.bind_addr);
    free(server.bind_port);
    close(server.listen_sock);

    while ((recved = recv(recv_sock, buf, PAYLOAD_SIZE, MSG_WAITALL)) > 0) {
        if (recved != PAYLOAD_SIZE) {
            /*
             * TODO: What to do in this case? The final packet?
             */
            fprintf(stderr, "recv(recv_sock) returned %ld\n", recved);
        }

    }

    exit(0);
}

int start_server(const char *bind_addr, const char *bind_port, int max_clients)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    server.curr_clients = 0;
    server.max_clients = max_clients;
    server.backlog = BACKLOG;
    server.bind_addr = strdup(bind_addr);
    server.bind_port = strdup(bind_port);
    if (setup_sockets() != 0) {
        free(server.bind_addr);
        free(server.bind_port);
        return 1;
    }
    sa.sa_handler = sigchld_handler;
    sigaction(SIGCHLD, &sa, NULL);

    return 0;
}

void main_loop(void)
{
    int recv_sock;
    pid_t pid;

    for (;;) {
        recv_sock = accept(server.listen_sock, NULL, NULL);
        if (recv_sock == -1) {
            // Currently we just go on to accept() another connection,
            // but we should properly handle different errors.
            perror("accept() error");
            continue;
        }
        fprintf(stderr, "Someone connected.\n");
        if (server.curr_clients >= server.max_clients) {
            fprintf(stderr, "Max number of clients (%d) is reached.\n",
                    server.max_clients);
            close(recv_sock);
            continue;
        }

        /*
         * Fork the connection handler child.
         */
        pid = fork();
        if (pid == -1) {
            fprintf(stderr, "Fork failed.\n");
            close(recv_sock);
        } else if (pid == 0) {
            child_main(recv_sock); // Will never return
            assert(0);
            exit(EXIT_FAILURE);
        } else {
            /*
             * We, the parent, is done with this socket. The child will handle it.
             */
            close(recv_sock);
            server.curr_clients++;
        }
    }
}

}

int main()
{
    int status = 0;
    status = start_server("0.0.0.0", "26598", 1);
    if (status != 0) {
        fprintf(stderr, "Failed to start server!\n");
        return 1;
    }
    /* The server has been started.
     * Time to start the main loop of the program.
     * This will accept() clients and start recv()'ing and playing.
     */
    main_loop();
    // THOU SHALT NOT PASS
    assert(0);
    return 1;
}