#include "reader.h"
#include <iostream>
#include <string>

using namespace std;

int main(int argc, char *argv[])
{
    if(argc != 4) {
        cerr << "Invalid number of arguments" << endl;
        return 1;
    }

    int64_t block_size = stoi(argv[3]);
    Reader reader(argv[1], block_size);
    reader.startReader(argv[2]);

    return 0;
}
