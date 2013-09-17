int socket_setup(struct addrinfo hints, char *port, char *host);
void *get_in_addr(struct sockaddr *sa);
void packi16(unsigned char *buf, unsigned int i);
void packi8(unsigned char *buf, unsigned char i);
unsigned int pack(unsigned char *buf, char *format, ...);
void unpack(unsigned char *buf, char *format, ...);
