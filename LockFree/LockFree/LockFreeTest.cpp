#include <windows.h>
#include <stdio.h>
#include <process.h>
#include <time.h>
#include <stdlib.h>

#include "CLockFree.h"
#include "LockFreeTest.h"
#include "CLockFreeQueue.h"

CLockFree<st_TEST_DATA*> g_Stack;
QueueT< st_TEST_DATA*> g_Queue;

LONG64 lPushTPS = 0;
LONG64 lPopTPS = 0;

LONG64 lPushCounter = 0;
LONG64 lPopCounter = 0;

unsigned __stdcall StackThread(void* pParam);
unsigned __stdcall QueueThread(void* pParam);
unsigned __stdcall EnQThread(void* pParam);
unsigned __stdcall DeQThread(void* pParam);
int StartStack()
{
	HANDLE hThread[dfTHREAD_MAX];
	DWORD dwThreadID;
	for (int iCnt = 0; iCnt < dfTHREAD_MAX; iCnt++)
	{
		hThread[iCnt] = (HANDLE)_beginthreadex(
			NULL,
			0,
			StackThread,
			0,
			0,
			(unsigned int*)&dwThreadID
		);
	}
	return 1;
}
int StartQueue()
{
	HANDLE hThread[dfTHREAD_MAX];
	DWORD dwThreadID;
	/*
	hThread[0] = (HANDLE)_beginthreadex(
		NULL,
		0,
		EnQThread,
		(LPVOID)1000,
		0,
		(unsigned int*)&dwThreadID
	);
	hThread[0] = (HANDLE)_beginthreadex(
		NULL,
		0,
		EnQThread,
		(LPVOID)1000,
		0,
		(unsigned int*)&dwThreadID
	);
	hThread[1] = (HANDLE)_beginthreadex(
		NULL,
		0,
		DeQThread,
		(LPVOID)1000,
		0,
		(unsigned int*)&dwThreadID
	);
	Sleep(INFINITE);*/
	for (int iCnt = 0; iCnt < dfTHREAD_MAX; iCnt++)
	{
		hThread[iCnt] = (HANDLE)_beginthreadex(
			NULL,
			0,
			QueueThread,
			0,
			0,
			(unsigned int*)&dwThreadID
		);
	}
	return 2;
}
void main()
{
	int ret = 0;
	//ret = StartStack();
	ret = StartQueue();
	while (1)
	{
		lPushTPS = lPushCounter;
		lPopTPS = lPopCounter;

		lPushCounter = 0;
		lPopCounter = 0;

		wprintf(L"------------------------------------------------\n");
		if (ret == 1)
		{
			wprintf(L"Push TPS : %d\n", lPushTPS);
			wprintf(L"Pop TPS : %d\n", lPopTPS);
			wprintf(L"Alloc Cnt : %d\n", g_Stack.GetMemoryPoolSize());
		}
		else
		{
			wprintf(L"Enqueue TPS : %d\n", lPushTPS);
			wprintf(L"Dequeue TPS : %d\n", lPopTPS);
			wprintf(L"Enqueue CompareExchange Fail : %d\n", E_CompareExchange_Fail);
			wprintf(L"Dequeue CompareExchange Fail : %d\n", D_CompareExchange_Fail);
			wprintf(L"Enqueue CompareExchange Success : %d\n", E_CompareExchange_Success);
			wprintf(L"Dequeue CompareExchange Success : %d\n", D_CompareExchange_Success);
			InterlockedExchange(&E_CompareExchange_Fail, 0);
			InterlockedExchange(&D_CompareExchange_Fail, 0);
			InterlockedExchange(&E_CompareExchange_Success, 0);
			InterlockedExchange(&D_CompareExchange_Success, 0);
			wprintf(L"Alloc Cnt : %d\n", g_Queue.GetMemoryPoolSize());
		}
		wprintf(L"------------------------------------------------\n\n");

		Sleep(999);
	}
}

unsigned __stdcall StackThread(void* pParam)
{
	int iCnt;
	st_TEST_DATA* pData;
	st_TEST_DATA* pDataArray[dfTHREAD_ALLOC];


	for (iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
	{
		pDataArray[iCnt] = new st_TEST_DATA;
		pDataArray[iCnt]->lData = 0x0000000055555555;
		pDataArray[iCnt]->lCount = 0;
	}

	for (iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
	{
		//g_Stack.Push(pDataArray[iCnt]);
		g_Stack.Push(pDataArray[iCnt]);
		InterlockedIncrement64(&lPushCounter);
	}

	while (1) {
		Sleep(1);

		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
			g_Stack.Pop(pData);
			InterlockedIncrement64(&lPopCounter);
			pDataArray[iCnt] = pData;
			/*if (g_Stack.Dequeue(pData) != -1)
			{
				InterlockedIncrement64(&lPopCounter);
				pDataArray[iCnt] = pData;
				iCnt++;
			}*/

		}

		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
			if ((pDataArray[iCnt]->lData != 0x0000000055555555) || (pDataArray[iCnt]->lCount != 0))
				wprintf(L"LockfreeStack pDataArray[%04d] is using in stack\n", iCnt);

		}

		//다른 쓰레드에서 중복으로 뺏는지를 확인한다
		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
			InterlockedIncrement64(&pDataArray[iCnt]->lCount);
			InterlockedIncrement64(&pDataArray[iCnt]->lData);
		}

		Sleep(1);

		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
			if ((pDataArray[iCnt]->lCount != 1) || (pDataArray[iCnt]->lData != 0x0000000055555556))
			{
				wprintf(L"LockfreeStack pDataArray[%04d] is using in another stack\n", iCnt);
			}
		}

		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
			pDataArray[iCnt]->lData = 0x0000000055555555;
			pDataArray[iCnt]->lCount = 0;
		}

		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
			g_Stack.Push(pDataArray[iCnt]);
			//g_Stack.Enqueue(pDataArray[iCnt]);
			InterlockedIncrement64(&lPushCounter);
		}
	}
}
unsigned __stdcall QueueThread(void* pParam)
{
	std::list<st_TEST_DATA*> eList;
	std::list<st_TEST_DATA*> dList;

	st_TEST_DATA* eNode = new st_TEST_DATA;
	st_TEST_DATA* dNode = new st_TEST_DATA;

	st_TEST_DATA aaa;
	int iCnt;
	st_TEST_DATA* pData;
	st_TEST_DATA* pDataArray[dfTHREAD_ALLOC];


	for (iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
	{
		pDataArray[iCnt] = new st_TEST_DATA;
		pDataArray[iCnt]->lData = 0x0000000055555555;
		pDataArray[iCnt]->lCount = 0;
		pDataArray[iCnt]->DeqCnt = 0;
		pDataArray[iCnt]->EnqCnt = 0;

	}

	for (iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
	{
//		eNode = new st_TEST_DATA;
		g_Queue.Enqueue(pDataArray[iCnt], &pDataArray[iCnt]->nodePtr, &pDataArray[iCnt]->nextPtr, &pDataArray[iCnt]->EnqCnt);
//		eList.push_back(eNode);
		InterlockedIncrement64(&lPushCounter);
	}
	while (1) {
		Sleep(1);

		if (g_Queue.GetSize() < dfTHREAD_ALLOC)
		{
			int a = 100;
			a++;
		}

		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
//			dNode = new st_TEST_DATA;
			g_Queue.Dequeue(pData, &dNode->nodePtr, &dNode->nextPtr, &dNode->DeqCnt, &dNode->overPtr);
//			dList.push_back(dNode);
			InterlockedIncrement64(&lPopCounter);
			pDataArray[iCnt] = pData;
			pData->nodePtr = dNode->nodePtr;
			pData->nextPtr = dNode->nextPtr;
			pData->overPtr = dNode->overPtr;
			pData->DeqCnt++;

			//좁아가기 첫번째
			if (pData->DeqCnt >= 2)
			{
				int a = 100;
				a++;
			}
		}
		Sleep(1);
		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
			if ((pDataArray[iCnt]->lData != 0x0000000055555555) || (pDataArray[iCnt]->lCount != 0))
				wprintf(L"LockfreeStack pDataArray[%04d] Used Data\n", iCnt);

		}

		//다른 쓰레드에서 중복으로 뺏는지를 확인한다
		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
			InterlockedIncrement64(&pDataArray[iCnt]->lCount);
			InterlockedIncrement64(&pDataArray[iCnt]->lData);
		}

		Sleep(1);

		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
			if ((pDataArray[iCnt]->lCount != 1) || (pDataArray[iCnt]->lData != 0x0000000055555556))
			{
				wprintf(L"LockfreeStack pDataArray[%04d] Double DeQ Data\n", iCnt);
			}
		}

		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
			pDataArray[iCnt]->lData = 0x0000000055555555;
			pDataArray[iCnt]->lCount = 0;
			pDataArray[iCnt]->DeqCnt = 0;
			pDataArray[iCnt]->EnqCnt = 0;
		}

		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
//			eNode = new st_TEST_DATA;
			g_Queue.Enqueue(pDataArray[iCnt], &eNode->nodePtr, &eNode->nextPtr, &eNode->EnqCnt);
			pDataArray[iCnt]->nodePtr = eNode->nodePtr;
			pDataArray[iCnt]->nextPtr = eNode->nextPtr;
			pDataArray[iCnt]->EnqCnt++;
//			eList.push_back(eNode);
			InterlockedIncrement64(&lPushCounter);
		}
	}
}
unsigned __stdcall EnQThread(void* pParam)
{
	Sleep(10);
	while (1)
	{
		st_TEST_DATA* pData = new st_TEST_DATA;
		g_Queue.Enqueue(pData);
	}
}
unsigned __stdcall DeQThread(void* pParam)
{
	Sleep(10);
	while (1)
	{
		st_TEST_DATA* pData = new st_TEST_DATA;
		g_Queue.Dequeue(pData);
	}
}
/*
unsigned __stdcall LockFreeThread(void* avg)
{
	std::list<st_TEST_DATA*> EnQList;
	std::list<st_TEST_DATA*> DeQList;
	int data = (int)avg;
	while (1)
	{
		for (int i = 0; i < 1000; i++)
		{
			st_TEST_DATA* nNode = new st_TEST_DATA;
			st_TEST_DATA* eNode = new st_TEST_DATA;
			nNode->lData = data;
			eNode->lData = data;
			g_Stack.Enqueue(nNode, &eNode->next, &eNode->Nodeptr);
			EnQList.push_back(eNode);
		}

		for (int i = 0; i < 1000; i++)
		{
			st_TEST_DATA* dNode = new st_TEST_DATA;

			dNode->lData = data;
			g_Stack.Dequeue(dNode, &dNode->next, &dNode->Nodeptr);
			DeQList.push_back(dNode);
			int a = 10;
			a++;
		}
	}
}*/