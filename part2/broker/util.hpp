/*
    Copyright (C) 2018 Yağmur Oymak

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef UTIL_HPP
#define UTIL_HPP

#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>
#include <queue>
#include <cstdint>
#include <chrono>

namespace Util {

/*
 * Sender timeout in milliseconds
 */
constexpr auto timeout_value = std::chrono::milliseconds(50);

/*
 * Window size as number of packets in the pipeline.
 */
constexpr auto window_size = 30;

/*
 * Payload size in bytes.
 */
constexpr auto payload_size = 980;
/*
 * Checksum size in bytes.
 * MD5 (128-bit, 16 bytes) is used as a checksum.
 */
constexpr auto checksum_size = 16;
/*
 * Header size in bytes. Size of the checksum + size of the seq. number.
 */
constexpr auto header_size = checksum_size + sizeof(std::uint32_t);
/*
 * The maximum size of a packet, including the header and the payload.
 * Not that the actual packet size may be less. XXX: Should we have a length field?
 * Without a length field, the receiver may know the actual packet length by
 * trying to recv() the maximum amount from its socket, and check the return value
 * of the recv() for the actual amount.
 */
constexpr auto max_packet_size = payload_size + header_size;

struct Packet {
    Packet() :
        seq_num {0},
        checksum {0},
        payload {0}
    {
    }
    std::uint32_t seq_num;
    std::uint8_t checksum[checksum_size];
    std::uint8_t payload[payload_size];
};

template <class T>
class Queue {
public:
    Queue() = default;

    Queue(const Queue &) = delete;
    Queue(Queue &&) = default;
    ~Queue() = default;

    void enqueue(T);

    T dequeue();
private:
    bool empty() const {
        return queue.empty();
    }

    std::mutex mutex;
    std::condition_variable notempty;

    std::queue<T> queue;
};

template <class T>
void Queue<T>::enqueue(T elem)
{
    std::unique_lock<std::mutex> lock(mutex);

    queue.push(std::move(elem));
    notempty.notify_one();
}

template <class T>
T Queue<T>::dequeue()
{
    std::unique_lock<std::mutex> lock(mutex);

    while (empty()) {
        notempty.wait(lock);
    }

    T elem = queue.front();
    queue.pop();
    return elem;
}

}

#endif /* UTIL_HPP */
