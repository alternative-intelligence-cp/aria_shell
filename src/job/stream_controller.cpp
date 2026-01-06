/**
 * AriaSH Hex-Stream Controller Implementation
 *
 * ARIA-021: Shell Job Control State Machine Design
 *
 * Implements the Threaded Draining Model for deadlock-free I/O.
 */

#include "job/stream_controller.hpp"
#include <cstring>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#ifdef __linux__
#include <sys/syscall.h>
#endif
#endif

namespace ariash {
namespace job {

// =============================================================================
// Ring Buffer Implementation
// =============================================================================

RingBuffer::RingBuffer(size_t cap)
    : buffer(cap), capacity(cap) {}

RingBuffer::~RingBuffer() = default;

size_t RingBuffer::write(const void* data, size_t size) {
    size_t free = freeSpace();
    size_t toWrite = std::min(size, free);

    if (toWrite == 0) return 0;

    size_t wpos = writePos.load(std::memory_order_relaxed);
    const uint8_t* src = static_cast<const uint8_t*>(data);

    // Write in up to two parts (wrap around)
    size_t firstPart = std::min(toWrite, capacity - wpos);
    std::memcpy(buffer.data() + wpos, src, firstPart);

    if (toWrite > firstPart) {
        std::memcpy(buffer.data(), src + firstPart, toWrite - firstPart);
    }

    writePos.store((wpos + toWrite) % capacity, std::memory_order_release);
    return toWrite;
}

size_t RingBuffer::read(void* data, size_t maxSize) {
    size_t avail = available();
    size_t toRead = std::min(maxSize, avail);

    if (toRead == 0) return 0;

    size_t rpos = readPos.load(std::memory_order_relaxed);
    uint8_t* dst = static_cast<uint8_t*>(data);

    // Read in up to two parts (wrap around)
    size_t firstPart = std::min(toRead, capacity - rpos);
    std::memcpy(dst, buffer.data() + rpos, firstPart);

    if (toRead > firstPart) {
        std::memcpy(dst + firstPart, buffer.data(), toRead - firstPart);
    }

    readPos.store((rpos + toRead) % capacity, std::memory_order_release);
    return toRead;
}

size_t RingBuffer::peek(void* data, size_t maxSize) const {
    size_t avail = available();
    size_t toPeek = std::min(maxSize, avail);

    if (toPeek == 0) return 0;

    size_t rpos = readPos.load(std::memory_order_acquire);
    uint8_t* dst = static_cast<uint8_t*>(data);

    size_t firstPart = std::min(toPeek, capacity - rpos);
    std::memcpy(dst, buffer.data() + rpos, firstPart);

    if (toPeek > firstPart) {
        std::memcpy(dst + firstPart, buffer.data(), toPeek - firstPart);
    }

    return toPeek;
}

size_t RingBuffer::available() const {
    size_t w = writePos.load(std::memory_order_acquire);
    size_t r = readPos.load(std::memory_order_acquire);
    return (w >= r) ? (w - r) : (capacity - r + w);
}

size_t RingBuffer::freeSpace() const {
    return capacity - available() - 1;  // -1 to distinguish full from empty
}

bool RingBuffer::empty() const {
    return available() == 0;
}

bool RingBuffer::full() const {
    return freeSpace() == 0;
}

void RingBuffer::clear() {
    readPos.store(0, std::memory_order_release);
    writePos.store(0, std::memory_order_release);
}

// =============================================================================
// Hex-Stream Pipes Implementation
// =============================================================================

HexStreamPipes::HexStreamPipes() {
#ifdef _WIN32
    for (int i = 0; i < 12; ++i) {
        handles[i] = INVALID_HANDLE_VALUE;
    }
#else
    for (int i = 0; i < 12; ++i) {
        fds[i] = -1;
    }
#endif
}

void HexStreamPipes::close() {
#ifdef _WIN32
    for (int i = 0; i < 12; ++i) {
        if (handles[i] != INVALID_HANDLE_VALUE) {
            CloseHandle(handles[i]);
            handles[i] = INVALID_HANDLE_VALUE;
        }
    }
#else
    for (int i = 0; i < 12; ++i) {
        if (fds[i] >= 0) {
            ::close(fds[i]);
            fds[i] = -1;
        }
    }
#endif
}

bool HexStreamPipes::isValid() const {
#ifdef _WIN32
    // Check at least stdin, stdout, stderr are valid
    return handles[0] != INVALID_HANDLE_VALUE &&
           handles[2] != INVALID_HANDLE_VALUE &&
           handles[4] != INVALID_HANDLE_VALUE;
#else
    return fds[0] >= 0 && fds[2] >= 0 && fds[4] >= 0;
#endif
}

// =============================================================================
// Stream Controller Implementation
// =============================================================================

StreamController::StreamController() {
    // Create ring buffers for output streams
    for (int i = 0; i < static_cast<int>(StreamIndex::COUNT); ++i) {
        buffers[i] = std::make_unique<RingBuffer>(64 * 1024);
    }
}

StreamController::~StreamController() {
    stopDraining();
    close();
}

bool StreamController::createPipes() {
#ifdef _WIN32
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    // Create pipes for each stream
    for (int i = 0; i < 6; ++i) {
        int readIdx = i * 2;
        int writeIdx = i * 2 + 1;

        if (!CreatePipe(&pipes.handles[readIdx], &pipes.handles[writeIdx],
                        &sa, 0)) {
            close();
            return false;
        }
    }
    return true;
#else
    // Create pipes with O_CLOEXEC for security
    for (int i = 0; i < 6; ++i) {
        int pipefd[2];
#ifdef __linux__
        if (pipe2(pipefd, O_CLOEXEC) < 0) {
#else
        if (pipe(pipefd) < 0) {
#endif
            close();
            return false;
        }
        pipes.fds[i * 2] = pipefd[0];      // Read end
        pipes.fds[i * 2 + 1] = pipefd[1];  // Write end
    }
    return true;
#endif
}

bool StreamController::setupChild() {
#ifndef _WIN32
    // Redirect FDs 0-5 to pipes
    // stdin: read from parent's write end
    if (dup2(pipes.fds[1], STDIN_FILENO) < 0) return false;

    // stdout: write to parent's read end
    if (dup2(pipes.fds[2], STDOUT_FILENO) < 0) return false;

    // stderr: write to parent's read end
    if (dup2(pipes.fds[4], STDERR_FILENO) < 0) return false;

    // stddbg (FD 3): write to parent's read end
    if (dup2(pipes.fds[6], 3) < 0) return false;

    // stddati (FD 4): read from parent's write end
    if (dup2(pipes.fds[9], 4) < 0) return false;

    // stddato (FD 5): write to parent's read end
    if (dup2(pipes.fds[10], 5) < 0) return false;

    // Close all original pipe FDs
    for (int i = 0; i < 12; ++i) {
        if (pipes.fds[i] >= 0) {
            ::close(pipes.fds[i]);
        }
    }
#endif
    return true;
}

bool StreamController::setupParent() {
#ifndef _WIN32
    // Close child-side FDs

    // stdin: close read end (child uses it)
    ::close(pipes.fds[0]);
    pipes.fds[0] = -1;

    // stdout: close write end (child uses it)
    ::close(pipes.fds[3]);
    pipes.fds[3] = -1;

    // stderr: close write end
    ::close(pipes.fds[5]);
    pipes.fds[5] = -1;

    // stddbg: close write end
    ::close(pipes.fds[7]);
    pipes.fds[7] = -1;

    // stddati: close read end
    ::close(pipes.fds[8]);
    pipes.fds[8] = -1;

    // stddato: close write end
    ::close(pipes.fds[11]);
    pipes.fds[11] = -1;
#endif
    return true;
}

bool StreamController::startDraining() {
    stopFlag.store(false);

#ifndef _WIN32
    // Start drain threads for output streams
    // stdout (fd index 2)
    if (pipes.fds[2] >= 0) {
        drainThreads[0] = std::thread(&StreamController::drainLoop, this,
                                       StreamIndex::STDOUT, pipes.fds[2]);
    }

    // stderr (fd index 4)
    if (pipes.fds[4] >= 0) {
        drainThreads[1] = std::thread(&StreamController::drainLoop, this,
                                       StreamIndex::STDERR, pipes.fds[4]);
    }

    // stddbg (fd index 6)
    if (pipes.fds[6] >= 0) {
        drainThreads[2] = std::thread(&StreamController::drainLoop, this,
                                       StreamIndex::STDDBG, pipes.fds[6]);
    }

    // stddato (fd index 10)
    if (pipes.fds[10] >= 0) {
        drainThreads[3] = std::thread(&StreamController::drainLoop, this,
                                       StreamIndex::STDDATO, pipes.fds[10]);
    }
#endif

    return true;
}

void StreamController::stopDraining() {
    stopFlag.store(true);

    // Wait for threads to finish
    for (int i = 0; i < 4; ++i) {
        if (drainThreads[i].joinable()) {
            drainThreads[i].join();
        }
    }
}

void StreamController::drainLoop(StreamIndex stream, int fd) {
#ifndef _WIN32
    uint8_t buf[4096];

    while (!stopFlag.load()) {
        // Use select with timeout for cancellation
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000;  // 100ms

        int ret = select(fd + 1, &readfds, nullptr, nullptr, &tv);
        if (ret < 0) {
            break;  // Error
        }
        if (ret == 0) {
            continue;  // Timeout, check stop flag
        }

        ssize_t n = read(fd, buf, sizeof(buf));
        if (n <= 0) {
            break;  // EOF or error
        }

        // In foreground mode, write directly to TTY for stdout/stderr
        if (foregroundMode.load() &&
            (stream == StreamIndex::STDOUT || stream == StreamIndex::STDERR)) {
            int ttyFd = (stream == StreamIndex::STDOUT) ? STDOUT_FILENO : STDERR_FILENO;
            ssize_t written = ::write(ttyFd, buf, n);
            (void)written;  // Ignore return value in foreground passthrough
        }

        // Always buffer the data
        int idx = static_cast<int>(stream);
        buffers[idx]->write(buf, n);

        // Notify callbacks
        notifyData(stream, buf, n);
    }
#endif
}

ssize_t StreamController::writeStdin(const void* data, size_t size) {
#ifdef _WIN32
    DWORD written;
    if (!WriteFile(pipes.handles[1], data, (DWORD)size, &written, NULL)) {
        return -1;
    }
    return written;
#else
    return write(pipes.fds[1], data, size);
#endif
}

size_t StreamController::readBuffer(StreamIndex stream, void* data, size_t maxSize) {
    int idx = static_cast<int>(stream);
    return buffers[idx]->read(data, maxSize);
}

size_t StreamController::availableData(StreamIndex stream) const {
    int idx = static_cast<int>(stream);
    return buffers[idx]->available();
}

bool StreamController::hasPendingData(StreamIndex stream) const {
    return availableData(stream) > 0;
}

void StreamController::onData(StreamCallback callback) {
    std::lock_guard<std::mutex> lock(callbackMutex);
    callbacks.push_back(callback);
}

void StreamController::notifyData(StreamIndex stream, const void* data, size_t size) {
    std::lock_guard<std::mutex> lock(callbackMutex);
    for (auto& cb : callbacks) {
        cb(stream, data, size);
    }
}

void StreamController::setForegroundMode(bool foreground) {
    foregroundMode.store(foreground);
}

void StreamController::flushBuffers() {
    uint8_t buf[4096];

    for (int i = 0; i < static_cast<int>(StreamIndex::COUNT); ++i) {
        while (!buffers[i]->empty()) {
            size_t n = buffers[i]->read(buf, sizeof(buf));
            if (n > 0) {
                notifyData(static_cast<StreamIndex>(i), buf, n);
            }
        }
    }
}

void StreamController::close() {
    stopDraining();
    pipes.close();
}

} // namespace job
} // namespace ariash
