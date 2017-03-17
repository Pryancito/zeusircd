#include "include.h"
#include <condition_variable>

using namespace std;

void Semaforo::notify()
{
	mtx.unlock();
	++count;
	cv.notify_one();
}

void Semaforo::wait()
{
	unique_lock<mutex> lck(mtx);
	while(count == 0)
	{
	  cv.wait(lck);
	}
	--count;
}
