#include "util.hpp"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <csignal>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <atomic>
#include <condition_variable>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>

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
    (void)sig;
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

std::atomic<bool> timer_expired {false};
void sigalrm_handler(int sig)
{
    if (sig == SIGALRM) {
        timer_expired = true;
        std::cerr << "ALARMAAAAAA" << std::endl;
    }
}

void start_timer()
{
    struct itimerval itimer = {
        .it_interval = { .tv_sec = 0, .tv_usec = 0},
        .it_value = { .tv_sec = 0, .tv_usec = Util::timeout_millis * 1000 }
    };
    if (setitimer(ITIMER_REAL, &itimer, NULL) == -1) {
        perror("start_timer()");
    }

    timer_expired = false;
}

void stop_timer()
{
    struct itimerval itimer = {
        .it_interval = { .tv_sec = 0, .tv_usec = 0},
        .it_value = { .tv_sec = 0, .tv_usec = 0 }
    };
    if (setitimer(ITIMER_REAL, &itimer, NULL) == -1) {
        perror("stop_timer()");
    }

    timer_expired = false;
}

/*
 * Main routine for fork()ed child
 */
void child_main(int recv_sock)
{
    // Child will not need the signal handlers of the parent.
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = SIG_IGN;
    sigaction(SIGCHLD, &sa, NULL);
    sa.sa_handler = SIG_DFL;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    /*
     * SIGALRM handler for timeouts.
     */
    sa.sa_handler = sigalrm_handler;
    sigaction(SIGALRM, &sa, NULL);

    // Just to satisfy myself, free these things
    free(server.bind_addr);
    free(server.bind_port);
    close(server.listen_sock);

    /*
     * Fancy stuff begins here.
     *
     * The pairs in the queue hold the packet and the size of the packet.
     */
    Util::Queue<std::pair<Util::Packet, int>> packet_and_len_q;

    /*
     * The sender function and shared variables of the rdt protocol.
     * Senders dequeue the packets and send them over disjoint links.
     * Will run in seperate threads for sending over r1 and r2.
     *
     * The mutex protects the window base and the sequence number and the window vector.
     * (base, next_seq_num, sender_window)
     * TODO: Those variables are related logically. A better C++ style could make
     * this relation explicit, no?
     * TODO: Maybe we should use separate mutexes for separate variables.
     * This single mutex is really getting out of hand.
     */
    std::mutex window_mutex;
    std::vector<std::pair<Util::Packet, int>> sender_window(Util::window_size);
    std::condition_variable window_not_full;
    std::condition_variable transmission_complete;
    std::uint32_t base {0};
    std::uint32_t next_seq_num {0};
    std::uint32_t final_seq_num;
    bool final_sent {false};
    bool final_acked {false};

    /*
     * XXX: This does not use sock2 right now.
     */
    auto rdt_timeout_handler = [&] {
        /*
         * TODO: Do I need to find a better way than locking everything here?
         ** We will eventually recv the ACKs when the lock is released.
         ** Calling send() for all packets should not take a lot of time anyways.
         * So I *think* it's fine. Needs a second look, though.
         */
        std::unique_lock<std::mutex> window_lock {window_mutex};
        for (auto i = base; i < next_seq_num; ++i) {
            auto &packet_and_len = sender_window[i % Util::window_size];
            auto &packet = packet_and_len.first;
            auto &len = packet_and_len.second;
#ifndef NDEBUG
            std::cerr << "RTX: " << ntohl(packet.seq_num) << std::endl;
#endif
            send(server.dr1_sock, &packet, len, 0);
        }
        start_timer();
    };

    auto rdt_sender = [&](int dest_sock) {
        for (;;) {
            /*
             * TODO: Explain this.
             */
            if (timer_expired.exchange(false)) {
                std::cerr << "Timer expired" << std::endl;
                rdt_timeout_handler();
                continue;
            }

            std::unique_lock<std::mutex> window_lock {window_mutex};
            /*
             * If we or the other sender has sent the final packet, we should
             * terminate here.
             */
            if (final_sent) {
                return;
            }
            /*
             * Sleep if the window is full. Will wake up if an ACK is received
             * (and the window slides).
             */
            window_not_full.wait(window_lock, [&]{return next_seq_num < base + Util::window_size;});
            auto packet_and_len = packet_and_len_q.dequeue();
            auto &packet = packet_and_len.first;
            auto &len = packet_and_len.second;

            /*
             * To signify the end of the data, we use a packet with no payload,
             * only consists of a header.
             */
            if (len == Util::header_size) {
                final_sent = true;
                final_seq_num = next_seq_num;
#ifndef NDEBUG
                std::cerr << "FIN: " << final_seq_num << std::endl;
#endif
            } else {
#ifndef NDEBUG
                std::cerr << "SEQ: " << ntohl(packet.seq_num) << std::endl;
#endif
            }

            /*
             * We set the sequence numbers here.
             */
            packet_and_len.first.seq_num = htonl(next_seq_num);
            sender_window[next_seq_num % Util::window_size] = packet_and_len;

            send(dest_sock, &packet, len, 0);
            if (base == next_seq_num) {
                start_timer();
            }
            ++next_seq_num;
        }
    };

    /*
     * rdt protocol ACK handler.
     */
    auto rdt_recver = [&](int dest_sock) {
        for (;;) {
            auto packet = Util::Packet();
            /*
             * ACK packets are packets without a payload.
             */
            if (recv(dest_sock, &packet, Util::header_size, 0) != Util::header_size) {
#ifndef NDEBUG
                fprintf(stderr, "You done fucked up.\n");
#endif
                continue;
            }

            std::unique_lock<std::mutex> window_lock {window_mutex};
            /*
             * If the final packet is ACKed, we return immediately.
             */
            if (final_acked || (final_sent && final_seq_num == ntohl(packet.seq_num))) {
                final_acked = true;
                transmission_complete.notify_one();
#ifndef NDEBUG
                std::cerr << "FIN-ACK: " << ntohl(packet.seq_num) << std::endl;
#endif
                return;
            }

            /*
             * XXX: Should check if the base is actually increased?
             * What about out-of-order ACK packets?
             * UPDATE: I think the protocol should work correctly regardless.
             * However, it still might be good to check this. Think about it
             * a little bit more.
             * UPDATE2: Now checking it.
             */
            base = std::max(base, ntohl(packet.seq_num) + 1);
#ifndef NDEBUG
            std::cerr << "ACK: " << ntohl(packet.seq_num) << std::endl;
#endif
            if (base == next_seq_num) {
                stop_timer();
            } else {
                start_timer();
            }
            window_not_full.notify_one();
        }
    };

    std::thread r1_sender {rdt_sender, server.dr1_sock};
    std::thread r2_sender {rdt_sender, server.dr2_sock};
    std::thread r1_recver {rdt_recver, server.dr1_sock};
    std::thread r2_recver {rdt_recver, server.dr2_sock};

    ssize_t recved;
    std::uint32_t seq_num = 0;
    /*
     * Packet constructor initializes fields to zero.
     * TODO: Checksum (MD5)
     */
    Util::Packet packet;
    while ((recved = recv(recv_sock, &packet.payload, Util::payload_size, MSG_WAITALL)) > 0) {
        if (recved != Util::payload_size) {
            /*
             * The last packet.
             */
#ifndef NDEBUG
            fprintf(stderr, "recv(recv_sock) returned %ld\n", recved);
#endif
        }
        /*
         * XXX: The sender threads also have to keep track of the seq. numbers.
         * If we htonl() the number here, they will have to ntohl(), do their thang
         * and htonl() again. Think of this issue before this thing turns into a
         * huge mess.
         */
        packet.seq_num = htonl(seq_num);
        /*
         * We should probably compute the checksum at this point.
         */
        packet_and_len_q.enqueue({packet, recved + Util::header_size});

        ++seq_num;
    }
    /*
     * At this point, we received and queued all data from the source.
     * To signify the end of the data, put a dummy packet in the queue with
     * only a header. This way a sender thread can know that there will be no more data,
     * set a variable for termination and gracefully exit together with the other sender.
     *
     * The destination will also know that the file is completely transferred this way.
     *
     * The content of the packet.payload does not matter, it will not be read anyways.
     */
    packet_and_len_q.enqueue({packet, Util::header_size});

    std::unique_lock<std::mutex> window_lock {window_mutex};
    transmission_complete.wait(window_lock, [&]{return final_acked;});

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
