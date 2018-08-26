
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

std::string get_next_token(std::string* str, char delimeter, int offset_after_delim = 1)
{
	int token_end = str->find(delimeter);
	std::string s = str->substr(0, token_end);
	str = str->substr(token_end + offset_after_delim);
	return s;
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
	char* request_line_c = read_line(f);
	std::string request_line(request_line_c);
	delete[] request_line_c;
	
	// Find and consume the method token.
	req->method = get_next_token(&request_line, ' ');
	// Find and consume the requestURI token.
	std::string requestURI = get_next_token(&request_line, ' ');
	req->endpoint = get_next_token(&requestURI, '?');
	// Read all the attributes.
	while(requestURI != "")
	{
		// Read the attribute name.
		std::string attribute_name = get_next_token(&requestURI, '=');
		// Read the attribute val.
		std::string attribute_val = get_next_token(&requestURI, '&');
		// Assign the attribute.
		req->attributes[attribute_name] = attribute_val;
	}
	
	// We shall ignore the HTTP version for now, since we only really care about supporting HTTP/1.1
	
	char* line_c;
	// As long as we have a valid line, and the line isn't just "\r\n\0", keep reading the header.
	while((line_c = read_line(f)) && line_c[2] != '\0')
	{
		// Create a c++ string from the C string.
		std::string line = line_c;
		delete[] line_c;
		// Read the header name.
		std::string header_name = get_next_token(&line, ':', 2);
		// Read the header val.
		std::string header_val = get_next_token(&line, '\r', 2);
		// Assign the header.
		req->header[header_name] = header_val;
	}
	// If line_c is valid, then it must have been the "\r\n\0" line, so we still have to clear it.
	if(line_c)
	{
		delete[] line_c;
	}

	// We will also leave the remaining data to be read from separately. That way incoming data can be
	// parsed without needing to read in the entire file. Yay for good design!

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

