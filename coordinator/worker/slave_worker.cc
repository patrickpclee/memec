#include "worker.hh"
#include "../main/coordinator.hh"

void CoordinatorWorker::dispatch( SlaveEvent event ) {
	bool connected, isSend;
	ssize_t ret;
	struct {
		size_t size;
		char *data;
	} buffer;
	uint32_t requestId;

	switch( event.type ) {
		case SLAVE_EVENT_TYPE_REGISTER_RESPONSE_SUCCESS:
			buffer.data = this->protocol.resRegisterSlave( buffer.size, event.id, true );
			isSend = true;
			break;
		case SLAVE_EVENT_TYPE_REGISTER_RESPONSE_FAILURE:
			buffer.data = this->protocol.resRegisterSlave( buffer.size, event.id, false );
			isSend = true;
			break;
		case SLAVE_EVENT_TYPE_REQUEST_SEAL_CHUNKS:
			requestId = CoordinatorWorker::idGenerator->nextVal( this->workerId );
			buffer.data = this->protocol.reqSealChunks( buffer.size, requestId );
			isSend = true;
			break;
		case SLAVE_EVENT_TYPE_REQUEST_FLUSH_CHUNKS:
			requestId = CoordinatorWorker::idGenerator->nextVal( this->workerId );
			buffer.data = this->protocol.reqFlushChunks( buffer.size, requestId );
			isSend = true;
			break;
		case SLAVE_EVENT_TYPE_REQUEST_SYNC_META:
			requestId = CoordinatorWorker::idGenerator->nextVal( this->workerId );
			buffer.data = this->protocol.reqSyncMeta( buffer.size, requestId );
			// add sync meta request to pending set
			Coordinator::getInstance()->pending.addSyncMetaReq( requestId, event.message.sync );
			isSend = true;
			break;
		// reampped parity migration
		case SLAVE_EVENT_TYPE_PARITY_MIGRATE:
			buffer.data = event.message.parity.packet->data;
			buffer.size = event.message.parity.packet->size;
			isSend = true;
			break;
		case SLAVE_EVENT_TYPE_REQUEST_RELEASE_DEGRADED_LOCK:
			this->handleReleaseDegradedLockRequest( event.socket, event.message.degraded.done );
			isSend = false;
			break;
		case SLAVE_EVENT_TYPE_PENDING:
			isSend = false;
			break;
		case SLAVE_EVENT_TYPE_ANNOUNCE_SLAVE_CONNECTED:
		case SLAVE_EVENT_TYPE_ANNOUNCE_SLAVE_RECONSTRUCTED:
			isSend = false;
			break;
		case SLAVE_EVENT_TYPE_DISCONNECT:
			isSend = false;
			break;
		default:
			return;
	}

	if ( event.type == SLAVE_EVENT_TYPE_ANNOUNCE_SLAVE_CONNECTED ) {
		ArrayMap<int, SlaveSocket> &slaves = Coordinator::getInstance()->sockets.slaves;
		uint32_t requestId = CoordinatorWorker::idGenerator->nextVal( this->workerId );

		buffer.data = this->protocol.announceSlaveConnected( buffer.size, requestId, event.socket );

		LOCK( &slaves.lock );
		for ( uint32_t i = 0; i < slaves.size(); i++ ) {
			SlaveSocket *slave = slaves.values[ i ];
			if ( event.socket->equal( slave ) || ! slave->ready() )
				continue; // No need to tell the new socket

			ret = slave->send( buffer.data, buffer.size, connected );
			if ( ret != ( ssize_t ) buffer.size )
				__ERROR__( "CoordinatorWorker", "dispatch", "The number of bytes sent (%ld bytes) is not equal to the message size (%lu bytes).", ret, buffer.size );
		}
		// notify the remap message handler of the new slave
		struct sockaddr_in slaveAddr = event.socket->getAddr();
		if ( Coordinator::getInstance()->remapMsgHandler )
			Coordinator::getInstance()->remapMsgHandler->addAliveSlave( slaveAddr );
		UNLOCK( &slaves.lock );
	} else if ( event.type == SLAVE_EVENT_TYPE_ANNOUNCE_SLAVE_RECONSTRUCTED ) {
		ArrayMap<int, SlaveSocket> &slaves = Coordinator::getInstance()->sockets.slaves;
		uint32_t requestId = CoordinatorWorker::idGenerator->nextVal( this->workerId );

		buffer.data = this->protocol.announceSlaveReconstructed( buffer.size, requestId, event.message.reconstructed.src, event.message.reconstructed.dst );

		LOCK( &slaves.lock );
		for ( uint32_t i = 0; i < slaves.size(); i++ ) {
			SlaveSocket *slave = slaves.values[ i ];
			if ( slave->equal( event.message.reconstructed.dst ) || ! slave->ready() )
				continue; // No need to tell the backup server

			ret = slave->send( buffer.data, buffer.size, connected );
			if ( ret != ( ssize_t ) buffer.size )
				__ERROR__( "CoordinatorWorker", "dispatch", "The number of bytes sent (%ld bytes) is not equal to the message size (%lu bytes).", ret, buffer.size );
		}
		// notify the remap message handler of the new slave
		struct sockaddr_in slaveAddr = event.message.reconstructed.dst->getAddr();
		if ( Coordinator::getInstance()->remapMsgHandler )
			Coordinator::getInstance()->remapMsgHandler->addAliveSlave( slaveAddr );
		UNLOCK( &slaves.lock );
	} else if ( event.type == SLAVE_EVENT_TYPE_DISCONNECT ) {
		this->handleReconstructionRequest( event.socket );
		// notify the remap message handler of a "removed" slave
		if ( Coordinator::getInstance()->remapMsgHandler )
			Coordinator::getInstance()->remapMsgHandler->removeAliveSlave( event.socket->getAddr() );
	} else if ( isSend ) {
		ret = event.socket->send( buffer.data, buffer.size, connected );
		if ( ret != ( ssize_t ) buffer.size )
			__ERROR__( "CoordinatorWorker", "dispatch", "The number of bytes sent (%ld bytes) is not equal to the message size (%lu bytes).", ret, buffer.size );
		if ( ! connected )
			__ERROR__( "CoordinatorWorker", "dispatch", "The slave is disconnected." );
		if ( event.type == SLAVE_EVENT_TYPE_PARITY_MIGRATE )
			Coordinator::getInstance()->packetPool.free( event.message.parity.packet );
	} else {
		// Parse requests from slaves
		ProtocolHeader header;
		WORKER_RECEIVE_FROM_EVENT_SOCKET();
		ArrayMap<int, MasterSocket> &masters = Coordinator::getInstance()->sockets.masters;
		while( buffer.size > 0 ) {
			WORKER_RECEIVE_WHOLE_MESSAGE_FROM_EVENT_SOCKET( "CoordinatorWorker" );

			// avvoid declaring variables after jump statements
			size_t bytes, offset, count = 0;
			buffer.data += PROTO_HEADER_SIZE;
			buffer.size -= PROTO_HEADER_SIZE;
			// Validate message
			if ( header.from != PROTO_MAGIC_FROM_SLAVE ) {
				__ERROR__( "CoordinatorWorker", "dispatch", "Invalid message source from slave." );
				goto quit_1;
			}

			event.id = header.id;
			switch( header.opcode ) {
				case PROTO_OPCODE_RELEASE_DEGRADED_LOCKS:
					switch( header.magic ) {
						case PROTO_MAGIC_RESPONSE_SUCCESS:
							this->handleReleaseDegradedLockResponse( event, buffer.data, header.length );
							break;
						default:
							__ERROR__( "CoordinatorWorker", "dispatch", "Invalid magic code from slave." );
					}
					break;
				case PROTO_OPCODE_RECONSTRUCTION:
					switch( header.magic ) {
						case PROTO_MAGIC_RESPONSE_SUCCESS:
							this->handleReconstructionResponse( event, buffer.data, header.length );
							break;
						default:
							__ERROR__( "CoordinatorWorker", "dispatch", "Invalid magic code from slave." );
					}
					break;
				case PROTO_OPCODE_BACKUP_SLAVE_PROMOTED:
					switch( header.magic ) {
						case PROTO_MAGIC_RESPONSE_SUCCESS:
							this->handlePromoteBackupSlaveResponse( event, buffer.data, header.length );
							break;
						default:
							__ERROR__( "CoordinatorWorker", "dispatch", "Invalid magic code from slave." );
					}
					break;
				case PROTO_OPCODE_SYNC:
					switch( header.magic ) {
						case PROTO_MAGIC_HEARTBEAT:
							this->processHeartbeat( event, buffer.data, header.length );
							break;
						case PROTO_MAGIC_REMAPPING:
						{
							struct RemappingRecordHeader remappingRecordHeader;
							struct SlaveSyncRemapHeader slaveSyncRemapHeader;
							if ( ! this->protocol.parseRemappingRecordHeader( remappingRecordHeader, buffer.data, buffer.size ) ) {
								__ERROR__( "CoordinatorWorker", "dispatch", "Invalid remapping record protocol header." );
								goto quit_1;
							}
							// start parsing the remapping records
							// TODO buffer.size >> total size of remapping records?
							offset = PROTO_REMAPPING_RECORD_SIZE;
							RemappingRecordMap *map = CoordinatorWorker::remappingRecords;
							for ( count = 0; offset < ( size_t ) buffer.size && count < remappingRecordHeader.remap; offset += bytes ) {
								if ( ! this->protocol.parseSlaveSyncRemapHeader( slaveSyncRemapHeader, bytes, buffer.data, buffer.size - offset, offset ) )
									break;
								count++;

								Key key;
								key.set( slaveSyncRemapHeader.keySize, slaveSyncRemapHeader.key );

								RemappingRecord remappingRecord;
								remappingRecord.set( slaveSyncRemapHeader.listId, slaveSyncRemapHeader.chunkId, 0 );

								if ( slaveSyncRemapHeader.opcode == 0 ) { // remove record
									map->erase( key, remappingRecord );
								} else if ( slaveSyncRemapHeader.opcode == 1 ) { // add record
									map->insert( key, remappingRecord );
								}
							}
							//map->print();
							//fprintf ( stderr, "Remapping Records no.=%lu (%u) upto=%lu size=%lu\n", count, remappingRecordHeader.remap, offset, buffer.size );

							// forward the copies of message to masters
							MasterEvent masterEvent;
							for ( uint32_t i = 0; i < masters.size() ; i++ ) {
								char *data = new char[ buffer.size ];
								memcpy( data, buffer.data, buffer.size );
								masterEvent.forwardRemappingRecords(
									masters.values[ i ], // socket
									buffer.size, // prevSize
									data
								);
								CoordinatorWorker::eventQueue->insert( masterEvent );
							}
						}
							break;
						default:
							__ERROR__( "CoordinatorWorker", "dispatch", "Invalid magic code from slave." );
							break;
					}
					break;
				case PROTO_OPCODE_PARITY_MIGRATE:
				{
					std::set<struct sockaddr_in> *counter;
					pthread_cond_t *allAcked;
					bool found = this->pending->decrementRemappedParityRequest( event.id, event.socket->getAddr(), &counter, &allAcked );
					if ( found && counter == 0 ) {
						pthread_cond_broadcast( allAcked );
					} else if ( ! found ) {
						__ERROR__( "CoordinatorWorker", "dispatch", "Invalid id reply to parity migrate request." );
					}
				}
					break;
				default:
					__ERROR__( "CoordinatorWorker", "dispatch", "Invalid opcode from slave." );
					goto quit_1;
			}
quit_1:
			buffer.data += header.length;
			buffer.size -= header.length;
		}
		if ( connected )
			event.socket->done();
		else
			__ERROR__( "CoordinatorWorker", "dispatch", "The slave is disconnected." );
	}
}

bool CoordinatorWorker::processHeartbeat( SlaveEvent event, char *buf, size_t size ) {
	uint32_t count, requestId = event.id;
	size_t processed, offset, failed = 0;
	struct HeartbeatHeader heartbeat;
	union {
		struct MetadataHeader metadata;
		struct KeyOpMetadataHeader op;
	} header;

	offset = 0;
	if ( ! this->protocol.parseHeartbeatHeader( heartbeat, buf, size ) ) {
		__ERROR__( "CoordinatorWorker", "dispatch", "Invalid heartbeat protocol header." );
		return false;
	}

	offset += PROTO_HEARTBEAT_SIZE;

	LOCK( &event.socket->map.chunksLock );
	for ( count = 0; count < heartbeat.sealed; count++ ) {
		if ( this->protocol.parseMetadataHeader( header.metadata, processed, buf, size, offset ) ) {
			event.socket->map.insertChunk(
				header.metadata.listId,
				header.metadata.stripeId,
				header.metadata.chunkId,
				false, false
			);
		} else {
			failed++;
		}
		offset += processed;
	}
	UNLOCK( &event.socket->map.chunksLock );

	LOCK( &event.socket->map.keysLock );
	for ( count = 0; count < heartbeat.keys; count++ ) {
		if ( this->protocol.parseKeyOpMetadataHeader( header.op, processed, buf, size, offset ) ) {
			SlaveSocket *s = event.socket;
			if ( header.op.opcode == PROTO_OPCODE_DELETE ) { // Handle keys from degraded DELETE
				s = CoordinatorWorker::stripeList->get( header.op.listId, header.op.chunkId );
			}
			s->map.insertKey(
				header.op.key,
				header.op.keySize,
				header.op.listId,
				header.op.stripeId,
				header.op.chunkId,
				header.op.opcode,
				false, false
			);
		} else {
			failed++;
		}
		offset += processed;
	}
	UNLOCK( &event.socket->map.keysLock );

	if ( failed ) {
		__ERROR__( "CoordinatorWorker", "processHeartbeat", "Number of failed objects = %lu", failed );
	// } else {
	// 	__ERROR__( "CoordinatorWorker", "processHeartbeat", "(sealed, keys, remap) = (%u, %u, %u)", heartbeat.sealed, heartbeat.keys, heartbeat.remap );
	}

	// check if this is the last packet for a sync operation
	// remove pending meta sync requests
	if ( requestId && heartbeat.isLast && ! failed ) {
		bool *sync = Coordinator::getInstance()->pending.removeSyncMetaReq( requestId );
		if ( sync )
			*sync = true;
	}

	return failed == 0;
}