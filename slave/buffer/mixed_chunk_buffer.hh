#ifndef __SLAVE_BUFFER_MIXED_CHUNK_BUFFER_HH__
#define __SLAVE_BUFFER_MIXED_CHUNK_BUFFER_HH__

#include <cstdio>
#include "data_chunk_buffer.hh"
#include "parity_chunk_buffer.hh"

enum ChunkBufferRole {
	CBR_DATA,
	CBR_PARITY
};

class MixedChunkBuffer {
public:
	ChunkBufferRole role;
	union {
		DataChunkBuffer *data;
		ParityChunkBuffer *parity;
	} buffer;

	MixedChunkBuffer( DataChunkBuffer *dataChunkBuffer );
	MixedChunkBuffer( ParityChunkBuffer *parityChunkBuffer );
	void set( char *key, uint8_t keySize, char *value, uint32_t valueSize, uint8_t opcode, uint32_t chunkId, Chunk **chunks = 0, Chunk *dataChunk = 0, Chunk *parityChunk = 0 );
	int lockChunk( Chunk *chunk );
	void updateAndUnlockChunk( int index );
	void update( uint32_t stripeId, uint32_t chunkId, uint32_t offset, uint32_t size, char *dataDelta, Chunk **dataChunks, Chunk *dataChunk, Chunk *parityChunk );
	void print( FILE *f = stdout );
	void stop();
	~MixedChunkBuffer();
};

#endif
