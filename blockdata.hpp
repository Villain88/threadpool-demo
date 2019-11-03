#include <queue>
#include <mutex>
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
        cout << "block data destructor" << endl;
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
    bool empty()
    {
        lock_guard<std::mutex> lock(mutex);
        return dataqueue_.empty();
    }

    int size()
    {
        lock_guard<std::mutex> lock(mutex);
        return dataqueue_.size();
    }

    void push(unique_ptr<block_data> data)
    {
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
        } else {
            throw out_of_range("Queue is empty");
        }
        return data;
    }

    void erase()
    {
        lock_guard<std::mutex> lock(mutex);
        //dataqueue_
    }

private:
    queue<unique_ptr<block_data>> dataqueue_;
    mutex mutex_;
};
