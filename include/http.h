
#pragma once

#include <netinet/in.h>

class HTTPRequest
{
public:
	HTTPRequest(struct sockaddr* address, int fd);
private:
	struct sockaddr address;
	int fd;
};

class HTTPServer
{
public:
	HTTPServer(int domain, int type, int port, const char* addr);

	bool bind();
	int listen();
	HTTPRequest* accept();
private:
	int socket_fd;
	int backlog;
};

