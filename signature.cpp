#include "signature.h"
#include "md5.h"
#include <sstream>
#include <iomanip>
#include <cstring>
#include <fstream>
#include <iostream>
#include <chrono>

Signature::Signature(const string &out_filename, int64_t blocks_count,
                     int64_t thread_count):out_filename_(out_filename), blocksCount_(blocks_count)
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
    if(ep) {
        rethrow_exception (ep);
    }
}

void Signature::appendData(SignatureData data)
{
    if(!ep) {
        in_data_queue_.push(move(data));
        cv_data_available_.notify_one();
    } else {
        exception_ptr tmp_ep = ep;
        ep = nullptr;
        rethrow_exception (tmp_ep);
    }
}

void Signature::workerLoop(int thread_id)
{
    try {
        while(isStart_ || in_data_queue_.size() > 0) {
            while(SignatureData data = in_data_queue_.take()) {
                calcHash(move(data));
            }
            if(isStart_) {
                unique_lock<mutex> lock(cv_data_available_mutex_);
                cv_data_available_.wait_for(lock, std::chrono::milliseconds(100));
            }
        }
    } catch (...) {
        throwException();
    }
}

void Signature::calcHash(SignatureData data)
{
    uint8_t result[digets_size_];
    md5(reinterpret_cast<uint8_t*>(data->get_data()), data->get_data_size(), result);
    stringstream stringStream;
    for (int i = 0; i < digets_size_; i++)
        stringStream << setfill('0') << setw(2) << hex  << static_cast<unsigned int>(result[i]);
    string md5sum = stringStream.str();
    SignatureData write_data(new block_data(md5sum.length(), data->get_block_num()));
    memcpy(write_data->get_data(), md5sum.c_str(), write_data->get_data_size());

    out_data_queue_.push(move(write_data));
    cv_write_available_.notify_one();
}

void Signature::writeData()
{
    try
    {
        int64_t blocksReady = 0;
        ofstream out;
        out.open(out_filename_, ios::out | ios::trunc);
        if (!out) {
            throw runtime_error("Can't open output file");
        }

        while(isWriterStart_ || out_data_queue_.size() > 0) {
            while(SignatureData data = out_data_queue_.take()) {
                out.seekp(data->get_block_num() * (data->get_data_size()), ios::beg);
                out.write(data->get_data(), data->get_data_size());
                blocksReady++;
                printf("Block %ld complete (%ld/%ld blocks ready)\n", data->get_block_num() + 1,
                       blocksReady, blocksCount_);
            }

            if(isWriterStart_) {
                unique_lock<mutex> lock(cv_write_available_mutex_);
                cv_write_available_.wait_for(lock, std::chrono::milliseconds(100));
            }
        }
    } catch (...) {
        throwException();
    }
}

void Signature::throwException()
{
    ep = current_exception();
    in_data_queue_.erase();
    out_data_queue_.erase();
    isStart_ = false;
    isWriterStart_ = false;
}
