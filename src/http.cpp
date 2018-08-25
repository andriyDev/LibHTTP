
#include "http.h"

HTTPServer::HTTPServer(int domain, int port, const char* addr, int backlog)
{
	struct sockaddr_in address;
	address.sin_family = domain;
	address.sin_port = htons(port);
	inet_aton(addr, &address.sin_addr.s_addr);
	
	this->socket_fd = socket(domain, SOCK_STREAM, 0);
	this->backlog = backlog;
}

bool HTTPServer::bind()
{
	return bind(socket_fd, (struct sockaddr*)address, sizeof(address)) < 0;
}

int HTTPServer::listen()
{
	return listen(socket_fd, 3);
}

HTTPRequest* HTTPServer::accept()
{
	struct sockaddr_in request_address;
	int addrlen;
	int fd = accept(socket_fd, (struct sockaddr*)request_address, (socklen_t*)&addrlen);

	// TODO: Parse the request
	
	return new HTTPRequest(request_address, fd);
}

