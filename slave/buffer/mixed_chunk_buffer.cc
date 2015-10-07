#include "mixed_chunk_buffer.hh"
#include "../worker/worker.hh"

MixedChunkBuffer::MixedChunkBuffer( DataChunkBuffer *dataChunkBuffer ) {
	this->role = CBR_DATA;
	this->buffer.data = dataChunkBuffer;
}

MixedChunkBuffer::MixedChunkBuffer( ParityChunkBuffer *parityChunkBuffer ) {
	this->role = CBR_PARITY;
	this->buffer.parity = parityChunkBuffer;
}

bool MixedChunkBuffer::set( SlaveWorker *worker, char *key, uint8_t keySize, char *value, uint32_t valueSize, uint8_t opcode, uint32_t chunkId, Chunk **dataChunks, Chunk *dataChunk, Chunk *parityChunk ) {
	switch( this->role ) {
		case CBR_DATA:
			this->buffer.data->set( worker, key, keySize, value, valueSize, opcode );
			return true;
		case CBR_PARITY:
			return this->buffer.parity->set( key, keySize, value, valueSize, chunkId, dataChunks, dataChunk, parityChunk );
		default:
			return false;
	}
}

size_t MixedChunkBuffer::seal( SlaveWorker *worker ) {
	switch( this->role ) {
		case CBR_DATA:
			return this->buffer.data->seal( worker );
		case CBR_PARITY:
		default:
			return false;
	}
}

bool MixedChunkBuffer::seal( uint32_t stripeId, uint32_t chunkId, uint32_t count, char *sealData, size_t sealDataSize, Chunk **dataChunks, Chunk *dataChunk, Chunk *parityChunk ) {
	switch( this->role ) {
		case CBR_PARITY:
			return this->buffer.parity->seal( stripeId, chunkId, count, sealData, sealDataSize, dataChunks, dataChunk, parityChunk );
		case CBR_DATA:
		default:
			return false;
	}
}

int MixedChunkBuffer::lockChunk( Chunk *chunk ) {
	switch( this->role ) {
		case CBR_DATA:
			return this->buffer.data->lockChunk( chunk );
		case CBR_PARITY:
		default:
			return -1;
	}
}

void MixedChunkBuffer::updateAndUnlockChunk( int index ) {
	switch( this->role ) {
		case CBR_DATA:
			this->buffer.data->updateAndUnlockChunk( index );
			break;
		case CBR_PARITY:
		default:
			break;
	}
}

bool MixedChunkBuffer::deleteKey( char *keyStr, uint8_t keySize ) {
	switch( this->role ) {
		case CBR_PARITY:
			return this->buffer.parity->deleteKey( keyStr, keySize );
		default:
			return false;
	}
}

bool MixedChunkBuffer::updateKeyValue( char *keyStr, uint8_t keySize, uint32_t offset, uint32_t length, char *valueUpdate ) {
	switch( this->role ) {
		case CBR_PARITY:
			return this->buffer.parity->updateKeyValue( keyStr, keySize, offset, length, valueUpdate );
		default:
			return false;
	}
}

void MixedChunkBuffer::update( uint32_t stripeId, uint32_t chunkId, uint32_t offset, uint32_t size, char *dataDelta, Chunk **dataChunks, Chunk *dataChunk, Chunk *parityChunk ) {
	switch( this->role ) {
		case CBR_PARITY:
			this->buffer.parity->update( stripeId, chunkId, offset, size, dataDelta, dataChunks, dataChunk, parityChunk );
			break;
		default:
			return;
	}
}

void MixedChunkBuffer::stop() {
	switch( this->role ) {
		case CBR_DATA:
			this->buffer.data->stop();
			break;
		case CBR_PARITY:
			this->buffer.parity->stop();
			break;
	}
}

void MixedChunkBuffer::print( FILE *f ) {
	switch( this->role ) {
		case CBR_DATA:
			this->buffer.data->print( f );
			break;
		case CBR_PARITY:
			this->buffer.parity->print( f );
			break;
	}
}

MixedChunkBuffer::~MixedChunkBuffer() {
	switch( this->role ) {
		case CBR_DATA:
			delete this->buffer.data;
			break;
		case CBR_PARITY:
			delete this->buffer.parity;
			break;
	}
}
