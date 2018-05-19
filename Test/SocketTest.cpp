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
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <EasyCpp.hpp>
#include <Socket.hpp>

#if 0
#define SOCKET_INFO(...) TRACE_INFO(__VA_ARGS__)
#else
#define SOCKET_INFO(...)
#endif

#define PORT 10086

void ClientTest(void)
{
	::sleep(1);

	SOCKET_INFO("Client: ", DEC(getpid()), " starting...", EOS);
	auto client(CreateSockClient("127.0.0.1", PORT));
	CConstStringPtr msg("client:", DEC(getpid()));

	for (int i = 0; i < 10; ++i) {

		SOCKET_INFO("[Client] Sending: ", msg, " size: ", DEC(msg->GetSize()), EOS);

		client->Write(
			msg,
			/* onWritten */
			[&](void) {
				SOCKET_INFO("[Client] sent\n");
			},
			/* onWrittenFail */
			[&](void) {
				SOCKET_INFO("[Client] fail to sent\n");
			});

		client->Read(
			/* onRead */
			[&msg](const CConstStringPtr &data) {
				SOCKET_INFO("[Client] receive response: ", data, EOS);
				if (data != msg) {
					TRACE_ERROR("Recv: ", data, " != ", msg);
					exit(1);
				}
			},
			/* onReadFail */
			[&](void) {
				SOCKET_INFO("[Client] Fail to receive\n");
			});

		SOCKET_INFO("[Client] Sending done", EOS);
	}

	SOCKET_INFO("Client: ", DEC(getpid()), " exit.", EOS);
	exit(0);
}

void TEST_CASE_ENTRY(void)
{
	for (int i = 0; i < 1; ++i) {
		if (0 == fork()) {
			ClientTest();
			return;
		}
	}

	auto server(CreateSockServer(10, PORT));

	server->Start(
		/* onConnection */
		[&](const IIoPtr &client) {
			SOCKET_INFO("[Server] Client connected\n");
			IIoPtr c(client);

			client->Read(
				/* onRead */
				[c](const CConstStringPtr &data) {
					SOCKET_INFO("[Server] Receive: ", data, EOS);
					c->Write(
						data,
						/* onWriten */
						[&]() {
							SOCKET_INFO("[Server] Data sent\n");
						},
						/* onWriteFail */
						[&]() {
							SOCKET_INFO("[Server] fail to send\n");
						});
					SOCKET_INFO("[Server] Receive out: ", data, EOS);
				},
				/* onReadFail */
				[c]() {
					SOCKET_INFO("[Server] fail to receive\n");
				});
		},
		[&](void) {
			SOCKET_INFO("[Server] die\n");
		});

	int status;
	pid_t pid;
	bool result = true;

	while ((pid = wait(&status)) > 0) { 
		SOCKET_INFO("pid: ", DEC(pid), " exit", EOS);
		if (0 != status) {
			result = false;
		}
	}

	::sleep(1);

	TRACE_INFO("Socket test ", result ? "succeed" : "fail", EOS);
}

