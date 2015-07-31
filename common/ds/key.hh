#ifndef __COMMON_DS_KEY_HH__
#define __COMMON_DS_KEY_HH__

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdint.h>

class Key {
public:
	uint8_t size;
	char *data;
	void *ptr; // Extra data to be augmented to the object

	void dup( uint8_t size, char *data, void *ptr = 0 ) {
		this->size = size;
		this->data = strndup( data, size );
		this->ptr = ptr;
	}

	void free() {
		if ( this->data )
			::free( this->data );
		this->data = 0;
	}

	bool equal( const Key &k ) const {
		return (
			this->size == k.size &&
			strncmp( this->data, k.data, this->size ) == 0
		);
	}

	bool operator<( const Key &k ) const {
		int ret;
		if ( this->size < k.size )
			return true;
		if ( this->size > k.size )
			return false;
		ret = strncmp( this->data, k.data, this->size );
		if ( ret < 0 )
			return true;
		if ( ret > 0 )
			return false;
		return this->ptr < k.ptr;
	}
};

#endif
