/**
 * Networking learning project - static server
 * @author Patrick Kage
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>


// HTTP methods enum
enum {
    HTTP_METHOD_UNKNOWN,
    HTTP_METHOD_GET,
    HTTP_METHOD_POST,
    HTTP_METHOD_PUT,
    HTTP_METHOD_PATCH,
    HTTP_METHOD_DELETE,
    HTTP_METHOD_OPTIONS,
    HTTP_METHOD_HEAD
};

struct Header {
    char* field;
    char* value;
    struct Header *next;
};

struct Request {
    int method;
    struct Header *headers;
    char* path;
    unsigned int body_len;
    char* body;
};

struct Response {
    unsigned int status;
    unsigned int body_len;
    char* body;
    struct Header *headers;
};

void send_headers(int fd, struct Header* header);

int create_sock(char* port);
int accept_client(int sockfd);
int parse_request(int clientfd, struct Request *request);

void handle_static(int clientfd, char* path);

void free_request(struct Request* req);
void send_file(int fd, char* path);
size_t get_file_size(FILE* fp, char* path);
bool file_exists(char* path);
void parse_headers(struct Request* req, char* inp);
void parse_header(struct Request* req, char* line);
void print_headers(struct Request* req);
struct Request* create_request();

int main(int argc, char** argv) {
    
    if (argc != 2) {
        printf("missing required parameter \"port\"\n");
        return 1;
    }
    
    printf("setting up structures...\n");
    int sockfd = create_sock(argv[1]);
    
    int clientfd;
    while (true) {
        clientfd = accept_client(sockfd);
        struct Request* req = create_request();
        
        parse_request(clientfd, req);
        
        // check if file exists
        char* headers;
        bool willsend;
        printf("attempting to open '%s'...\n", req->path);
        if ( access( req->path, R_OK ) != -1 ) {
            headers = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nKeep-Alive: none\r\n\r\n";
            willsend = true;
            printf("File OK: %s\n", req->path);
        } else {
            headers = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nKeep-Alive: none\r\n\r\nNot found!\r\n";
            willsend = false;
            printf("File not opened! %s\n", strerror(errno));
        }
        
        size_t sent, len;
        len = strlen(headers);
        if ((sent = send(clientfd, headers, len, 0)) == -1) {
            fprintf(stderr, "sending headers went wrong!: %s\n", strerror(errno));
        }
        printf("recieved request, sent back %ld/%ld bytes of headers\n", sent, len);
        
        if (willsend == true) {
            // write the file into the fd
            printf("sending file\n");
            send_file(clientfd, req->path);
        }
        
        free_request(req);
        req = NULL;
        close(clientfd);
    }
    // close(sockfd);
    
    return 0;
}

int create_sock(char* port) {
    int status, sockfd;
    struct addrinfo hints;
    struct addrinfo *servinfo;
    
    // zero out hints struct for getaddrinfo
    memset(&hints, 0, sizeof hints);
    
    // fill out our hints structure
    hints.ai_family = AF_INET; // specify IPv4 (bc local)
    hints.ai_socktype = SOCK_STREAM; // we'll be talking TCP
    hints.ai_flags = AI_PASSIVE; // don't specify a local IP
    
    printf("getting address information...\n");
    
    // create the local server
    if ((status = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
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
     }
    */
    
    // create a socket
    printf("creating a socket...\n");
    if ((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1) {
        fprintf(stderr, "creating a socket went wrong!: %s\n", strerror(errno));
        exit(3);
    }
    
    // set socket option to be able to snag a port off the kernel
    // if it's still being held
    int yes = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
        fprintf(stderr, "setsockopt failed: %s\n", strerror(errno));
        exit(4);
    }
    
    // binding the socket to a port
    printf("binding socket...\n");
    if ((status = bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen)) == -1) {
        fprintf(stderr, "binding a socket went wrong!: %s\n", strerror(errno));
        exit(4);
    }
    
    printf("socket listening!\n");
    listen(sockfd, 10);
    freeaddrinfo(servinfo);
    
    
    return sockfd;
}

int accept_client(int sockfd) {
    struct sockaddr_storage client_addr;
    socklen_t addr_size = sizeof client_addr;
    return accept(sockfd, (struct sockaddr*)&client_addr, &addr_size);
}


int parse_request(int clientfd, struct Request *request) {
    //char* inbuf = (char*)malloc(1024);
    char inbuf[1024];
    size_t recvd = -1;
    
    // TODO: make this better ( >1024 byte payloads )
    if ((recvd = recv(clientfd, &inbuf, 1024, 0)) == -1) {
        fprintf(stderr, "getting request went wrong: %s\n", strerror(errno));
    }
    
    
    // First, determine the method used
    int method = HTTP_METHOD_UNKNOWN;
    if (strncmp(inbuf, "GET", 3) == 0) {
        method = HTTP_METHOD_GET;
    } else if (strncmp(inbuf, "POST", 4) == 0) {
        method = HTTP_METHOD_POST;
    } else if (strncmp(inbuf, "PUT", 3) == 0) {
        method = HTTP_METHOD_PUT;
    } else if (strncmp(inbuf, "PATCH", 5) == 0) {
        method = HTTP_METHOD_PATCH;
    } else if (strncmp(inbuf, "DELETE", 6) == 0) {
        method = HTTP_METHOD_PATCH;
    } else if (strncmp(inbuf, "OPTIONS", 7) == 0) {
        method = HTTP_METHOD_PATCH;
    } else if (strncmp(inbuf, "HEAD", 4) == 0) {
        method = HTTP_METHOD_PATCH;
    }
    request->method = method;
    printf("method determined to be: %d\n", method);
    
    // fail unknown methods
    if (method == HTTP_METHOD_UNKNOWN) {
        return 1;
    }
    
    // Next, extract the path:
    //
    // GET /foo/bar.html HTTP/1.1\r\n
    //     ^^^^^^^^^^^^^
    //    s             e
    char *start, *end, *eol, *path;
    start = strchr(inbuf, ' ');
    end = strchr(start + 1, ' ');
    eol = strchr(inbuf, '\r');
    path = malloc(eol - inbuf);
    *start = '.'; // dirty hack to append a '.' to the beginning
    strncpy(path, start, end - start);
    path[end - start] = '\0';
    // if the path ends with a '/', just slap an "/index.html" onto that
    // TODO: replace this with proper static routing
    if (path[strlen(path) - 1] == '/') {
        strcat(path, "index.html");
    }
    
    request->path = malloc(strlen(path) + 1);
    memcpy(request->path, path, strlen(path) + 1);
    free(path);
    
    //char* bodystart = strstr(inbuf, "\r\n\r\n");
    
    parse_headers(request, eol + 1);
    print_headers(request);

    
    printf("requested path '%s'\n", request->path);
    
    // Headers?
    // skip for now
    printf("%s\n", inbuf);
    request->headers = NULL;
    
    return 0;
}

void free_request(struct Request* req) {
    printf("freeing request...\n");
    
    // clear the path
    free(req->path);
    printf("path freed\n");
    
    // free the linked header list
    struct Header *next, *track = req->headers;
    printf("got Header pointer: %p\n", track);
    while (track != NULL) {
        if (track && track->next) {
            next = track->next;
            printf("\tnext pointer: %p\n", track);
            free(track->field);
            free(track->value);
            free(track);
            track = next;
        } else {
            free(track);
        }
    }
    
    // freed full request object
    free(req);
}

void send_file(int fd, char* path) {
    printf("sending file '%s'\n", path);
    
    // file contents
    FILE* fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "File failed to open: %s\n", strerror(errno));
        exit(5);
    }
    
    // size
    size_t size = get_file_size(fp, NULL);
    
    printf("preparing to send %ld bytes of '%s'...\n", size, path);
    
    // stream into fd
    size_t c, len, total = 0;
    while ((c = fgetc(fp)) != EOF) {
        if ((len = send(fd, &c, 1, 0)) == -1) {
            fprintf(stderr, "sending body went wrong!: %s\n", strerror(errno));
        } else {
            total += len;
        }
    }
    
    printf("Sent %ld/%ld bytes of body payload\n", total, size);
    
    // close the file pointer
    fclose(fp);
}

size_t get_file_size(FILE* fp, char* path) {
    // if there isn't a pointer supplied, there better be a path
    if (!fp) {
        
        if (!path) {
            fprintf(stderr, "No file pointer or path specified to get_file_size!\n");
        }
        fp = fopen(path, "r");
        if (!fp) {
            fprintf(stderr, "File failed to open: %s\n", strerror(errno));
            return 0;
        }
    }
    
    // fastforward to end, read position, rewind
    fseek(fp, 0, SEEK_END);
    size_t position = ftell(fp);
    fseek(fp, 0, 0);
    
    return position;
}

void handle_static(int clientfd, char* path) {
    // Handle a static request
    
}

void send_headers(int fd, struct Header* header) {
    // For each header, dump "field: value\r\n" onto the file pointer
    while (header != NULL) {
        send(fd, header->field, strlen(header->field), 0);
        send(fd, ": ", 2, 0);
        send(fd, header->value, strlen(header->value), 0);
        send(fd, "\r\n", 2, 0);
        header = header->next;
    }

    // Finish the header block
    send(fd, "\r\n", 2, 0);
}

void add_header(struct Request* req, char* field, char* value) {
    
    if (!req->headers) {
        req->headers = malloc(sizeof(struct Header));
        memset(req->headers, sizeof(struct Header), 0);
        req->headers->next = NULL;
    }

    // march the header to the end of the linked list
    struct Header* last = req->headers;
    if (last->next) {
        while (last->next != NULL) {
            last = last->next;
        }
    }
    
    last->next = malloc(sizeof(struct Header));
    memset(last->next, sizeof(struct Header), 0);
    last->next->next = NULL;
    
    // set the field and value
    last->next->field = field;
    last->next->value = value;
    //memcpy(last->field, field, strlen(field));
    //memcpy(last->value, value, strlen(value));
}

void parse_header(struct Request* req, char* line) {
    char* midpoint = strchr(line, ':');
    char* field = malloc(midpoint - line);
    char* value = malloc(strlen(midpoint));
    strncpy(field, line, midpoint - line);
    strcpy(value, midpoint + 2);
    
    add_header(req, field, value);
}

void parse_headers(struct Request* req, char* inp) {
    //char* headerend = strstr(inp, "\r\n\r\n");
    char* token = strtok(inp, "\r\n");
    while(token) {
        if (strlen(token) == 0) break;
        parse_header(req, token);
        token = strtok(NULL, "\r\n");
    }
    // first header entry is gonna be null
    struct Header* tofree = req->headers;
    req->headers = req->headers->next;
    free(tofree);
}

void print_headers(struct Request *req) {
    struct Header* header = req->headers;
    printf("Extracted headers: \n");
    while (header != NULL) {
        printf("\t(%s, %s)\n", header->field, header->value);
        header = header->next;
    }

}

struct Request* create_request() {
    struct Request* req = malloc(sizeof(struct Request));
    memset(req, sizeof(struct Request), 0);
    req->headers = NULL;
    return req;
}
