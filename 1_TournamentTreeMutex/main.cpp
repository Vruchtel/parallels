#include "tournamentTree.h"
#include <thread>

TreeMutex treeMutex(5);

void threadFunction(unsigned threadId)
{
	treeMutex.Lock(threadId);
	std::cout << "Critical section! Thread " << threadId << " is working" << std::endl;
	std::cout << std::endl;
	std::cout << "Critical section finish! Thread " << threadId << " finish" << std::endl;
	treeMutex.Unlock(threadId);
}

int main()
{
	std::thread thread0(threadFunction, 0);
	std::thread thread1(threadFunction, 1);
	std::thread thread2(threadFunction, 2);
	std::thread thread3(threadFunction, 3);
	std::thread thread4(threadFunction, 4);
	//std::thread thread5(threadFunction, 5);
	//std::thread thread6(threadFunction, 6);
	//std::thread thread7(threadFunction, 7);

	if (thread0.joinable())
		thread0.join();
	if (thread1.joinable())
		thread1.join();
	if (thread2.joinable())
		thread2.join();
	if (thread3.joinable())
		thread3.join();
	if (thread4.joinable())
		thread4.join();
	/*if (thread5.joinable())
		thread5.join();
	if (thread6.joinable())
		thread6.join();
	if (thread7.joinable())
		thread7.join();*/

	//TreeMutex mtx(11);
	//mtx.lockInTree(10);
	
	//std::cout << "critic section" << std::endl;
	//std::cout << std::endl;
	//mtx.unlockInTree(10);

	system("pause");
	return 0;
}