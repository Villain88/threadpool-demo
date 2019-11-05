#include "signature.h"
#include "md5.h"
#include <sstream>
#include <iomanip>
#include <cstring>
#include <fstream>
#include <iostream>

Signature::Signature(const string &out_filename, size_t blocks_count,
                     size_t thread_count):out_filename_(out_filename), blocksCount_(blocks_count)
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

void Signature::force_stop()
{
    in_data_queue_.erase();
    out_data_queue_.erase();
    join();
}

void Signature::join()
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

void Signature::appendData(unique_ptr<block_data> data)
{
    in_data_queue_.push(move(data));
    cv_data_available_.notify_one();
}

void Signature::workerLoop(int thread_id)
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

void Signature::calcHash(unique_ptr<block_data> data)
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

void Signature::writeData()
{
    size_t blocksReady = 0;
    ofstream out;
    out.open(out_filename_, ios::out | ios::trunc);

    while(1) {
        if(isWriterStart_) {
            unique_lock<mutex> lock(cv_write_available_mutex_);
            cv_write_available_.wait(lock);
        } else {
            break;
        }

        while(unique_ptr<block_data> data = out_data_queue_.take()) {
            out.seekp(data->get_block_num() * (data->get_data_size()), ios::beg);
            out.write(data->get_data(), data->get_data_size());
            blocksReady++;
            printf("Block %ld complite (%ld/%ld blocks ready)\n", data->get_block_num() + 1,
                   blocksReady, blocksCount_);
        }
    }
}
