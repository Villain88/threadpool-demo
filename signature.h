#include <atomic>
#include <thread>
#include <memory>
#include "blockdata.h"

using namespace std;

class Signature
{
public:
    Signature(const string &out_filename, size_t blocks_count, size_t thread_count = thread::hardware_concurrency());
    void force_stop();
    void join();
    void appendData(unique_ptr<block_data> data);

private:
    atomic_bool isStart_ = {false};
    atomic_bool isWriterStart_ = {false};
    vector<thread> threads_;
    thread writerThread_;
    condition_variable cv_data_available_;
    mutex cv_data_available_mutex_;
    condition_variable cv_write_available_;
    mutex cv_write_available_mutex_;
    DataQueue in_data_queue_;
    DataQueue out_data_queue_;
    const int digets_size_ = 16;
    void workerLoop(int thread_id);
    void calcHash(unique_ptr<block_data> data);
    void writeData();
    const string out_filename_;
    const size_t blocksCount_;
};

