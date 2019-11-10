#include "reader.h"
#include <iostream>
#include <string>
#include <exception>

using namespace std;

int main(int argc, char *argv[])
{
    try
    {
        if(argc < 3) {
            cerr << "Invalid number of arguments" << endl;
            return 1;
        }
        int64_t block_size = 1024*1024;
        if(argc > 3)
            block_size = stoi(argv[3]);
        Reader reader(argv[1], block_size);
        reader.startReader(argv[2]);
    } catch (const exception &e) {
        cerr << e.what() << endl;
        return 1;
    }

    return 0;
}
