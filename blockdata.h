#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>

using namespace std;

class block_data
{
public:
    block_data(int64_t data_size, int64_t block_num);
    ~block_data();
    char *get_data();
    int64_t get_block_num();
    int64_t get_data_size();

private:
    unique_ptr<char []> data_;
    int64_t data_size_;
    int64_t block_num_;
};


class DataQueue
{
public:
    DataQueue(int max_size = 0);
    void setMaxSize(int max_size);
    void push(unique_ptr<block_data> data);
    unique_ptr<block_data> take();
    void erase();

private:
    queue<unique_ptr<block_data>> dataqueue_;
    mutex mutex_;
    condition_variable cv_item_pop_;
    mutex cv_item_pop_mutex_;
    int max_size_ = 0;
};
