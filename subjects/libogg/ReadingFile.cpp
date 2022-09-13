// Copyright (C) 2009, Chris Double. All Rights Reserved.
// See the license at the end of this file.
#include <cassert>
#include <map>
#include <iostream>
#include <fstream>
#include <ogg/ogg.h>

using namespace std;

class OggStream
{
public:
  int mSerial;
  ogg_stream_state mState;
  int mPacketCount;

public:
  OggStream(int serial = -1) : 
    mSerial(serial),
    mPacketCount(0)
  { 
  }

  ~OggStream() {
    int ret = ogg_stream_clear(&mState);
    assert(ret == 0);
  }
};

typedef map<int, OggStream*> StreamMap; 

class OggDecoder
{
public:
  StreamMap mStreams;  

public:
  bool read_page(istream& stream, ogg_sync_state* state, ogg_page* page);
  void play(istream& stream);
};

bool OggDecoder::read_page(istream& stream, ogg_sync_state* state, ogg_page* page) {
  int ret = 0;
  if (!stream.good())
    return false;

  while(ogg_sync_pageout(state, page) != 1) {
    // Returns a buffer that can be written too
    // with the given size. This buffer is stored
    // in the ogg synchronisation structure.
    char* buffer = ogg_sync_buffer(state, 4096);
    assert(buffer);

    // Read from the file into the buffer
    stream.read(buffer, 4096);
    int bytes = stream.gcount();
    if (bytes == 0) {
      // End of file
      return false;
    }

    // Update the synchronisation layer with the number
    // of bytes written to the buffer
    ret = ogg_sync_wrote(state, bytes);
    assert(ret == 0);
  }
  return true;
}

void OggDecoder::play(istream& stream) {
  ogg_sync_state state;
  ogg_page page;
  int packets = 0;

  int ret = ogg_sync_init(&state);
  assert(ret == 0);
  
  while (read_page(stream, &state, &page)) {
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
      
    // Return a complete packet of data from the stream
    ogg_packet packet;
    ret = ogg_stream_packetout(&stream->mState, &packet);
    assert(ret == 0 || ret == 1);
    if (ret == 0) {
      // Need more data to be able to complete the packet
      continue;
    }

    // A packet is available, this is what we pass to the vorbis or
    // theora libraries to decode.
    stream->mPacketCount++;
  }

  // Cleanup
  ret = ogg_sync_clear(&state);
  assert(ret == 0);
}

void usage() {
  cout << "Usage: plogg <filename>" << endl;
}

int main(int argc, const char* argv[]) {
  if (argc != 2) { 
    usage();
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
      cout << stream->mSerial << " has " << stream->mPacketCount << " packets" << endl;
      delete stream;
    }
  }
  
  return 0;
}