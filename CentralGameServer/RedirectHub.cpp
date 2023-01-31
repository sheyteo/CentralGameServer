#include "RedirectHub.h"

std::unordered_map < uint16_t, std::unordered_map< uint16_t, RedirectFunction>> RedirectHub::funcHandle;
std::unordered_map<uint64_t, std::shared_ptr<RedirectMemoryBase>> RedirectHub::memory;

uint64_t RedirectHub::memoryIdCounter = 0;