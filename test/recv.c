/**
 * Networking learning project - receiver
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
	fprintf(stderr, "setting up structures...\n");

	int status, sockfd;
	struct addrinfo hints;
	struct addrinfo *servinfo;
	struct addrinfo *p; // use to crawl later

	// zero out hints struct for getaddrinfo
	memset(&hints, 0, sizeof hints);

	// fill out our hints structure
	hints.ai_family = AF_INET; // specify IPv4 (bc local)
	hints.ai_socktype = SOCK_STREAM; // we'll be talking TCP
	hints.ai_flags = AI_PASSIVE; // don't specify a local IP

	fprintf(stderr, "getting address information...\n");
	
	// create the local server
	if ((status = getaddrinfo("localhost", "8080", &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		exit(2);
	}

	/*
	// print all results
	fprintf(stderr, "Server available at: ");
	char ipstr[INET_ADDRSTRLEN];
	for (p = servinfo; p != NULL; p = p->ai_next) {
		void* addr;
		char* ipver;

		struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
		inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);

		fprintf(stderr, "%s\n", ipstr);
	} */

	// create a socket
	fprintf(stderr, "creating a socket...\n");
	if ((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1) {
		fprintf(stderr, "creating a socket went wrong!: %s\n", strerror(errno));
		exit(3);
	}

	// connecting socket
	if (connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen)) {
		fprintf(stderr, "creating a socket went wrong!: %s\n", strerror(errno));
		exit(4);
	}


	char* request = "GET / HTTP/1.1\r\n\r\n";
	int len = send(sockfd, request, strlen(request), 0);
	fprintf(stderr, "sent %d/%d bytes of request\n", len, (int)strlen(request));

	char buf[2];
	while ((len = recv(sockfd, buf, 1, 0)) != 0) {
		//printf("%d\n", len);
		putchar(buf[0]);
		fflush(stdout);
	}

	close(sockfd);

	freeaddrinfo(servinfo);

	return 0;
}
