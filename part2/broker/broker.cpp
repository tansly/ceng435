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

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>

#include <openssl/md5.h>

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
 * Main routine for fork()ed child.
 *
 * TODO: Extracting the lambdas to standalone functions may aid readability.
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
     * It grew out to protect other transmission window related variables as well:
     * (sender_window, base, next_seq_num, final_seq_num, final_sent, final_acked)
     * It also protects timer_cancelled in a way.
     * TODO: Maybe we should use separate mutexes for separate variables.
     * This single mutex is really getting out of hand. I think this is bad practice.
     * Moreover, it already caused hard to detect deadlock-like situations already.
     * It works now, though. Maybe I should just leave this alone.
     * TODO: Those variables are related logically. A better C++ style could make
     * this relation explicit, no?
     */
    std::mutex window_mutex;
    std::vector<std::pair<Util::Packet, int>> sender_window(Util::window_size);
    /*
     * Condition variable that our sender threads wait for when the window is full:
     *  Senders acquire window_mutex, check the if the window is full
     *  and wait on this condition variable if it is full.
     * When an ACK is received by our receiver threads, the window may have
     * additional space, so the receiver threads signals this variable accordingly.
     */
    std::condition_variable window_not_full;
    /*
     * The main thread (that receives data from the source) waits on this when
     * there is no more data coming from the source. When the transmission to
     * destination is completed, that is when the final ACK packet is received
     * by one of our receiver threads, this variable is signalled. In this case
     * the main thread continues and exits. The main *process*, though, keeps
     * listening for incoming connections from the source.
     */
    std::condition_variable transmission_complete;
    std::uint32_t base {1};
    std::uint32_t next_seq_num {1};
    std::uint32_t final_seq_num;
    bool final_sent {false};
    bool final_acked {false};

    /*
     * XXX: This does not use sock2 right now.
     */
    auto rdt_timer_handler = [&] {
        for (;;) {
            if (timer_expired.exchange(false)) {
                /*
                 * TODO: Do I need to find a better way than locking everything here?
                 ** We will eventually recv the ACKs when the lock is released.
                 ** Calling send() for all packets should not take a lot of time anyways.
                 * So I *think* it's fine. Needs a second look, though.
                 */
                std::unique_lock<std::mutex> window_lock {window_mutex};

                /*
                 * XXX: Is this necessary?
                 */
                if (final_acked) {
#ifndef NDEBUG
                    std::cerr << "rdt_timer_handler(): Final ACKed" << std::endl;
#endif
                    return;
                }

                start_timer();
                for (auto i = base; i < next_seq_num; ++i) {
                    auto &packet_and_len = sender_window[(i - 1) % Util::window_size];
                    auto &packet = packet_and_len.first;
                    auto &len = packet_and_len.second;
#ifndef NDEBUG
                    std::cerr << "RTX: " << ntohl(packet.seq_num) << std::endl;
#endif
                    send(server.dr1_sock, &packet, len, 0);
                }
            }
        }
    };

    auto rdt_sender = [&](int dest_sock) {
        for (;;) {
            /*
             * Sleep if the window is full. Will wake up if an ACK is received
             * (and the window slides).
             */
            auto packet_and_len = packet_and_len_q.dequeue();
            auto &packet = packet_and_len.first;
            auto &len = packet_and_len.second;

            std::unique_lock<std::mutex> window_lock {window_mutex};
            /*
             * If we or the other sender has sent the final packet, we should
             * terminate here.
             */
            if (final_sent) {
                return;
            }

            window_not_full.wait(window_lock, [&]{return ntohl(packet.seq_num) < base + Util::window_size;});

            sender_window[(ntohl(packet.seq_num) - 1) % Util::window_size] = packet_and_len;

            send(dest_sock, &packet, len, 0);

            /*
             * To signify the end of the data, we use a packet with no payload,
             * only consists of a header.
             */
            if (len == Util::header_size) {
                final_sent = true;
                final_seq_num = ntohl(packet.seq_num);
#ifndef NDEBUG
                std::cerr << "FIN: " << final_seq_num << std::endl;
#endif
            } else {
#ifndef NDEBUG
                std::cerr << "SEQ: " << ntohl(packet.seq_num) << std::endl;
#endif
            }

            if (base == next_seq_num) {
                start_timer();
            }

            next_seq_num = std::max(next_seq_num, ntohl(packet.seq_num) + 1);
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
                std::cerr << "You done fucked up." << std::endl;
#endif
                continue;
            }

            if (!packet.check_checksum(Util::header_size)) {
#ifndef NDEBUG
                std::cerr << "CHECKSUM FAIL" << std::endl;
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
    std::thread timer_thread {rdt_timer_handler};

    std::uint32_t seq_num = 1;
    Util::Packet packet;
    bool source_finished = false;
    while (!source_finished) {
        ssize_t recved = recv(recv_sock, &packet.payload, Util::payload_size, MSG_WAITALL);
        if (recved == 0) {
            /*
             * Source has sent all data, we're done recving.
             *
             * At this point, we received and queued all data from the source.
             * To signify the end of the data, put a dummy packet in the queue with
             * only a header. This way a sender thread can know that there will be no more data,
             * set a variable for termination and gracefully exit together with the other sender.
             *
             * The destination will also know that the file is completely transferred this way.
             *
             * The content of the packet.payload does not matter, it will not be read anyways.
             */
            source_finished = true;
#ifndef NDEBUG
            std::cerr << "recv(source) returned 0" << std::endl;
#endif
        } else if (recved == -1) {
            if (errno == EINTR) {
                /*
                 * The recv() function was  interrupted  by  a  signal
                 * that was caught, before any data was available.
                 *
                 * In this case, we just continue to recv().
                 */
#ifndef NDEBUG
                perror("child_main()");
#endif
                continue;
            } else {
                /*
                 * There is a serious error. Just exit(), I don't think we should
                 * do anything more in this case. I also don't know what to do.
                 */
                perror("child_main()");
                exit(EXIT_FAILURE);
            }
        }

        auto packet_size = recved + Util::header_size;

        /*
         * XXX: The sender threads also have to keep track of the seq. numbers.
         * If we htonl() the number here, they will have to ntohl(), do their thang
         * and htonl() again. Think of this issue before this thing turns into a
         * huge mess.
         * UPDATE: If we do not do it here, worse things happen. Just leave this alone.
         */
        packet.seq_num = htonl(seq_num);

        /*
         * Checksum calculation. Definition in util.hpp.
         */
        packet.set_checksum(packet_size);

        packet_and_len_q.enqueue({packet, packet_size});

        ++seq_num;
    }
    /*
     * We wait until the transmission is completed, e.g. the final ACK is received.
     * It is obvious that we should, but I forgot to do that and wasted a lot of
     * time debugging it.
     */
    std::unique_lock<std::mutex> window_lock {window_mutex};
    transmission_complete.wait(window_lock, [&]{return final_acked;});

    exit(EXIT_SUCCESS);
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
