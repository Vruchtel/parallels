#pragma once

#include <vector>
#include <math.h>
#include <iostream>

#include "petersonMutex.h"

class TreeMutex
{
public:
	// Конструктор по количеству конкурирующих потоков
	TreeMutex(unsigned numThreads);

	// Конструктор копирования - запрещён
	TreeMutex(const TreeMutex &tmx) = delete;

	// Оператор присваивания - запрещён
	TreeMutex &operator=(const TreeMutex &tmx) = delete;

	// Войти в критическую секцию
	void Lock(unsigned threadId);

	// Освободить критическую секцию
	void Unlock(unsigned threadId);

private:
	unsigned _numThreads;
	std::vector<PetersonMutex> _locks;// Вектор мютексов, из которых и строится tournament tree

	// Идентификатор потока в мютексе Петерсона (для каждого из потоков, для каждого из _lock, за который поток соревнуется)
	std::vector<std::vector<unsigned> > _threadsPetersId;

	unsigned _levelsNumber;// Количество уровней в турнирном дереве

	// Метод подсчёта количества уровней в турнирном дереве
	void countLevelsNumber();

	// Метод подсчёта индекса в мютексе Петерсона (0 или 1) для текущей ноды в турнирном дереве
	unsigned findPetersonIdToNode(unsigned currentNode, unsigned level, unsigned threadId);// 2, 3 арг-ты - для обработки краев. сл.

	// Метод, вычисляющий количество элементов (lock'ов) на последнем уровне в бинарном дереве _locks
	unsigned numberOfNodesInDownLevelLocks();

	// Метод, вычисляющий номер первого lock'а на последнем уровне в дереве _locks
	unsigned firstDownLockId(unsigned numberOfNodesInDownLevelLocks); // Принимает количество lock'ов на последнем уровне

	// Метод, вычисляющий номер lock'а, соответствующего выбранному потоку в бинарном дереве _locks
	unsigned lockToThreadId(unsigned threadId, unsigned &level);// Второй аргумент - для обработки краевого случая
};