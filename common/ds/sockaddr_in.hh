#ifndef __COMMON_DS_SOCKADDR_IN_HH__
#define __COMMON_DS_SOCKADDR_IN_HH__

#include <netinet/in.h>
#include <cstddef>
#include <functional>
#include "../hash/hash_func.hh"

bool operator==( const struct sockaddr_in &lhs, const struct sockaddr_in &rhs );
bool operator<( const struct sockaddr_in &lhs, const struct sockaddr_in &rhs );

// hash function for unordered_*
namespace std {
	template<> struct hash<struct sockaddr_in> {
		std::size_t operator()( struct sockaddr_in const &s ) const {
			return HashFunc::hash( ( char * ) &s.sin_addr.s_addr, ( unsigned int ) sizeof( s.sin_addr.s_addr ) );
		}
	};
}

#endif
