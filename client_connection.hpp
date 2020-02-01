
#ifndef _H_CLIENT_CONNECTION
#define _H_CLIENT_CONNECTION

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
//#include <arpa/inet.h>
//#include <sys/stat.h>

class Server;

#define ADMIN_KEY 3000000
#define BUFFER_LEN 256

int custom_read(int socket_fd, char *buf, const int buf_len);

// First 4 bytes - message type
enum MESSAGE_TYPE
{
	MTYPE_RESPONSE_OK = 30,
	
	MTYPE_DISCONNECT,
	MTYPE_TEXT,
	
	MTYPE_ERROR_NAME,
	MTYPE_ERROR_NUM,
	MTYPE_ERROR_RIGHTS,
	MTYPE_ERROR_DISCONNECT,
	MTYPE_ERROR_COMMAND,
	MTYPE_ERROR_PRICE
};

enum STATE
{
	STATE_WORKING,  // Thread is active - listening client messages
	STATE_FINISHED  // CLient disconnected
};

enum CLIENT_STATUS
{
	CSTATUS_USER,
	CSTATUS_ADMIN
};

class Client_connection
{
private:
	Server *server;
	pthread_t client_thread;
	pthread_mutex_t     mutex_client_socket;
	pthread_mutexattr_t matr_client_socket;
	
	static void* thread_start(void *inst);
	
public:
	STATE state;
	const int socket_fd;
	sockaddr_in addr;
	CLIENT_STATUS status;
	const std::string id;
	
	
	Client_connection(Server *server, const int socket_fd, const sockaddr_in addr, const CLIENT_STATUS status, const std::string id);
	~Client_connection(void);
	static std::string get_socket_str(Client_connection *instance);
	void send(const std::string &message);
};

#endif
