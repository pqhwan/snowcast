



struct sockaddr_in sa_clientudp;
socklen_t addr_len = sizeof sa_clientudp;
sa_clientudp.sin_family = client_head->ci_family;
sa_clientudp.sin_port = htons(client_head->ci_udp);
sa_clientudp.sin_addr = *((struct in_addr *) gethostbyname(client_head->ci_addr)->h_addr);

//struct sockaddr *addr = client_udp_in? client_udp_in:client_udp_in6;

//Vardec for streaming
ssize_t bytes_sent;
char *filename = "test.text";
char line[512];
while(1){
	FILE *file = fopen(filename, "r");
	memset(line, 0, sizeof line);
	//might be an EOF instead of a NULL
	//read one line and put it into line (nullchar taken care of)
	while( fgets(line, sizeof line, file) != NULL){
		bytes_sent = sendto(udpfd, line, sizeof line, 0,
			(struct sockaddr *)&sa_clientudp, addr_len);
		if(bytes_sent == -1){
			perror("sendto()");
			exit(1);
		}
		sleep(1);
	}
	fclose(file);
}
