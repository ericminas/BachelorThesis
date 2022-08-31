#define DR_FLAC_IMPLEMENTATION
#include "../dr_flac.h"


int main() {
unsigned int channels;
unsigned int sampleRate;
drflac_uint64 totalPCMFrameCount;
drflac_int32* pSampleData = drflac_open_file_and_read_pcm_frames_s32("./../../BachelorThesis/fLaC/sample2.flac", &channels, &sampleRate, &totalPCMFrameCount, NULL);

if (pSampleData == NULL) {
    // Failed to open and decode FLAC file.
    printf("failed to open FLAC file");
}

printf("#channels: %d, smaple rate: %d\n", channels, sampleRate);

//drflac_free(pSampleData, NULL);

}