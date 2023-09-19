#pragma once

#include <list>
#include <stdio.h>
#include <Windows.h>

#define CRASH() do \
{ \
	int* ptr = nullptr; \
	*ptr = 100; \
} while(0)

template<typename T>
class CMemoryPool
{
public:
	CMemoryPool(bool _placement = false) { placement = _placement; freeNode = nullptr; };
	~CMemoryPool() {};

	T* Alloc();
	bool Free(T* data);
	DWORD GetAllocCount() { return AllocCnt; }
	DWORD GetFreeCnt() { return FreeCnt; }

private:
	struct Node
	{
		CMemoryPool<T>* ovcheck;
		T node;
		__int64 UniqueAddress;
		CMemoryPool<T>* uncheck;
		Node* next;
	};
	
	bool CAS(Node** Dest, Node* Exchange, Node* Src)
	{
		LONG64 ret = InterlockedCompareExchange64((LONG64*)Dest, (LONG64)Exchange, (LONG64)Src);
		if (ret == (LONG64)Src)
		{
			return true;
		}
		return false;
	}
	
	short UniqueValue = 1;
	__int64 UniqueAddress = 0;
	bool placement;
	DWORD AllocCnt = 0;
	DWORD FreeCnt = 0;
	Node* freeNode;
};

template<typename T>
inline T* CMemoryPool<T>::Alloc()
{
	unsigned short value = (unsigned short)_InterlockedIncrement16(&UniqueValue);
	Node* pNode;
	Node* nextTop;
	Node* compareNode = nullptr;

	while (1)
	{
		if (freeNode == nullptr)
		{
			InterlockedIncrement(&AllocCnt);
			pNode = (Node*)malloc(sizeof(Node) + sizeof(T));
			pNode->ovcheck = this;
			pNode->uncheck = this;
			if (placement)
			{
				new(&pNode->node) T;
			}
			break;
		}
		pNode = freeNode;
		nextTop = (Node*)((Node*)((__int64)pNode & 0x00007fffffffffff))->next;

		if (nextTop != nullptr)
			compareNode = (Node*)nextTop->UniqueAddress;

		if (CAS(&freeNode, compareNode, pNode))
		{
			break;
		}

	}
	

	pNode = (Node*)((__int64)pNode & 0x00007fffffffffff);
	pNode->UniqueAddress = (__int64)pNode | ((__int64)value << 48);


	return &pNode->node;
}

template<typename T>
inline bool CMemoryPool<T>::Free(T* data)
{
	Node* pNode = (Node*)((PVOID*)data-1);
	Node* preTop;
	if (pNode->ovcheck != this) 
		CRASH();
	if (pNode->uncheck != this) 
		CRASH();
	while (1)
	{
		preTop = freeNode;
		pNode->next = (Node*)((__int64)preTop & 0x00007fffffffffff);

		if (CAS(&freeNode, (Node*)pNode->UniqueAddress, preTop))
		{
			InterlockedIncrement(&FreeCnt);
			break;
		}
	}

	return true;
}
