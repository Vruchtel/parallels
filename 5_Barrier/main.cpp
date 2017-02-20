#include <iostream>
#include <vector>
#include "Barrier.h"

std::mutex printingMutex;
Barrier br(10);

void SafetyPrintResult(const std::size_t &toPrint)
{
	std::lock_guard<std::mutex> lock(printingMutex);
	std::cout << toPrint << std::endl;
}

// Функция потока
void ThreadWork(std::size_t threadId)
{
	for (unsigned i = 0; i < 5; i++)
	{
		for (unsigned j = 0; j < 1000000; j++) {}
		br.Enter();
		SafetyPrintResult(threadId);
	}	
}


int main()
{
	std::vector<std::thread> workingThreads;
	
	for (unsigned i = 0; i < 10; i++)
	{
		workingThreads.push_back(std::thread(ThreadWork, i));
	}

	for (unsigned i = 0; i < workingThreads.size(); i++)
	{
		if (workingThreads[i].joinable())
			workingThreads[i].join();
	}

	system("pause");

	return 0;
}