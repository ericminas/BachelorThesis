// Copyright (C) 2009, Chris Double. All Rights Reserved.
// See the license at the end of this file.
#include <cassert>
#include <cmath>
#include <map>
#include <iostream>
#include <fstream>
#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <SDL/SDL.h>

using namespace std;

enum StreamType {
  TYPE_VORBIS,
  TYPE_THEORA,
  TYPE_UNKNOWN
};

class OggStream;

class VorbisDecode {
public:
  vorbis_info mInfo;
  vorbis_comment mComment;
  vorbis_dsp_state mDsp;
  vorbis_block mBlock;

public:
  VorbisDecode()
  {
    vorbis_info_init(&mInfo);
    vorbis_comment_init(&mComment);    
  }

  void initForData(OggStream* stream);
};

class OggStream
{
public:
  int mSerial;
  ogg_stream_state mState;
  StreamType mType;
  bool mActive;
  VorbisDecode mVorbis;

public:
  OggStream(int serial = -1) : 
    mSerial(serial),
    mType(TYPE_UNKNOWN),
    mActive(true)
  { 
  }

  ~OggStream() {
    int ret = ogg_stream_clear(&mState);
    assert(ret == 0);
  }
};

void VorbisDecode::initForData(OggStream* stream) {
  int ret = vorbis_synthesis_init(&stream->mVorbis.mDsp, &stream->mVorbis.mInfo);
  assert(ret == 0);
  ret = vorbis_block_init(&stream->mVorbis.mDsp, &stream->mVorbis.mBlock);
  assert(ret == 0);
}

typedef map<int, OggStream*> StreamMap; 

class OggDecoder
{
public:
  StreamMap mStreams;  
  SDL_Surface* mSurface;
  SDL_Overlay* mOverlay;
  ogg_int64_t  mGranulepos;
  
private:
  bool handle_vorbis_header(OggStream* stream, ogg_packet* packet);
  void read_headers(istream& stream, ogg_sync_state* state);

  bool read_page(istream& stream, ogg_sync_state* state, ogg_page* page);
  bool read_packet(istream& is, ogg_sync_state* state, OggStream* stream, ogg_packet* packet);

public:
  OggDecoder() :
    mSurface(0),
    mOverlay(0),
    mGranulepos(0)
  {
  }

  ~OggDecoder() {
    
    if (mSurface) {
      SDL_FreeSurface(mSurface);
      mSurface = 0;
    }
  }
  void play(istream& stream);
};

bool OggDecoder::read_page(istream& stream, ogg_sync_state* state, ogg_page* page) {
  int ret = 0;

  // If we've hit end of file we still need to continue processing
  // any remaining pages that we've got buffered.
  if (!stream.good())
    return ogg_sync_pageout(state, page) == 1;

  while((ret = ogg_sync_pageout(state, page)) != 1) {
    // Returns a buffer that can be written too
    // with the given size. This buffer is stored
    // in the ogg synchronisation structure.
    char* buffer = ogg_sync_buffer(state, 4096);
    assert(buffer);

    // Read from the file into the buffer
    stream.read(buffer, 4096);
    int bytes = stream.gcount();
    if (bytes == 0) {
      // End of file. 
      continue;
    }

    // Update the synchronisation layer with the number
    // of bytes written to the buffer
    ret = ogg_sync_wrote(state, bytes);
    assert(ret == 0);
  }
  return true;
}

bool OggDecoder::read_packet(istream& is, ogg_sync_state* state, OggStream* stream, ogg_packet* packet) {
  int ret = 0;

  while ((ret = ogg_stream_packetout(&stream->mState, packet)) != 1) {
    ogg_page page;
    if (!read_page(is, state, &page))
      return false;

    int serial = ogg_page_serialno(&page);
    assert(mStreams.find(serial) != mStreams.end());
    OggStream* pageStream = mStreams[serial];
    
    // Drop data for streams we're not interested in.
    if (stream->mActive) {
      ret = ogg_stream_pagein(&pageStream->mState, &page);
      assert(ret == 0);
    }
  }
  return true;
}

void OggDecoder::read_headers(istream& stream, ogg_sync_state* state) {
  ogg_page page;

  bool headersDone = false;
  while (!headersDone && read_page(stream, state, &page)) {
    int ret = 0;
    int serial = ogg_page_serialno(&page);
    OggStream* stream = 0;

    if(ogg_page_bos(&page)) {
      // At the beginning of the stream, read headers
      // Initialize the stream, giving it the serial
      // number of the stream for this page.
      stream = new OggStream(serial);
      ret = ogg_stream_init(&stream->mState, serial);
      assert(ret == 0);
      mStreams[serial] = stream;
    }

    assert(mStreams.find(serial) != mStreams.end());
    stream = mStreams[serial];

    // Add a complete page to the bitstream
    ret = ogg_stream_pagein(&stream->mState, &page);
    assert(ret == 0);

    // Process all available header packets in the stream. When we hit
    // the first data stream we don't decode it, instead we
    // return. The caller can then choose to process whatever data
    // streams it wants to deal with.
    ogg_packet packet;
    while (!headersDone &&
	   (ret = ogg_stream_packetpeek(&stream->mState, &packet)) != 0) {
      assert(ret == 1);

      // A packet is available. If it is not a header packet we exit.
      // If it is a header packet, process it as normal.
      headersDone = headersDone || handle_vorbis_header(stream, &packet);
      if (!headersDone) {
	// Consume the packet
	ret = ogg_stream_packetout(&stream->mState, &packet);
	assert(ret == 1);
      }
    }
  } 
}

void OggDecoder::play(istream& is) {
  ogg_sync_state state;

  int ret = ogg_sync_init(&state);
  assert(ret == 0);

  // Read headers for all streams
  read_headers(is, &state);

  // Find and initialize the first theora and vorbis
  // streams. According to the Theora spec these can be considered the
  // 'primary' streams for playback.
  
  OggStream* audio = 0;
  for(StreamMap::iterator it = mStreams.begin(); it != mStreams.end(); ++it) {
    OggStream* stream = (*it).second;
    if (!audio && stream->mType == TYPE_VORBIS)
    {
      audio = stream;
      audio->mVorbis.initForData(audio);
    }
    else
      stream->mActive = false;
  }

  assert(audio);

  cout << "Audio stream is " 
       << audio->mSerial << " "
       << audio->mVorbis.mInfo.channels << " channels "
       << audio->mVorbis.mInfo.rate << "KHz"
       << endl;

  // Cleanup
  ret = ogg_sync_clear(&state);
  assert(ret == 0);
}

bool OggDecoder::handle_vorbis_header(OggStream* stream, ogg_packet* packet) {
  int ret = vorbis_synthesis_headerin(&stream->mVorbis.mInfo,
				      &stream->mVorbis.mComment,
				      packet);
  // Unlike libtheora, libvorbis does not provide a return value to
  // indicate that we've finished loading the headers and got the
  // first data packet. To detect this I check if I already know the
  // stream type and if the vorbis_synthesis_headerin call failed.
  if (stream->mType == TYPE_VORBIS && ret == OV_ENOTVORBIS) {
    // First data packet
    return true;
  }
  else if (ret == 0) {
    stream->mType = TYPE_VORBIS;
  }
  return false;
}

void usage() {
  cout << "Usage: plogg <filename>" << endl;
}

int main(int argc, char* argv[]) {
  if (argc != 2) { 
    usage();
    return 0;
  }

  ifstream file(argv[1], ios::in | ios::binary);
  if (file) {
    OggDecoder decoder;
    decoder.play(file);
    file.close();
    for(StreamMap::iterator it = decoder.mStreams.begin();
	it != decoder.mStreams.end();
	++it) {
      OggStream* stream = (*it).second;
      delete stream;
    }
  }
  SDL_Quit();
  return 0;
}