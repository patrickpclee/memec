#include "coding.hh"
#include "all_coding.hh"
#include "../util/debug.hh"

char *Coding::bitwiseXOR( char *dst, char *srcA, char *srcB, uint32_t len ) {
	uint64_t *srcA64 = ( uint64_t * ) srcA;
	uint64_t *srcB64 = ( uint64_t * ) srcB;
	uint64_t *dst64 = ( uint64_t * ) dst;

	uint64_t xor64Count = len / sizeof( uint64_t );
	uint64_t i = 0;

	// Word-by-word XOR
	for ( i = 0; i < xor64Count; i++ ) {
		dst64[ i ] = srcA64[ i ] ^ srcB64[ i ];
	}

	i = xor64Count * sizeof( uint64_t );

	for ( ; i < len; i++ ) {
		dst[ i ] = srcA[ i ] ^ srcB[ i ];
	}

	return dst;
}

Chunk *Coding::bitwiseXOR( Chunk *dst, Chunk *srcA, Chunk *srcB, uint32_t size ) {
	this->bitwiseXOR(
		dst->data,
		srcA->data,
		srcB->data,
		size
	);
	dst->size = size > dst->size ? size : dst->size;
	return dst;
}

Coding::~Coding() {}

Coding *Coding::instantiate( CodingScheme scheme, CodingParams &params, uint32_t chunkSize ) {
	switch( scheme ) {
		case CS_RAID0:
			break;
		case CS_RAID1:
			break;
		case CS_RAID5:
			return new Raid5Coding2( params.getDataChunkCount(), chunkSize );
		case CS_RS:
			break;
		case CS_EMBR:
			break;
		case CS_RDP:
			return new RDPCoding( params.getK(), chunkSize );
		case CS_EVENODD:
			break;
		case CS_CAUCHY:
			return new CauchyCoding( params.getK(), params.getM(), chunkSize );
		default:
			break;
	}

	__ERROR__( "Coding", "instantiate", "Coding scheme is not yet implemented." );
	return 0;
}

void Coding::destroy( Coding *coding ) {
	switch( coding->scheme ) {
		case CS_RAID0:
			break;
		case CS_RAID1:
			break;
		case CS_RAID5:
			delete static_cast<Raid5Coding2 *>( coding );
			break;
		case CS_RS:
			break;
		case CS_EMBR:
			break;
		case CS_RDP:
			delete static_cast<RDPCoding *>( coding );
			break;
		case CS_EVENODD:
			break;
		case CS_CAUCHY:
			delete static_cast<CauchyCoding *>( coding );
			break;
		default:
			return;
	}
}