#include "protocol.hh"

char *SlaveProtocol::resReleaseDegradedLock( size_t &size, uint32_t id, uint32_t count ) {
	// -- common/protocol/degraded_protocol.cc --
	size = this->generateDegradedReleaseResHeader(
		PROTO_MAGIC_RESPONSE_SUCCESS,
		PROTO_MAGIC_TO_COORDINATOR,
		PROTO_OPCODE_RELEASE_DEGRADED_LOCKS,
		id,
		count
	);
	return this->buffer.send;
}