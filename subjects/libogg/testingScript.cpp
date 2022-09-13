#include <ogg/ogg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vorbis/codec.h>

#include <fstream>
#include <iostream>

#ifdef _WIN32 /* We need the following two to set stdin/stdout to binary */
#include <fcntl.h>
#include <io.h>
#endif

#if defined(__MACOS__) && defined(__MWERKS__)
#include <console.h> /* CodeWarrior's Mac "command-line" support */
#endif

ogg_int16_t
    convbuffer[4096]; /* take 8k out of the data segment, not the stack */
int convsize = 4096;

extern void _VDBG_dump(void);

using namespace std;

/* help articles:
- https://bluishcoder.co.nz/2009/06/24/reading-ogg-files-using-libogg.html
- https://xiph.org/ogg/doc/libogg/decoding.html
- https://github.com/xiph/vorbis/blob/master/examples/decoder_example.c
*/

int main(int argc, char** argv) {
    // check that a file path has been provided
    if (argc < 2) {
        printf("ERROR: did not provide a file path as the first argument!\n");
        return -1;
    }

    // define libogg stuff
    ogg_sync_state oy;   /* sync and verify incoming physical bitstream */
    ogg_stream_state os; /* take physical pages, weld into a logical
                          stream of packets */
    ogg_page og;         /* one Ogg bitstream page. Vorbis packets are inside */
    ogg_packet op;       /* one raw packet of data for decode */

    vorbis_info vi; /* struct that stores all the static vorbis bitstream
                     settings */

    // open the file
    ifstream file(string(argv[1]), ios::in | ios::binary);

    // init libogg
    int ret = ogg_sync_init(&oy);

    cout <<"init done"<<endl;

    if (ret != 0) {
        abort();
    }
    cout <<"read the pages"<<endl;

    // get the pages
    int eos = 0; // = end of stream
    char* buffer;
    int bytes;
    
    while (!eos) {
        int result = ogg_sync_pageout(&oy, &og);

        if(result < 0) {
            // this means that there is an error
            abort();
        }

        if(ogg_page_eos(&og))eos=1;
         if(!eos){
          buffer=ogg_sync_buffer(&oy,4096);
          bytes=fread(buffer,1,4096,stdin);
          ogg_sync_wrote(&oy,bytes);
          if(bytes==0)eos=1;
         }
    }
}