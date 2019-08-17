#ifndef Poller_H
#define Poller_H
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
#include <errno.h>
#include <time.h>   
#include <unistd.h>
#include <sys/poll.h>

/** 
 * A class to monitor a set of file descriptors for readiness events.
 * Current an efficient wrapper around the poll() system call.
 * This is the code you would have written yourself if you had had the time...
 */
class Poller {
public:
	class Client;

	/// A class to represent a readiness event
	struct PollEvent {
		/// the file descriptor that is ready for I/O (same as in struct pollfd).
		int fd;

		/// the kind of I/O this fd is currently ready for (same as in struct pollfd).
		short revents;

		/// the object to call to handle I/O on this fd.
		Client *client;
	};

	/// A class to handle a particular file descriptor's readiness events.
	class Client {
	public:
		/**
		 * Handle a particular file descriptor's readiness events.
		 * @return 0 if ok, nonzero to cause client to be deleted from Poller.
		 */
		virtual int notifyPollEvent(PollEvent *e) = 0;
	};

	/// Initialize the Poller.
	virtual int init();

	/**
	 Tell the Poller which signal number to use.  Call once after init(), before add().
	 Only used by Poller_sigio.
	*/
	virtual int setSignum(int signum) { (void) signum; return 0; }

	/// Release any resouces allocated internally by this Poller.
	virtual void shutdown();

	/**
	 Add a file descriptor to the set we monitor. 
	 @param fd file descriptor to add
	 @param client object to handle events for this fd.  May use same client with more than one fd.
	 @param eventmask initial event mask for this fd
	 */
	virtual int add(int fd, Client *client, short eventmask) = 0;

	/// Remove a file descriptor
	virtual int del(int fd) = 0;

	/// Give a new value for the given file descriptor's eventmask.
	virtual int setMask(int fd, short eventmask) = 0;

	/// Set fd's eventmask to its present value OR'd with the given one
	virtual int orMask(int fd, short eventmask) = 0;

	/// Set fd's eventmask to its present value AND'd with the given one
	virtual int andMask(int fd, short eventmask) = 0;

	/** 
	 Reset the given readiness bits.  
	 Do this after you finish handling an event and don't want to
	 be notified about it again until the OS says it's newly ready.
	 This does nothing except in one-shot notification schemes like Poller_sigio.
	 */
	virtual void clearReadiness(int fd, short eventmask) { (void) fd; (void) eventmask; }


	/**
	 Sleep at most timeout_millisec waiting for an I/O readiness event
	 on the file descriptors we're watching.  Fills internal array
	 of readiness events.  Call getNextEvent() repeatedly to read its
	 contents.
	 @return 0 on success, EWOULDBLOCK if no events ready
	 */
	virtual int waitForEvents(int timeout_millisec) = 0;

	/**
	 Get the next event that was found by waitForEvents.
	 @return 0 on success, EWOULDBLOCK if no more events
	 */
	virtual int getNextEvent(PollEvent *e) = 0;

	/**
	 Sleep at most timeout_millisec waiting for an I/O readiness event
	 on the file descriptors we're watching, and dispatch events to
	 the handler for each file descriptor that is ready for I/O.
	 This is included as an example of how to use
	 waitForEvents and getNextEvent.  Real programs should probably
	 avoid waitAndDispatchEvents and call waitForEvents and getNextEvent
	 instead for maximum control over client deletion.
	 */
	virtual int waitAndDispatchEvents(int timeout_millisec) = 0;

	/**
	 Get ready for future calls to wakeUp().  Allocates a pipe internally.
	 @return 0 on success; Unix error code on failure.
	*/	
	int initWakeUpPipe();

	/**
	 Wake up a thread which is currently sleeping in a call to waitForEvents().
	 May be called from any thread, but you must have called initWakeUpPipe() 
	 once before the first call to waitForEvents().
	 @return 0 on success; Unix error code on failure.
	 @note Threadsafe.
	*/	
	int wakeUp();

private:

	/// Write one byte (value not important) to m_wakeup_pipe[1] to wake up this thread.
	int m_wakeup_pipe[2];

	/// Empty client used only to break us out of poll
	class WakeUpClient : public Client {
	public:
		/**
		 * Read the bytes that were written to m_wakeup_pipe[0] by wakeUp()
		 * @return 0 always, because the wakeup pipe should never be deleted.
		 */
		virtual int notifyPollEvent(PollEvent *e);

		/// The Poller that owns this WakeUpClient.
		Poller *m_owner;
	};
		
	/// Empty client used only to break us out of poll
	WakeUpClient m_pipe_client;
	
};
#endif
