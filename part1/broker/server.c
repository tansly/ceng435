#include "globals.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <signal.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define BACKLOG 10

/*
 * TODO: Better return some error codes on failure?
 */

static struct {
    int curr_clients;   // current number of clients 
    int max_clients;    // max number of clients

    /*
     * XXX: Why do I use dynamic allocation for these?
     */
    char *bind_addr;    // address to bind
    char *bind_port;    // port to bind
    int backlog;
    int listen_sock;    // socket that the server is listening on

    /*
     * We send messages received from the source to r1.
     */
    const char *r1_addr;
    const char *r1_port;
    int r1_sock;

    /*
     * We receive feedback messages from r2.
     */
    const char *r2_bind_addr;
    const char *r2_bind_port;
    int r2_sock;
} server = {
    .r1_addr = "10.10.2.2",
    .r1_port = "26600",

    .r2_bind_addr = "0.0.0.0",
    .r2_bind_port = "26598"
};

static void sigchld_handler(int sig)
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
static int listen_server_socket(void)
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
 * XXX: The following two functions look like each other except a few lines.
 */

/*
 * connect() the r1 socket and return it.
 * Return -1 on failure.
 */
static int connect_r1_socket(void)
{
    int status;
    int sockfd;
    struct addrinfo *servinfo, *rp;
    struct addrinfo hints;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    status = getaddrinfo(server.r1_addr, server.r1_port, &hints, &servinfo);
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
        fprintf(stderr, "Could not connect() r1 socket\n");
        return -1;
    }

    return sockfd;
}

/*
 * bind() the r2 socket and return it.
 * Return -1 on failure.
 */
static int bind_r2_socket()
{
    int status;
    int sockfd;
    struct addrinfo *servinfo, *rp;
    struct addrinfo hints;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    status = getaddrinfo(server.r2_bind_addr, server.r2_bind_port, &hints, &servinfo);
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
        fprintf(stderr, "Could not bind() r2 socket\n");
        return -1;
    }

    return sockfd;
}

/*
 * Assume that the server struct is filled.
 * Setup all sockets:
 * * The socket that the server listens on,
 * * The socket that sends to r1,
 * * The socket that receives from r2.
 * Return 0 on success, non-zero on error.
 */
static int setup_sockets(void)
{
    int sockfd;

    sockfd = listen_server_socket();
    if (sockfd == -1) {
        return 1;
    }
    server.listen_sock = sockfd;

    sockfd = connect_r1_socket();
    if (sockfd == -1) {
        return 1;
    }
    server.r1_sock = sockfd;

    sockfd = bind_r2_socket();
    if (sockfd == -1) {
        return 1;
    }
    server.r2_sock = sockfd;

    return 0;
}

/*
 * This function will handle incoming messages (feedback) from r2.
 * It receives datagrams from r2 and sends it to source over @source_sock.
 * @source_sock the socket that accept() returned; the socket
 * that is connected to the source. It is shared with the parent process that
 * receives data from the source.
 * When the source socket is closed, that is when the source terminates,
 * this process is killed by its parent.
 * This function does not return.
 */
static void router_handler(int source_sock)
{
    char buf[MSG_SIZE];
    ssize_t recved;

    for (;;) {
        if ((recved = read(server.r2_sock, buf, MSG_SIZE)) != MSG_SIZE) {
            fprintf(stderr, "read(source_sock) returned %ld\n", recved);
        } else {
            ssize_t written;
            written = write(source_sock, buf, MSG_SIZE);
            if (written != MSG_SIZE) {
                /*
                 * TODO: Handle partial send properly instead of printing
                 * an error message.
                 */
                fprintf(stderr, "write(source_sock) returned %ld\n", written);
            }
        }
    }

    exit(0);
}

/*
 * Main routine for fork()ed child
 */
static void child_main(int recv_sock)
{
    char buf[MSG_SIZE];
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

    /*
     * Fork the router handler child.
     */
    pid_t pid = fork();
    if (pid == -1) {
        fprintf(stderr, "Fork failed.\n");
        exit(0);
    } else if (pid == 0) {
        /*
         * We, the child, share the file descriptors with our parent. This way,
         * we can use the recv_sock together with our parent. Unlike our parent,
         * however, we use this socket not to recv from the source, but to send
         * to the source.
         */
        router_handler(recv_sock);
        /* NO RETURN */
        assert(0);
    }

    /*
     * We, the parent, recv bytes from the source, and send them in datagrams to r1.
     */
    while ((recved = recv(recv_sock, buf, MSG_SIZE, MSG_WAITALL)) > 0) {
        if (recved != MSG_SIZE) {
            fprintf(stderr, "recv(recv_sock) returned %ld\n", recved);
            continue;
        }

        ssize_t written;
        if ((written = write(server.r1_sock, buf, recved)) != MSG_SIZE) {
            fprintf(stderr, "write(server.r1_sock) returned %ld\n", written);
            fprintf(stderr, "This should not have happened.\n");
        }
    }

    kill(pid, SIGTERM);

    /*
     * We should somehow clear remaining messages in server.r2_sock. Why?
     * Because when we kill our child (when the source node terminates the connection)
     * some messages (from r2) remain in the socket, ready to be recved. When a new connection
     * is established with the source, the new child starts recving from server.r2_sock
     * and receives messages from the previous communication.
     * This is undesirable, hence the following code.
     * XXX: This works as long as we don't stop and start the source too quickly.
     * Seriously, give it a second. Why the rush?
     */
    sleep(1); // Pray and hope for the best.
    while (recv(server.r2_sock, buf, MSG_SIZE, MSG_DONTWAIT) > 0)
        ;

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

/*
 * TODO:
 * 1) Error checking
 * 2) Get client address, log it or print it or something
 */
void main_loop(void)
{
    int recv_sock;
    pid_t pid;

    for (;;) {
        recv_sock = accept(server.listen_sock, NULL, NULL); // TODO 1) 2)
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
