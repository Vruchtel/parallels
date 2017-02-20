#include <iostream>
#include <thread>
#include "stripedHashSet.hpp"

StripedHashSet<int> shs(4);

void threadFunctionAdd(int elem)
{
	shs.Add(elem);
}

void threadFunctionContains(int elem)
{
	std::cout << shs.Contains(elem) << std::endl;
}

void threadFunctionErase(int elem)
{
	shs.Remove(elem);
}

int main()
{
	srand(time(0));

	std::vector<int> elems(10);
	for (unsigned i = 0; i < elems.size(); i++)
		elems[i] = rand();

	std::vector<std::thread> workingThreads;

	for (unsigned i = 0; i < 10; i++)
		workingThreads.push_back(std::thread(threadFunctionAdd, elems[i]));

	for (unsigned i = 0; i < 10; i++)
		workingThreads.push_back(std::thread(threadFunctionContains, elems[i]));

	for (unsigned i = 0; i < 10; i++)
		workingThreads.push_back(std::thread(threadFunctionErase, elems[i]));

	for (unsigned i = 0; i < 10; i++)
		workingThreads.push_back(std::thread(threadFunctionContains, elems[i]));

	for (unsigned i = 0; i < workingThreads.size(); i++)
	{
		if (workingThreads[i].joinable())
			workingThreads[i].join();
	}

	system("pause");

	return 0;
}