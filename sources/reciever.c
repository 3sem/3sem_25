#include <stdio.h>

#include "barterer.h"
#include "common.h"

int main(int argc, const char** argv) {
    if (argc != 2) {
        ERR_R("Required one argument - destination file\n");
        return 0;
    }
    const char* file_name = argv[1];

    barter_t* data = barter_ctor();
    recieve_file(data, file_name);
    barter_dtor(data);

    return 0;
}
