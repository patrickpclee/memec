#ifndef __COMMON_DS_METADATA_HH__
#define __COMMON_DS_METADATA_HH__

#include <stdint.h>

class Metadata {
public:
	uint8_t opcode; // Last opcode that updates the key-value pair
	uint32_t listId;
	uint32_t stripeId;
	uint32_t chunkId;

	bool equal( const Metadata &m ) const {
		return (
			this->listId == m.listId &&
			this->stripeId == m.stripeId &&
			this->chunkId == m.chunkId
		);
	}

	bool operator<( const Metadata &m ) const {
		if ( this->listId < m.listId )
			return true;
		if ( this->listId > m.listId )
			return false;

		if ( this->stripeId < m.stripeId )
			return true;
		if ( this->stripeId > m.stripeId )
			return false;

		return this->chunkId < m.chunkId;
	}
};

#endif