#pragma once

#include <vector>
#include <atomic>
#include <cassert>

template <class T>
class SpScRingBuffer
{
public:
	// Конструктор, принимающий размер циклического буфера
	SpScRingBuffer(size_t capacity);
	~SpScRingBuffer() = default;

	// Записывает элемент elem в хвост очереди и возвращает true, если в буфере есть место для очередного элемента, в случае переполнения
	// возвращает false
	bool Enqueue(const T& elem);

	// Если очередь не пуста, то извлечь элемент из головы очереди и вернуть true, в противном случае вернуть false
	bool Dequeue(T& elem);

private:
	size_t _capacity; // Вместимость очереди, указывается в конструкторе

	char padd0[128]; // ...для попадания полей объекта класса в разные кэш-линии

	std::atomic<size_t> _head; // Индекс головы очереди, указывает на элемент, который будет извлечён при следующем вызове Dequeue

	char padd1[128];

	std::atomic<size_t> _tail; // Индекс свободного слота за последним элементом в очереди, куда будет записан очередной элемент при следующем
							   // вызове Enqueue

	struct _node
	{
		_node(const T& nodeData) : _nodeData(nodeData) {}
		T _nodeData;
		char padd[128]; // насильно увеличиваем размер структуры для попадания различных элементов в разные кэш-линии
	};

	std::vector<_node> _data; // Вектор данных, хранящихся в буфере
};

template<class T>
inline SpScRingBuffer<T>::SpScRingBuffer(size_t capacity) : _capacity(capacity + 1), _data(capacity + 1, _node(-1)), _head(0), _tail(0)
{
	assert(capacity > 0);
}

template<class T>
inline bool SpScRingBuffer<T>::Enqueue(const T & elem)
{
	size_t currHead = _head.load(std::memory_order_acquire);
	size_t currTail = _tail.load(std::memory_order_relaxed);

	// Проверим на переполнение
	if (currHead == (currTail + 1) % _capacity)
		return false;

	_data[currTail] = _node(elem);
	currTail = (currTail + 1) % _capacity;

	_tail.store(currTail, std::memory_order_release);

	return true;
}

template<class T>
inline bool SpScRingBuffer<T>::Dequeue(T & elem)
{
	size_t currHead = _head.load(std::memory_order_relaxed);
	size_t currTail = _tail.load(std::memory_order_acquire);

	// Проверка буфера на пустоту
	if (currHead == currTail)
		return false;

	elem = _data[currHead]._nodeData;

	if (currHead > 0)
		currHead--;
	else
		currHead = _capacity - 1;

	_head.store(currHead, std::memory_order_release);

	return true;
}
