#pragma once
#include "pch.h"
#include "NewClientHandle.h"

template <typename T>
class ThreadSafeQueue {
public:
    ThreadSafeQueue()
        : head_(new Node), tail_(head_.load(std::memory_order_relaxed))
    {
        Node* front = head_.load(std::memory_order_relaxed);
        front->next = nullptr;
    }

    ~ThreadSafeQueue()
    {
        while (pop());
    }

    void push(const T& value)
    {
        std::unique_ptr<Node> new_node(new Node);
        new_node->value = value;
        new_node->next = nullptr;

        Node* tail = tail_.load(std::memory_order_relaxed);
        tail->next = new_node.release();

        tail_.store(new_node.get(), std::memory_order_release);
    }

    std::unique_ptr<T> pop()
    {
        Node* head = head_.load(std::memory_order_relaxed);
        Node* next = head->next;
        if (!next) {
            return std::unique_ptr<T>();
        }
        std::unique_ptr<T> result(new T(next->value));
        head_.store(next, std::memory_order_release);
        delete head;
        return result;
    }

private:
    struct Node {
        T value;
        Node* next = nullptr;
    };
    std::atomic<Node*> head_;
    std::atomic<Node*> tail_;
};

struct GameInstanceInfo
{
    std::string GameName;
    uint32_t ID;
    uint16_t PlayerNum;
};

class GameInstanceBase
{
    
protected:
    std::atomic<bool> stopped = false;
    std::vector<std::shared_ptr<ClientHandle>> clHandle;
    GameInstanceInfo GameInfo;
    ThreadSafeQueue<std::shared_ptr<Send_Message>> sQueue;
    ThreadSafeQueue<std::shared_ptr<Recv_Message>> rQueue;

    virtual void start();
    virtual void stop();
    bool is_stopped();
    virtual ~GameInstanceBase(){ }
};