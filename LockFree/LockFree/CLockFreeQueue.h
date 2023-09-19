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
    Node* _head;        // ���۳�带 ����Ʈ�Ѵ�.
    Node* _tail;        // ��������带 ����Ʈ�Ѵ�.
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
                //���� tail next �� ����ٸ�
                //tail->next �� Enq ������ �Ѵ�
                if (InterlockedCompareExchangePointer((PVOID*)&((Node*)((__int64)tail & 0x00007fffffffffff))->next, node, nullptr) == nullptr)
                {
                    //LONG64 ret = InterlockedCompareExchange64((LONG64*)&_tail->UniqueAddress, node->UniqueAddress, UnqiueAddress);
                    //_tail ��¥ ���Ͽ� ��ġ�� �ű�� ����
                    //������ ���ϰ� ������ ������ ��
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
        //_tail �� 1 CAS ������ -> Deq ������ head �� �̵� ���� _tail �� ����Ű�� ���� ��ȯ
        //_tail �� ����Ű�� ��尡 �����̵Ǹ鼭 _tail->next == nullptr �̵Ǹ�
        //EnQ �� �����ѻ��� But
        //���� node �� �� �ּҸ� ���� node ��� _tail �� �̹� ������ ���̳�带 ����Ű�� �ִ� �����̴�
        //_tail �� ���� �� ���Ͽ� Q�� ��������( head �� _tail �� ����Ǿ� ���� ��������)
        //�̷��� ���� size �� 0 �϶� �߻� �ϴ� ����
        // 
        //1. ���� size �� ���� ��Ű�� ���ϰ�(���а�) �� 0���� Ȯ��
        /*if (InterlockedExchangeAdd(&_size, 1) == 1)
        {
            //���ϰ��� 1 �̶�� _tail �� �̹� ����� ���̳�带 ����Ű�� ���� Ȯ���� ����
            //�̶��� _tail ���� _head �� �ٲپ� �־���Ѵ�
            //      ���� ���ο� node �� ���� �ϱ� ��
            //LockFree �ݺ��� ���� ���� ������ ������ ���·� ����� �ش�
            // ���⼭ �̹� Tail �� �Ű� �ֱ⶧���� CAS2 ���� �����Ҽ� �ִ�
            //������ ���и� �ϴ��� �̹� �ƹ� ���� ����
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
                //���� tail next �� ����ٸ�
                //tail->next �� Enq ������ �Ѵ�
                if (InterlockedCompareExchangePointer((PVOID*) &((Node*)((__int64)tail & 0x00007fffffffffff))->next , node, nullptr) == nullptr)
                {
                    //LONG64 ret = InterlockedCompareExchange64((LONG64*)&_tail->UniqueAddress, node->UniqueAddress, UnqiueAddress);
                    //_tail ��¥ ���Ͽ� ��ġ�� �ű�� ����
                    //������ ���ϰ� ������ ������ ��
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
            //����
            Node* head = _head;

            //���� head->next �� �����޵� �Ұ��ϰ� nullptr �� �����°��
            //�̽������� �ٸ� �����忡�� head �� ������� ������ ������ �̷�����ٸ�
            // ���� EnQ �������� Alloc �� ������ next �� nullptr �� �ʱ�ȭ��
            // ������ ������ �̷�����鼭 next �� nullptr�� �̷������

            //��¥ ������ next
            //���� EnQ �� ��ŭ DeQ �� ���ֱ� ������ �����Ͱ� ���� ��Ȳ�� ����� �Ѵ�
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
                �̻��̿� �ι� DeQ �Ǵ°� Ȯ���ؾߵǴ� ����
                ���� ���⼭ ����� �ι� DeQ �Ǵ°� Ȯ���� �Ǹ�? �״�����?
                DeQ �� ������ head ���� ���°��ݾ�
                1. _tail �� ���ؼ� �ι� DeQ �� �Ǵ°�?
                    _tail �� ���ؼ� �ι� �Ͼ�ٸ� EnQ�� �ι� �Ͼ��
                2. _head �� ���ؼ� �ι� DeQ �� �Ǵ°�?
                    �����͸� DeQ �����ŵ� Q�� ��ȭ�� ������

                �ΰ��� ����ʿ� ������ �Ѱ��̳�

                _tail �� �������� �ʾƼ� ���� -> ���� nullptr �� �ǵ帮�� ����
                ���� �����Ͱ� �������� EnQ �Ǵ� ��찡 ����

                _head �� �������� ���� -> ������ ������ ���ݾ� �������� ���д� next == nullptr �϶�

                ����� ���� Q�� ����Ʈ �� �����ɷ� ���� ����

                Q�� ����Ʈ�� ��� ����?

                �ϴ� ���� ���� ���� �Ҷ����� next = nullptr �� �ʱ�ȭ ��
                �׷��� �������� ������ �ΰ���
                ��� ����??
                ��� EnQ �� �ι��ϴ°�???
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
            //����
            Node* head = _head;

            //���� head->next �� �����޵� �Ұ��ϰ� nullptr �� �����°��
            //�̽������� �ٸ� �����忡�� head �� ������� ������ ������ �̷�����ٸ�
            // ���� EnQ �������� Alloc �� ������ next �� nullptr �� �ʱ�ȭ��
            // ������ ������ �̷�����鼭 next �� nullptr�� �̷������
            
            //��¥ ������ next
            //���� EnQ �� ��ŭ DeQ �� ���ֱ� ������ �����Ͱ� ���� ��Ȳ�� ����� �Ѵ�
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

            //head->next �� ���� �Ұ��
            //���� �����Ͱ� ���� �ϴٴ°��� �ǹ�
            //�̶� head(���̳��, ��������) �� _tail �� ���ٴ°���
            //_tail �� _head ���� ������ ��Ȳ
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
                �̻��̿� �ι� DeQ �Ǵ°� Ȯ���ؾߵǴ� ����
                ���� ���⼭ ����� �ι� DeQ �Ǵ°� Ȯ���� �Ǹ�? �״�����?
                DeQ �� ������ head ���� ���°��ݾ�
                1. _tail �� ���ؼ� �ι� DeQ �� �Ǵ°�?
                    _tail �� ���ؼ� �ι� �Ͼ�ٸ� EnQ�� �ι� �Ͼ��
                2. _head �� ���ؼ� �ι� DeQ �� �Ǵ°�?
                    �����͸� DeQ �����ŵ� Q�� ��ȭ�� ������
                
                �ΰ��� ����ʿ� ������ �Ѱ��̳�

                _tail �� �������� �ʾƼ� ���� -> ���� nullptr �� �ǵ帮�� ����
                ���� �����Ͱ� �������� EnQ �Ǵ� ��찡 ����

                _head �� �������� ���� -> ������ ������ ���ݾ� �������� ���д� next == nullptr �϶�
                
                ����� ���� Q�� ����Ʈ �� �����ɷ� ���� ����

                Q�� ����Ʈ�� ��� ����?
                
                �ϴ� ���� ���� ���� �Ҷ����� next = nullptr �� �ʱ�ȭ ��
                �׷��� �������� ������ �ΰ���
                ��� ����??
                ��� EnQ �� �ι��ϴ°�???
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
