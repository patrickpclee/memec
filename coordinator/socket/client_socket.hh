#ifndef __COORDINATOR_SOCKET_CLIENT_SOCKET_HH__
#define __COORDINATOR_SOCKET_CLIENT_SOCKET_HH__

#include "../../common/ds/array_map.hh"
#include "../../common/socket/socket.hh"

class ClientSocket : public Socket {
private:
	static ArrayMap<int, ClientSocket> *clients;

public:
	uint16_t instanceId;
	struct {
		uint32_t addr;
		uint16_t port;
	} listenAddr;

	static void setArrayMap( ArrayMap<int, ClientSocket> *clients );
	bool start();
	void stop();
	void setListenAddr( uint32_t addr, uint16_t port );
};

#endif
