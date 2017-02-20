#include <iostream>
#include <thread>
#include <ctime>
#include <vector>
#include <future>

#include "ThreadPool.hpp"

// Функция, проверяющая число на простоту
bool PrimeNumber(int num)
{
	// Разбор по всем возможным делителям num
	for (int divider = 2; divider <= sqrt(num); ++divider)
	{
		if (num % divider == 0)
			return false;
	}

	return true;
}

int main()
{
	std::cout << "START" << std::endl;
	std::srand(std::time(0));

	ThreadPool<bool> tp;

	std::vector<int> tasks;
	std::vector<std::shared_ptr<std::future<bool> > > results;

	for (unsigned i = 0; i < 20; i++)
	{
		tasks.push_back(std::rand());
		results.push_back(tp.Submit(std::bind(PrimeNumber, tasks[i])));
	}

	for (unsigned i = 0; i < 20; i++)
	{
		std::cout << "number " << tasks[i] << " ";
		if (results[i]->get())
			std::cout << "is prime";
		else
			std::cout << "is not prime";
		std::cout << std::endl;
	}

	tp.Shutdown();

	std::cout << "FINISH" << std::endl;

	system("pause");

	return 0;
}