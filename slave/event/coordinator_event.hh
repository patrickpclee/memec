#ifndef __SLAVE_EVENT_COORDINATOR_EVENT_HH__
#define __SLAVE_EVENT_COORDINATOR_EVENT_HH__

#include "../socket/coordinator_socket.hh"
#include "../../common/event/event.hh"

enum CoordinatorEventType {
	COORDINATOR_EVENT_TYPE_REGISTER_REQUEST,
	COORDINATOR_EVENT_TYPE_SYNC,
	COORDINATOR_EVENT_TYPE_RELEASE_DEGRADED_LOCK_RESPONSE_SUCCESS,
	COORDINATOR_EVENT_TYPE_RECONSTRUCTION_RESPONSE_SUCCESS,
	COORDINATOR_EVENT_TYPE_PROMOTE_BACKUP_SERVER_RESPONSE_SUCCESS,
	COORDINATOR_EVENT_TYPE_RESPONSE_PARITY_MIGRATE,
	COORDINATOR_EVENT_TYPE_PENDING
};

class CoordinatorEvent : public Event {
public:
	CoordinatorEventType type;
	CoordinatorSocket *socket;
	uint32_t id;
	union {
		struct {
			uint32_t addr;
			uint16_t port;
		} address;
		struct {
			uint32_t count;
		} degraded;
		struct {
			uint32_t listId;
			uint32_t chunkId;
			uint32_t numStripes;
		} reconstruction;
		struct {
			uint32_t addr;
			uint16_t port;
			uint32_t count;
		} promote;
	} message;

	void reqRegister( CoordinatorSocket *socket, uint32_t addr, uint16_t port );
	void sync( CoordinatorSocket *socket, uint32_t id = 0 );
	void resRemappedData( CoordinatorSocket *socket = 0, uint32_t *id = 0 );
	void resReleaseDegradedLock( CoordinatorSocket *socket, uint32_t id, uint32_t count );
	void resReconstruction( CoordinatorSocket *socket, uint32_t id, uint32_t listId, uint32_t chunkId, uint32_t numStripes );
	void resPromoteBackupSlave( CoordinatorSocket *socket, uint32_t id, uint32_t addr, uint16_t port, uint32_t count );
	void pending( CoordinatorSocket *socket );
};

#endif
