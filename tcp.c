
#include "tcp.h"

// READ N BYTES FROM FILE DESCRIPTOR INTO CHAR BUFFER
int custom_read(int socket_fd, char *buf, const int buf_len)
{
	int sum = 0;
	int n;
	while (sum < buf_len)
	{
		n = read(socket_fd, buf+sum, buf_len-sum);
		if (n <= 0) return 0;
		sum += n;
	}
	return sum;
}

// WRITE N BYTES FROM CHAR BUFFER INTO FILE DESCRIPTOR
int custom_write(int socket_fd, char *buf, const int buf_len)
{
	return write(
		socket_fd,
		buf,
		buf_len
	);
}
