/*--------------------------------------------------------------------------
 Portable clock functions.
--------------------------------------------------------------------------*/
#ifndef eclock_h
#define eclock_h

//#include "dprint.h"
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

/*--------------------------------------------------------------------------
 Get current time, in clock ticks.
 Return type is int.
--------------------------------------------------------------------------*/
#define eclock_HERTZ 10000
#define eclock_TICKSPERUSEC 100

extern struct timeval eclock_timeval;
#define eclock() (gettimeofday(&eclock_timeval, NULL), \
	((int)((eclock_timeval.tv_sec * eclock_HERTZ) + (eclock_timeval.tv_usec / eclock_TICKSPERUSEC))))

/*--------------------------------------------------------------------------
 Get number of clock ticks per second, in hertz.  This evaluates to a 
 system call, so call it just once at the start of the program.
 Return type is int.
--------------------------------------------------------------------------*/
#define eclock_hertz() eclock_HERTZ

/*--------------------------------------------------------------------------
 Safe way to compare to eclock() values to see which is after the other.
 Assumes the largest interval to be compared is one half the maximum
 representable in a long.

 Returns true if time a is after time b.
--------------------------------------------------------------------------*/
//#define eclock_after(a, b) (((long)((a)-(b))) > 0)
inline bool eclock_after(clock_t a, clock_t b)
{
	long diff = (long) (a - b);
	//DPRINT(("eclock_after(%d, %d): diff %d\n", a, b, diff));
	return (diff > 0);
}

/*--------------------------------------------------------------------------
 Safe way to compare to eclock() values to see which is after the other.
 Assumes the largest interval to be compared is one half the maximum
 representable in a long.

 Returns true if time a is before time b.
--------------------------------------------------------------------------*/
//#define eclock_before(a, b) (((long)((a)-(b))) < 0)
inline bool eclock_before(clock_t a, clock_t b)
{
	long diff = (long) (a - b);
	//DPRINT(("eclock_before(%d, %d): diff %d\n", a, b, diff));
	return (diff < 0);
}

#endif
