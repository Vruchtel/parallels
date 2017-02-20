#include <iostream>
#include <thread>
#include <string>

#include "ThreadSafeQueue.hpp"

ThreadSafeQueue<int> threadSafeQueue(10);
std::mutex printingMutex;
int TASK_NUM = 11;

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

// Потокобезопасная функция печати в стандартный поток вывода
void SafetyPrintString(const std::string &str)
{
	std::lock_guard<std::mutex> lock(printingMutex);
	std::cout << std::endl;
	std::cout << str << std::endl;
}

void SafetyPrintResult(const int &num, const bool &b)
{
	std::lock_guard<std::mutex> lock(printingMutex);
	std::cout << std::endl;
	if (b == true)
		std::cout << num << " - is prime" << std::endl;
	else
		std::cout << num << " - is not prime" << std::endl;
}

void producerFunction()
{
	int taskId = 0;
	while (taskId < TASK_NUM)
	{
		SafetyPrintString("Producer working");
		threadSafeQueue.Enqueue(rand());

		++taskId;
	}
	
	SafetyPrintString("Producer is going to shutdown consumers");
	for (int i = 0; i < 5; i++)
		threadSafeQueue.Enqueue(-1);
}

// Если поток извлекает из очереди -1 -> завершается
void consumerFunction(unsigned consumerId)
{
	int exploreNum;

	while (true)
	{
		threadSafeQueue.Pop(exploreNum);

		if (exploreNum == -1)
		{
			SafetyPrintString("Consumer is going to shutdown");

			//shutdown
			std::cout << "Shutdown" << std::endl;
			break;
		}
		else
		{
			SafetyPrintResult(exploreNum, PrimeNumber(exploreNum));
		}
	}

}

int main()
{
	srand(time(0));

	std::cout << "START TESTING" << std::endl;

	std::thread producer(producerFunction);

	std::thread consumer0(consumerFunction, 0);
	std::thread consumer1(consumerFunction, 1);
	std::thread consumer2(consumerFunction, 2);
	std::thread consumer3(consumerFunction, 3);
	std::thread consumer4(consumerFunction, 4);

	if(producer.joinable())
		producer.join();

	if(consumer0.joinable())
		consumer0.join();

	if (consumer1.joinable())
		consumer1.join();

	if (consumer2.joinable())
		consumer2.join();

	if (consumer3.joinable())
		consumer3.join();

	if (consumer4.joinable())
		consumer4.join();

	std::cout << "FINISH TESTING" << std::endl;

	system("pause");

	return 0;
}