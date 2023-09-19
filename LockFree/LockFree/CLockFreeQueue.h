#pragma once
#include <Windows.h>
#include "MemoryPool.h"
#include "LockFreeTest.h"
template<typename T>
class QueueT
{
private:
    long _size;

    struct Node
    {
        T data;
        __int64 UniqueAddress;
        bool Alive;
        Node* next;
    };
    short Unique = 0;
    Node* _head;        // 시작노드를 포인트한다.
    Node* _tail;        // 마지막노드를 포인트한다.
    Node* _headUniqueAddress = 0;
    Node* _tailUniqueAddress = 0;
    CMemoryPool<Node> LockFreePool;
public:
    int GetMemoryPoolSize() { return LockFreePool.GetAllocCount(); }
    int GetSize() { return _size; }
    QueueT()
    {
        _size = 0;
        _head = LockFreePool.Alloc();
        _head->next = NULL;
        _tail = _head;
        Unique = 0;
    }

    void Enqueue(T t)
    {
        unsigned short value = (unsigned short)InterlockedIncrement16(&Unique);

        Node* node = LockFreePool.Alloc();
        if (node->Alive == true)
        {
            int a = 100;
            a++;
        }
        _mm_mfence;

        node->data = t;
        node->next = NULL;
        node->UniqueAddress = (__int64)node | ((__int64)value << 48);
        node->Alive = true;
        while (true)
        {
            Node* tail = _tail;
            Node* next = ((Node*)((__int64)tail & 0x00007fffffffffff))->next;
            if (next == nullptr)
            {
                //지금 tail next 가 비었다면
                //tail->next 에 Enq 행위를 한다
                if (InterlockedCompareExchangePointer((PVOID*)&((Node*)((__int64)tail & 0x00007fffffffffff))->next, node, nullptr) == nullptr)
                {
                    //LONG64 ret = InterlockedCompareExchange64((LONG64*)&_tail->UniqueAddress, node->UniqueAddress, UnqiueAddress);
                    //_tail 진짜 테일에 위치를 옮기는 구간
                    //현재의 테일과 과거의 테일을 비교
                    PVOID ret = InterlockedCompareExchangePointer((PVOID*)&_tail, (PVOID)node->UniqueAddress, tail);
                    if (ret != tail)
                    {
                        int* a = nullptr;
                        *a = 10;
                    }
                    //InterlockedCompareExchangePointer((PVOID*)&_tail, node, tail);
                    InterlockedIncrement(&E_CompareExchange_Success);
                    break;
                }
                else
                {
                    InterlockedIncrement(&E_CompareExchange_Fail);
                }
            }
        }

        InterlockedExchangeAdd(&_size, 1);
    }

    void Enqueue(T t, void** nodeptr, void** nextptr, volatile LONG64* EnQCnt)
    {
        unsigned short value = (unsigned short)InterlockedIncrement16(&Unique);

        Node* node = LockFreePool.Alloc();

        node->data = t;
        node->next = NULL;
        node->UniqueAddress = (__int64)node | ((__int64)value << 48);
        node->Alive = true;
        InterlockedExchangeAdd(&_size, 1);
        //_tail 이 1 CAS 성공후 -> Deq 로인해 head 가 이동 기존 _tail 이 가리키는 노드는 반환
        //_tail 이 가리키는 노드가 재사용이되면서 _tail->next == nullptr 이되며
        //EnQ 가 가능한상태 But
        //만약 node 가 새 주소를 가는 node 라면 _tail 은 이미 삭제된 더미노드를 가리키고 있는 상태이다
        //_tail 의 재사용 에 의하여 Q가 깨진산태( head 랑 _tail 이 연결되어 있지 않은상태)
        //이러한 경우는 size 가 0 일때 발생 하는 문제
        // 
        //1. 먼저 size 를 증가 시키고 리턴값(증분값) 이 0인지 확인
        /*if (InterlockedExchangeAdd(&_size, 1) == 1)
        {
            //리턴값이 1 이라면 _tail 이 이미 사라진 더미노드를 가리키고 있을 확률이 있음
            //이때는 _tail 값을 _head 로 바꾸어 주어야한다
            //      아직 새로운 node 가 연결 하기 전
            //LockFree 반복을 들어가기 전에 삽입이 가능한 상태로 만들어 준다
            // 여기서 이미 Tail 을 옮겨 주기때문에 CAS2 에서 실패할수 있다
            //하지만 실패를 하더라도 이미 아무 문제 없다
            InterlockedExchangePointer((PVOID*)& _tail, (PVOID)_head);

        }*/
        while (true)
        {
            Node* tail = _tail;
            Node* next = ((Node*)((__int64)tail & 0x00007fffffffffff))->next;
            *nodeptr = node;
            *nextptr = next;
            if (next == nullptr)
            {
                //지금 tail next 가 비었다면
                //tail->next 에 Enq 행위를 한다
                if (InterlockedCompareExchangePointer((PVOID*) &((Node*)((__int64)tail & 0x00007fffffffffff))->next , node, nullptr) == nullptr)
                {
                    //LONG64 ret = InterlockedCompareExchange64((LONG64*)&_tail->UniqueAddress, node->UniqueAddress, UnqiueAddress);
                    //_tail 진짜 테일에 위치를 옮기는 구간
                    //현재의 테일과 과거의 테일을 비교
                    PVOID ret = InterlockedCompareExchangePointer((PVOID*)&_tail, (PVOID)node->UniqueAddress, tail);
                    if (ret != tail)
                    {
                        int* a = nullptr;
                        *a = 10;
                    }
                    //InterlockedCompareExchangePointer((PVOID*)&_tail, node, tail);
                    InterlockedIncrement(&E_CompareExchange_Success);
                    *EnQCnt++;
                    break;
                }
                else
                {
                    InterlockedIncrement(&E_CompareExchange_Fail);
                }
            }
        }

        
    }
    int Dequeue(T& t)
    {
        while (true)
        {
            int size = this->_size;
            //더미
            Node* head = _head;

            //문제 head->next 가 있으메도 불가하고 nullptr 로 나오는경우
            //이시점에서 다른 스레드에서 head 를 대상으로 삭제와 재사용이 이루어진다면
            // 현재 EnQ 시점에서 Alloc 을 받을때 next 를 nullptr 로 초기화중
            // 삭제와 재사용이 이루어지면서 next 가 nullptr로 이루어진다

            //진짜 데이터 next
            //현재 EnQ 한 만큼 DeQ 를 해주기 때문에 데이터가 없는 상황은 없어야 한다
            Node* next = ((Node*)((__int64)head & 0x00007fffffffffff))->next;
            if (next == NULL && size <= 0)
            {
                wprintf(L"LockFreeQueue UseSize %d\n", size);
                return -1;
            }

            if (next == NULL && size > 0)
            {
                continue;
            }
            /*if (next == NULL)
            {
                return -1;
            }*/
            PVOID ret = InterlockedCompareExchangePointer((PVOID*)&_head, (PVOID)next->UniqueAddress, head);
            if (ret == head)
            {
                InterlockedExchangeAdd(&_size, -1);
                t = next->data;
                /*
                이사이에 두번 DeQ 되는걸 확인해야되는 시점
                만약 여기서 멈췄어 두번 DeQ 되는게 확인이 되면? 그다음은?
                DeQ 에 문제는 head 에서 오는거잖아
                1. _tail 에 의해서 두번 DeQ 가 되는가?
                    _tail 에 의해서 두번 일어난다면 EnQ가 두번 일어난것
                2. _head 에 의해서 두번 DeQ 가 되는가?
                    데이터를 DeQ 했으매도 Q에 변화가 없을때

                두개중 어느쪽에 비중을 둘것이냐

                _tail 이 움직이지 않아서 실패 -> 라기앤 nullptr 을 건드리고 있음
                같은 데이터가 연속으로 EnQ 되는 경우가 없음

                _head 의 움직임이 실패 -> 뽑을수 있을땐 뽑잖아 움직임의 실패는 next == nullptr 일때

                결론을 보자 Q의 리스트 가 깨진걸로 볼수 있음

                Q의 리스트가 어떻게 깨짐?

                일단 재사용 부터 보면 할때마다 next = nullptr 로 초기화 중
                그러면 뭐때문에 같은게 두개나
                어떻게 문제??
                어떻게 EnQ 를 두번하는가???
                */
                if (next->Alive == false)
                {
                    int a = 100;
                    a++;
                }
                _mm_mfence;
                next->Alive = false;
                ((Node*)((__int64)head & 0x00007fffffffffff))->next = NULL;
                LockFreePool.Free((Node*)((__int64)head & 0x00007fffffffffff));
                InterlockedIncrement(&D_CompareExchange_Success);
                break;
            }
            else
            {
                InterlockedIncrement(&D_CompareExchange_Fail);
            }
            /*if (InterlockedCompareExchangePointer((PVOID*)&_head, next, head) == head)
            {
                t = next->data;
                delete head;
                break;
            }*/
        }
        return 0;
    }
    int Dequeue(T& t, void** nodeptr, void** nextptr, volatile LONG64* DeQCnt, void** overPtr)
    {
        while (true)
        {
            int size = this->_size;
            //더미
            Node* head = _head;

            //문제 head->next 가 있으메도 불가하고 nullptr 로 나오는경우
            //이시점에서 다른 스레드에서 head 를 대상으로 삭제와 재사용이 이루어진다면
            // 현재 EnQ 시점에서 Alloc 을 받을때 next 를 nullptr 로 초기화중
            // 삭제와 재사용이 이루어지면서 next 가 nullptr로 이루어진다
            
            //진짜 데이터 next
            //현재 EnQ 한 만큼 DeQ 를 해주기 때문에 데이터가 없는 상황은 없어야 한다
            Node* next = ((Node*)((__int64)head & 0x00007fffffffffff))->next;
            if (next == NULL && size <= 0)
            {
                wprintf(L"LockFreeQueue UseSize %d\n", size);
                return -1;
            }
            
            if (next == NULL && size > 0)
            {
                continue;
            }

            //head->next 가 존재 할경우
            //다음 데이터가 존재 하다는것을 의미
            //이때 head(더미노드, 사라질노드) 가 _tail 가 같다는것은
            //_tail 이 _head 보다 뒤쳐진 상황
            if (next != nullptr)
            {
                InterlockedCompareExchangePointer((PVOID*)&_tail, _head, head);
            }

            PVOID ret = InterlockedCompareExchangePointer((PVOID*)&_head, (PVOID)next->UniqueAddress, head);
            if ( ret == head)
            {
                InterlockedExchangeAdd(&_size, -1);
                t = next->data;
                /*
                이사이에 두번 DeQ 되는걸 확인해야되는 시점
                만약 여기서 멈췄어 두번 DeQ 되는게 확인이 되면? 그다음은?
                DeQ 에 문제는 head 에서 오는거잖아
                1. _tail 에 의해서 두번 DeQ 가 되는가?
                    _tail 에 의해서 두번 일어난다면 EnQ가 두번 일어난것
                2. _head 에 의해서 두번 DeQ 가 되는가?
                    데이터를 DeQ 했으매도 Q에 변화가 없을때
                
                두개중 어느쪽에 비중을 둘것이냐

                _tail 이 움직이지 않아서 실패 -> 라기앤 nullptr 을 건드리고 있음
                같은 데이터가 연속으로 EnQ 되는 경우가 없음

                _head 의 움직임이 실패 -> 뽑을수 있을땐 뽑잖아 움직임의 실패는 next == nullptr 일때
                
                결론을 보자 Q의 리스트 가 깨진걸로 볼수 있음

                Q의 리스트가 어떻게 깨짐?
                
                일단 재사용 부터 보면 할때마다 next = nullptr 로 초기화 중
                그러면 뭐때문에 같은게 두개나
                어떻게 문제??
                어떻게 EnQ 를 두번하는가???
                */
                next->Alive = false;
                *nodeptr = next;
                *nextptr = next->next;
                *overPtr = head;
                ((Node*)((__int64)head & 0x00007fffffffffff))->next = NULL;
                LockFreePool.Free((Node*)((__int64)head & 0x00007fffffffffff));
                InterlockedIncrement(&D_CompareExchange_Success);
                *DeQCnt++;
                break;
            }
            else
            {
                InterlockedIncrement(&D_CompareExchange_Fail);
            }
            /*if (InterlockedCompareExchangePointer((PVOID*)&_head, next, head) == head)
            {
                t = next->data;
                delete head;
                break;
            }*/
            
        }
        
        return 0;
    }
};
