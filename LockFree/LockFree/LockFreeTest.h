#ifndef __LOCKFREESTACK_TEST__H__
#define __LOCKFREESTACK_TEST__H__

struct st_TEST_DATA
{
	volatile LONG64	lData;
	volatile LONG64	lCount;
	void* overPtr;
	void* nodePtr;
	void* nextPtr;
	volatile LONG64 EnqCnt;
	volatile LONG64 DeqCnt;
};

DWORD E_CompareExchange_Fail;
DWORD D_CompareExchange_Fail;

DWORD E_CompareExchange_Success;
DWORD D_CompareExchange_Success;

#define dfTHREAD_ALLOC 1000
#define dfTHREAD_MAX 8

#endif