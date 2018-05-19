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
#include <arpa/inet.h>
#include <fcntl.h>

#include "SocketClient.hpp"

CSocketClient::CSocketClient(void) :
	mClientFd(-1)
{
	/* Does nothing */
}

CSocketClient::~CSocketClient(void)
{
	Deinit();
}

void CSocketClient::Init(const CConstStringPtr &addr, int port)
{
	CHECK_PARAM(addr, "Invalidate address: ", addr);

	/* Start socket transfer */
	mClientFd = socket(PF_INET, SOCK_STREAM, 0);
	if (mClientFd < 0) {
		throw E("Fail to create socket: ", DEC(errno));
	}

	struct sockaddr_in saddr;
	memset(&saddr, 0, sizeof(addr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);
	inet_pton(AF_INET, addr->ToCStr(), &saddr.sin_addr);

	int ret = connect(mClientFd, (struct sockaddr *)&saddr, sizeof(saddr));
	if (ret < 0) {
		close(mClientFd);
		mClientFd = -1;
		throw E("Fail to connect to ", addr, " , error: ", DEC(errno));
	}

	signal(SIGPIPE, SIG_IGN);
}

void CSocketClient::Read(const OnReadFn &onRead,
						 const OnReadFailFn &onFail)
{
	CHECK_PARAM(onRead, "onRead is null");

	CStringPtr data;

	int ret = recv(mClientFd, data,
				   data->GetCapacity(), 0);
	if (ret < 0) {
		/* Error happens */
		/* Notify read failure */
		TRACE_ERROR("Fail to recv, err: ", String::DEC(errno), EOS);
		if (onFail) {
			onFail();
		}
		Deinit();
		return;
	} else if (0 == ret) {
		/* Connection is closed */
		TRACE_ERROR("Connection is closed, err: ", String::DEC(errno), EOS);

		/* Notify read failure */
		if (onFail) {
			onFail();
		}
		Deinit();
		return;
	} else {
		/* Handle data */
		data->SetSize(ret);
		onRead(data);
	}
}

void CSocketClient::Write(const CConstStringPtr &data,
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
			TRACE_ERROR("Connection is closed, err: ", String::DEC(errno), EOS);

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

void CSocketClient::Deinit(void)
{
	if (mClientFd >= 0) {
		close(mClientFd);
		mClientFd = -1;
	}
}

