#include <boost/thread/executors/basic_thread_pool.hpp>
#include <boost/uuid/sha1.hpp>
#include <atomic>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <mutex>
#include "md5.h"
#include <sys/stat.h>

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

bool readBlock(char* buffer, size_t *block_num)
{
    lock_guard<mutex> lock(g_readfile_mutex);
    file_part++;
    *block_num = file_part;
    in.seekg(file_part*block_size, ios::beg);

    if(*block_num+1 != blocks_count) {
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

void calcBlockHash()
{
    char *buffer = new char[block_size];
    size_t block_num = 0;
    readBlock(buffer, &block_num);
    uint8_t result[digets_size];
    md5(reinterpret_cast<uint8_t*>(buffer), block_size, result);
    stringstream stringStream;
    for (int i = 0; i < digets_size; i++)
        stringStream << setfill('0') << setw(2) << std::hex  << static_cast<unsigned int>(result[i]);
    string md5sum = stringStream.str();
    cout << "Block " << block_num << " md5 " << md5sum << endl;
    delete[] buffer;
    writeBlock(md5sum.c_str(), block_num);
    return;
}

int main()
{
    //using default hardware_concurency thread count
    boost::executors::basic_thread_pool pool;
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
    cout << "blockscount " << blocks_count << endl;
    out.open(outname, ios::out | ios::trunc);


    for(size_t i = 0; i < blocks_count; i++) {
        pool.submit(&calcBlockHash);
    }
    pool.close();
    pool.join();
    in.close();

    return 0;
}
