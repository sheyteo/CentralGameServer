#pragma once
#include "pch.h"

class TaskExecutor
{
	static std::unique_ptr<asio::thread_pool> thread_pool;
	static bool running;
public:
	static void start(uint16_t thread_num)
	{
		//thread_pool->join();
		if (running)
		{
			return;
		}
		thread_pool.reset(new asio::thread_pool(thread_num));
		running = true;
	}

	template<class T, class... Args, class... Fargs>
	static void addTask(std::_Binder<std::_Unforced, std::function<T(Fargs...)>, Args...> bnd)
	{
		asio::post(*thread_pool, bnd);
	}

	template<class T, class... Args, class... Fargs>
	static void addTask(std::_Binder<std::_Unforced, std::function<T(Fargs...)>&, Args...> bnd)
	{
		asio::post(*thread_pool, bnd);
	}

	template<class T, class... Args, class... Fargs>
	static std::future<T> addTaskFuture(std::_Binder<std::_Unforced, std::function<T(Fargs...)>, Args...> bnd)
	{
		std::packaged_task<T()> task(bnd);
		return asio::post(*thread_pool, std::move(task));
	}

	template<class T, class... Args, class... Fargs>
	static std::future<T> addTaskFuture(std::_Binder<std::_Unforced, std::function<T(Fargs...)>&, Args...> bnd)
	{
		std::packaged_task<T()> task(bnd);
		return asio::post(*thread_pool, std::move(task));
	}

	template<class T, class... Args, class... Fargs>
	static void addAsyncTask(std::_Binder<std::_Unforced, std::function<T(Fargs...)>, Args...> bnd, std::chrono::steady_clock::duration dur)
	{
		std::shared_ptr<asio::steady_timer> stimer = std::make_shared<asio::steady_timer>(*thread_pool, dur);
		stimer->async_wait([stimer, bnd](const asio::error_code&) {bnd();});
	}

	template<class T, class... Args, class... Fargs>
	static void addAsyncTask(std::_Binder<std::_Unforced, std::function<T(Fargs...)&>, Args...> bnd, std::chrono::steady_clock::duration dur)
	{
		std::shared_ptr<asio::steady_timer> stimer = std::make_shared<asio::steady_timer>(*thread_pool, dur);
		stimer->async_wait([stimer, bnd](const asio::error_code&) {bnd(); });
	}

	template<class T, class... Args, class... Fargs>
	static std::future<T> addAsyncTaskFuture(std::_Binder<std::_Unforced, std::function<T(Fargs...)>, Args...> bnd, std::chrono::steady_clock::duration dur)
	{
		std::shared_ptr<asio::steady_timer> stimer = std::make_shared<asio::steady_timer>(*thread_pool, dur);

		std::shared_ptr<std::packaged_task<T()>> task(new std::packaged_task<T()>(bnd));
		std::future<T> future = task->get_future();
		auto temp = [task,stimer](const asio::error_code&) {
			(*task)();
		};
		stimer->async_wait(temp);
		return future;
	}

	template<class T, class... Args, class... Fargs>
	static std::future<T> addAsyncTaskFuture(std::_Binder<std::_Unforced, std::function<T(Fargs...)&>, Args...> bnd, std::chrono::steady_clock::duration dur)
	{
		std::shared_ptr<asio::steady_timer> stimer = std::make_shared<asio::steady_timer>(*thread_pool, dur);

		std::shared_ptr<std::packaged_task<T()>> task(new std::packaged_task<T()>(bnd));
		std::future<T> future = task->get_future();
		auto temp = [task, stimer](const asio::error_code&) {
			(*task)();
		};
		stimer->async_wait(temp);
		return future;
	}
};