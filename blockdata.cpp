#include "blockdata.h"
#include <chrono>

block_data::block_data(int64_t data_size, int64_t block_num)
{
    data_size_ = data_size;
    block_num_ = block_num;
    data_ = unique_ptr<char []>(new char[data_size_]);
}

block_data::~block_data()
{

}

char *block_data::get_data()
{
    return data_.get();
}

int64_t block_data::get_block_num()
{
    return block_num_;
}

int64_t block_data::get_data_size()
{
    return data_size_;
}

DataQueue::DataQueue(int max_size):max_size_(max_size)
{

}

void DataQueue::setMaxSize(int max_size)
{
    max_size_ = max_size;
}

void DataQueue::push(SignatureData data)
{
    //Too manu items in queue
    if(max_size_ > -1) {
        while(dataqueue_.size() > max_size_) {
            std::unique_lock<std::mutex> lock(cv_item_pop_mutex_);
            cv_item_pop_.wait_for(lock, std::chrono::milliseconds(100));
        }
    }
    unique_lock<std::mutex> lock(queue_mutex_);
    dataqueue_.push(move(data));
}

SignatureData DataQueue::take()
{
    lock_guard<std::mutex> lock(queue_mutex_);
    SignatureData data(nullptr);
    if(dataqueue_.size() > 0) {
        data = move(dataqueue_.front());
        dataqueue_.pop();
        cv_item_pop_.notify_one();
    }
    return data;
}

void DataQueue::erase()
{
    lock_guard<std::mutex> lock(queue_mutex_);
    while (!dataqueue_.empty()) {
        dataqueue_.pop();
    }
    cv_item_pop_.notify_one();
}

size_t DataQueue::size()
{
    lock_guard<std::mutex> lock(queue_mutex_);
    return dataqueue_.size();
}
