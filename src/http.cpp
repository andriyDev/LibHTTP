
#include "http.h"

HTTPServer::HTTPServer(int domain, int port, const char* addr, int backlog)
{
	struct sockaddr_in address;
	address.sin_family = domain;
	address.sin_port = htons(port);
	inet_aton(addr, &address.sin_addr.s_addr);
	
	this->active = true;
	this->socket_fd = socket(domain, SOCK_STREAM, 0);
	this->backlog = backlog;
}

bool HTTPServer::bindSocket()
{
	return bind(socket_fd, (struct sockaddr*)address, sizeof(address)) < 0;
}

int HTTPServer::listenSocket()
{
	return listen(socket_fd, this->backlog);
}

HTTPRequest* HTTPServer::acceptConnection()
{
	struct sockaddr_in request_address;
	int addrlen;
	int fd = accept(socket_fd, (struct sockaddr*)request_address, (socklen_t*)&addrlen);

	// TODO: Parse the request
	
	return new HTTPRequest(request_address, fd);
}

void HTTPServer::acceptToQueue()
{
	HTTPRequest req = this->acceptConnection();
	request_queue_lock.lock();
	
	request_queue.push(req);

	request_queue_lock.unlock();
}

HTTPRequest* HTTPServer::popOffQueue()
{
	request_queue_lock.lock();

	HTTPRequest* req = request_queue[0];
	request_queue.erase(request_queue.begin());

	request_queue_lock.unlock();

	return req;
}

void HTTPServer::closeSocket()
{
	close(this->socket_fd);
}

void ServerThread(void* server_ptr)
{
	HTTPServer* server = (HTTPServer*)server_ptr;
	
	while(true)
	{
		if(server->listenSocket() < 0)
		{
			server->setActive(false);
			break;
		}
		server->acceptToQueue();
	}
	server->closeSocket();
}

HTTPServer* StartServerThread(int domain, int port, const char* addr, int backlog)
{
	HTTPServer* server = new HTTPServer(domain, port, addr, backlog);
#if CMAKE_SYSTEM_NAME == "Windows"
	HANDLE thread;
	thread = CreateThread(NULL, 0, ServerThread, server, 0, NULL);
	if(!thread)
	{
		return nullptr;
	}
	return server;
#elif CMAKE_SYSTEM_NAME == "Linux" || CMAKE_SYSTEM_NAME == "Darwin"
	pthread_t thread;
	if(pthread_create(&thread, NULL, ServerThread, server))
	{
		return nullptr;
	}
	return server;
#endif
}

