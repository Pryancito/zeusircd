#ifndef Poller_sigio_H
#define Poller_sigio_H
/*--------------------------------------------------------------------------
 Copyright 1999,2001, Dan Kegel http://www.kegel.com/
 See the file COPYING

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
#include "system.h"
#ifdef LINUX
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <time.h>   
#include <unistd.h>
#include "Poller.h"
#include "Poller_poll.h"
#include "dprint.h"

/** 
 * A class to monitor a set of file descriptors for readiness events.
 * This implementation is a wrapper around the F_SETSIG event delivery 
 * scheme provided by Linux.
 *
 * Warning: the 2.4 linux kernel works fine with all parts of
 * this class except wakeUp().  The 2.4 linux kernel does not by default 
 * support using SIGIO with pipes.  See Jeremy Elson's patch,
 * http://www.cs.helsinki.fi/linux/linux-kernel/2002-13/0191.html
 * if you want to use this class with pipes, or if you want to use
 * the wakeUp method of this class.
 */
class Poller_sigio : public Poller {
public:
	Poller_sigio() { m_fds = NULL; }

	/// Tell the Poller which signal number to use.  Call once after init(), before add().
	virtual int setSignum(int signum) {
		// Remember the signal number.
		m_signum = signum;
		// Save it as a sigset, too.
		sigemptyset(&m_sigset);
		sigaddset(&m_sigset, signum);
		// Put SIGIO in the same set, since we need to listen for that, too.
		sigaddset(&m_sigset, SIGIO);
		// Finally, block delivery of those signals.
		return sigprocmask(SIG_BLOCK, &m_sigset, NULL);
	}

	/// Initialize the Poller.
	int init();

	/// Release any resouces allocated internally by this Poller.
	void shutdown();

	/**
	 Add a file descriptor to the set we monitor. 
	 Caller should already have established a handler for SIGIO.
	 @param fd file descriptor to add
	 @param client object to handle events for this fd.  May use same client with more than one fd.
	 @param eventmask initial event mask for this fd
	 */
	virtual int add(int fd, Client *client, short eventmask);

	/// Remove a file descriptor.
	virtual int del(int fd);

	/// Give a new value for the given file descriptor's eventmask.
	virtual int setMask(int fd, short eventmask);

	/// Set fd's eventmask to its present value OR'd with the given one
	virtual int orMask(int fd, short eventmask);

	/// Set fd's eventmask to its present value AND'd with the given one
	virtual int andMask(int fd, short eventmask);

	/**
	 Reset the given readiness bits.  
	 Do this after you finish handling an event and don't want to
	 be notified about it again until the OS says it's newly ready.
	 This does nothing except in one-shot notification schemes like Poller_sigio.
	 */
	virtual void clearReadiness(int fd, short eventmask);

	/**
	 Sleep at most timeout_millisec waiting for an I/O readiness event
	 on the file descriptors we're watching.  Fills internal array
	 of readiness events.  Call getNextEvent() repeatedly to read its
	 contents.
	 @return 0 on success, EWOULDBLOCK if no events ready
	 */
	virtual int waitForEvents(int timeout_millisec);

	/**
	 Get the next event that was found by waitForEvents.
	 @return 0 on success, EWOULDBLOCK if no more events
	 */
	virtual int getNextEvent(PollEvent *e);

	/**
	 Sleep at most timeout_millisec waiting for an I/O readiness event
	 on the file descriptors we're watching, and dispatch events to
	 the handler for each file descriptor that is ready for I/O.
	 This is included as an example of how to use
	 waitForEvents and getNextEvent.  Real programs should probably
	 avoid waitAndDispatchEvents and call waitForEvents and getNextEvent
	 instead for maximum control over client deletion.
	 */
	virtual int waitAndDispatchEvents(int timeout_millisec);

private:
	/// Fallback Poller for when the realtime signal queue overflows
	Poller_poll m_backup;

	/// Which realtime signal number to use for I/O readiness notification.
	int m_signum;

	/// Set including just the above signal
	sigset_t m_sigset;

	/// Each fd we watch may have an entry of this type in the 'ready' list.
	struct fdstate_t {
		/// Who's interested in this fd.
		Client *m_client;

		/// Index of the next fdstate in the list, or -1.
		int m_next;

		/// Index of the previous fdstate in the list, or -1.
		int m_prev;

		/// What poll bits the caller is interested in for this fd.
		short m_mask;

		/// What operations the fd is currently ready for.
		short m_readybits;

		void clear() {
			m_mask = 0;
			m_readybits = -1;
			m_next = -1;
			m_prev = -2;
			m_client = NULL;
		}

		// opposite of isClear
		void setInUse() { m_prev = -1; }

		bool isClear() { return (m_prev == -2); }

		bool isReady() { return ((m_mask & m_readybits) != 0); }

		bool isOnReadyList() { return (m_next >= 0); }
	};

	/// The head of the 'ready' list.  Offset into m_fds, or -1.
	int m_ready_head;

	/// The next element to return from the 'ready' list.  Offset into m_fds, or -1.
	int m_ready_next;

	/// Count of fds in ready list.
	int m_ready_count;

	/// Each fd we watch has an entry in this array.
	fdstate_t *m_fds;

	/// Number of elements worth of heap allocated for m_fds.
	int m_fds_alloc;

	/// How many fds we are watching.
	int m_fds_used;

	/// insert given fd into doubly-linked list of ready fd's
	void readyListAdd(int fd);

	/// remove given fd from doubly-linked list of ready fd's
	void readyListDel(int fd);

	/// If fd is ready, put it on ready list; if it's not ready, remove it from ready list.
	void readyListUpdate(int fd);

	void readyListDump(const char *msg = "") {
#ifdef USE_DPRINT
		DPRINT(("readyListDump: head %d, count %d, msg %s\n", m_ready_head, m_ready_count, msg));
		for (int i=0; i<m_fds_alloc; i++)
			DPRINT(("%d: prev %d next %d\n", i, m_fds[i].m_prev, m_fds[i].m_next));
#else
		(void) msg;
#endif
	}
};
#endif

#endif
