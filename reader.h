/*C++ headers*/
#include <fstream>

using namespace std;

class Reader
{
public:
    Reader(const string &filename, size_t block_size);
    int startReader(const string &outname);

private:
    const string filename_;
    const size_t block_size_;
    size_t blocks_count_;
    size_t last_section_size_;
    ifstream in;

    bool readBlock(char* buffer, size_t block_num);
};
