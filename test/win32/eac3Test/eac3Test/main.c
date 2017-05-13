#include <stdlib.h>

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#define MY_DEBUG_NEW new(_CLIENT_BLOCK, __FILE__, __LINE__)   
#define new MY_DEBUG_NEW 

void TestDecoder();

int main(int argc, char **argv)
{
    TestDecoder();

    char *test = malloc(10);
    _CrtDumpMemoryLeaks(); 

    return 0;
}
