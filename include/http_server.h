
#pragma once

#include <netinet/in.h>
#include <vector>
#include <mutex>
#include <string>
#include <map>

#ifdef windows
	#include <windows.h>
#endif
#ifdef posix
	#include <pthread.h>
#endif

class HTTPRequest
{
public:
	HTTPRequest(struct sockaddr_in address, int fd)
	{
		this->address = address;
		this->fd = fd;
	}

	int get_fd() { return fd; }

	std::string method;
	std::string endpoint;
	std::map<std::string, std::string> attributes;
	std::map<std::string, std::string> header;
private:
	struct sockaddr_in address;
	int fd;

	friend class HTTPServer;
};

class HTTPServer
{
public:
	HTTPServer(int domain, int port, int backlog);

	bool bindSocket();
	int listenSocket();
	HTTPRequest* acceptConnection();
	void acceptToQueue();
	void closeSocket();

	HTTPRequest* popOffQueue();

	void setActive(bool active) { this->active = active; }
	bool isActive() { return active; }
private:
	bool active;
	int socket_fd;
	struct sockaddr_in address;

	int backlog;

	std::mutex request_queue_lock;
	std::vector<HTTPRequest*> request_queue;
};

void ServerThread(void* server_ptr);

HTTPServer* StartServerThread(int domain, int port, int backlog);

