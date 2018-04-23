/**
 * Networking learning project - sender
 * @author Patrick Kage
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>

int main(int argc, char** argv) {
	printf("setting up structures...\n");

	int status, sockfd, clientfd;
	struct addrinfo hints;
	struct addrinfo *servinfo;
	struct addrinfo *p; // use to crawl later

	// zero out hints struct for getaddrinfo
	memset(&hints, 0, sizeof hints);

	// fill out our hints structure
	hints.ai_family = AF_INET; // specify IPv4 (bc local)
	hints.ai_socktype = SOCK_STREAM; // we'll be talking TCP
	hints.ai_flags = AI_PASSIVE; // don't specify a local IP

	printf("getting address information...\n");
	
	// create the local server
	if ((status = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		exit(2);
	}

	/*
	// print all results
	printf("Server available at: ");
	char ipstr[INET_ADDRSTRLEN];
	for (p = servinfo; p != NULL; p = p->ai_next) {
		void* addr;
		char* ipver;

		struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
		inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);

		printf("%s\n", ipstr);
	}*/

	// create a socket
	printf("creating a socket...\n");
	if ((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1) {
		fprintf(stderr, "creating a socket went wrong!: %s\n", strerror(errno));
		exit(3);
	}

	// binding a socket to a port
	printf("binding socket...\n");
	if ((status = bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen)) == -1) {
		fprintf(stderr, "binding a socket went wrong!: %s\n", strerror(errno));
		exit(4);
		
	}

	printf("socket listening!\n");
	listen(sockfd, 10);

	while (true) {
		struct sockaddr_storage client_addr;
		socklen_t addr_size = sizeof client_addr;
		clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &addr_size);

		char inbuf[1024];

		int recvd;
		if ((recvd = recv(clientfd, &inbuf, 1024, 0)) == -1) {
			fprintf(stderr, "getting request went wrong!: %s\n", strerror(errno));
		}

		printf("%s", inbuf);

		//char* msg = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nhello world\r\n";
		char* msg = "HTTP/1.1 200 OK\x0d\nContent-Type: text/plain\r\nKeep-Alive: none\r\n\r\nhello world!\r\n";
		int sent, len;

		len = strlen(msg);
		//sent = send(clientfd, msg, len, 0);
		if ((sent = send(clientfd, msg, len, 0)) == -1) {
			fprintf(stderr, "sending response went wrong!: %s\n", strerror(errno));
		}
		printf("recieved request, sent back %d/%d bytes\n", sent, len);


		close(clientfd);
		close(sockfd);
		break;
	}

	freeaddrinfo(servinfo);
	return 0;
}
