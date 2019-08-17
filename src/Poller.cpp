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
#include "dprint.h"
#include "Poller.h"

#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <fcntl.h>

int Poller::init()
{
	m_wakeup_pipe[0] = -1;
	m_wakeup_pipe[1] = -1;
	m_pipe_client.m_owner = NULL;
	return 0;
}

int Poller::initWakeUpPipe()
{
	// Create the pipe used by wakeUp
	int err;
	if (pipe(m_wakeup_pipe)) {
		err = errno;
		LOG_ERROR(("init: pipe() failed, errno %d\n", err));
		return err;
	}
	// Set the pipe to be nonblocking.  The pipe carries hints only;
	// we never want to sleep if we can't read or write it.
	if (fcntl(m_wakeup_pipe[0], F_SETFL, O_NONBLOCK) ||
	    fcntl(m_wakeup_pipe[1], F_SETFL, O_NONBLOCK)) {
		err = errno;
		LOG_ERROR(("init: fcntl() failed, errno %d\n", err));
		return err;
	}

	// Wake this Poller up whenever a byte gets written to the pipe by wakeUp().
	add(m_wakeup_pipe[0], &m_pipe_client, POLLIN);

	m_pipe_client.m_owner = this;
	return 0;
}

void Poller::shutdown()
{
	if (m_wakeup_pipe[0] != -1) {
		close(m_wakeup_pipe[0]);
		m_wakeup_pipe[0] = -1;
		close(m_wakeup_pipe[1]);
		m_wakeup_pipe[1] = -1;
		m_pipe_client.m_owner = NULL;
	}
}

int Poller::wakeUp() 
{
	// This Poller is monitoring m_wakeup_pipe[0] for readability.
	// Writing a byte to m_wakeup_pipe[1] will cause a readiness event,
	// which will cause waitForEvents() to return.  QED. 
	// See 'man pipe'.
	//DPRINT(("wakeUp: writing 1 byte to fd %d\n", m_wakeup_pipe[1]));
	if (write(m_wakeup_pipe[1], "a", 1) == 1)
		return 0;
	int err = errno;
	DPRINT(("wakeUp: write fails, err %d\n", err));
	return err;
}


int Poller::WakeUpClient::notifyPollEvent(Poller::PollEvent *event) 
{
	if (event->revents & POLLERR) {
		DPRINT(("notifyPollEvent: m_wakeup_pipe broken?!\n"));
		assert(false);
	}
	if (event->revents & POLLIN) {
		char buf[64];
		// Clear the event that got us here.  If more than one, get 'em all.
		DPRINT(("WakeUpClient::notifyPollEvent: reading from fd %d to clear wakeup event\n", event->fd));
		while (read(event->fd, buf, sizeof(buf)) == sizeof(buf) ) 
			;
		// assert(errno == EWOULDBLOCK); 
		/* When we get EWOULDBLOCK is exactly when we must clear readiness for Poller_sigio et al */
		m_owner->clearReadiness(event->fd, POLLIN);
	}

	return 0;
}
