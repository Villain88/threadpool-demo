#include "blockdata.h"

block_data::block_data(int64_t data_size, int64_t block_num)
{
    data_size_ = data_size;
    block_num_ = block_num;
    data_ = unique_ptr<char []>(new char[data_size_]);
}

block_data::~block_data()
{
    //cout << "block data destructor" << endl;
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

void DataQueue::push(unique_ptr<block_data> data)
{
    if(max_size_ > -1) {
        while(dataqueue_.size() > max_size_) {
            std::unique_lock<std::mutex> lock(cv_item_pop_mutex_);
            cv_item_pop_.wait(lock);
        }
    }
    unique_lock<std::mutex> lock(mutex);
    dataqueue_.push(move(data));
}

unique_ptr<block_data> DataQueue::take()
{
    lock_guard<std::mutex> lock(mutex);
    unique_ptr<block_data> data;
    if(dataqueue_.size() > 0) {
        data = std::move(dataqueue_.front());
        dataqueue_.pop();
        cv_item_pop_.notify_one();
    } else {
        return unique_ptr<block_data>(nullptr);
    }
    return data;
}

void DataQueue::erase()
{
    lock_guard<std::mutex> lock(mutex);
    while (!dataqueue_.empty()) {
        dataqueue_.pop();
    }
    cv_item_pop_.notify_one();
}
