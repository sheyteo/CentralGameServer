#include "TaskExecutor.h"

std::unique_ptr<asio::thread_pool> TaskExecutor::thread_pool;
bool TaskExecutor::running = false;