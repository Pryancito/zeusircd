#ifndef Poller_select_H
#define Poller_select_H
/*--------------------------------------------------------------------------
 Copyright 1999,2000, Dan Kegel http://www.kegel.com/
 See the file COPYING
 (Also freely licensed to Disappearing, Inc. under a separate license
 which allows them to do absolutely anything they want with it, without
 regard to the GPL.)

 This module is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This module is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
--------------------------------------------------------------------------*/
#include <assert.h>
#include <errno.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "Poller.h"

/**
 * A class to monitor a set of file descriptors for readiness events.
 * Current an efficient wrapper around the poll() system call.
 * This is the code you would have written yourself if you had had the time...
 */
class Poller_select: public Poller {
public:
	
	/// Initialize the Poller.
	int init();
	
	/// Release any resouces allocated internally by this Poller.
	void shutdown();

	/**
	 Add a file descriptor to the set we monitor.
	 @param fd file descriptor to add
	 @param client object to handle events for this fd.  May use same client with more than one fd.
	 @param eventmask initial event mask for this fd
	 */
	int add(int fd, Client *client, short eventmask);

	/// Remove a file descriptor
	int del(int fd);

	/// Give a new value for the given file descriptor's eventmask.
	int setMask(int fd, short eventmask);

	/// Set fd's eventmask to its present value OR'd with the given one
	int orMask(int fd, short eventmask);

	/// Set fd's eventmask to its present value AND'd with the given one
	int andMask(int fd, short eventmask);
	
	/**
	 Sleep at most timeout_millisec waiting for an I/O readiness event
	 on the file descriptors we're watching.  Fills internal array
	 of readiness events.  Call getNextEvent() repeatedly to read its
	 contents.
	 @return 0 on success, EWOULDBLOCK if no events ready
	 */
	int waitForEvents(int timeout_millisec);

	/**
	 Get the next event that was found by waitForEvents.
	 @return 0 on success, EWOULDBLOCK if no more events
	 */
	int getNextEvent(Poller::PollEvent *e);

	/**
	 Sleep at most timeout_millisec waiting for an I/O readiness event
	 on the file descriptors we're watching, and dispatch events to
	 the handler for each file descriptor that is ready for I/O.
	 This is included as an example of how to use
	 waitForEvents and getNextEvent.  Real programs should probably
	 avoid waitAndDispatchEvents and call waitForEvents and getNextEvent
	 instead for maximum control over client deletion.
	 */
	int waitAndDispatchEvents(int timeout_millisec);
	
private:
	/// ptrs to event handlers, indexed by file descriptor.  May contain duplicates.
	Client *m_clients[FD_SETSIZE];

	/// highest fd being monitored by this Poller
	int m_max_fd;

	/// number of bits in m_test*fds still set after last call to getNextEvent()
	int m_rbits;

	/// used by getNextEvent to iterate through the fd's
	int m_cur_fd;
	
	/// The master copy of the fdset's we pass to select()
	fd_set m_readfds, m_writefds;

#ifdef Poller_URGENT
	fd_set m_exceptfds;
#endif
	
	/// The fdset's we pass to select()
	fd_set m_testrfds, m_testwfds;

#ifdef Poller_URGENT
	fd_set m_testefds;
#endif
	
	/// checks if requested events have occured and sets revent	
	void checkevents(struct pollfd *);
};
#endif
