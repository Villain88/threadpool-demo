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

/*Own headers*/
#include "md5.h"
#include "blockdata.hpp"

using namespace std;

size_t file_part = -1;
ifstream in;
ofstream out;
size_t block_size = 1024*1024;
size_t blocks_count = 0;
size_t last_section_size = block_size;
const int digets_size = 16;

class Signature
{
public:
    Signature(size_t thread_count = thread::hardware_concurrency())
    {
        in_data_queue_.setMaxSize(thread_count*10);
        out_data_queue_.setMaxSize(thread_count*10);
        isStart_ = true;
        threads_.reserve(thread_count);
        for(auto i = 0; i < thread_count; i++) {
            threads_.push_back(thread([=] {workerLoop(i+1);}));
        }
        threads_.push_back(thread([=] {writeData();}));
    }

    void force_stop()
    {
        in_data_queue_.erase();
        out_data_queue_.erase();
        join();
    }

    void join()
    {
        isStart_ = false;
        cv_data_available_.notify_all();
        cv_write_available_.notify_all();
        for(auto &th: threads_) {
            if(th.joinable()) {
                th.join();
            }
        }
    }

    void appendData(unique_ptr<block_data> data)
    {
        in_data_queue_.push(move(data));
        cv_data_available_.notify_one();
    }

private:
    atomic_bool isStart_ = {false};
    vector<thread> threads_;
    condition_variable cv_data_available_;
    mutex cv_data_available_mutex_;
    condition_variable cv_write_available_;
    mutex cv_write_available_mutex_;
    DataQueue in_data_queue_;
    DataQueue out_data_queue_;
    void workerLoop(int thread_id)
    {
        while(1) {
            if(isStart_) {
                std::unique_lock<std::mutex> lock(cv_data_available_mutex_);
                cv_data_available_.wait(lock);
            } else {
                break;
            }

            while(unique_ptr<block_data> data = in_data_queue_.take()) {
                calcHash(move(data));
            }
        }
    }

    void calcHash(unique_ptr<block_data> data)
    {
        uint8_t result[digets_size];
        md5(reinterpret_cast<uint8_t*>(data->get_data()), data->get_data_size(), result);
        stringstream stringStream;
        for (int i = 0; i < digets_size; i++)
            stringStream << setfill('0') << setw(2) << std::hex  << static_cast<unsigned int>(result[i]);
        string md5sum = stringStream.str();
        //cout << "Block " << data->get_block_num() << " md5 " << md5sum <<endl;
        unique_ptr<block_data> write_data(new block_data(md5sum.length(), data->get_block_num()));
        memcpy(write_data->get_data(), md5sum.c_str(), write_data->get_data_size());
        out_data_queue_.push(move(write_data));
        cv_write_available_.notify_one();
    }

    void writeData()
    {
        ofstream out;
        const char* outname = "/home/petr/tmp_file_md5";
        out.open(outname, ios::out | ios::trunc);

        while(1) {
            if(isStart_) {
                std::unique_lock<std::mutex> lock(cv_write_available_mutex_);
                cv_write_available_.wait(lock);
            } else {
                break;
            }

            while(unique_ptr<block_data> data = out_data_queue_.take()) {
                cout << "Block " << data->get_block_num() << " md5 ready for write " << data->get_data() <<endl;
                out.seekp(data->get_block_num() * (data->get_data_size()), ios::beg);
                out.write(data->get_data(), data->get_data_size());
            }
        }
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
    }

    if (!in)
        throw std::runtime_error("Error reading file");

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
        unique_ptr<block_data> data(new block_data(block_size, i));
        readBlock(data->get_data(), data->get_block_num());
        signature.appendData(move(data));
    }
    signature.join();
    in.close();

    return 0;
}
