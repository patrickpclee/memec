#include "chunk.hh"

Chunk::Chunk() {
	this->count = 0;
	this->size = 0;
	this->data = 0;
}

void Chunk::init( uint32_t capacity ) {
	this->data = new char[ capacity ];
}

void Chunk::free() {
	delete this->data;
}

char *Chunk::serialize() {
	uint32_t tmp;

	tmp = htonl( this->count );
	*( ( uint32_t * ) this->data ) = tmp;

	tmp = htonl( this->size );
	*( ( uint32_t * )( this->data + 4 ) ) = size;

	return this->data;
}

char *Chunk::deserialize() {
	this->count = *( ( uint32_t * ) data );
	this->size = *( ( uint32_t * )( data + 4 ) );
	this->count = ntohl( this->count );
	this->size = ntohl( this->size );
	return this->data + 8;
}