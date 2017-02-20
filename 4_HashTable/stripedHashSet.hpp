#pragma once

#include <mutex>
#include <vector>
#include <forward_list>
#include <shared_mutex>
#include <cassert>

#define DEFAULT_GROWTH_FACTOR 2
#define DEFAULT_MAX_LOAD_FACTOR 0.5

/** Хэш-таблица с цепочками, использующая технику lock striping для конкурентной работы нескольких операций, поддерживает автоматическое
** расширение по мере заполнения
**/
template <typename T, class  H = std::hash<T> >
class StripedHashSet
{
public:
	StripedHashSet(unsigned numStripes);
	StripedHashSet(unsigned numStripes, unsigned growthFactor);
	StripedHashSet(unsigned numStripes, unsigned growthFactor, double maxLoadFactor);

	// Конструктор по умолчанию, конструктор копирования и оператор присваивания запрещены
	StripedHashSet() = delete;
	StripedHashSet(const StripedHashSet &otherSHS) = delete;
	StripedHashSet &operator=(const StripedHashSet &otherSHS) = delete;

	// Деструктор дефолтный
	~StripedHashSet() = default;


	/*Основные операции*/

	// Добавляет элемент, если он ещё не содержится в множестве (если он уже содержится в множестве, ничего не происходит),
	// после вызова этой функции elem точно принадлежит множеству
	void Add(const T & elem);

	// Удаляет элемент, если он принадлежит множеству (если элемент множеству не принадлежит,  ничего не просиходит),
	// после вызова этой функции elem точно не принадлежит множеству
	void Remove(const T & elem);

	// Возвращает true, если elem принадлежит множеству, и false - иначе
	bool Contains(const T & elem);

private:
	std::vector<std::shared_timed_mutex> _mutexes;

	std::vector<std::forward_list<T> > _baskets;  // Собственно хранилище элементов таблицы

	unsigned _numStripes;  // Число мьютексов таблицы (соответственно - число страйпов), если == 1, то работать с таблицей может
						   // только один поток

	unsigned _currentNumBaskets;  // Текущее число корзин в таблице, изначально == _numStripes

	unsigned _growthFactor;  // Коэффициент роста таблицы (во сколько раз увеличивается таблица при расширении),
							 // опциональный параметр конструктора, имеет значение по умолчанию = 2
	
	double _currentLoadFactor;  // Отношение числа элементов в таблице к числу корзин

	double _maxLoadFactor;  // Порог для _currentLoadFactor, при превышении которого таблица автоматически расширяется,
	                        // опциональный параметр конструктора, значение по умолчанию 0.5

	unsigned _currentNumElements;  // Текущее число элементов в таблице
 
	std::mutex _lockerClassData;  // Мьютекс на изменение данных внутри класса

	/*private methods*/

	// Возвращает хэш элемента elem
	unsigned getHash(const T & elem) const;

	// Возвращает номер корзинки, которой соответствует элемент
	unsigned getBasketId(const T & elem) const;

	// Возвращает номер страйпа (мьютекса), которой соответствует корзинка
	unsigned getMutexId(const T & elem) const;

	// Метод расширения таблицы
	void expandSet();

	
};

template<typename T, class H>
inline StripedHashSet<T, H>::StripedHashSet(unsigned numStripes) : StripedHashSet(numStripes, DEFAULT_GROWTH_FACTOR)
{
}

template<typename T, class H>
inline StripedHashSet<T, H>::StripedHashSet(unsigned numStripes, unsigned growthFactor) : StripedHashSet(numStripes,
	                                                                                                     growthFactor,
	                                                                                                     DEFAULT_MAX_LOAD_FACTOR)
{
}

template<typename T, class H>
inline StripedHashSet<T, H>::StripedHashSet(unsigned numStripes, unsigned growthFactor, double maxLoadFactor) : 
	_mutexes(numStripes),
	_baskets(numStripes),
	_numStripes(numStripes), 
	_currentNumBaskets(numStripes), 
	_growthFactor(growthFactor), 
	_maxLoadFactor(maxLoadFactor),
	_currentNumElements(0)
{
	assert(numStripes > 0);
	assert(growthFactor > 1);
	assert(maxLoadFactor <= 1);

	_currentLoadFactor = static_cast<double>(_currentNumElements) / _currentNumBaskets; //!!!!!!!!!!!
}

template<typename T, class H>
inline void StripedHashSet<T, H>::Add(const T & elem)
{
	// Если такой элемент уже содержится в множестве, ничего делать не надо
	if (Contains(elem))
		return;

	unsigned suitableStripe = getMutexId(elem);
	unsigned suitableBasketId = getBasketId(elem);

	// Блокируем подходящий мьютекс на запись (write lock)
	std::unique_lock<std::shared_timed_mutex> stripeLock(_mutexes[suitableStripe]);

	// Добавляем элемент в заблокированном страйпе
	_baskets[suitableBasketId].push_front(elem);
	stripeLock.unlock();

	// Увеличиваем количество элементов в табличке
	std::unique_lock<std::mutex> increasedElems(_lockerClassData);
	_currentNumElements++;
	_currentLoadFactor = static_cast<double>(_currentNumElements) / _currentNumBaskets;
	increasedElems.unlock();

	// Табличку надо расширять, если currentLoadFactor превысил maxLoadFactor
	if (_currentLoadFactor > _maxLoadFactor)
		expandSet();
}

template<typename T, class H>
inline void StripedHashSet<T, H>::Remove(const T & elem)
{
	// Если такого элемента в табличке нет, ничего делать не надо
	if (!Contains(elem))
		return;

	unsigned suitableStripe = getMutexId(elem);
	unsigned suitableBasketId = getBasketId(elem);

	// Блокируем подходящий мьютекс на запись (write lock)
	std::unique_lock<std::shared_timed_mutex> stripeLock(_mutexes[suitableStripe]);

	// Удаляем элемент из таблицы
	_baskets[suitableBasketId].remove(elem);

	// Говорим, что количество элементов в таблице стало меньше
	std::unique_lock<std::mutex> decreasedElems(_lockerClassData);
	_currentNumElements--;
	_currentLoadFactor = static_cast<double>(_currentNumElements) / _currentNumBaskets;
}

template<typename T, class H>
inline bool StripedHashSet<T, H>::Contains(const T & elem)
{
	// Определяем, какому страйпу (мьютексу) соответствует элемент
	unsigned suitableStripe = getMutexId(elem);

	// Определяем, какой корзинке соответствует элемент
	unsigned suitableBasketId = getBasketId(elem);

	// Заблокируем этот мьютекс (read lock)
	std::shared_lock<std::shared_timed_mutex> stripeLock(_mutexes[suitableStripe]);

	// Завершаем работу метода, как только элемент найден
	for (auto it = _baskets[suitableBasketId].begin(); it != _baskets[suitableBasketId].end(); it++)
	{
		if (*it == elem)
			return true;
	}

	return false;
}


/**private methods**/

template<typename T, class H>
inline unsigned StripedHashSet<T, H>::getHash(const T & elem) const
{
	return H()(elem);
}

template<typename T, class H>
inline unsigned StripedHashSet<T, H>::getBasketId(const T & elem) const
{
	return getHash(elem) % _currentNumBaskets;
}

template<typename T, class H>
inline unsigned StripedHashSet<T, H>::getMutexId(const T & elem) const
{
	return getBasketId(elem) % _numStripes;
}

template<typename T, class H>
inline void StripedHashSet<T, H>::expandSet()
{
	unsigned myCurrentNumBaskets = _currentNumBaskets;

	// Захватываем первый мьютекс (на write lock)
	std::unique_lock<std::shared_timed_mutex> firstLock(_mutexes[0]);

	// Если число корзинок к этому моменту успело увеличиться, значит, кто-то расширил таблицу за нас, и надо завершить работу
	if (myCurrentNumBaskets < _currentNumBaskets)
		return;

	// Если мы оказались здесь, значит, расширяться придётся нам самим, для этого сначала захватываем все мьютексы (write lock)
	std::vector<std::unique_lock<std::shared_timed_mutex> > locks;
	for (unsigned i = 1; i < _mutexes.size(); i++)
	{
		locks.emplace_back(_mutexes[i]);
	}

	// Определяем, каким должен быть новый размер таблички
	unsigned newSize = _currentNumBaskets * _growthFactor;
	_currentNumBaskets = newSize; // Т.к. метод нахождения подходящей корзинки использует _currentNumBaskets

	std::vector<std::forward_list<T> > newBaskets(newSize);
	unsigned suitableBasket;
	for (auto it1 = _baskets.begin(); it1 != _baskets.end(); it1++)
	{
		for (auto it2 = it1->begin(); it2 != it1->end(); it2++)
		{
			suitableBasket = getBasketId(*it2);
			newBaskets[suitableBasket].push_front(*it2);
		}
	}

	_baskets = newBaskets;

}
