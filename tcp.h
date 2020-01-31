
#ifndef _H_TCP
#define _H_TCP

// READ N BYTES FROM FILE DESCRIPTOR INTO CHAR BUFFER
int custom_read(int socket_fd, char *buf, const int buf_len);
// WRITE N BYTES FROM CHAR BUFFER INTO FILE DESCRIPTOR
int custom_write(int socket_fd, char *buf, const int buf_len);

#endif 
