#ifndef __COMMON_CODING_CAUCHYCODING_HH__
#define __COMMON_CODING_CAUCHYCODING_HH__

#include "coding.hh"

class CauchyCoding : public Coding {

private:

    /**
     * Get the minimal no. of packets per chunk, w
     *
     * this->_w will be set
     * @return the minimal no. of packets per chunk
     */
    uint32_t getW ();

    /**
     * Generate the matrix and schedule for jerasure
     *
     * this->__jmatrix, this->_jbitmatrix, this->_jschedule will be set
     *
     */
    void generateCodeMatrix();

    uint32_t _k;
    uint32_t _m;
    uint32_t _w;
    uint32_t _chunkSize;

    int *_jmatrix;
    int *_jbitmatrix;
    int **_jschedule;
    
public:
    
    CauchyCoding ( uint32_t k = 0, uint32_t m = 0, uint32_t chunkSize = 0 );
    ~ CauchyCoding ();

    void encode ( Chunk **dataChunks, Chunk *parityChunk, uint32_t index );

    bool decode ( Chunk **chunks, BitmaskArray * chunkStatus );

};

#endif