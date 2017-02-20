#include <thread>
#include <cassert>
#include "petersonMutex.h"

PetersonMutex::PetersonMutex()
{
	_want[0].store(false);
	_want[1].store(false);

	_victim.store(0);
}

void PetersonMutex::lock(int threadId)
{
	assert(threadId == 0 || threadId == 1);

	_want[threadId].store(true);
	_victim.store(threadId);

	while (_want[1 - threadId].load() && _victim.load() == threadId)
	{
		std::this_thread::yield();
	}
}

void PetersonMutex::unlock(int threadId)
{
	assert(threadId == 0 || threadId == 1);

	_want[threadId].store(false);
}
