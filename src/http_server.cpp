
#include "http_server.h"

#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>

HTTPServer::HTTPServer(int domain, int port, int backlog)
{
	this->address.sin_family = domain;
	this->address.sin_addr.s_addr = INADDR_ANY;
	this->address.sin_port = htons(port);
	
	this->active = true;
	this->socket_fd = socket(domain, SOCK_STREAM, 0);
	this->backlog = backlog;
}

bool HTTPServer::bindSocket()
{
	return bind(socket_fd, (struct sockaddr*)&this->address, sizeof(this->address)) >= 0;
}

int HTTPServer::listenSocket()
{
	return listen(socket_fd, this->backlog);
}

char* read_line(FILE* f)
{
	// Store all the buffers that we'll read.
	std::vector<char*> bufs;
	// Keep track of the total number of bytes.
	int total_bytes = 0;
	// Keep reading buffers until we read an entire line.
	while(true)
	{
		// Create a new buffer
		char* buf = new char[256];
		// Add the buffer to our vector
		bufs.push_back(buf);
		// Try to read a line.
		if(!fgets(buf, 256, f))
		{
			break;
		}
		// Get the length of the buffer.
		int bytes_read = strlen(buf);
		// Add to the total number of bytes.
		total_bytes += bytes_read;
		// If we have hit a \n, we need to stop. So if we've read fewer than 255 characters,
		// or the last character is a \n
		if(bytes_read != 255 || buf[254] == '\n')
		{
			break;
		}
	}
	// Create a single character buffer for the entire line (include the null terminator)
	char* line = new char[total_bytes + 1];
	// The last byte should be a null terminator.
	line[total_bytes] = '\0';
	// Add a pointer to the current line.
	char* line_curr = line;
	// Go through each buffer.
	for(char* buf : bufs)
	{
		// Copy the string from the buffer to the current line.
		strcpy(line_curr, buf);
		// Move the line forward by the number of bytes in the buffer.
		line_curr += strlen(buf);
		// Delete the buffer.
		delete[] buf;
	}
	// Return the line.
	return line;
}

HTTPRequest* HTTPServer::acceptConnection()
{
	// Define vars.
	struct sockaddr_in request_address;
	int addrlen;
	// Accept the connection.
	int fd = accept(socket_fd, (struct sockaddr*)&request_address, (socklen_t*)&addrlen);

	// Create the request object.
	HTTPRequest* req = new HTTPRequest(request_address, fd);
	// Convert the file descriptor into a FILE object
	FILE* f = fdopen(fd, "rb");
	// Read the line
	char* request_line = read_line(f);

	// Set the current index pointer.
	char* curr = request_line;

	// Find the first space.
	char* delim = strchr(curr, ' ');
	// Get the length of the method.
	int len = delim - curr;
	// Create a buffer for the method.
	char* method = new char[len + 1];
	// Add the null terminator.
	method[len] = '\0';
	// Copy the bytes to the new method buffer.
	std::memcpy(method, curr, len);
	// Move the pointer to after the space.
	curr = delim + 1;

	// Find the next space.
	delim = strchr(curr, ' ');
	// Get the length of the Request-URI.
	len = delim - curr;
	// Create a buffer for the Request-URI.
	char* requestURI = new char[len + 1];
	// Add the null terminator.
	requestURI[len] = '\0';
	// Copy the bytes to the new requestURI buffer.
	std::memcpy(requestURI, curr, len);
	// Move the pointer to after the space.
	curr = delim + 1;

	// We shall ignore the HTTP version for now, since we only really care about supporting HTTP/1.1
	// We will also leave the remaining data to be read from separately. That way incoming data can be
	// parsed without needing to read in the entire file. Yay for good design!
	
	req->method = method;
	req->resource = requestURI;

	delete[] method;
	delete[] requestURI;

	return req;
}

void HTTPServer::acceptToQueue()
{
	HTTPRequest* req = this->acceptConnection();
	request_queue_lock.lock();
	
	request_queue.push_back(req);

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

HTTPServer* StartServerThread(int domain, int port, int backlog)
{
	HTTPServer* server = new HTTPServer(domain, port, backlog);
#ifdef windows
	HANDLE thread;
	thread = CreateThread(NULL, 0, ServerThread, server, 0, NULL);
	if(!thread)
	{
		return nullptr;
	}
	return server;
#endif 
#ifdef posix
	pthread_t thread;
	if(pthread_create(&thread, NULL, ServerThread, server))
	{
		return nullptr;
	}
	return server;
#endif
}

