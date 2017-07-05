#ifndef STAFF_H
#define STAFF_H

int setnonblock (int fd);
int unix_socket_init (struct sockaddr_un* socket_un, char* sock_path);
int server_unix_init (struct ev_server * server, char * sock_path, int max_queue);
int inet_socket_init (struct sockaddr_in * socket_in, uint16_t port);
int server_inet_init (struct ev_server * server, uint16_t port, int max_queue);

#endif
