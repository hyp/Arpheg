#include <stdlib.h>

#include "mersenneTwister.h"

void MersenneTwister::seed(uint32 seed){
	srand(seed);
	for (int i = 0; i < MT_LEN; i++)
		mt_buffer[i] = rand();
	mt_index = 0;
}

#define MT_IA           397
#define MT_IB           (MT_LEN - MT_IA)
#define UPPER_MASK      0x80000000
#define LOWER_MASK      0x7FFFFFFF
#define MATRIX_A        0x9908B0DF
#define TWIST(b,i,j)    ((b)[i] & UPPER_MASK) | ((b)[j] & LOWER_MASK)
#define MAGIC(s)        (((s)&1)*MATRIX_A)

uint32 MersenneTwister::random(){
	uint32* b = mt_buffer;
	uint32 idx = mt_index;
	uint32 s;
	uint32 i;
	
	if (idx == MT_LEN*sizeof(uint32)){
		idx = 0;
		i = 0;
		for (; i < MT_IB; i++) {
			s = TWIST(b, i, i+1);
			b[i] = b[i + MT_IA] ^ (s >> 1) ^ MAGIC(s);
		}
		for (; i < MT_LEN-1; i++) {
			s = TWIST(b, i, i+1);
			b[i] = b[i - MT_IB] ^ (s >> 1) ^ MAGIC(s);
		}
        
		s = TWIST(b, MT_LEN-1, 0);
		b[MT_LEN-1] = b[MT_IA-1] ^ (s >> 1) ^ MAGIC(s);
	}
	mt_index = idx + sizeof(uint32);
	return *(uint32 *)((uint8 *)b + idx);
}
