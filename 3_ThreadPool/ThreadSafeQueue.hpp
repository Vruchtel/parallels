/**Модифицированная версия ThreadSafeQueue из задачи producers-consumers - очередь неограниченного размера**/

#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>

template <class T>
class  ThreadSafeQueue
{
public:
	// Конструктор по умолчанию - создаётся очередь нефиксированного размера
	 ThreadSafeQueue();

	// Конструктор копирования - запрещён
	 ThreadSafeQueue(ThreadSafeQueue<T> &tsq) = delete;
	 // Оператор присваивания - аналогично запрещён
	 ThreadSafeQueue &operator=(const ThreadSafeQueue &tsq) = delete;

	~ ThreadSafeQueue() = default;

	// Добавление элемента в очередь
	void Enqueue(const T &item);

	// Удаление элемента из очереди, блокируется, если очередь пуста
	void Pop(T &item);

private:
	std::queue<T> _taskQueue;// Собственно очередь, в которой хранятся задачи
	std::mutex _queueMutex;// Мютекс, захватив который, поток получает доступ к очереди
	std::condition_variable cv_pop;// Условная переменная, на возможность извлечения из очереди
};


 template<class T>
 ThreadSafeQueue<T>::ThreadSafeQueue() { }

 template<class T>
 inline void ThreadSafeQueue<T>::Enqueue(const T &item)
 {
	 std::unique_lock<std::mutex> lock(_queueMutex);

	_taskQueue.push(item);

	 cv_pop.notify_one();
 }

 template<class T>
 inline void ThreadSafeQueue<T>::Pop(T & item)
 {
	 std::unique_lock<std::mutex> lock(_queueMutex);
	 
	 cv_pop.wait(lock, [this]() {return !_taskQueue.empty(); });

	 item = _taskQueue.front();
	 _taskQueue.pop();
 }
