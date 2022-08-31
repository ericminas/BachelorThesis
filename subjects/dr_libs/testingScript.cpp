#define DR_FLAC_IMPLEMENTATION
#include <stdio.h>
#include <stdlib.h>
#include "./dr_flac.h"

int main(int argc, char **argv)
{

    // check that a file path has been provided
    if (argc < 2)
    {
        printf("ERROR: did not provide a file path as the first argument!\n");
        return -1;
    }

    // decoding the file
    unsigned int channels;
    unsigned int sampleRate;
    drflac_uint64 totalPCMFrameCount;

    drflac_int32 *pSampleData = drflac_open_file_and_read_pcm_frames_s32(argv[1], &channels, &sampleRate, &totalPCMFrameCount, NULL);

    if (pSampleData == NULL)
    {
        // Failed to open and decode FLAC file.
        printf("failed to open FLAC file (path: %s)\n", argv[1]);
        abort();
    }

    printf("file at: %s has:\t#channels: %d, smaple rate: %d\n", argv[1], channels, sampleRate);

    // actually plays the file.
    // TODO can it be played but without an audio device?
    // drflac_free(pSampleData, NULL);
    return 0;
}