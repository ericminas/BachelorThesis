#define MINIAUDIO_IMPLEMENTATION
#include "../../miniaudio.h"
#include <stdio.h>
#include <stdlib.h>

/* complie command:
gcc examples/testingScript/testingscript.cpp -o testScript -lm
*/

// floating point error is wanted (line 27), 
// but the decoder should be able to read the valid file(s)

int main(int argc, char**argv){
     // check that a file path has been provided
    if (argc < 2)
    {
        printf("ERROR: did not provide a file path as the first argument!\n");
        return -1;
    }

    ma_result result;
    ma_decoder decoder;

      result = ma_decoder_init_file(argv[1], NULL, &decoder);
    if (result != MA_SUCCESS) {
        printf("Could not load file: %s\n", argv[1]);
        // force crash
        int a = 10 / 0;
        return -2;
    }

     ma_decoder_uninit(&decoder);
     return 0;
}