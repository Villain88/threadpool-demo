#include "reader.h"

using namespace std;

int main()
{
    const char* inname = "/home/petr/tmp_file";
    const char* outname = "/home/petr/tmp_file_md5";
    size_t block_size = 1024*1024;
    Reader reader(inname, block_size);
    reader.startReader(outname);

    return 0;
}
