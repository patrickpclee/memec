#ifndef __SERVER_EVENT_SERVER_PEER_EVENT_HH__
#define __SERVER_EVENT_SERVER_PEER_EVENT_HH__

#include "../protocol/protocol.hh"
#include "../socket/server_peer_socket.hh"
#include "../../common/ds/metadata.hh"
#include "../../common/ds/chunk.hh"
#include "../../common/ds/packet_pool.hh"
#include "../../common/ds/value.hh"
#include "../../common/event/event.hh"

class MixedChunkBuffer;

enum ServerPeerEventType {
	SERVER_PEER_EVENT_TYPE_UNDEFINED,
	// Register
	SERVER_PEER_EVENT_TYPE_REGISTER_REQUEST,
	SERVER_PEER_EVENT_TYPE_REGISTER_RESPONSE_SUCCESS,
	SERVER_PEER_EVENT_TYPE_REGISTER_RESPONSE_FAILURE,
	// REMAPPING_SET
	SERVER_PEER_EVENT_TYPE_REMAPPING_SET_RESPONSE_SUCCESS,
	SERVER_PEER_EVENT_TYPE_REMAPPING_SET_RESPONSE_FAILURE,
	// SET
	SERVER_PEER_EVENT_TYPE_SET_REQUEST,
	SERVER_PEER_EVENT_TYPE_SET_RESPONSE_SUCCESS,
	SERVER_PEER_EVENT_TYPE_SET_RESPONSE_FAILURE,
	// FORWARD_KEY
	SERVER_PEER_EVENT_TYPE_FORWARD_KEY_REQUEST,
	SERVER_PEER_EVENT_TYPE_FORWARD_KEY_RESPONSE_SUCCESS,
	SERVER_PEER_EVENT_TYPE_FORWARD_KEY_RESPONSE_FAILURE,
	// GET
	SERVER_PEER_EVENT_TYPE_GET_REQUEST,
	SERVER_PEER_EVENT_TYPE_GET_RESPONSE_SUCCESS,
	SERVER_PEER_EVENT_TYPE_GET_RESPONSE_FAILURE,
	// DELETE
	SERVER_PEER_EVENT_TYPE_DELETE_RESPONSE_SUCCESS,
	SERVER_PEER_EVENT_TYPE_DELETE_RESPONSE_FAILURE,
	// UPDATE
	SERVER_PEER_EVENT_TYPE_UPDATE_RESPONSE_SUCCESS,
	SERVER_PEER_EVENT_TYPE_UPDATE_RESPONSE_FAILURE,
	// REMAPPED_UPDATE
	SERVER_PEER_EVENT_TYPE_REMAPPED_UPDATE_RESPONSE_SUCCESS,
	SERVER_PEER_EVENT_TYPE_REMAPPED_UPDATE_RESPONSE_FAILURE,
	// REMAPPED_DELETE
	SERVER_PEER_EVENT_TYPE_REMAPPED_DELETE_RESPONSE_SUCCESS,
	SERVER_PEER_EVENT_TYPE_REMAPPED_DELETE_RESPONSE_FAILURE,
	// UPDATE_CHUNK
	SERVER_PEER_EVENT_TYPE_UPDATE_CHUNK_RESPONSE_SUCCESS,
	SERVER_PEER_EVENT_TYPE_UPDATE_CHUNK_RESPONSE_FAILURE,
	// DELETE_CHUNK
	SERVER_PEER_EVENT_TYPE_DELETE_CHUNK_RESPONSE_SUCCESS,
	SERVER_PEER_EVENT_TYPE_DELETE_CHUNK_RESPONSE_FAILURE,
	// GET_CHUNK
	SERVER_PEER_EVENT_TYPE_GET_CHUNK_REQUEST,
	SERVER_PEER_EVENT_TYPE_GET_CHUNK_RESPONSE_SUCCESS,
	SERVER_PEER_EVENT_TYPE_GET_CHUNK_RESPONSE_FAILURE,
	// Batch GET_CHUNK
	SERVER_PEER_EVENT_TYPE_BATCH_GET_CHUNKS,
	// SET_CHUNK
	SERVER_PEER_EVENT_TYPE_SET_CHUNK_REQUEST,
	SERVER_PEER_EVENT_TYPE_SET_CHUNK_RESPONSE_SUCCESS,
	SERVER_PEER_EVENT_TYPE_SET_CHUNK_RESPONSE_FAILURE,
	// FORWARD_CHUNK
	SERVER_PEER_EVENT_TYPE_FORWARD_CHUNK_REQUEST,
	SERVER_PEER_EVENT_TYPE_FORWARD_CHUNK_RESPONSE_SUCCESS,
	SERVER_PEER_EVENT_TYPE_FORWARD_CHUNK_RESPONSE_FAILURE,
	// SEAL_CHUNK
	SERVER_PEER_EVENT_TYPE_SEAL_CHUNK_REQUEST,
	SERVER_PEER_EVENT_TYPE_SEAL_CHUNK_RESPONSE_SUCCESS,
	SERVER_PEER_EVENT_TYPE_SEAL_CHUNK_RESPONSE_FAILURE,
	// Seal chunk buffer
	SERVER_PEER_EVENT_TYPE_SEAL_CHUNKS,
	// Reconstructed unsealed keys
	SERVER_PEER_EVENT_TYPE_UNSEALED_KEYS_RESPONSE_SUCCESS,
	SERVER_PEER_EVENT_TYPE_UNSEALED_KEYS_RESPONSE_FAILURE,
	// Defer
	SERVER_PEER_EVENT_TYPE_DEFERRED,
	// Send
	SERVER_PEER_EVENT_TYPE_SEND,
	// Pending
	SERVER_PEER_EVENT_TYPE_PENDING
};

class ServerPeerEvent : public Event {
public:
	ServerPeerEventType type;
	uint16_t instanceId;
	uint32_t requestId;
	ServerPeerSocket *socket;
	uint32_t timestamp;
	struct {
		struct {
			Metadata metadata;
			uint32_t offset;
			uint32_t length;
			uint32_t updatingChunkId;
		} chunkUpdate;
		struct {
			uint32_t listId, chunkId;
			Key key;
			KeyValue keyValue;
		} get;
		struct {
			uint32_t listId, stripeId, chunkId, valueUpdateOffset, chunkUpdateOffset, length;
			Key key;
		} update;
		struct {
			uint32_t listId, stripeId, chunkId;
			Key key;
		} del;
		struct {
			Metadata metadata;
			Chunk *chunk;
			uint32_t chunkBufferIndex;
			bool needsFree;
			uint8_t sealIndicatorCount;
			bool *sealIndicator;
		} chunk;
		MixedChunkBuffer *chunkBuffer;
		struct {
			Packet *packet;
		} send;
		struct {
			std::vector<uint32_t> *requestIds;
			std::vector<Metadata> *metadata;
		} batchGetChunks;
		struct {
			uint32_t listId, chunkId;
			Key key;
		} remap;
		struct {
			Key key;
			Value value;
		} set;
		struct {
			uint8_t opcode;
			uint32_t listId, stripeId, chunkId;
			uint8_t keySize;
			uint32_t valueSize;
			char *key, *value;
			struct {
				uint32_t offset, length;
				char *data;
			} update;
		} forwardKey;
		struct {
			Key key;
			uint32_t valueUpdateOffset;
			uint32_t valueUpdateSize;
		} remappingUpdate;
		struct {
			Key key;
		} remappingDel;
		struct {
			struct BatchKeyValueHeader header;
		} unsealedKeys;
		struct {
			uint8_t opcode;
			char *buf;
			size_t size;
		} defer;
	} message;

	// Register
	void reqRegister( ServerPeerSocket *socket );
	void resRegister( ServerPeerSocket *socket, uint16_t instanceId, uint32_t requestId, bool success = true );
	// REMAPPING_SET
	void resRemappingSet( ServerPeerSocket *socket, uint16_t instanceId, uint32_t requestId, Key &key, uint32_t listId, uint32_t chunkId, bool success );
	// SET
	void reqSet( ServerPeerSocket *socket, uint16_t instanceId, uint32_t requestId, Key key, Value value );
	void resSet( ServerPeerSocket *socket, uint16_t instanceId, uint32_t requestId, Key key, bool success );
	// Degraded SET
	void reqForwardKey(
		ServerPeerSocket *socket,
		uint8_t opcode,
		uint16_t instanceId, uint32_t requestId,
		uint32_t listId, uint32_t stripeId, uint32_t chunkId,
		uint8_t keySize, uint32_t valueSize,
		char *key, char *value,
		uint32_t valueUpdateOffset = 0, uint32_t valueUpdateSize = 0, char *valueUpdate = 0
	);
	void resForwardKey(
		ServerPeerSocket *socket, bool success,
		uint8_t opcode,
		uint16_t instanceId, uint32_t requestId,
		uint32_t listId, uint32_t stripeId, uint32_t chunkId,
		uint8_t keySize, uint32_t valueSize,
		char *key,
		uint32_t valueUpdateOffset = 0, uint32_t valueUpdateSize = 0
	);
	// GET
	void reqGet( ServerPeerSocket *socket, uint16_t instanceId, uint32_t requestId, uint32_t listId, uint32_t chunkId, Key &key );
	void resGet( ServerPeerSocket *socket, uint16_t instanceId, uint32_t requestId, KeyValue &keyValue );
	void resGet( ServerPeerSocket *socket, uint16_t instanceId, uint32_t requestId, Key &key );
	// UPDATE
	void resUpdate( ServerPeerSocket *socket, uint16_t instanceId, uint32_t requestId, uint32_t listId, uint32_t stripeId, uint32_t chunkId, Key &key, uint32_t valueUpdateOffset, uint32_t length, uint32_t chunkUpdateOffset, bool success );
	// DELETE
	void resDelete( ServerPeerSocket *socket, uint16_t instanceId, uint32_t requestId, uint32_t listId, uint32_t stripeId, uint32_t chunkId, Key &key, bool success );
	// REMAPPED_UPDATE
	void resRemappedUpdate( ServerPeerSocket *socket, uint16_t instanceId, uint32_t requestId, Key &key, uint32_t valueUpdateOffset, uint32_t valueUpdateSize, bool success );
	// REMAPPED_DELETE
	void resRemappedDelete( ServerPeerSocket *socket, uint16_t instanceId, uint32_t requestId, Key &key, bool success );
	// UPDATE_CHUNK
	void resUpdateChunk( ServerPeerSocket *socket, uint16_t instanceId, uint32_t requestId, Metadata &metadata, uint32_t offset, uint32_t length, uint32_t updatingChunkId, bool success );
	// DELETE_CHUNK
	void resDeleteChunk( ServerPeerSocket *socket, uint16_t instanceId, uint32_t requestId, Metadata &metadata, uint32_t offset, uint32_t length, uint32_t updatingChunkId, bool success );
	// GET_CHUNK
	void reqGetChunk( ServerPeerSocket *socket, uint16_t instanceId, uint32_t requestId, Metadata &metadata );
	void resGetChunk( ServerPeerSocket *socket, uint16_t instanceId, uint32_t requestId, Metadata &metadata, bool success, uint32_t chunkBufferIndex, Chunk *chunk, uint8_t sealIndicatorCount, bool *sealIndicator );
	// Batch GET_CHUNK
	void batchGetChunks( ServerPeerSocket *socket, std::vector<uint32_t> *requestIds, std::vector<Metadata> *metadata );
	// SET_CHUNK
	void reqSetChunk( ServerPeerSocket *socket, uint16_t instanceId, uint32_t requestId, Metadata &metadata, Chunk *chunk, bool needsFree );
	void resSetChunk( ServerPeerSocket *socket, uint16_t instanceId, uint32_t requestId, Metadata &metadata, bool success );
	// FORWARD_CHUNK
	void reqForwardChunk( ServerPeerSocket *socket, uint16_t instanceId, uint32_t requestId, Metadata &metadata, Chunk *chunk, bool needsFree );
	void resForwardChunk( ServerPeerSocket *socket, uint16_t instanceId, uint32_t requestId, Metadata &metadata, bool success );
	// SEAL_CHUNK
	void reqSealChunk( Chunk *chunk );
	void resSealChunk( ServerPeerSocket *socket, uint16_t instanceId, uint32_t requestId, Metadata &metadata, bool success );
	// Seal chunk buffer
	void reqSealChunks( MixedChunkBuffer *chunkBuffer );
	// Reconstructed unsealed keys
	void resUnsealedKeys( ServerPeerSocket *socket, uint16_t instanceId, uint32_t requestId, struct BatchKeyValueHeader &header, bool success );
	// Defer
	void defer( ServerPeerSocket *socket, uint16_t instanceId, uint32_t requestId, uint8_t opcode, char *buf, size_t size );
	// Send
	void send( ServerPeerSocket *socket, Packet *packet );
	// Pending
	void pending( ServerPeerSocket *socket );
};

#endif