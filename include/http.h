
#pragma once

#include <netinet/in.h>
#include <vector>
#include <mutex>
#include <string>
#include <map>

#if CMAKE_SYSTEM_NAME == "Windows"
	#include <windows.h>
#elif CMAKE_SYSTEM_NAME == "Linux" || CMAKE_SYSTEM_NAME == "Darwin"
	#include <pthread.h>
#endif

class HTTPRequest
{
public:
	HTTPRequest(struct sockaddr* address, int fd);
private:
	struct sockaddr address;
	int fd;

	std::string method;
	std::string resource;
	std::map<std::string, std::string> attributes;

	friend class HTTPServer;
};

class HTTPServer
{
public:
	HTTPServer(int domain, int port, const char* addr, int backlog);

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
	int backlog;

	std::mutex request_queue_lock;
	std::vector<HTTPRequest*> request_queue;
};

void ServerThread(void* server_ptr);

HTTPServer* StartServerThread(int domain, int port, const char* addr, int backlog);

