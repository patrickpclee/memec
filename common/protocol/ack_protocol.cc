#include "protocol.hh"

size_t Protocol::generateAcknowledgementHeader( uint8_t magic, uint8_t to, uint8_t opcode, uint16_t instanceId, uint32_t requestId, uint32_t fromTimestamp, uint32_t toTimestamp, char *sendBuf ) {
	if ( ! sendBuf ) sendBuf = this->buffer.send;
	char *buf = sendBuf + PROTO_HEADER_SIZE;
	size_t bytes = this->generateHeader( magic, to, opcode, PROTO_ACK_BASE_SIZE, instanceId, requestId, sendBuf );

	*( ( uint32_t * )( buf     ) ) = htonl( fromTimestamp );
	*( ( uint32_t * )( buf + 4 ) ) = htonl( toTimestamp );

	bytes += PROTO_ACK_BASE_SIZE;

	return bytes;
}

bool Protocol::parseAcknowledgementHeader( size_t offset, uint32_t &fromTimestamp, uint32_t &toTimestamp, char *buf, size_t size ) {
	if ( size - offset < PROTO_ACK_BASE_SIZE )
		return false;

	char *ptr = buf + offset;
	fromTimestamp = ntohl( *( ( uint32_t * )( ptr     ) ) );
	toTimestamp   = ntohl( *( ( uint32_t * )( ptr + 4 ) ) );

	return true;
}

bool Protocol::parseAcknowledgementHeader( struct AcknowledgementHeader &header, char *buf, size_t size, size_t offset ) {
	if ( ! buf || ! size ) {
		buf = this->buffer.recv;
		size = this->buffer.size;
	}
	return this->parseAcknowledgementHeader(
		offset,
		header.fromTimestamp,
		header.toTimestamp,
		buf, size
	);
}

size_t Protocol::generateParityDeltaAcknowledgementHeader( uint8_t magic, uint8_t to, uint8_t opcode, uint16_t instanceId, uint32_t requestId, uint32_t fromTimestamp, uint32_t toTimestamp, uint16_t targetId, char *sendBuf ) {
	if ( ! sendBuf ) sendBuf = this->buffer.send;
	char *buf = sendBuf + PROTO_HEADER_SIZE;
	size_t bytes = this->generateHeader( magic, to, opcode, PROTO_ACK_PARITY_DELTA_SIZE, instanceId, requestId, sendBuf );

	*( ( uint32_t * )( buf     ) ) = htonl( fromTimestamp );
	*( ( uint32_t * )( buf + 4 ) ) = htonl( toTimestamp );
	*( ( uint32_t * )( buf + 8 ) ) = htonl( targetId );

	bytes += PROTO_ACK_PARITY_DELTA_SIZE;

	return bytes;
}

bool Protocol::parseParityDeltaAcknowledgementHeader( size_t offset, uint32_t &fromTimestamp, uint32_t &toTimestamp, uint16_t &targetId, char *buf, size_t size ) {
	if ( size - offset < PROTO_ACK_PARITY_DELTA_SIZE )
		return false;

	char *ptr = buf + offset;
	fromTimestamp = ntohl( *( ( uint32_t * )( ptr     ) ) );
	toTimestamp   = ntohl( *( ( uint32_t * )( ptr + 4 ) ) );
	targetId      = ntohl( *( ( uint32_t * )( ptr + 8 ) ) );

	return true;
}

bool Protocol::parseParityDeltaAcknowledgementHeader( struct ParityDeltaAcknowledgementHeader &header, char *buf, size_t size, size_t offset ) {
	if ( ! buf || ! size ) {
		buf = this->buffer.recv;
		size = this->buffer.size;
	}
	return this->parseParityDeltaAcknowledgementHeader(
		offset,
		header.fromTimestamp,
		header.toTimestamp,
		header.targetId,
		buf, size
	);
}
