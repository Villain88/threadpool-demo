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

class Signature
{
public:
    Signature(size_t thread_count = thread::hardware_concurrency())
    {
        in_data_queue_.setMaxSize(thread_count*10);
        out_data_queue_.setMaxSize(thread_count*10);
        isStart_ = true;
        isWriterStart_ = true;
        threads_.reserve(thread_count);
        for(auto i = 0; i < thread_count; i++) {
            threads_.push_back(thread([=] {workerLoop(i+1);}));
        }
        writerThread_ = thread([=] {writeData();});
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

        for(auto &th: threads_) {
            if(th.joinable()) {
                th.join();
            }
        }

        isWriterStart_ = false;
        cv_write_available_.notify_all();
        if(writerThread_.joinable())
            writerThread_.join();
    }

    void appendData(unique_ptr<block_data> data)
    {
        in_data_queue_.push(move(data));
        cv_data_available_.notify_one();
    }

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
    void workerLoop(int thread_id)
    {
        while(1) {
            if(isStart_) {
                unique_lock<mutex> lock(cv_data_available_mutex_);
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
        uint8_t result[digets_size_];
        md5(reinterpret_cast<uint8_t*>(data->get_data()), data->get_data_size(), result);
        stringstream stringStream;
        for (int i = 0; i < digets_size_; i++)
            stringStream << setfill('0') << setw(2) << hex  << static_cast<unsigned int>(result[i]);
        string md5sum = stringStream.str();
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
            if(isWriterStart_) {
                unique_lock<mutex> lock(cv_write_available_mutex_);
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

class Reader
{
public:
    Reader(const string &filename, size_t block_size):filename_(filename), block_size_(block_size)
    {

    }

    int startReader()
    {
        in.open(filename_, ios::in | ios::binary);
        if (!in) {
            cout << "Cannot open file.\n" << endl;
            return 1;
        }
        struct stat st;
        if(stat(filename_.c_str(), &st) < 0) {
          cerr << "stat failed, fname = "
                    << filename_ << ", " << strerror(errno) << endl;
          return 1;
        }

        last_section_size_ = block_size_;

        size_t filesize = (size_t)st.st_size;
        cout << "Filesize: " << filesize << endl;
        blocks_count_ = filesize/block_size_;
        size_t remaining_part = filesize % block_size_;
        if(remaining_part) {
            blocks_count_++;
            last_section_size_ = remaining_part;
        }
        cout << "Blocks count: " << blocks_count_ << endl;

        Signature signature;
        for(size_t i = 0; i < blocks_count_; i++) {
            unique_ptr<block_data> data(new block_data(block_size_, i));
            readBlock(data->get_data(), data->get_block_num());
            signature.appendData(move(data));
        }
        signature.join();
        in.close();
    }

private:
    const string filename_;
    const size_t block_size_;
    size_t blocks_count_;
    size_t last_section_size_;
    ifstream in;

    bool readBlock(char* buffer, size_t block_num)
    {
        in.seekg(block_num*block_size_, ios::beg);
        //check for last section
        if(block_num+1 != blocks_count_) {
           in.read(buffer, block_size_);
        } else {
            memset(buffer, 0, block_size_);
            in.read(buffer, last_section_size_);
        }

        if (!in)
            throw runtime_error("Error reading file");

        return true;
    }
};

int main()
{
    const char* fname = "/home/petr/tmp_file";

    size_t block_size = 1024*1024;
    Reader reader(fname, block_size);
    reader.startReader();

    return 0;
}
