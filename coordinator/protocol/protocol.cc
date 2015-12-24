#include "protocol.hh"

char *CoordinatorProtocol::reqSyncMeta( size_t &size, uint16_t instanceId, uint32_t requestId ) {
	// -- common/protocol/protocol.cc --
	size = this->generateHeader(
		PROTO_MAGIC_REQUEST,
		PROTO_MAGIC_TO_SLAVE,
		PROTO_OPCODE_SYNC_META,
		0, // length
		instanceId, requestId
	);
	return this->buffer.send;
}

char *CoordinatorProtocol::reqSealChunks( size_t &size, uint16_t instanceId, uint32_t requestId ) {
	// -- common/protocol/protocol.cc --
	size = this->generateHeader(
		PROTO_MAGIC_REQUEST,
		PROTO_MAGIC_TO_SLAVE,
		PROTO_OPCODE_SEAL_CHUNKS,
		0, // length
		instanceId, requestId
	);
	return this->buffer.send;
}

char *CoordinatorProtocol::reqFlushChunks( size_t &size, uint16_t instanceId, uint32_t requestId ) {
	// -- common/protocol/protocol.cc --
	size = this->generateHeader(
		PROTO_MAGIC_REQUEST,
		PROTO_MAGIC_TO_SLAVE,
		PROTO_OPCODE_FLUSH_CHUNKS,
		0, // length
		instanceId, requestId
	);
	return this->buffer.send;
}
