#include "client_socket.hh"
#include "../../common/util/debug.hh"

ArrayMap<int, ClientSocket> *ClientSocket::clients;

void ClientSocket::setArrayMap( ArrayMap<int, ClientSocket> *clients ) {
	ClientSocket::clients = clients;
	clients->needsDelete = false;
}

bool ClientSocket::start() {
	return this->connect();
}

void ClientSocket::stop() {
	ClientSocket::clients->remove( this->sockfd );
	Socket::stop();
	// TODO: Fix memory leakage!
	// delete this;
}
