#include <atomic>
#include <thread>
#include <memory>
#include "blockdata.h"
#include <exception>

using namespace std;

class Signature
{
public:
    Signature(const string &out_filename, int64_t blocks_count, int64_t thread_count = thread::hardware_concurrency());
    void force_stop();
    void join();
    void appendData(SignatureData data);

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
    void calcHash(SignatureData data);
    void writeData();
    void throwException();
    const string out_filename_;
    const int64_t blocksCount_;
    exception_ptr ep = nullptr;
};

