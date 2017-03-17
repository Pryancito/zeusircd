#include "include.h"
#include <condition_variable>

using namespace std;

void Semaforo::notify()
{
	mtx.unlock();
	cv.notify_one();
}

void Semaforo::wait()
{
	unique_lock<mutex> lck(mtx);
	while(cola.size() == 0)
	{
	  cv.wait(lck);
	}
}
