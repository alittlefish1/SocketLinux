/*
 * Copyright (c) 2018 Guo Xiang
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <signal.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <fcntl.h>

#include "SocketTrans.hpp"

CSocketTrans::CSocketTrans(int clientFd, int pollFd) :
	mClientFd(clientFd),
	mPollFd(pollFd),
	mToken(nullptr),
	mOnRead(nullptr),
	mOnReadFail(nullptr)
{
	/* Set to nonblock mode */
	int flags = fcntl(mClientFd, F_GETFL, 0);
	if (flags < 0) {
		throw E("Fail to get fcntl, err: ", String::DEC(errno));
	}

	if (fcntl(mClientFd, F_SETFL, flags | O_NONBLOCK) < 0) {
		throw E("Fail to set fcntl, err: ", String::DEC(errno));
	}
}

CSocketTrans::~CSocketTrans(void)
{
	Deinit();
}

void CSocketTrans::Read(const OnReadFn &onRead,
						const OnReadFailFn &onFail)
{
	CHECK_PARAM(onRead, "onRead is null");

	if (mClientFd < 0) {
		throw E("Client die");
	}

	mOnRead = onRead;
	mOnReadFail = onFail;

	mToken = RunnableFn(([&](void) {
		DoRead();
	})).ToToken();

	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLET;
	ev.data.ptr = (void *)mToken;

	if (epoll_ctl(mPollFd, EPOLL_CTL_ADD, mClientFd, &ev) == -1) {
		mToken->Release();
		mToken = NULL;
		throw E("Fail to add epoll, err: ", String::DEC(errno));
	}
}

void CSocketTrans::Write(const CConstStringPtr &data,
						 const OnWriteFailFn &onFail,
						 const OnWrittenFn &onWritten)
{
	CHECK_PARAM(data, "data is null");

	uint32_t size = data->GetSize();
	const char *buf = data->ToCStr();
	int ret;

	do {
		ret = send(mClientFd, buf, size, 0);
		if (ret < 0) {
			/* Error happens */
			TRACE_ERROR("Fail to send, err: ", String::DEC(errno), EOS);
			if (onFail) {
				onFail();
			}
			return;

		} else if (0 == ret) {
			/* Connection is closed */
//			TRACE_ERROR("Connection is closed, err: ", String::DEC(errno), EOS);

			/* Notify read failure */
			if (onFail) {
				onFail();
			}
			return;

		} else {
			/* Data written */
			if (onWritten) {
				onWritten();
			}
			size -= size;

			buf += size;
		}
	} while (size > 0);
}

void CSocketTrans::DoRead(void)
{
	CStringPtr data;

	while (true) {
		int ret = recv(mClientFd, data,
					   data->GetCapacity(), 0);
		if (ret < 0) {
			/* No more data */
			if ((EAGAIN == errno) || (EWOULDBLOCK == errno)) {
				return;
			}

			/* Error happens */
			/* Notify read failure */
			TRACE_ERROR("Fail to recv, err: ", String::DEC(errno), EOS);
			if (mOnReadFail) {
				mOnReadFail();
			}
			Deinit();
			return;
		} else if (0 == ret) {
			/* Connection is closed */
//			TRACE_ERROR("Connection is closed, err: ", String::DEC(errno), EOS);

			/* Notify read failure */
			if (mOnReadFail) {
				mOnReadFail();
			}
			Deinit();
			return;
		} else {
			/* Handle data */
			data->SetSize(ret);
			mOnRead(data);
			data->SetSize(0);
		}
	}
}

void CSocketTrans::Deinit(void)
{
	if (mClientFd > 0) {
		if (NULL != mToken) {
			epoll_ctl(mPollFd, EPOLL_CTL_DEL, mClientFd, NULL);
			mToken->Release();
			mToken = NULL;
		}

		close(mClientFd);
		mClientFd = -1;
	}
}

