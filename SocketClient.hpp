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

#ifndef __SOCKET_CLIENT_HPP__
#define __SOCKET_CLIENT_HPP__

#include <EasyCpp.hpp>
#include <IIo.hpp>

DEFINE_CLASS(SocketClient);

class CSocketClient :
	public IIo
{
public:
	CSocketClient(void);
	virtual ~CSocketClient(void);

	void Init(const CConstStringPtr &addr, int port);

	virtual void Read(const OnReadFn &onRead,
					  const OnReadFailFn &onFail);

	virtual void Write(const CConstStringPtr &data,
					   const OnWriteFailFn &onFail,
					   const OnWrittenFn &onWritten);

private:
	void Deinit(void);

private:
	int mClientFd;
};

#endif /* __SOCKET_CLIENT_HPP__ */

