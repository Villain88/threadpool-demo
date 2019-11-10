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
using SignatureData = unique_ptr<block_data>;

class DataQueue
{
public:
    DataQueue(int max_size = 0);
    void setMaxSize(int max_size);
    void push(SignatureData data);
    SignatureData take();
    void erase();
    size_t size();

private:
    queue<SignatureData> dataqueue_;
    mutex queue_mutex_;
    condition_variable cv_item_pop_;
    mutex cv_item_pop_mutex_;
    int max_size_ = -1;
};
