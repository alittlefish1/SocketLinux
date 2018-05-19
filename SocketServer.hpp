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

#ifndef __SOCKET_SERVER_HPP__
#define __SOCKET_SERVER_HPP__

#include <IServer.hpp>
#include "SocketTrans.hpp"

DEFINE_CLASS(SocketServer);

class CSocketServer :
	public IServer
{
public:
	CSocketServer(void);
	virtual ~CSocketServer(void);

	void Init(int nClient, int port);

	virtual void Start(const OnConnectFn &onConnect,
					   const OnServerDieFn &onDie);
	virtual void Stop(void);

private:
	void WorkLoop(void);

	void InitSocket(int nClient, int port);
	void InitPoll(int nClient);

	void OnAccept(void);

private:
	bool mRun;
	int mFd;
	int mPollFd;
	struct epoll_event *mEvents;
	int mEventNum;
	OnConnectFn mOnConnect;
	OnServerDieFn mOnDie;

/* Debug feature */
private:
	CList<CSocketTransWeakPtr> mClients;
};

#endif /* __SOCKET_SERVER_HPP__ */

