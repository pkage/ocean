//
//  ocean.c
//  ocean-web
//
//  Created by Patrick Kage on 4/14/18.
//  Copyright © 2018 kagelabs. All rights reserved.
//

#include "web.h"

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


void send_headers(int fd, struct Header* header);

int create_sock(char* port);
int accept_client(int sockfd);
int parse_request(int clientfd, struct Request *request);


void send_file(int fd, char* path);
size_t get_file_size(FILE* fp, char* path);
bool file_exists(char* path);
void parse_headers(struct Request* req, char* inp);
void parse_header(struct Request* req, char* line);

void send_response(int clientfd, struct Response* res);
struct Request* create_request(void);
void free_request(struct Request* req);
void free_response(struct Response* res);

struct Response* test_handler(struct Request* req);

void add_request_header(struct Request* req, char* field, char* value);

struct Response* handle_route(struct Rule* table, struct Request* req);
bool match_route(char* rule, char* path);

void call_pre_middlewares(struct Request* req, struct Middleware* mws);
void call_post_middlewares(struct Response* res, struct Middleware* mws);

char* g_static_root;

int ocean_loop(struct Rule* table, struct Middleware* mws, char* static_root, int port) {
    printf("setting up structures...\n");
    char port_str[10];
    sprintf(port_str, "%d", port);
    int sockfd = create_sock(port_str);
    
    // static_root
    g_static_root = static_root;
    
    int clientfd;
    while (true) {
        clientfd = accept_client(sockfd);
        struct Request* req = create_request();
        
        
        parse_request(clientfd, req);
        call_pre_middlewares(req, mws);
        
        struct Response* res = handle_route(table, req);
        
        call_post_middlewares(res, mws);
        
        send_response(clientfd, res);
        print_headers(res->headers);
        
        free_request(req);
        free_response(res);
        close(clientfd);
    }
    //close(sockfd);
    
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
    
    size_t  inbuf_size = 1024;
    char* inbuf = (char*)malloc(inbuf_size);
    memset(inbuf, 0, inbuf_size);
    char buf[1024];
    //char inbuf[1024];
    size_t recvd = -1;
    size_t offset = 0;
    
    do  {
        if (offset >= inbuf_size) {
            inbuf_size += 1024;
            inbuf = realloc(inbuf, inbuf_size);
        }
        
        recvd = recv(clientfd, &buf, 1024, 0);
        memcpy(inbuf + offset, buf, 1024);
        offset += recvd;
        if (recvd < 1024) break;
    } while (recvd > 0);
    
    if (recvd == -1) {
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
    strncpy(path, start + 1, end - start);
    path[end - start - 1] = '\0';
    
    request->path = malloc(strlen(path) + 1);
    memcpy(request->path, path, strlen(path) + 1);
    free(path);
    
    //char* bodystart = strstr(inbuf, "\r\n\r\n");
    
    parse_headers(request, eol + 1);
    print_headers(request->headers);
    
    
    printf("requested path '%s'\n", request->path);
    
    // Headers?
    // skip for now
    printf("%s\n", inbuf);
    //request->headers = NULL;
    
    free(inbuf);
    
    return 0;
}

void free_request(struct Request* req) {
    printf("freeing request...\n");
    
    // clear the path
    free(req->path);
    printf("path freed\n");
    
    // free the linked header list
    struct Header *next;
    while (req->headers != NULL) {
        free(req->headers->field);
        free(req->headers->value);
        next = req->headers->next;
        free(req->headers);
        req->headers = next;
        
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

struct Response* handle_static(struct Request* req) {
    // Handle a static request
    char* path = malloc(strlen(req->path) + 11 + strlen(g_static_root));
    path[0] = '.';
    strcpy(path, g_static_root);
    strcpy(path + strlen(g_static_root), req->path);
    printf("serving %s\n", path);
    if (path[strlen(path) - 1] == '/') {
        strcat(path, "index.html");
    }
    
    
    struct Response* res = create_response();
    FILE* fp = fopen(path, "r");
    if (!fp) {
        set_response_status(res, 404, "Not Found");
        add_response_header(res, "Content-Type", "text/plain");
        set_response_body(res, "Not found!");
    } else {
        set_response_status(res, 200, "OK");
        //add_response_header(res, "Content-Type", "text/plain");
        
        // prepare body
        res->body_len = get_file_size(fp, NULL);
        res->body = malloc(res->body_len);
        fread(res->body, res->body_len, 1, fp);
    }
    fclose(fp);
    free(path);
    add_response_header(res, "Connection", "close");
    return res;
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

void add_request_header(struct Request* req, char* field, char* value) {
    
    if (!req->headers){
        req->headers = malloc(sizeof(struct Header));
        memset(req->headers, 0, sizeof(struct Header));
    }
    
    // march the header to the end of the linked list
    struct Header* last = req->headers;
    if (last->next) {
        while (last->next != NULL) {
            last = last->next;
        }
    }
    
    last->next = malloc(sizeof(struct Header));
    memset(last->next, 0, sizeof(struct Header));
    last->next->next = NULL;
    
    // set the field and value
    last->next->field = field;
    last->next->value = value;
    
    if (!req->headers->field) {
        struct Header* tofree = req->headers;
        req->headers = req->headers->next;
        free(tofree);
    }
}

void add_response_header(struct Response* res, char* field, char* value) {
    if (!res->headers){
        res->headers = malloc(sizeof(struct Header));
        memset(res->headers, 0, sizeof(struct Header));
    }
    
    // march the header to the end of the linked list
    struct Header* last = res->headers;
    if (last->next) {
        while (last->next != NULL) {
            last = last->next;
        }
    }
    
    last->next = malloc(sizeof(struct Header));
    memset(last->next, 0, sizeof(struct Header));
    last->next->next = NULL;
    
    // set the field and value
    last->next->field = malloc(strlen(field) + 1);
    last->next->value = malloc(strlen(value) + 1);
    strcpy(last->next->field, field);
    strcpy(last->next->value, value);
    
    if (!res->headers->field) {
        struct Header* tofree = res->headers;
        res->headers = res->headers->next;
        free(tofree);
    }
}

void parse_header(struct Request* req, char* line) {
    char* midpoint = strchr(line, ':');
    if (midpoint == NULL) {
        printf("something has gone terribly wrong\n");
    }
    char* field = malloc(midpoint - line + 1);
    char* value = malloc(strlen(midpoint) + 1);
    strncpy(field, line, midpoint - line);
    strcpy(value, midpoint + 2);
    field[midpoint - line] = '\0';
    value[strlen(midpoint)] = '\0';
    
    //printf("\tprocessed header (%s, %s)\n", field, value);
    add_request_header(req, field, value);
}

void parse_headers(struct Request* req, char* inp) {
    char* headerend = strstr(inp, "\r\n\r\n");
    inp[headerend - inp] = '\0';
    // strtok doesn't seem to work reliably
    int begin = 1;
    for (int c = 0; c < (headerend - inp); c++) {
        if (inp[c] == '\r') {
            inp[c] = '\0';
            parse_header(req, inp + begin);
            begin = c + 2;
        }
    }
    parse_header(req, inp + begin);
}

void print_headers(struct Header* header) {
    printf("Headers: \n");
    while (header != NULL) {
        printf("\t(%s, %s)\n", header->field, header->value);
        header = header->next;
    }
}

struct Request* create_request() {
    struct Request* req = malloc(sizeof(struct Request));
    memset(req, 0, sizeof(struct Request));
    
    return req;
}

struct Response* create_response() {
    struct Response* res = malloc(sizeof(struct Response));
    memset(res, 0, sizeof(struct Response));
    res->headers = NULL;
    return res;
}

void free_response(struct Response* res) {
    struct Header *next;
    while (res->headers != NULL) {
        free(res->headers->field);
        free(res->headers->value);
        next = res->headers->next;
        free(res->headers);
        res->headers = next;
    }
    
    free(res->status_text);
    free(res->body);
    free(res);
}

void set_response_status(struct Response* res, int status, char* text) {
    res->status = status;
    res->status_text = malloc(strlen(text));
    strcpy(res->status_text, text);
}

void set_response_body(struct Response* res, char* body) {
    res->body_len = strlen(body);
    res->body = malloc(res->body_len);
    strcpy(res->body, body);
}

void send_response(int clientfd, struct Response* res) {
    char* statusline = malloc(9 + 3 + strlen(res->status_text) + 3);
    sprintf(statusline, "HTTP/1.1 %d %s\r\n", res->status, res->status_text);
    
    // send the status line
    send(clientfd, statusline, strlen(statusline), 0);
    
    // send the headers & separator
    send_headers(clientfd, res->headers);
    
    // send the rest of the body
    send(clientfd, res->body, res->body_len, 0);
    
    free(statusline);
}


// Routing table
struct Rule* create_route_table() {
    struct Rule* table = malloc(sizeof(struct Rule));
    memset(table, 0, sizeof(struct Rule));
    return table;
}

void add_route(struct Rule* root, char* route, Handler handler) {
    struct Rule* target = root;
    
    // if target's path is NULL, the table is brand new
    if (target->route != NULL) {
        while (target->next != NULL) {
            target = target->next;
        }
        
        target->next = malloc(sizeof(struct Rule));
        memset(target->next, 0, sizeof(struct Rule));
        target = target->next;
    }
    
    target->handler = handler;
    target->route = malloc(strlen(route));
    strcpy(target->route, route);
}

struct Response* handle_route(struct Rule* table, struct Request* req) {
    while (table != NULL) {
        if (match_route(table->route, req->path)) {
            return table->handler(req);
        }
        table = table->next;
    }
    return NULL;
}

bool match_route(char* rule, char* path) {
    size_t pathlen = strlen(path);
    size_t rulelen = strlen(rule);
    
    for (int c = 0; c < rulelen; c++) {
        if (c >= pathlen) {
            if (rule[c] == '/' && c + 1 == rulelen) {
                return true;
            } else if (rule[c - 1] == '/' && rule[c] == '*') {
                return true;
            } else {
                return false;
            }
        }
        
        if (rule[c] == '*') {
            return true;
        }
        
        if (path[c] != rule[c]) {
            return false;
        }
    }
    return true;
}

void free_table(struct Rule* table) {
    while (table != NULL) {
        free(table->route);
        table = table->next;
    }
}

// Middleware functions
/*
struct MiddlewareStack* create_middlewares(void) {
    struct MiddlewareStack* mws = malloc(sizeof(struct MiddlewareStack));
    memset(mws, 0, sizeof(struct MiddlewareStack));
    return mws;
}

void register_middleware(struct MiddlewareStack* mws, Middleware handler) {
    if (mws->handler == NULL) {
        mws->handler = handler;
    } else {
        while (mws->next != NULL) {
            mws = mws->next;
        }
        mws->next = create_middlewares();
        mws->next->handler = handler;
    }
}

struct Response* call_next_middleware(struct Request* req, struct MiddlewareStack* mws) {
    return mws->handler(req, mws->next);
}*/
struct Middleware* create_middlewares(void) {
    struct Middleware* mws = malloc(sizeof(struct Middleware));
    memset(mws, 0, sizeof(struct Middleware));
    return mws;
}

void register_middleware(struct Middleware* mws, MiddlewarePre pre_handler, MiddlewarePost post_handler) {
    if (mws->pre_handler == NULL && mws->post_handler == NULL) {
        mws->pre_handler = pre_handler;
        mws->post_handler = post_handler;
    } else {
        while (mws->next != NULL) {
            mws = mws->next;
        }
        mws->next = create_middlewares();
        mws->next->pre_handler = pre_handler;
        mws->next->post_handler = post_handler;
    }
}

void call_pre_middlewares(struct Request* req, struct Middleware* mws) {
    while (mws != NULL) {
        if (mws->pre_handler) {
            req = mws->pre_handler(req);
        }
        mws = mws->next;
    }
}
void call_post_middlewares(struct Response* res, struct Middleware* mws) {
    while (mws != NULL) {
        if (mws->post_handler) {
            res = mws->post_handler(res);
        }
        mws = mws->next;
    }
}
