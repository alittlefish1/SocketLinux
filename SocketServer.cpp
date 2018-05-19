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

#include <Thread.hpp>
#include "SocketServer.hpp"

CSocketServer::CSocketServer(void) :
	mRun(true),
	mFd(-1),
	mPollFd(-1),
	mEvents(NULL),
	mEventNum(0),
	mOnConnect(nullptr),
	mOnDie(nullptr)
{
	/* Does nothing */
}

CSocketServer::~CSocketServer(void)
{
	mRun = false;

	if (NULL != mEvents) {
		delete [] mEvents;
		mEvents = NULL;
	}

	if (mPollFd >= 0) {
		close(mPollFd);
		mPollFd = -1;
	}

	if (mFd >= 0) {
		close(mFd);
		mFd = -1;
	}

	mClients.Iter()->ForEach([&](const CSocketTransWeakPtr &tran) {
		TRACE_INFO("client: ", DEC(tran.GetRef()));
	});
}

void CSocketServer::Init(int nClient, int port)
{
	CHECK_PARAM(nClient > 0, "nClient %d is too small", nClient);
	CHECK_PARAM(port > 0, "port %d is too small", port);

	InitSocket(nClient, port);
	InitPoll(nClient);
}

void CSocketServer::Start(const OnConnectFn &onConnect, const OnDieFn &onDie)
{
	mOnConnect = onConnect;
	mOnDie = onDie;

	Platform::CreateThread([&](void) {
		WorkLoop();
	});
}

void CSocketServer::Stop(void)
{
	mRun = false;
	/* Todo: Adds stop wakeup and wait */
}

void CSocketServer::WorkLoop(void)
{
	while (mRun) {
		int nfds = epoll_wait(mPollFd, mEvents, mEventNum, -1);
		if (nfds <= 0) {
			TRACE_ERROR("Fail to epoll_wait, err: ", String::DEC(errno), EOS);
			mOnDie();
			mRun = false;
			return;
		}

		for (int i = 0; i < nfds; ++i) {
			RunnableFn task((RunnableFn::Token *)mEvents[i].data.ptr, true);
			task();
		}
	}
}

void CSocketServer::InitSocket(int nClient, int port)
{
	mFd = socket(PF_INET, SOCK_STREAM, 0);
	if (mFd < 0) {
		throw E("Fail to create socket: ", DEC(errno));
	}

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

	int ret = bind(mFd, (struct sockaddr *)&addr, sizeof(addr));
	if (ret < 0) {
		throw E("Fail to bind: ", DEC(errno));
	}

	ret = listen(mFd, nClient);
	if (ret < 0) {
		throw E("Fail to listen: ", DEC(errno));
	}

	signal(SIGPIPE, SIG_IGN);
}

void CSocketServer::InitPoll(int nClient)
{
	mPollFd = epoll_create(1);
	if (mPollFd < 0) {
		throw E("Fail to create epoll: ", DEC(errno));
	}

	/* 1 for acctpt.
	 * 2 for stop */
	mEventNum = nClient + 2;
	mEvents = new struct epoll_event[mEventNum];

	auto token = RunnableFn([&](void) {
		OnAccept();
	}).ToToken();

	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLET;
	ev.data.ptr = (void *)token;

	if (epoll_ctl(mPollFd, EPOLL_CTL_ADD, mFd, &ev) == -1) {
		token->Release();
		throw E("Fail to add events: ", DEC(errno));
	}
}

void CSocketServer::OnAccept(void)
{
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);

	int fd = accept(mFd, (struct sockaddr *)&addr, &len);
	if (fd < 0) {
		TRACE_ERROR("Fail to accept, err: ", String::DEC(errno), EOS);
		mOnDie();
		mRun = false;
		return;
	} else {
		CSocketTransPtr trans(fd, mPollFd);
		mOnConnect(trans);
		mClients.PushBack(trans);
	}
}

