/*Platform specific*/
#include <sys/stat.h>

/*C++ headers*/
#include <atomic>
#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <cstring>
#include <thread>
#include <condition_variable>

/*own headers*/
#include "md5.h"
#include "blockdata.hpp"

using namespace std;
mutex g_readfile_mutex;
mutex g_writefile_mutex;
size_t file_part = -1;
ifstream in;
ofstream out;
size_t block_size = 1024*1024;
size_t blocks_count = 0;
size_t last_section_size = block_size;
const int digets_size = 16;
const int digets_str_size = digets_size*2;


class Signature
{
public:
    Signature(size_t thread_count = thread::hardware_concurrency())
    {
        cout << thread_count << endl;
        isStart_ = true;
        threads_.reserve(thread_count);
        for(auto i = 0; i < thread_count; i++) {
            threads_.push_back(thread([=] {calcBlockHash(i+1);}));
        }
    }

    void force_stop()
    {
        data_queue_.erase();
    }

    void join()
    {
        isStart_ = false;
        cv_data_available_.notify_all();
        for(auto &th: threads_) {
            if(th.joinable()) {
                th.join();
            }
        }
    }

    void appendData(block_data *data)
    {
        data_queue_.push(unique_ptr<block_data>(data));
        cv_data_available_.notify_one();
    }

private:
    atomic_bool isStart_ = {false};
    vector<thread> threads_;
    condition_variable cv_data_available_;
    mutex cv_data_available_mutex_;
    DataQueue data_queue_;
    void calcBlockHash(int thread_id)
    {
        cout << "thread: " << thread_id << " start" << endl;
        std::unique_lock<std::mutex> lock(cv_data_available_mutex_);
        while(isStart_ || !data_queue_.empty()) {
            cout << "thread: " << thread_id << " while" << endl;
            if(!data_queue_.empty()) {
                unique_ptr<block_data> data =data_queue_.take();
                uint8_t result[digets_size];
                md5(reinterpret_cast<uint8_t*>(data.get()->get_data()), data.get()->get_data_size(), result);
                stringstream stringStream;
                for (int i = 0; i < digets_size; i++)
                    stringStream << setfill('0') << setw(2) << std::hex  << static_cast<unsigned int>(result[i]);
                string md5sum = stringStream.str();
                cout << "Block " << data->get_block_num() << " md5 " << md5sum << endl;
                this_thread::sleep_for(std::chrono::milliseconds(100));
            } else {
                cv_data_available_.wait(lock);
            }
        }
        cout << "thread: " << thread_id << " stop" << endl;
    }
};

bool readBlock(char* buffer, size_t block_num)
{
    in.seekg(block_num*block_size, ios::beg);
    if(block_num+1 != blocks_count) {
       in.read(buffer, block_size);
    } else {
        memset(buffer, 0, block_size);
        in.read(buffer, last_section_size);
        cout << "Last section size " << last_section_size << endl;
    }

    if (!in)
        cout << "error: only " << in.gcount() << " could be read";

    return true;
}

bool writeBlock(const char* str, size_t block_num)
{
    lock_guard<mutex> lock(g_writefile_mutex);
    out.seekp(block_num * digets_str_size, ios::beg);
    out.write(str, digets_str_size);
    return true;
}


int main()
{
    const char* fname = "/home/petr/tmp_file";
    const char* outname = "/home/petr/tmp_file_md5";
    in.open(fname, ios::in | ios::binary);
    if (!in) {
        cout << "Cannot open file.\n";
        return 1;
    }
    struct stat st;
    if(stat(fname, &st) < 0) {
      std::cerr << "stat failed, fname = "
                << fname << ", " << strerror(errno) << std::endl;
      return 1;
    }

    size_t filesize = (size_t)st.st_size;
    cout << "filesize " << filesize << endl;
    blocks_count = filesize/block_size;
    size_t remaining_part = filesize % block_size;
    if(remaining_part) {
        blocks_count++;
        last_section_size = remaining_part;
    }
    cout << "blocks count " << blocks_count << endl;
    out.open(outname, ios::out | ios::trunc);

    Signature signature;
    for(size_t i = 0; i < blocks_count; i++) {
        block_data *data = new block_data(block_size, i);
        readBlock(data->get_data(), data->get_block_num());
        signature.appendData(data);
    }
    signature.join();
    in.close();

    return 0;
}
