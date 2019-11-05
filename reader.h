/*C++ headers*/
#include <fstream>

using namespace std;

class Reader
{
public:
    Reader(const string &filename, int64_t block_size);
    int startReader(const string &outname);

private:
    const string filename_;
    const int64_t block_size_;
    int64_t blocks_count_;
    int64_t last_section_size_;
    ifstream in;

    bool readBlock(char* buffer, int64_t block_num);
};
