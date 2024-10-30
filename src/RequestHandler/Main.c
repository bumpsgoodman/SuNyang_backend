// 작성자: bumpsgoodman

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "../Common/PrimitiveType.h"

typedef struct SERVER_INFO_DESC
{
    uint_t Port;
} SERVER_INFO_DESC;

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Error: Unable to read server information file.\n");
        return -1;
    }



    return 0;
}