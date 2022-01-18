// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include <spdlog/details/log_msg_buffer.h>
#include <spdlog/details/lock_free_queue.h>
#include <spdlog/details/os.h>

#include <chrono>
#include <memory>
#include <thread>
#include <vector>
#include <functional>

namespace spdlog {
class async_logger;

namespace details {

using async_logger_ptr = std::shared_ptr<spdlog::async_logger>;

enum class async_msg_type
{
    log,
    flush,
    terminate
};

// Async msg to move to/from the queue
// Movable only. should never be copied
struct async_msg : log_msg_buffer
{
    async_msg_type msg_type{async_msg_type::log};

    async_msg() = default;
    ~async_msg() = default;

    // should only be moved in or out of the queue..
    async_msg(const async_msg &) = delete;

// support for vs2013 move
#if defined(_MSC_VER) && _MSC_VER <= 1800
    async_msg(async_msg &&other)
        : log_msg_buffer(std::move(other))
        , msg_type(other.msg_type)
    {}

    async_msg &operator=(async_msg &&other)
    {
        *static_cast<log_msg_buffer *>(this) = std::move(other);
        msg_type = other.msg_type;
        return *this;
    }
#else // (_MSC_VER) && _MSC_VER <= 1800
    async_msg(async_msg &&) = default;
    async_msg &operator=(async_msg &&) = default;
#endif

    // construct from log_msg with given type
    async_msg(async_msg_type the_type, const details::log_msg &m)
        : log_msg_buffer{m}
        , msg_type{the_type}
    {}

    explicit async_msg(async_msg_type the_type)
        : log_msg_buffer{}
        , msg_type{the_type}
    {}
};

class SPDLOG_API async_log_writer
{
public:
    using item_type = async_msg;
    using q_type = details::lock_free_queue<item_type,4096>;

    async_log_writer(size_t q_max_items, std::function<void()> on_thread_start);
    async_log_writer(size_t q_max_items);

    // message all threads to terminate gracefully join them
    ~async_log_writer();

    async_log_writer(const async_log_writer &) = delete;
    async_log_writer &operator=(async_log_writer &&) = delete;

    void post_log(const details::log_msg &msg, async_overflow_policy overflow_policy);
    void post_flush(async_overflow_policy overflow_policy);
    size_t overrun_counter();
    size_t queue_size();

private:
    q_type q_;

    std::thread async_log_threads_;

    void post_async_msg_(async_msg &&new_msg, async_overflow_policy overflow_policy);
    void worker_loop_();

    // process next message in the queue
    // return true if this thread should still be active (while no terminate msg
    // was received)
    bool process_next_msg_();
};

} // namespace details
} // namespace spdlog

#ifdef SPDLOG_HEADER_ONLY
#    include "async_log_writer-inl.h"
#endif
