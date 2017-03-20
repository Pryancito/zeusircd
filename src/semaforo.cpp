#include "include.h"
#include <condition_variable>

using namespace std;

void Semaforo::notify()
{
	unique_lock<mutex> lck(mtx);
	cv.notify_one();
	lock = false;
	lck.unlock();
}

void Semaforo::wait()
{
	unique_lock<mutex> lck(mtx);
	while(cola.size() == 0 && lock == true)
	{
	  cv.wait(lck);
	}
	lck.unlock();
}

void Semaforo::close()
{
	lock = true;
}

void Semaforo::open()
{
	lock = false;
}
