#ifndef __COMMON_PROTOCOL_PROTOCOL_HH__
#define __COMMON_PROTOCOL_PROTOCOL_HH__

/**************************************
 * Packet format:
 * - Magic byte (1 byte)
 * - Opcode (1 byte)
 * - Reserved (2 bytes)
 * - Message length (4 bytes)
 ***************************************/

/************************
 *  Magic byte (1 byte) *
 ************************/
// (Bit: 0-2) //
#define PROTO_MAGIC_HEARTBEAT			0x00 // -----000
#define PROTO_MAGIC_REQUEST				0x01 // -----001
#define PROTO_MAGIC_RESPONSE_SUCCESS	0x02 // -----010
#define PROTO_MAGIC_RESPONSE_FAILURE	0x03 // -----011
#define PROTO_MAGIC_RESERVED_1			0x04 // -----100
#define PROTO_MAGIC_RESERVED_2			0x05 // -----101
#define PROTO_MAGIC_RESERVED_3			0x06 // -----110
#define PROTO_MAGIC_RESERVED_4			0x07 // -----111

// (Bit: 3-4) //
#define PROTO_MAGIC_FROM_APPLICATION	0x00 // ---00---
#define PROTO_MAGIC_FROM_COORDINATOR	0x08 // ---01---
#define PROTO_MAGIC_FROM_MASTER			0x10 // ---10---
#define PROTO_MAGIC_FROM_SLAVE			0x18 // ---11---
 // (Bit: 5-6) //
#define PROTO_MAGIC_TO_APPLICATION		0x00 // -00-----
#define PROTO_MAGIC_TO_COORDINATOR		0x50 // -01-----
#define PROTO_MAGIC_TO_MASTER			0x40 // -10-----
#define PROTO_MAGIC_TO_SLAVE			0x60 // -11-----
// (Bit: 7): Reserved //

/*******************
 * Opcode (1 byte) *
 *******************/
// Coordinator-specific opcodes //
#define PROTO_OPCODE_REGISTER			0x00
#define PROTO_OPCODE_GET_CONFIG			0x01

// Application <-> Master //
#define PROTO_OPCODE_GET				0x02
#define PROTO_OPCODE_SET				0x03
#define PROTO_OPCODE_REPLACE			0x04
#define PROTO_OPCODE_UPDATE				0x05
#define PROTO_OPCODE_DELETE				0x06
#define PROTO_OPCODE_FLUSH				0x07

/*********************
 * Key size (1 byte) *
 *********************/
#define MAXIMUM_KEY_SIZE				255
/***********************
 * Value size (3 byte) *
 ***********************/
 #define MAXIMUM_VALUE_SIZE				16777215

/****************
 * Internal use *
 ****************/
#define PROTO_HEADER_SIZE				8
#define PROTO_KEY_SIZE					1
#define PROTO_KEY_VALUE_SIZE			4

#include <stdint.h>
#include <arpa/inet.h>

enum Role {
	ROLE_APPLICATION,
	ROLE_COORDINATOR,
	ROLE_MASTER,
	ROLE_SLAVE
};

struct ProtocolHeader {
	uint8_t magic, from, to, opcode;
	uint32_t length;
};

struct KeyHeader {
	uint8_t keySize;
	char *key;
};

struct KeyValueHeader {
	uint8_t keySize;
	uint32_t valueSize;
	char *key;
	char *value;
};

class Protocol {
protected:
	uint8_t from, to;

	size_t generateHeader( uint8_t magic, uint8_t to, uint8_t opcode, uint32_t length );
	size_t generateKeyHeader( uint8_t magic, uint8_t to, uint8_t opcode, uint8_t keySize, char *key );
	size_t generateKeyValueHeader( uint8_t magic, uint8_t to, uint8_t opcode, uint8_t keySize, char *key, uint32_t valueSize, char *value );

	bool parseHeader( uint8_t &magic, uint8_t &from, uint8_t &to, uint8_t &opcode, uint32_t &length, char *buf, size_t size );
	bool parseKeyHeader( size_t offset, uint8_t &keySize, char *&key, char *buf, size_t size );
	bool parseKeyValueHeader( size_t offset, uint8_t &keySize, char *&key, uint32_t &valueSize, char *&value, char *buf, size_t size );

public:
	struct {
		size_t size;
		char *data;
	} buffer;

	Protocol( Role role );
	bool init( size_t size = 0 );
	void free();
	bool parseHeader( struct ProtocolHeader &header, char *buf = 0, size_t size = 0 );
	bool parseKeyHeader( struct KeyHeader &header, size_t offset, char *buf = 0, size_t size = 0 );
	bool parseKeyValueHeader( struct KeyValueHeader &header, size_t offset, char *buf = 0, size_t size = 0 );

	static size_t getSuggestedBufferSize( uint32_t keySize, uint32_t chunkSize );
};

#endif
