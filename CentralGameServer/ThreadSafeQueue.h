#pragma once

#include "pch.h"

template<class T>
class ThreadSafeQueue
{
public:
    void push(const std::shared_ptr<T>& value) noexcept
    {
        std::lock_guard lg(mtx);
        list.push_front(value);
    }

    std::shared_ptr<T> pop() noexcept
    {
        std::lock_guard lg(mtx);
        if (!list.empty())
        {
            auto temp = list.back();
            list.pop_back();
            return temp;
        }
        else
        {
            return nullptr;
        }

    }

    size_t getSize() const noexcept
    {
        std::lock_guard lg(mtx);
        return list.size();
    }

private:
    std::list<std::shared_ptr<T>> list;
    std::mutex mtx;
};