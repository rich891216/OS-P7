#ifndef _SHM_SLOT_
#define _SHM_SLOT_

typedef struct {
	pthread_t id;  // thread id
	int amount;    // number of new requests
	int requests;  // total requests
	int s_req; 	   // static requests
	int d_req;	   // dynamic requests
} slot_t;

#endif