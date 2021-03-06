#include <cstring>
#include <ctype.h>
#include <unistd.h>
#include "server.hh"

uint16_t Server::instanceId;

Server::Server() {
	this->isRunning = false;
	Server::instanceId = 0;
}

void Server::free() {
	/* ID generator */
	this->idGenerator.free();
	/* Event queue */
	this->eventQueue.free();
	/* Coding */
	Coding::destroy( this->coding );
	/* Stripe list */
	delete this->stripeList;
	/* Chunk buffer */
	for ( size_t i = 0, size = this->chunkBuffer.size(); i < size; i++ ) {
		if ( this->chunkBuffer[ i ] )
			delete this->chunkBuffer[ i ];
	}
}

void Server::sync( uint32_t requestId ) {
	CoordinatorEvent event;
	for ( int i = 0, len = this->config.global.coordinators.size(); i < len; i++ ) {
		// Can only sync with one coordinator
		event.sync( this->sockets.coordinators[ i ], Server::instanceId, requestId );
		this->eventQueue.insert( event );
	}
}

void Server::signalHandler( int signal ) {
	Server *server = Server::getInstance();
	switch( signal ) {
		case SIGALRM:
			server->sync();
			server->alarm();
			break;
		default:
			Signal::setHandler(); // disable signal handler
			server->stop();
			fclose( stdin );
	}
}

bool Server::init( char *path, OptionList &globalOptions, OptionList &serverOptions, bool verbose ) {
	// Parse configuration files //
	if ( ( ! this->config.global.parse( path ) ) ||
	     ( ! this->config.global.override( globalOptions ) ) ||
	     ( ! this->config.global.validate() ) ||
	     ( ! this->config.server.parse( path ) ) ||
	     ( ! this->config.server.override( serverOptions ) ) ||
		 ( ! this->config.server.validate() )
	) {
		return false;
	}
	this->myServerIndex = this->config.server.validate( this->config.global.servers );

	// Initialize modules //
	/* Socket */
	if ( ! this->sockets.epoll.init(
			this->config.global.epoll.maxEvents,
			this->config.global.epoll.timeout
		) || ! this->sockets.self.init(
			this->config.server.server.addr.type,
			this->config.server.server.addr.addr,
			this->config.server.server.addr.port,
			this->config.server.server.addr.name,
			&this->sockets.epoll
		) ) {
		__ERROR__( "Server", "init", "Cannot initialize socket." );
		return false;
	}
	/* Vectors and other sockets */
	Socket::init( &this->sockets.epoll );
	CoordinatorSocket::setArrayMap( &this->sockets.coordinators );
	ClientSocket::setArrayMap( &this->sockets.clients );
	ServerPeerSocket::setArrayMap( &this->sockets.serverPeers );
	this->sockets.coordinators.reserve( this->config.global.coordinators.size() );
	for ( int i = 0, len = this->config.global.coordinators.size(); i < len; i++ ) {
		CoordinatorSocket *socket = new CoordinatorSocket();
		int fd;

		socket->init( this->config.global.coordinators[ i ], &this->sockets.epoll );
		fd = socket->getSocket();
		this->sockets.coordinators.set( fd, socket );
	}
	this->sockets.serverPeers.reserve( this->config.global.servers.size() );
	for ( int i = 0, len = this->config.global.servers.size(); i < len; i++ ) {
		ServerPeerSocket *socket = new ServerPeerSocket();
		int tmpfd = - ( i + 1 );
		socket->init(
			tmpfd,
			this->config.global.servers[ i ],
			&this->sockets.epoll,
			i == myServerIndex && myServerIndex != -1 // indicate whether this is a self-socket
		);
		this->sockets.serverPeers.set( tmpfd, socket );
	}
	/* Coding */
	this->coding = Coding::instantiate(
		this->config.global.coding.scheme,
		this->config.global.coding.params,
		this->config.global.size.chunk
	);
	/* Stripe list */
	this->stripeList = new StripeList<ServerPeerSocket>(
		this->config.global.coding.params.getChunkCount(),
		this->config.global.coding.params.getDataChunkCount(),
		this->config.global.stripeLists.count,
		this->sockets.serverPeers.values
	);
	/* Stripe list index */
	if ( myServerIndex != -1 )
		this->stripeListIndex = this->stripeList->list( myServerIndex );
	/* Chunk pool */
	// Chunk::init( this->config.global.size.chunk );
	ChunkUtil::init(
		this->config.global.size.chunk,
		this->config.global.coding.params.getDataChunkCount()
	);
	this->chunkPool.init(
		this->config.global.size.chunk, // chunkSize
		this->config.server.pool.chunks // capacity
	);
	LargeObjectUtil::init( this->config.global.size.chunk );
	/* Chunk buffer */
	ChunkBuffer::init();
	this->chunkBuffer.reserve( this->config.global.stripeLists.count );
	for ( uint32_t i = 0; i < this->config.global.stripeLists.count; i++ )
		this->chunkBuffer.push_back( 0 );

	if ( myServerIndex != -1 ) {
		for ( uint32_t i = 0, size = this->stripeListIndex.size(); i < size; i++ ) {
			uint32_t listId = this->stripeListIndex[ i ].listId,
			         stripeId = this->stripeListIndex[ i ].stripeId,
			         chunkId = this->stripeListIndex[ i ].chunkId;
			if ( this->stripeListIndex[ i ].isParity ) {
				this->chunkBuffer[ listId ] = new MixedChunkBuffer(
					new ParityChunkBuffer(
						this->config.server.buffer.chunksPerList,
						listId, stripeId, chunkId, true
					)
				);
			} else {
				this->chunkBuffer[ listId ] = new MixedChunkBuffer(
					new DataChunkBuffer(
						this->config.server.buffer.chunksPerList,
						listId, stripeId, chunkId, true
					)
				);
			}
		}
	}
	// Map //
	this->map.setTimestamp( &this->timestamp );
	this->degradedChunkBuffer.map.init( &this->map );

	/* Workers, ID generator, packet pool and event queues */
	this->idGenerator.init( this->config.global.workers.count );
	this->packetPool.init(
		this->config.global.workers.count * this->config.global.coding.params.getChunkCount(),
		Protocol::getSuggestedBufferSize(
			this->config.global.size.key,
			this->config.global.size.chunk
		)
	);
	this->eventQueue.init(
		this->config.global.eventQueue.block,
		this->config.global.eventQueue.size,
		this->config.global.eventQueue.prioritized
	);
	ServerWorker::init();
	this->workers.reserve( this->config.global.workers.count );
	for ( int i = 0, len = this->config.global.workers.count; i < len; i++ ) {
		this->workers.push_back( ServerWorker() );
		this->workers[ i ].init(
			this->config.global,
			this->config.server,
			i // worker ID
		);
	}
	/* Remapping message handler; Remapping scheme */
	if ( ! this->config.global.states.disabled ) {
		char serverName[ 11 ];
		memset( serverName, 0, 11 );
		sprintf( serverName, "%s%04d", SERVER_PREFIX, this->config.server.server.addr.id );
		stateTransitHandler.init( this->config.global.states.spreaddAddr.addr, this->config.global.states.spreaddAddr.port, serverName );
		LOCK( &this->sockets.serverPeers.lock );
		for ( uint32_t i = 0; i < this->sockets.serverPeers.size(); i++ ) {
			stateTransitHandler.addAliveServer( this->sockets.serverPeers.values[ i ]->getAddr() );
		}
		UNLOCK( &this->sockets.serverPeers.lock );
		// stateTransitHandler.listAliveServers();
	}

	// Set signal handlers //
	Signal::setHandler( Server::signalHandler );

	// Init lock for instance id to socket mapping //
	LOCK_INIT( &this->sockets.clientsIdToSocketLock );
	LOCK_INIT( &this->sockets.serversIdToSocketLock );

	// Set status //
	this->status.isRecovering = ( myServerIndex == -1 );
	LOCK_INIT( &this->status.lock );

	// Show configuration //
	if ( verbose )
		this->info();
	return true;
}

bool Server::init( int myServerIndex ) {
	this->myServerIndex = myServerIndex;
	if ( myServerIndex == -1 )
		return false;

	this->stripeListIndex = this->stripeList->list( myServerIndex );

	for ( uint32_t i = 0, size = this->stripeListIndex.size(); i < size; i++ ) {
		uint32_t listId = this->stripeListIndex[ i ].listId,
				 chunkId = this->stripeListIndex[ i ].chunkId;
		uint32_t stripeId = 0;
		if ( this->stripeListIndex[ i ].isParity ) {
			// The stripe ID is not used
			this->chunkBuffer[ listId ] = new MixedChunkBuffer(
				new ParityChunkBuffer(
					this->config.server.buffer.chunksPerList,
					listId, stripeId, chunkId, false
				)
			);
		} else {
			// Get minimum stripe ID
			stripeId = 0;

			this->chunkBuffer[ listId ] = new MixedChunkBuffer(
				new DataChunkBuffer(
					this->config.server.buffer.chunksPerList,
					listId, stripeId, chunkId, false
				)
			);
		}
	}

	return true;
}

bool Server::initChunkBuffer() {
	if ( this->myServerIndex == -1 )
		return false;

	for ( uint32_t i = 0, size = this->stripeListIndex.size(); i < size; i++ ) {
		uint32_t listId = this->stripeListIndex[ i ].listId;
		this->chunkBuffer[ listId ]->init();
	}

	return true;
}

bool Server::start() {
	/* Workers and event queues */
	this->eventQueue.start();
	for ( int i = 0, len = this->config.global.workers.count; i < len; i++ ) {
		this->workers[ i ].start();
	}

	/* Sockets */
	// Connect to coordinators
	for ( int i = 0, len = this->config.global.coordinators.size(); i < len; i++ ) {
		if ( ! this->sockets.coordinators[ i ]->start() )
			__ERROR__( "Server", "start", "Cannot connect to coordinator #%d.", i );
	}
	// Do not connect to servers until a server connected message is announcement by the coordinator

	// Start listening
	if ( ! this->sockets.self.start() ) {
		__ERROR__( "Server", "start", "Cannot start socket." );
		return false;
	}

	/* Remapping message handler */
	if ( ! this->config.global.states.disabled && ! this->stateTransitHandler.start() ) {
		__ERROR__( "Server", "start", "Cannot start remapping message handler." );
		return false;
	}

	this->startTime = start_timer();
	this->isRunning = true;

	/* Alarm */
	this->alarm();

	return true;
}

bool Server::stop() {
	if ( ! this->isRunning )
		return false;

	int i, len;

	/* Sockets */
	this->sockets.self.stop();

	/* Workers */
	len = this->workers.size();
	for ( i = len - 1; i >= 0; i-- )
		this->workers[ i ].stop();

	/* Event queues */
	this->eventQueue.stop();

	/* Workers */
	for ( i = len - 1; i >= 0; i-- )
		this->workers[ i ].join();

	/* Sockets */
	for ( i = 0, len = this->sockets.coordinators.size(); i < len; i++ )
		this->sockets.coordinators[ i ]->stop();
	this->sockets.coordinators.clear();
	for ( i = 0, len = this->sockets.clients.size(); i < len; i++ )
		this->sockets.clients[ i ]->stop();
	this->sockets.clients.clear();
	for ( i = 0, len = this->sockets.serverPeers.size(); i < len; i++ ) {
		this->sockets.serverPeers[ i ]->stop();
		this->sockets.serverPeers[ i ]->free();
	}
	this->sockets.serverPeers.clear();

	 /* Remapping message handler */
	if ( ! this->config.global.states.disabled ) {
		this->stateTransitHandler.stop();
		this->stateTransitHandler.quit();
	}

	/* Chunk buffer */
	for ( size_t i = 0, size = this->chunkBuffer.size(); i < size; i++ ) {
		if ( this->chunkBuffer[ i ] )
			this->chunkBuffer[ i ]->stop();
	}

	this->free();
	this->isRunning = false;
	printf( "\nBye.\n" );
	return true;
}

void Server::seal() {
	size_t count = 0;
	ServerPeerEvent event;
	for ( int i = 0, size = this->chunkBuffer.size(); i < size; i++ ) {
		if ( this->chunkBuffer[ i ] ) {
			event.reqSealChunks( this->chunkBuffer[ i ] );
			this->eventQueue.insert( event );
			count++;
		}
	}
	printf( "\nSealing %lu chunk buffer:\n", count );
}

void Server::flush( bool parityOnly ) {
	IOEvent ioEvent;
	Chunk *chunk;

	uint32_t total, numFlushed = 0;
	std::atomic<unsigned int> count;
	char *startAddress;

	this->chunkPool.exportVars( &total, &count, &startAddress );

	for ( unsigned int i = 0; i < total; i++ ) {
		chunk = ( Chunk * )( startAddress + i * ( CHUNK_IDENTIFIER_SIZE + ChunkUtil::chunkSize ) );
		if ( parityOnly && ! ChunkUtil::isParity( chunk ) )
			continue;

		if ( ChunkUtil::getSize( chunk ) == 0 )
			continue;

		ioEvent.flush( chunk );
		this->eventQueue.insert( ioEvent );

		numFlushed++;
		if ( numFlushed == count )
			break;
	}

	printf( "Flushing %u chunks...\n", numFlushed );
}

void Server::memory( FILE *f ) {
	uint32_t numDataChunks = 0, numParityChunks = 0, numKeyValues = 0;
	uint64_t occupied = 0, allocated = 0, bytesParity = 0;
	Chunk *chunk;

	uint32_t total, numFlushed = 0;
	std::atomic<unsigned int> count;
	char *startAddress;

	this->chunkPool.exportVars( &total, &count, &startAddress );

	for ( unsigned int i = 0; i < total; i++ ) {
		chunk = ( Chunk * )( startAddress + i * ( CHUNK_IDENTIFIER_SIZE + ChunkUtil::chunkSize ) );

		if ( ChunkUtil::getSize( chunk ) == 0 )
			continue;

		if ( ChunkUtil::isParity( chunk ) ) {
			numParityChunks++;
			bytesParity += ChunkUtil::chunkSize;
		} else {
			numDataChunks++;
			numKeyValues += ChunkUtil::getCount( chunk );
			occupied += ChunkUtil::getSize( chunk );
			allocated += ChunkUtil::chunkSize;
		}

		numFlushed++;
		if ( numFlushed == count )
			break;
	}

	int width = 25;
	fprintf(
		f,
		"Parity chunks\n"
		"\t- %-*s : %u\n"
		"\t- %-*s : %lu\n"
		"Data chunks\n"
		"\t- %-*s : %u\n"
		"\t- %-*s : %u\n"
		"\t- %-*s : %lu\n"
		"\t- %-*s : %lu\n"
		"\t- %-*s : %6.4lf%%\n",
		width, "Number of parity chunks", numParityChunks,
		width, "Total size (bytes)", bytesParity,

		width, "Number of data chunks", numDataChunks,
		width, "Number of key-value pairs", numKeyValues,
		width, "Occupied size (bytes)", occupied,
		width, "Total size (bytes)", allocated,
		width, "Utilization", ( double ) occupied / allocated * 100.0
	);
}

void Server::setDelay() {
	unsigned int delay;

	printf( "How much delay (in usec)? " );
	fflush( stdout );
	if ( scanf( "%u", &delay ) == 1 )  {
		ServerWorker::delay = delay;
	} else {
		printf( "Invalid input.\n" );
	}
}

double Server::getElapsedTime() {
	return get_elapsed_time( this->startTime );
}

void Server::info( FILE *f ) {
	this->config.global.print( f );
	this->config.server.print( f );
	this->stripeList->print( f );

	fprintf( f, "\n### Stripe List Index ###\n" );
	for ( int i = 0, size = this->stripeListIndex.size(); i < size; i++ ) {
		fprintf(
			f, "%d. List #%d: %s chunk #%d\n", i,
			this->stripeListIndex[ i ].listId,
			this->stripeListIndex[ i ].isParity ? "Parity" : "Data",
			this->stripeListIndex[ i ].chunkId
		);
	}
	fprintf( f, "\n" );
}

void Server::debug( FILE *f ) {
	int i, len;
	fprintf( f, "Server socket\n------------\n" );
	this->sockets.self.print( f );

	fprintf( f, "\nCoordinator sockets\n-------------------\n" );
	for ( i = 0, len = this->sockets.coordinators.size(); i < len; i++ ) {
		fprintf( f, "%d. ", i + 1 );
		this->sockets.coordinators[ i ]->print( f );
	}
	if ( len == 0 ) fprintf( f, "(None)\n" );

	fprintf( f, "\nClient sockets\n---------------\n" );
	for ( i = 0, len = this->sockets.clients.size(); i < len; i++ ) {
		fprintf( f, "%d. ", i + 1 );
		this->sockets.clients[ i ]->print( f );
	}
	if ( len == 0 ) fprintf( f, "(None)\n" );

	fprintf( f, "\nServer peer sockets\n------------------\n" );
	for ( i = 0, len = this->sockets.serverPeers.size(); i < len; i++ ) {
		fprintf( f, "%d. ", i + 1 );
		this->sockets.serverPeers[ i ]->print( f );
	}
	if ( len == 0 ) fprintf( f, "(None)\n" );

	fprintf( f, "\nChunk pool\n----------\n" );
	this->chunkPool.print( f );

	fprintf( f, "\nChunk buffer\n------------\n" );
	for ( i = 0, len = this->chunkBuffer.size(); i < len; i++ ) {
		if ( ! this->chunkBuffer[ i ] )
			continue;
		fprintf( f, "(#%d)\n", i + 1 );
		this->chunkBuffer[ i ]->print( f );
		fprintf( f, "\n" );
	}
	if ( len == 0 ) fprintf( f, "(None)\n" );

	fprintf( f, "\nWorkers\n-------\n" );
	for ( i = 0, len = this->workers.size(); i < len; i++ ) {
		fprintf( f, "%d. ", i + 1 );
		this->workers[ i ].print( f );
	}

	fprintf( f, "\nOther threads\n--------------\n" );
	this->sockets.self.printThread();

	fprintf( f, "\n" );

	fprintf( f, "\nPacket pool\n-----------\n" );
	this->packetPool.print( f );

	fprintf( f, "\nServer event queue\n-----------------\n" );
	this->eventQueue.print( f );
}

void Server::interactive() {
	char buf[ 4096 ];
	char *command;
	bool valid;
	int i, len;

	this->help();
	while( this->isRunning ) {
		valid = false;
		printf( "> " );
		fflush( stdout );
		if ( ! fgets( buf, sizeof( buf ), stdin ) ) {
			printf( "\n" );
			break;
		}

		// Trim
		len = strnlen( buf, sizeof( buf ) );
		for ( i = len - 1; i >= 0; i-- ) {
			if ( isspace( buf[ i ] ) )
				buf[ i ] = '\0';
			else
				break;
		}

		command = buf;
		while( isspace( command[ 0 ] ) ) {
			command++;
		}
		if ( strlen( command ) == 0 )
			continue;

		if ( strcmp( command, "help" ) == 0 ) {
			valid = true;
			this->help();
		} else if ( strcmp( command, "exit" ) == 0 ) {
			break;
		} else if ( strcmp( command, "info" ) == 0 ) {
			valid = true;
			this->info();
		} else if ( strcmp( command, "debug" ) == 0 ) {
			valid = true;
			this->debug();
		} else if ( strcmp( command, "id" ) == 0 ) {
			valid = true;
			this->printInstanceId();
		} else if ( strcmp( command, "lookup" ) == 0 ) {
			valid = true;
			this->lookup();
		} else if ( strcmp( command, "seal" ) == 0 ) {
			valid = true;
			this->seal();
		} else if ( strcmp( command, "flush" ) == 0 ) {
			valid = true;
			this->flush();
		} else if ( strcmp( command, "p2disk" ) == 0 ) {
			valid = true;
			this->flush( true );
		} else if ( strcmp( command, "memory" ) == 0 ) {
			valid = true;
			this->memory();

			FILE *f = fopen( "memory.log", "w" );
			this->memory( f );
			fclose( f );
		} else if ( strcmp( command, "delay" ) == 0 ) {
			valid = true;
			this->setDelay();
		} else if ( strcmp( command, "pending" ) == 0 ) {
			valid = true;
			this->printPending();
		} else if ( strcmp( command, "chunk" ) == 0 ) {
			valid = true;
			this->printChunk();
		} else if ( strcmp( command, "sync" ) == 0 ) {
			valid = true;
			this->sync();
		} else if ( strcmp( command, "backup" ) == 0 ) {
			valid = true;
			this->backupStat( stdout );
		} else if ( strcmp( command, "remapping" ) == 0 ) {
			valid = true;
			this->printRemapping();
		} else if ( strcmp( command, "time" ) == 0 ) {
			valid = true;
			this->time();
		} else {
			valid = false;
		}

		if ( ! valid ) {
			fprintf( stderr, "Invalid command!\n" );
		}
	}
}

void Server::printPending( FILE *f ) {
	PendingType types[] = {
		PT_CLIENT_GET, PT_CLIENT_UPDATE, PT_CLIENT_DEL,
		PT_SERVER_PEER_UPDATE, PT_SERVER_PEER_DEL,
		PT_SERVER_PEER_UPDATE_CHUNK, PT_SERVER_PEER_DEL_CHUNK,
		PT_SERVER_PEER_GET_CHUNK, PT_SERVER_PEER_SET_CHUNK,
		PT_SERVER_PEER_PARITY
	};
	for ( int i = 0; i < 8; i++ )
		this->pending.print( types[ i ], f );
}

void Server::printChunk() {
	uint32_t listId, stripeId, chunkId;
	printf( "Which chunk (List ID, Stripe ID, Chunk ID)? " );
	fflush( stdout );
	if ( scanf( "%u %u %u", &listId, &stripeId, &chunkId ) == 3 ) {
		Chunk *chunk = this->map.findChunkById( listId, stripeId, chunkId );
		if ( chunk ) {
			ChunkUtil::print( chunk );
		} else {
			printf( "Not found.\n" );
		}
	} else {
		printf( "Invalid input.\n" );
	}
}

void Server::printInstanceId( FILE *f ) {
	fprintf( f, "Instance ID = %u\n", Server::instanceId );
}

void Server::help() {
	fprintf(
		stdout,
		"Supported commands:\n"
		"- help: Show this help message\n"
		"- info: Show configuration\n"
		"- debug: Show debug messages\n"
		"- id: Print instance ID\n"
		"- lookup: Search for the metadata of an input key\n"
		"- seal: Seal all chunks in the chunk buffer\n"
		"- flush: Flush all dirty chunks to disk\n"
		"- delay: Add constant delay to each client response\n"
		"- sync: Synchronize with coordinator\n"
		"- chunk: Print the debug message for a chunk\n"
		"- pending: Print all pending requests\n"
		"- remapping: Show remapping info\n"
		"- memory: Print memory usage\n"
		"- backup : Show the backup stats\n"
		"- time: Show elapsed time\n"
		"- exit: Terminate this client\n"
	);
	fflush( stdout );
}

void Server::lookup() {
	char key[ 256 + SPLIT_OFFSET_SIZE ];
	uint8_t keySize;
	uint32_t offset;
	char *obj = 0;

	printf( "Input key: " );
	fflush( stdout );
	if ( ! fgets( key, sizeof( key ), stdin ) ) {
		fprintf( stderr, "Invalid input!\n" );
		return;
	}
	keySize = ( uint8_t ) strnlen( key, sizeof( key ) ) - 1;
	memset( key + keySize, 0, SPLIT_OFFSET_SIZE );

	bool found = false;

	KeyMetadata keyMetadata;
	if ( ( obj = this->map.findObject( key, keySize ) ) || ( obj = this->map.findLargeObject( key, keySize ) ) ) {
		Chunk *chunk = this->chunkPool.getChunk( obj, offset );
		if ( chunk ) {
			Metadata m = ChunkUtil::getMetadata( chunk );

			keyMetadata.listId = m.listId;
			keyMetadata.stripeId = m.stripeId;
			keyMetadata.chunkId = m.chunkId;
			keyMetadata.length = KeyValue::getSize( obj );
			keyMetadata.offset = offset;

			uint8_t _keySize;
			uint32_t _valueSize, _splitOffset;
			char *_key, *_value;
			KeyValue::deserialize( obj, _key, _keySize, _value, _valueSize, _splitOffset );
			printf(
				"Metadata: (%u, %u, %u); key size: %u, value size: %u, split offset: %u, offset: %u, length: %u\n",
				keyMetadata.listId, keyMetadata.stripeId, keyMetadata.chunkId,
				_keySize, _valueSize, _splitOffset,
				keyMetadata.offset, keyMetadata.length
			);
		} else {
			printf( "Key is found but chunk is not found.\n" );
		}

		found = true;
	}

	RemappedKeyValue remappedKeyValue;
	if ( this->remappedBuffer.find( keySize, key, false, &remappedKeyValue ) ) {
		printf( "Remapped key found [%u, %u]: ", remappedKeyValue.listId, remappedKeyValue.chunkId );
		for ( uint32_t i = 0; i < remappedKeyValue.remappedCount; i++ ) {
			uint8_t keySize;
			uint32_t valueSize;
			char *keyStr, *valueStr;
			remappedKeyValue.keyValue._deserialize( keyStr, keySize, valueStr, valueSize );
			printf(
				"%s(%u, %u) |-> (%u, %u); length: %u%s",
				i == 0 ? "" : "; ",
				remappedKeyValue.original[ i * 2     ],
				remappedKeyValue.original[ i * 2 + 1 ],
				remappedKeyValue.remapped[ i * 2     ],
				remappedKeyValue.remapped[ i * 2 + 1 ],
				valueSize,
				i == remappedKeyValue.remappedCount - 1 ? "\n" : ""
			);
		}
		found = true;
	}

	bool isSealed;
	KeyValue keyValue;
	if ( this->degradedChunkBuffer.map.findValueByKey( key, keySize, false, isSealed, &keyValue, 0, &keyMetadata ) ) {
		if ( isSealed ) {
			uint8_t keySize;
			uint32_t valueSize;
			char *keyStr, *valueStr;
			keyValue._deserialize( keyStr, keySize, valueStr, valueSize );
			printf(
				"Reconstructed chunk found: (%u, %u, %u); offset: %u, length: %u; is sealed? %s\n",
				keyMetadata.listId, keyMetadata.stripeId, keyMetadata.chunkId, keyMetadata.offset, keyMetadata.length,
				isSealed ? "yes" : "no"
			);
			fflush( stdout );
		} else {
			uint8_t keySize;
			uint32_t valueSize;
			char *keyStr, *valueStr;
			keyValue._deserialize( keyStr, keySize, valueStr, valueSize );
			printf(
				"Reconstructed key found: %.*s; key size: %u, value size: %u; is sealed? %s\n",
				keySize, keyStr, keySize, valueSize,
				isSealed ? "yes" : "no"
			);
		}
		found = true;
	}

	if ( ! found )
		printf( "Key not found.\n" );
}

void Server::printRemapping( FILE *f ) {
	fprintf(
		f,
		"\nList of Tracking Servers\n"
		"------------------------\n"
	);
	this->stateTransitHandler.listAliveServers();
}

void Server::time() {
	fprintf( stdout, "Elapsed time: %12.6lf s\n", this->getElapsedTime() );
	fflush( stdout );
}

void Server::alarm() {
	::alarm( this->config.global.timeout.metadata / 1000 );
}

void Server::backupStat( FILE *f ) {
	fprintf( f, "\nServer delta backup stats\n============================\n" );
	for ( int i = 0, len = this->sockets.clients.size(); i < len; i++ ) {
		fprintf( f,
			">> Client FD = %u\n-------------------\n",
			this->sockets.clients.keys[ i ]
		);
		this->sockets.clients.values[ i ]->backup.print( f );
		fprintf( f, "\n" );
	}
}
