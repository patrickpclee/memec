#include "client_socket.hh"
#include "../../common/util/debug.hh"

ArrayMap<int, ClientSocket> *ClientSocket::masters;

void ClientSocket::setArrayMap( ArrayMap<int, ClientSocket> *masters ) {
	ClientSocket::masters = masters;
	masters->needsDelete = false;
}

bool ClientSocket::start() {
	return this->connect();
}

void ClientSocket::stop() {
	ClientSocket::masters->remove( this->sockfd );
	Socket::stop();
	// TODO: Fix memory leakage!
	// delete this;
}

ssize_t ClientSocket::send( char *buf, size_t ulen, bool &connected ) {
	return Socket::send( this->sockfd, buf, ulen, connected );
}

ssize_t ClientSocket::recv( char *buf, size_t ulen, bool &connected, bool wait ) {
	return Socket::recv( this->sockfd, buf, ulen, connected, wait );
}

ssize_t ClientSocket::recvRem( char *buf, size_t expected, char *prevBuf, size_t prevSize, bool &connected ) {
	return Socket::recvRem( this->sockfd, buf, expected, prevBuf, prevSize, connected );
}

bool ClientSocket::done() {
	return Socket::done( this->sockfd );
}