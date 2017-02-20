#include <cassert>
#include "tournamentTree.h"

TreeMutex::TreeMutex(unsigned numThreads) : _numThreads(numThreads), _locks(_numThreads - 1)
{
	//_locks.resize(_numThreads - 1);// Количество мютексов должно быть на 1 меньше количества работающих потоков

	countLevelsNumber();

	_threadsPetersId.resize(_numThreads);// Идентификаторы для каждого из потоков...
	for (unsigned i = 0; i < _threadsPetersId.size(); i++)
	{
		_threadsPetersId[i].resize(_levelsNumber);// У каждого потока для каждого из уровней свой идентификатор в мютексе Петерсона
	}
}

void TreeMutex::Lock(unsigned threadId)
{
	assert(threadId < _numThreads);

	unsigned currentNode = threadId + (_numThreads - 1);// Фиктивный лист в дереве, из которого стартует threadId

	//std::cout << "currentNode: " << currentNode << std::endl;// Отладочный вывод

	for (unsigned level = 0; level < _levelsNumber; level++)// Обход дерева, начиная со стартового листа и до корня
	{
		_threadsPetersId[threadId][level] = findPetersonIdToNode(currentNode, level, threadId);
		//std::cout << "PetersId: " << _threadsPetersId[threadId][level] << std::endl;// Отладочный вывод
		
		// Переходим на уровень выше: новая текущая нода - родитель предыдущей текущей ноды
		//Если мы уже не в листе, то родитель текущей ноды - это её номер, делённый целочисленно на 2, при условии что номер текущей ноды
		//целочисленно не делится на 2, иначе нужно ещё вычесть 1 (из-за смещения индексов, т.к считаем от нуля (тут рисовать нужно) )
		if (level > 0)
		{
			if (currentNode % 2 == 0)
				currentNode = (currentNode / 2) - 1;
			else
				currentNode /= 2;
		}
		else // level == 0
		{
			// Вычисляем номер lock'а, соответствующего выбранному потоку в бинарном дереве _locks
			currentNode = lockToThreadId(threadId, level);
			
			// Один краевой случай - когда поток "крепится" к lock'у с последнего уровня, это вызывает пропуск уровня потоком
			// поэтому дублируем его
			if (level == 1)// В этом случае самый нижний мютекс на уровне 0 не захватывается, он пропускается т.к. является фиктивным
				_threadsPetersId[threadId][level] = _threadsPetersId[threadId][level-1];
		}
		//std::cout << std::endl;
		//std::cout << std::endl;
		//std::cout << "next node: " << currentNode << std::endl;
		//std::cout << "current level: " << level << std::endl;
		//std::cout << "PetersId: " << _threadsPetersId[threadId][level] << std::endl;// Отладочный вывод
		//break;

		_locks[currentNode].lock(_threadsPetersId[threadId][level]);
	}
}

void TreeMutex::Unlock(unsigned threadId)
{
	assert(threadId < _numThreads);

	unsigned currentNode = 0;
	//std::cout << "levels number: " << _levelsNumber << std::endl;

	for (int level = _levelsNumber - 1; level > -1; level--)
	{
		// ШАГ 1: Разблокировка мютекса, находящегося на текущем уровне уровне

			// Отладочный вывод
			//std::cout << std::endl;
			//std::cout << "UNLOCK" << std::endl;
			//std::cout << std::endl;
			//std::cout << "currentNode: " << currentNode << std::endl;
			//std::cout << "level: " << level << std::endl;
			//std::cout << "threadId: " << threadId << std::endl;
			//std::cout << "PetersId: " << _threadsPetersId[threadId][level] << std::endl;

		_locks[currentNode].unlock(_threadsPetersId[threadId][level]);

		// ШАГ 2: Спуск вниз по дереву

		// Отладочный вывод
		//std::cout << std::endl;
		//std::cout << "GO DOWN" << std::endl;
		//std::cout << std::endl;
		//std::cout << "currentNode: " << currentNode << std::endl;
		//std::cout << "level: " << level << std::endl;
		//std::cout << "threadId: " << threadId << std::endl;
		//std::cout << "PetersId: " << _threadsPetersId[threadId][level] << std::endl;

		if (level > 1)
		{
			currentNode = 2 * currentNode + _threadsPetersId[threadId][level] + 1;

			//std::cout << "new node: " << currentNode << std::endl;// Отладочный вывод
		}
		// Разбор особых случаев
		else
		{
			if (level == 1)
			{
				unsigned possibleNewCurrentNode = 2 * currentNode + _threadsPetersId[threadId][level] + 1;

				if (possibleNewCurrentNode < (_numThreads - 1))
					currentNode = possibleNewCurrentNode;
				else // Случай, когда новой возможной ноды не оказалось
				{
					level = -1;// Получается, что все локи, какие было можно, мы уже разблокировали
					//std::cout << "no more nodes" << std::endl;
				}

				//std::cout << "new node: " << currentNode << std::endl;// Отладочный вывод	
				
			}
			if (level == 0)
			{
				// Здесь тоже все локи, какие было можно, мы уже разблокировали, работа на этом заканчивается
				//std::cout << std::endl;
				//std::cout << "no more nodes" << std::endl;
			}
		}
	}
}


/**private methods**/

void TreeMutex::countLevelsNumber()
{
	double logFloat = log10(_numThreads) / log10(2);
	int logInt = log10(_numThreads) / log10(2);
	//std::cout << logFloat << std::endl;// Отладочный вывод
	//std::cout << logInt << std::endl;// Отладочный вывод
	if (logFloat == logInt)
		_levelsNumber = logInt;
	else
		_levelsNumber = logInt + 1;
	//std::cout << "levelsNumber: " << _levelsNumber << std::endl;// Отладочный вывод

}

unsigned TreeMutex::findPetersonIdToNode(unsigned currentNode, unsigned level, unsigned threadId)
{
	assert(threadId < _numThreads);

	unsigned petersonId;

	if (level > 0)
	{	
		if (currentNode % 2 == 1)
			petersonId = 0;
		else
			petersonId = 1;
	}
	else
	{
		// Сначала посчитаем, сколько должно быть lock'ов в полностью заполненном дереве
		unsigned fullLocks = 0;
		for (unsigned i = 0; i < _levelsNumber; i++)
		{
			fullLocks += pow(2, i);
		}
		//std::cout << "fullLocks: " << fullLocks << std::endl;// Отладочный вывод

		// Теперь посчитаем, сколько lock'ов не хватает до полного заполнения
		unsigned lackOfLocks = fullLocks - (_numThreads - 1);
		//std::cout << "lackOfLocks: " << lackOfLocks << std::endl;// Отладочный вывод

		// Если последний уровень в дереве lock'ов заполнен полностью, то petersonId равен остатку от деления ид потока на 2
		// Аналогично, когда последний уровень заполнен не полностью, но поток всё же относится к lock'ам последнего уровня
		if (lackOfLocks == 0 || (threadId / 2) < numberOfNodesInDownLevelLocks())
			petersonId = threadId % 2;
		// Иначе это самый крайний поток, и он "крепится" к предпоследнему уровню lock'ов
		else
			if (threadId % 2 == 0)
				petersonId = 1;
			else
				petersonId = 0;
	}

	return petersonId;
}

unsigned TreeMutex::numberOfNodesInDownLevelLocks()
{
	// Сначала посчитаем, сколько должно быть lock'ов в полностью заполненном дереве
	unsigned fullLocks = 0;
	for (unsigned i = 0; i < _levelsNumber; i++)
	{
		fullLocks += pow(2, i);
	}
	//std::cout << "fullLocks: " << fullLocks << std::endl;// Отладочный вывод

	// Теперь посчитаем, сколько lock'ов не хватает до полного заполнения
	unsigned lackOfLocks = fullLocks - (_numThreads - 1);
	//std::cout << "lackOfLocks: " << lackOfLocks << std::endl;// Отладочный вывод

	// И найдём количество lock'ов на последнем уровне
	unsigned downLevelLocks = pow(2, _levelsNumber - 1) - lackOfLocks;
	//std::cout << "downLevelLocks: " << downLevelLocks << std::endl;// Отладочный вывод

	return downLevelLocks;
}

unsigned TreeMutex::firstDownLockId(unsigned numberOfNodesInDownLevelLocks)
{
	return (_numThreads - 1) - numberOfNodesInDownLevelLocks;
}

unsigned TreeMutex::lockToThreadId(unsigned threadId, unsigned &level)
{
	assert(threadId < _numThreads);

	unsigned lockId;

	// Посчитаем число locks на последнем уровне в бинарном дереве _locks
	unsigned downLevelLocksNum = numberOfNodesInDownLevelLocks();

	// Посчитаем номер первого lock на последнем уровне в дереве _locks
	unsigned firstDownLockNum = firstDownLockId(downLevelLocksNum);
	//std::cout << "firstDownLockNum: " << firstDownLockNum << std::endl;// Отладочный вывод

	// Случай, когда поток "крепится" к lock'у с последнего уровня, а не предпоследнего
	if (threadId / 2 < downLevelLocksNum)
	{
		lockId = firstDownLockNum + (threadId / 2);
	}
	// Случай, когда поток крепится именно к lock'у с предпоследнего уровня
	else 
	{
		lockId = firstDownLockNum - 1 - ((_numThreads - 1 - threadId) / 2);
		++level;
	}

	return lockId;
}
