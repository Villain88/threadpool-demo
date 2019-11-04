#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>

using namespace std;

class block_data
{
public:
    block_data(size_t data_size, size_t block_num)
    {
        data_size_ = data_size;
        block_num_ = block_num;
        data_ = unique_ptr<char []>(new char[data_size_]);
    }
    ~block_data()
    {
        //cout << "block data destructor" << endl;
    }
    char *get_data()
    {
        return data_.get();
    }
    size_t get_block_num()
    {
        return block_num_;
    }
    size_t get_data_size()
    {
        return data_size_;
    }

private:
    unique_ptr<char []> data_;
    size_t data_size_;
    size_t block_num_;
};



class DataQueue
{
public:
    void setMaxSize(int max_size)
    {
        max_size_ = max_size;
    }
    void push(unique_ptr<block_data> data)
    {
        while(dataqueue_.size() > max_size_) {
            std::unique_lock<std::mutex> lock(cv_item_pop_mutex_);
            cv_item_pop_.wait(lock);
        }
        lock_guard<std::mutex> lock(mutex);
        dataqueue_.push(move(data));
    }

    unique_ptr<block_data> take()
    {
        lock_guard<std::mutex> lock(mutex);
        unique_ptr<block_data> data;
        if(dataqueue_.size() > 0) {
            data = std::move(dataqueue_.front());
            dataqueue_.pop();
            cv_item_pop_.notify_one();
        } else {
            return unique_ptr<block_data>();
        }
        return data;
    }

    void erase()
    {
        lock_guard<std::mutex> lock(mutex);
        while (!dataqueue_.empty()) {
             dataqueue_.pop();
        }
        cv_item_pop_.notify_one();
    }

private:
    queue<unique_ptr<block_data>> dataqueue_;
    mutex mutex_;
    condition_variable cv_item_pop_;
    mutex cv_item_pop_mutex_;
    int max_size_ = -1;
};
