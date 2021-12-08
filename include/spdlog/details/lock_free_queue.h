
#pragma once

#include <atomic>
#include <array>
#include <thread>
#include <memory>


namespace spdlog {
namespace details {

template <typename T,size_t SIZE>
class free_lock_queue{
private:
    std::array<std::shared_ptr<T>,SIZE> v_;
    std::atomic<size_t> write_index_ = 0;
    std::atomic<size_t> write_finish_index_ = 0;
    std::atomic<size_t> read_index_ = 0;

public:
    free_lock_queue() = default;
    ~free_lock_queue() = default;
    
    free_lock_queue(const free_lock_queue &) = delete;
    free_lock_queue &operator=(const free_lock_queue &) = delete;

    bool empty(){
        return write_finish_index_ == write_index_ && write_finish_index_ == read_index_;
    }

    size_t size(){
        if(read_index_ < write_finish_index_){
            return write_finish_index_ - read_index_;
        }
        else if(read_index_ == write_finish_index_){
            return 0;
        }
        
        return SIZE_T_MAX - read_index_ + write_finish_index_ + 1;
    }

    void push_back(std::shared_ptr<T> &itemptr){
        size_t index = write_index_++;
        v_[index % SIZE] = itemptr;
        size_t e_index = index;
        while(!write_finish_index_.compare_exchange_weak(e_index,e_index + 1,std::memory_order_release)) {
            e_index = index;
            std::this_thread::yield();
        }
    }

    std::shared_ptr<T> pop_front(){
        size_t index = read_index_.load(std::memory_order_relaxed);
        while(index < write_finish_index_.load(std::memory_order_relaxed)){
            if(read_index_.compare_exchange_strong(index,index + 1)){
                return std::move(v_[index % SIZE]);
            }
        }

        return nullptr;
    }
};

}
}