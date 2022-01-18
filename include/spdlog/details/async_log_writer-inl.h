// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#ifndef SPDLOG_HEADER_ONLY
#    include <spdlog/details/async_log_writer.h>
#endif

#include <spdlog/common.h>
#include <cassert>

namespace spdlog {
namespace details {

SPDLOG_INLINE async_log_writer::async_log_writer(size_t q_max_items, std::function<void()> on_thread_start)
{
    async_log_threads_ = std::thread([this, on_thread_start] {
        on_thread_start();
        this->async_log_writer::worker_loop_();
    });
}

SPDLOG_INLINE async_log_writer::async_log_writer(size_t q_max_items)
    : async_log_writer(q_max_items, [] {})
{}

// message all threads to terminate gracefully join them
SPDLOG_INLINE async_log_writer::~async_log_writer()
{
    SPDLOG_TRY
    {
        post_async_msg_(async_msg(async_msg_type::terminate), async_overflow_policy::block);

        if(async_log_threads_.joinable()){
            async_log_threads_.join();
        }
    }
    SPDLOG_CATCH_STD
}

void SPDLOG_INLINE async_log_writer::post_log(const details::log_msg &msg, async_overflow_policy overflow_policy)
{
    async_msg async_m(async_msg_type::log, msg);
    post_async_msg_(std::move(async_m), overflow_policy);
}

void SPDLOG_INLINE async_log_writer::post_flush(async_overflow_policy overflow_policy)
{
    post_async_msg_(async_msg(async_msg_type::flush), overflow_policy);
}

size_t SPDLOG_INLINE async_log_writer::overrun_counter()
{
    return q_.overrun_counter();
}

size_t SPDLOG_INLINE async_log_writer::queue_size()
{
    return q_.size();
}

void SPDLOG_INLINE async_log_writer::post_async_msg_(async_msg &&new_msg, async_overflow_policy overflow_policy)
{
    if (overflow_policy == async_overflow_policy::block)
    {
        q_.push_back(std::move(new_msg));
    }
    else
    {
        q_.push_back(std::move(new_msg));
    }
}

void SPDLOG_INLINE async_log_writer::worker_loop_()
{
    while (process_next_msg_()) {}
}

// process next message in the queue
// return true if this thread should still be active (while no terminate msg
// was received)
bool SPDLOG_INLINE async_log_writer::process_next_msg_()
{
    auto msgptr = q_.pop_front();
    if (incoming_async_msg == nullptr)
    {
        return true;
    }

    switch (msgptr->msg_type)
    {
    case async_msg_type::log: {
        //TODO 写日志到sink
        // incoming_async_msg.worker_ptr->backend_sink_it_(incoming_async_msg);
        return true;
    }
    case async_msg_type::flush: {
        //flush日志到sink
        // incoming_async_msg.worker_ptr->backend_flush_();
        return true;
    }

    case async_msg_type::terminate: {
        return false;
    }

    default: {
        assert(false);
    }
    }

    return true;
}

} // namespace details
} // namespace spdlog
