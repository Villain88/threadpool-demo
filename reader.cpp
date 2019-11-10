#include "reader.h"
#include "signature.h"
#include <iostream>
#include <cstring>
#include <sys/stat.h>

Reader::Reader(const string &filename, int64_t block_size):filename_(filename), block_size_(block_size)
{

}

int Reader::startReader(const string &outname)
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

    int64_t filesize = st.st_size;
    cout << "Filesize: " << filesize << endl;
    blocks_count_ = filesize/block_size_;
    int64_t remaining_part = filesize % block_size_;
    if(remaining_part) {
        blocks_count_++;
        last_section_size_ = remaining_part;
    }
    cout << "Blocks count: " << blocks_count_ << endl;

    Signature signature(outname, blocks_count_);
    for(int64_t i = 0; i < blocks_count_; i++) {
        SignatureData data(new block_data(block_size_, i));
        readBlock(data->get_data(), data->get_block_num());
        signature.appendData(move(data));
    }
    signature.join();
    in.close();
}

bool Reader::readBlock(char *buffer, int64_t block_num)
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
