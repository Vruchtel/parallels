#pragma once

#include <thread>
#include <vector>
#include <future>
#include <memory>
#include "ThreadSafeQueue.hpp"


template <class T>
class ThreadPool
{
public:
	// Конструктор по умолчанию, число воркеров подбирается программой
	ThreadPool();

	// Конструктор с указанием количества потоков-воркеров
	ThreadPool(std::size_t numThreads);

	// Конструктор копирования и оператор присваивания запрещены
	ThreadPool(const ThreadPool<T>& threadPool) = delete;
	ThreadPool<T>& operator=(const ThreadPool<T>& threadPool) = delete; 

	~ThreadPool(); // Вызывает Shutdown

	// Запуск задачи на исполнение
	std::shared_ptr<std::future<T> > Submit(std::function<T()> taskFunction); // done

	// Метод остановки всех потоков-воркеров
	void Shutdown();

private:
	std::size_t _numThreads; // Количество потоков-воркеров

	std::vector<std::thread> _workers;// Вектор воркеров


	// "Результат", возвращаемый любой задачей, передаваемой потокам-воркерам на обработку
	class TaskResult
	{
	public:
		// В случае, когда задача - "пилюля с ядом", оба параметра конструктора - nullptr
		TaskResult(std::shared_ptr<std::promise<T> > pptr, std::function<T()> tsk) : promise_ptr(pptr), task(tsk) { }

		// По умолчанию генерируется "пилюля с ядом"
		TaskResult() :promise_ptr(nullptr), task(nullptr) { }

		std::shared_ptr<std::promise<T> > promise_ptr; // Указатель на объект, который может получить некоторый результат вычислений от
													   // соответствующего future, и передать его пользователю

		std::function<T()> task; // Задача, которую необходимо выполнить
	};


	ThreadSafeQueue<TaskResult> _threadSafeQueue; // Потокобезопасная очередь задач для воркеров,
												  // состоит из значений, которые возвращают задачи

	// Метод, подбирающий число потоков-воркеров по умолчанию
	std::size_t defaultNumWorkers();

	// Метод, отправляющий в очередь задач "пилюли с ядом", количество пилюль == количеству воркеров
	void sendShutdownTasks();

	// Завершение работы всех потоков
	void joinAllWorkers();
};

template<class T>
inline ThreadPool<T>::ThreadPool() : ThreadPool(defaultNumWorkers())
{ }

template<class T>
inline ThreadPool<T>::ThreadPool(std::size_t numThreads) : _numThreads(numThreads)
{
	auto workerFunction = [this]() -> void
	{
		// Получение первой задачи
		TaskResult newTask;
		_threadSafeQueue.Pop(newTask);

		// Пока задача - не яд, выполняем её...
		while (newTask.task != nullptr)
		{
			newTask.promise_ptr->set_value(newTask.task());

			// И достаём новую задачку из очереди
			_threadSafeQueue.Pop(newTask);
		}
	};

	// Запуск всех воркеров
	for (unsigned i = 0; i < _numThreads; ++i)
		_workers.push_back(std::thread(workerFunction));
}


template<class T>
inline ThreadPool<T>::~ThreadPool()
{
	Shutdown();
}

template<class T>
inline std::shared_ptr<std::future<T> > ThreadPool<T>::Submit(std::function<T()> taskFunction)
{
	// Создаём указатель на promise, который будет передан воркеру
	auto pointerToPromise = std::make_shared<std::promise<T> >(std::promise<T>());

	// Создаём указатель на future, который будет возвращён функцией
	auto pointerToFuture = std::make_shared<std::future<T> >(pointerToPromise->get_future());

	// Отправляем задачу на исполнение
	_threadSafeQueue.Enqueue(TaskResult(pointerToPromise, taskFunction));

	return pointerToFuture;
}


template<class T>
inline void ThreadPool<T>::Shutdown()
{
	sendShutdownTasks();
	joinAllWorkers();
}


/*private methods*/

template<class T>
inline std::size_t ThreadPool<T>::defaultNumWorkers()
{
	std::size_t suppose = std::thread::hardware_concurrency(); // может вернуть 0

	if (suppose == 0)
		suppose = 2; // количество ядер процессора

	return suppose;
}

template<class T>
inline void ThreadPool<T>::sendShutdownTasks()
{
	// Генерируем "пилюлю"
	std::shared_ptr<std::promise<T> > nullPromise = nullptr;
	std::function<T()> nullFunction = nullptr;

	for (std::size_t i = 0; i < _numThreads; i++)
		_threadSafeQueue.Enqueue(TaskResult(nullPromise, nullFunction));
}

template<class T>
inline void ThreadPool<T>::joinAllWorkers()
{
	for (unsigned i = 0; i < _workers.size(); i++)
		if (_workers[i].joinable())
			_workers[i].join();
}
