#pragma once

#include <atomic>
#include <array>

class PetersonMutex
{
public:
	PetersonMutex();

	// Конструктор копирования - запрещён
	PetersonMutex(const PetersonMutex &othermtx) = delete;

	~PetersonMutex() = default;

	// Заполучить мютекс
	void lock(int threadId);

	// Отдать мютекс
	void unlock(int threadId);

private:
	// Массив, в котором для каждого из двух потоков ("0" и "1") хранится true - "хочу заполучить мютекс", или false - иначе
	std::array<std::atomic<bool>, 2> _want;

	// Идентификатор потока, который сейчас "хочет", но не может попасть в критическую секцию
	std::atomic<int> _victim;
};