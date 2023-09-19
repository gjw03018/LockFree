#pragma once
#include <Windows.h>
#include <stdio.h>

#include <list>
#include "MemoryPool.h"

template<typename DATA>

class CLockFree
{
public:
	CLockFree() { /*Top.data = 0; Top.UniqueAddress = 0; Top.next = nullptr;*/ };
	~CLockFree() {};

private:
	struct Node
	{
		DATA data;
		__int64 UniqueAddress;
		Node* next;
	};
	CMemoryPool<Node> LockFreePool;
	Node* Top = 0;
	DWORD Cnt = 0;
	short UniqueValue;

	bool CAS(Node** Dest, Node* Exchange, Node* Src)
	{
		LONG64 ret = InterlockedCompareExchange64( (LONG64*)Dest, (LONG64)Exchange, (LONG64)Src);
		if (ret == (LONG64)Src)
		{
			return true;
		}
		return false;
	}
public:
	int GetMemoryPoolSize() { return LockFreePool.GetAllocCount(); }
	void Push(DATA _data)
	{
		short value = InterlockedIncrement16(&UniqueValue);
		Node* nNode = LockFreePool.Alloc();
		Node* newNode_next = nNode->next;
		nNode->data = _data;
		nNode->UniqueAddress = (__int64)nNode | ((__int64)value << 48);

		Node* preTop;

		while (1)
		{
			preTop = Top;
			nNode->next = (Node*)((__int64)preTop & 0x00007fffffffffff);

			if (CAS(&Top, (Node*)nNode->UniqueAddress, preTop))
			{
				InterlockedIncrement(&Cnt);
				break;
			}
		}
	}

	void Pop(DATA& _data)
	{
		Node* nextTop;
		Node* popNode;

		Node* compareNode = nullptr;
		while (1)
		{
			if (Top == nullptr)
				break;

			popNode = Top;

			nextTop = (Node*) ( (Node*) (  (__int64)popNode & 0x00007fffffffffff) )->next;

			if (nextTop != nullptr)
				compareNode = (Node*)nextTop->UniqueAddress;

			if (CAS(& Top, compareNode, popNode))
			{
				//printf("%d\n",((Node*)((__int64)popNode & 0x00007fffffffffff))->data);
				_data = ((Node*)((__int64)popNode & 0x00007fffffffffff))->data;
				LockFreePool.Free((Node*)((__int64)popNode & 0x00007fffffffffff));
				
				InterlockedDecrement(&Cnt);
				break;
			}
		}
	}
};