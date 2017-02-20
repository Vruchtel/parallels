#include <cassert>

#include "Barrier.h"

Barrier::Barrier(std::size_t numThreads) : _numThreads(numThreads), _wantCounter(0), _eraCounter(0)
{
	assert(numThreads > 0);
}

void Barrier::Enter()
{
	std::unique_lock<std::mutex> gateLock(_barrierGate);
	_wantCounter++;

	if (_wantCounter == _numThreads)
	{
		_eraCounter++;
		_wantCounter = 0;
		_waitAll.notify_all();
	}
	else
	{
		int currentEra = _eraCounter;
		_waitAll.wait(gateLock, [this, currentEra]() {return currentEra < _eraCounter;});
	}
}
