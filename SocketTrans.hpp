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

#ifndef __SOCKET_TRANS_HPP__
#define __SOCKET_TRANS_HPP__

#include <EasyCpp.hpp>

#include <IIo.hpp>

DEFINE_CLASS(SocketTrans);

class CSocketTrans :
	public IIo
{
public:
	CSocketTrans(int clientFd, int pollFd);
	virtual ~CSocketTrans(void);

	virtual void Read(const OnReadFn &onRead,
					  const OnReadFailFn &onFail);

	virtual void Write(const CConstStringPtr &data,
					   const OnWriteFailFn &onFail,
					   const OnWrittenFn &onWritten);

private:
	void DoRead(void);

	void Deinit(void);

private:
	int mClientFd;
	int mPollFd;
	RunnableFn::Token *mToken;

	OnReadFn mOnRead;
	OnReadFailFn mOnReadFail;
};

#endif /* __SOCKET_TRANS_HPP__ */

