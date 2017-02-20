#include "spsc_ring_buffer.hpp"
#include <future>
#include <iostream>
#include <thread>

#define L 1024
#define N 10000000

SpScRingBuffer<int> buff(L);

// Поток-продьюсер последовательно добавляет в очередь N чисел от 1 до N, заодно считает их сумму, после чего возвращает значение
long long producerWorkLoop()
{
	long long sum = 0;

	for (unsigned i = 0; i < N; i++)
	{
		while (!buff.Enqueue(i))
			std::this_thread::yield();
		
		sum += i;
	}

	return sum;
}

// Поток-консьюмер получает все числа и тоже складывает их
long long consumerWorkLoop()
{
	long long sum = 0;

	int currElem;
	for (unsigned i = 0; i < N; i++)
	{
		while (!buff.Dequeue(currElem))
			std::this_thread::yield();

		sum += currElem;
	}

	return sum;
}


int main()
{
	std::future<long long> producerRes = async(std::launch::async, producerWorkLoop);
	std::future<long long> consumerRes = async(std::launch::async, consumerWorkLoop);
	if (producerRes.get() - consumerRes.get() == 0)
	{
		std::cout << "correct" << std::endl;
	}
	else
	{
		std::cout << "wrong answer" << std::endl;
	}

	return 0;
}