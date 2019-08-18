/*--------------------------------------------------------------------------
 Copyright 1999,2001, Dan Kegel http://www.kegel.com/
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
#include "system.h"
#ifdef LINUX
#include "dprint.h"
#include "Poller_sigio.h"

#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <fcntl.h>
#include "eclock.h"

int Poller_sigio::init()
{
	DPRINT(("init()\n"));

	m_fds_used = 0;
	m_fds_alloc = 16;
	m_fds = (fdstate_t *)malloc(sizeof(fdstate_t) * m_fds_alloc);
	if (!m_fds)
		return ENOMEM;
	
	for (int i=m_fds_used; i<m_fds_alloc; i++)
		m_fds[i].clear();

	// No fd ready, so list is empty.
	m_ready_head = -1;
	m_ready_next = -1;
	m_ready_count = 0;

	Poller::init();

	return m_backup.init();
}

void Poller_sigio::shutdown()
{
	// Remove all remaining fd's
	for (int i=m_fds_alloc-1; (i >= 0) && (m_fds_used > 0); i--)
		if (!m_fds[i].isClear())
			del(i);

	// purge any pending signals
	waitForEvents(0);

	// re-enable the signals we blocked
	// FIXME: is this right?
	sigprocmask(SIG_UNBLOCK, &m_sigset, NULL);

	// no longer need the backup Poller
	m_backup.shutdown();

	// Free all memory
	if (m_fds) {
		free(m_fds);
		m_fds = NULL;
	}

	// Chain to superclass method
	Poller::shutdown();
}

/// insert given fd into doubly-linked list of ready fd's
void Poller_sigio::readyListAdd(int fd)
{
	DPRINT(("readyListAdd(fd %d)\n", fd));
	// make sure node wasn't in list
	assert(m_fds[fd].m_next < 0);
	assert(m_fds[fd].m_prev < 0);

	if (m_ready_count == 0) {
		// new element is entire list
		m_fds[fd].m_next = fd;
		m_fds[fd].m_prev = fd;
		// new node is new head
		m_ready_head = fd;
	} else {
		int head_prev = m_fds[m_ready_head].m_prev;

		// new node links to existing first and last nodes
		m_fds[fd].m_prev = head_prev;
		m_fds[fd].m_next = m_ready_head;

		// old last node links forward to new one
		m_fds[head_prev].m_next = fd;

		// old first node links back to new one
		m_fds[m_ready_head].m_prev = fd;
	}
	m_ready_count++;

	// If we were done with the ready list, we've got something now.
	// FIXME: probably we should put newly ready fd's on end of list
	// instead of head, so it doesn't trigger going through old ready fd's
	if (m_ready_next == -1)
		m_ready_next = fd;
}

/// remove given fd from doubly-linked list of ready fd's
void Poller_sigio::readyListDel(int fd)
{
	DPRINT(("readyListDel(fd %d)\n", fd));
	int next = m_fds[fd].m_next;
	int prev = m_fds[fd].m_prev;

	// make sure node was in list
	assert(next >= 0);
	assert(prev >= 0);
	assert(m_ready_count > 0);
	assert(m_ready_head >= 0);

	if (m_ready_count == 1) {
		// node was head, and was entire list
		assert(fd == m_ready_head);
		assert(next == prev);
		m_ready_head = -1;
		m_ready_next = -1;
	} else {
		// head moves if it's getting deleted
		if (m_ready_head == fd)
			m_ready_head = next;
		if (m_ready_next == fd)
			m_ready_next = next;
		
		// splice gap
		m_fds[prev].m_next = next;
		m_fds[next].m_prev = prev;
	}
	m_fds[fd].m_next = -1;
	m_fds[fd].m_prev = -1;
	m_ready_count--;
}

/// If fd is ready, put it on ready list; if it's not ready, remove it from ready list.
void Poller_sigio::readyListUpdate(int fd)
{
	if (m_fds[fd].isOnReadyList()) {
		if (!m_fds[fd].isReady()) {
			// we were on the list before, but aren't ready now, so take us off list
			readyListDel(fd);
		}
	} else {
		if (m_fds[fd].isReady()) {
			// we weren't on the list before, but we're ready now, so put us on list
			readyListAdd(fd);
		}
	}
}

int Poller_sigio::add(int fd, Client *client, short eventmask)
{
	int i, n;

	// Resize array indexed by fd if fd is beyond what we've seen.
	if (fd >= m_fds_alloc) {
		n = m_fds_alloc * 2;
		if (n < fd + 1)
			n = fd + 1;

		fdstate_t *newfds = (fdstate_t *)realloc(m_fds, n * sizeof(fdstate_t));
		if (!newfds)
			return ENOMEM;

		for (i=m_fds_alloc; i<n; i++)
			newfds[i].clear();

		m_fds = newfds;
		m_fds_alloc = n;
	}

	int err;
	int flags;
	// Set this fd to emit signals.
	if (fcntl(fd, F_GETFL, &flags) < 0) {
		err = errno;
		LOG_ERROR(("add: fcntl(fd %d, F_GETFL) returns err %d\n", fd, err));
		return err;
	}
	flags |= O_NONBLOCK|O_ASYNC;
	if (fcntl(fd, F_SETFL, flags) < 0) {
		err = errno;
		LOG_ERROR(("add: fcntl(fd %d, F_SETFL, 0x%x) returns err %d\n",
				fd, flags, err));
		return err;
	}

	// Set this fd to emit the right signal.
	if (fcntl(fd, F_SETSIG, m_signum) < 0) {
		err = errno;
		LOG_ERROR(("add: fcntl(fd %d, F_SETSIG, %d) returns err %d\n",
				fd, m_signum, err));
		return err;
	}

	// Set this fd to send its signals to the current process.
	// This might need adjusting (on Linux, each thread has its own pid...)
	int pid = getpid();
	if (fcntl(fd, F_SETOWN, pid) < 0) {
		err = errno;
		LOG_ERROR(("add: fcntl(fd %d, F_SETOWN, %d) returns err %d\n",
				fd, pid, err));
		return err;
	}

	// Update limits.
	m_fds_used++;

	m_fds[fd].m_mask = eventmask;

	// Start out claiming "all ready", and let user program try 
	// reading/writing/accept/connecting.  An EWOULDBLOCK should be harmless;
	// the user must then call clearReadiness() to tell us we were wrong.
	// This should handle the case where the fd is already ready for something
	// when we start.  (FIXME: Could call poll() here, instead.  Should we?)
	m_fds[fd].m_readybits = POLLIN|POLLOUT;

	m_fds[fd].setInUse();
	readyListUpdate(fd);

	m_fds[fd].m_client = client;

	DPRINT(("add(%d, %p, %x) m_fds_used %d\n", fd, client, eventmask, m_fds_used));

	// For our fallback, register interest in all allowed bits, not just the
	// ones the user is interested in at the moment.
	return m_backup.add(fd, client, POLLIN|POLLPRI|POLLOUT);
}

int Poller_sigio::del(int fd)
{
	int err;

	// Sanity checks
	if ((fd < 0) || (fd >= m_fds_alloc) || (m_fds_used == 0)) {
		LOG_ERROR(("del(fd %d): fd out of range\n", fd));
		return EINVAL;
	}

	// Set this fd to no longer emit signals.
	int flags;
	if (fcntl(fd, F_GETFL, &flags) < 0) {
		err = errno;
		LOG_ERROR(("del: fcntl(fd %d, F_GETFL) returns err %d\n", fd, err));
		return err;
	}
	flags &= ~O_ASYNC;
	if (fcntl(fd, F_SETFL, flags) < 0) {
		err = errno;
		LOG_ERROR(("del: fcntl(fd %d, F_SETFL, 0x%x) returns err %d\n",
				fd, flags, err));
		return err;
	}

	DPRINT(("del(fd %d): isOnReadyList %d\n", fd, m_fds[fd].isOnReadyList()));


	// If it's in the ready list, remove it
	if (m_fds[fd].isOnReadyList())
		readyListDel(fd);

	m_fds[fd].clear();

	m_fds_used--;

	return m_backup.del(fd);
}

int Poller_sigio::setMask(int fd, short eventmask)
{
	if ((fd < 0) || (fd >= m_fds_alloc) || (m_fds_used == 0) || m_fds[fd].isClear()) {
		DPRINT(("setMask(fd %d, %x): fd out of range\n", fd, eventmask));
		return ENOENT;
	}
	m_fds[fd].m_mask = eventmask;

	DPRINT(("setMask(fd %d, %x): new mask %x\n", fd, eventmask, m_fds[fd].m_mask));

	readyListUpdate(fd);

	return 0;
}

int Poller_sigio::orMask(int fd, short eventmask)
{
	if ((fd < 0) || (fd >= m_fds_alloc) || (m_fds_used == 0) || m_fds[fd].isClear()) {
		DPRINT(("orMask(fd %d, %x): fd out of range\n", fd, eventmask));
		return ENOENT;
	}
	m_fds[fd].m_mask |= eventmask;

	DPRINT(("orMask(fd %d, %x): new mask %x\n", fd, eventmask, m_fds[fd].m_mask));
	readyListUpdate(fd);

	return 0;
}

int Poller_sigio::andMask(int fd, short eventmask)
{
	if ((fd < 0) || (fd >= m_fds_alloc) || (m_fds_used == 0) || m_fds[fd].isClear()) {
		DPRINT(("andMask(fd %d, %x): fd out of range\n", fd, eventmask));
		return ENOENT;
	}
	m_fds[fd].m_mask &= eventmask;

	DPRINT(("andMask(fd %d, %x): new mask %x\n", fd, eventmask, m_fds[fd].m_mask));
	readyListUpdate(fd);

	return 0;
}

/**
 Reset the given readiness bits.  
 Do this after you finish handling an event and don't want to
 be notified about it again until the OS says it's newly ready.
 This does nothing except in one-shot notification schemes like Poller_sigio.
 */
void Poller_sigio::clearReadiness(int fd, short eventmask)
{
	if ((fd < 0) || (fd >= m_fds_alloc) || (m_fds_used == 0) || m_fds[fd].isClear()) {
		// hmm.  whatever.
		DPRINT(("clearReadiness(fd %d, %x): skipping; m_fds_alloc %d, m_fds_used %d, m_prev %d, isClear %d\n", 
					fd, eventmask, m_fds_alloc, m_fds_used, m_fds[fd].m_prev, m_fds[fd].isClear()));
		return;
	}

	m_fds[fd].m_readybits &= ~eventmask;

	DPRINT(("clearReadiness(fd %d, %x): new readiness %x\n", fd, eventmask, m_fds[fd].m_readybits));
	readyListUpdate(fd);
}


/**
 Sleep at most timeout_millisec waiting for an I/O readiness event
 on the file descriptors we're watching.  Fills internal array
 of readiness events.  Call getNextEvent() repeatedly to read its
 contents.
 @return 0 on success, EWOULDBLOCK if no events ready
 */
int Poller_sigio::waitForEvents(int timeout_millisec)
{
	int fd;
	int signum;
	siginfo_t info;
	struct timespec timeout;

	DPRINT(("waitForEvents(%d): m_ready_head %d, next %d\n", timeout_millisec, m_ready_head, m_ready_next));
	// Reset to head of ready list.
	m_ready_next = m_ready_head;

	// If there are no ready fds, sleep until we have one event.
	if (m_ready_head < 0) {
		timeout.tv_sec = timeout_millisec / 1000;
		timeout.tv_nsec = (timeout_millisec % 1000) * 1000000;
	} else {
		timeout.tv_sec = 0;
		timeout.tv_nsec = 0;
	}

	// Pull signals off the queue until we would block.
	int npulled = 0;
	bool overflowed = false;
	do {
		DPRINT(("%s %d: now %d, calling sigtimedwait with timeout %d,%d\n", 
			__FILE__, __LINE__, eclock(),
			timeout.tv_sec, timeout.tv_nsec));
		signum = sigtimedwait(&m_sigset, &info, &timeout);
		timeout.tv_sec = 0;
		timeout.tv_nsec = 0;
		DPRINT(("%s %d: now %d, sigtimedwait returns %d, errno %d, fd %d, band %x\n",
			__FILE__, __LINE__, eclock(), signum, errno, info.si_fd, info.si_band));
		if (signum == -1) 
			break;
		if (signum == SIGIO) {
			// uh-oh. the realtime signal queue overflowed.  ditch all
			// remaining signals, and call poll() to get current status.
			overflowed = true;
			break;
		}

		npulled++;
		assert(signum == m_signum);
		// Remember the new event
		fd = info.si_fd;
		if ((fd < 0) || (fd >= m_fds_alloc) || (m_fds_used == 0) || m_fds[fd].isClear()) {
			DPRINT(("waitForEvents: ignoring event on fd %d.  alloc %d used %d isClear %d\n", 
				fd, m_fds_alloc, m_fds_used, m_fds[fd].isClear()));
			readyListDump();
			// silently ignore.  Might be stale (aren't time skews fun?)
			continue;
		}
		m_fds[fd].m_readybits |= info.si_band & (POLLIN|POLLPRI|POLLOUT|POLLERR|POLLHUP|POLLNVAL);
		readyListUpdate(fd);
	} while (1);

	if (npulled > 64) 
		EDPRINT(("waitForEvents: queue getting deep -- pulled %d signals.  readycount %d\n", npulled, m_ready_count));

	if (overflowed) {
		PollEvent event;
		DPRINT(("waitForEvents: SIGIO received.  Calling m_backup.waitForEvents().\n"));

		// We missed some events.  Ditch remaining signals.  
		signal(m_signum, SIG_IGN);	// POSIX says this clears the queue
		// Make sure that cleared the queue.  (Note that timeout is zero here.)
		//assert(sigtimedwait(&m_sigset, &info, &timeout) != m_signum);
		// Can't leave signal ignored, or it might be.  (POSIX is fuzzy here.)
		signal(m_signum, SIG_DFL);
		// Call poll() to get up-to-date state.
		m_backup.waitForEvents(0);
		// Iterate through result.  FIXME: lousy error handling here.
		while (0 == m_backup.getNextEvent(&event)) {
			fd = event.fd;
			if ((fd < 0) || (fd >= m_fds_alloc) || (m_fds_used == 0) || m_fds[fd].isClear())
				continue;
			// Skip events we're up to date on (is this safe?)
			if (m_fds[fd].m_readybits == event.revents) {
				continue;
			}
			// m_readybits must be *all* the readiness bits, not just
			// the ones we're interested in at the moment.
			// It's ok if there are some false positives here.
			// To be paranoid, we should OR in the new bits,
			// but let's live dangerously; this way we'll find out sooner
			// if we're losing any bits somehow.
			DPRINT(("waitForEvents: fd %d readybits were %x, setting to %x\n", 
				fd, m_fds[fd].m_readybits, event.revents));
			m_fds[fd].m_readybits = event.revents;
			readyListUpdate(fd);
		}
	}

	DPRINT(("waitForEvents: %d fds ready, npulled %d\n", m_ready_count, npulled));
	return m_ready_count ? 0 : EWOULDBLOCK;
}

/**
 Get the next event that was found by waitForEvents.
 @return 0 on success, EWOULDBLOCK if no more events
 */
int Poller_sigio::getNextEvent(PollEvent *e)
{
	if (m_ready_next < 1)
		return EWOULDBLOCK;

	e->fd = m_ready_next;
	e->revents = m_fds[m_ready_next].m_readybits & m_fds[m_ready_next].m_mask;	/* or should we mask? */
	e->client = m_fds[m_ready_next].m_client;
	m_ready_next = m_fds[m_ready_next].m_next;
	if (m_ready_next == m_ready_head)
		m_ready_next = -1;

	DPRINT(("getNextEvent: fd %d revents %x client %p ready_count %d ready_next %d\n",
		e->fd, e->revents, e->client, m_ready_count, m_ready_next));

	return 0;
}

int Poller_sigio::waitAndDispatchEvents(int timeout_millisec)
{
	int err;
	PollEvent event;

	err = waitForEvents(timeout_millisec);
	if (err)
		return err;

	// Pump any network traffic into the appropriate Clients
	for (;;) {
		err = getNextEvent(&event);
		if (err) {
			if (err != EWOULDBLOCK) {
				DPRINT(("waitAndDispatchEvents: getNextEvent() returned %d\n", err));
				return err;
			}
			break;
		}
		err = event.client->notifyPollEvent(&event);
		if (err) {
			DPRINT(("waitAndDispatchEvents: %p->notifyPollEvent(fd %d) returned %d, deleting\n",
				event.client, event.fd, err));
			del(event.fd);
		}
	}

	return 0;
}
#endif
