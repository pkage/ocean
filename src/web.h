//
//  ocean.h
//  ocean-web
//
//  Created by Patrick Kage on 4/14/18.
//  Copyright © 2018 kagelabs. All rights reserved.
//

#ifndef ocean_h
#define ocean_h

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

// Basic request/response structs
struct Request {
    int method;
    struct Header *headers;
    char* path;
    unsigned int body_len;
    char* body;
};

struct Response {
    unsigned int status;
    char* status_text;
    unsigned long body_len;
    char* body;
    struct Header *headers;
};

// Routing handlers
typedef struct Response* (*Handler)(struct Request*);
struct Rule {
    char* route;
    Handler handler;
    struct Rule* next;
};

struct Rule* create_route_table(void);
void add_route(struct Rule* root, char* route, Handler handler);

// Middleware structure
/*
struct MiddlewareStack;
typedef struct Response* (*Middleware)(struct Request*, struct MiddlewareStack* next);
struct MiddlewareStack {
    Middleware handler;
    struct MiddlewareStack* next;
};

void register_middleware(struct MiddlewareStack* mws, Middleware func);
struct Response* call_next_middleware(struct Request* req, struct MiddlewareStack* mws);
*/
// Middleware
typedef struct Request* (*MiddlewarePre)(struct Request*);
typedef struct Response* (*MiddlewarePost)(struct Response*);
struct Middleware {
    MiddlewarePre pre_handler;
    MiddlewarePost post_handler;
    struct Middleware* next;
};
struct Middleware* create_middlewares(void);
void register_middleware(struct Middleware* mws, MiddlewarePre pre_handler, MiddlewarePost post_handler);


// Response helpers
void set_response_status(struct Response* res, int status, char* text);
struct Response* create_response(void);
void add_response_header(struct Response* res, char* field, char* value);
void set_response_body(struct Response* res, char* body);

void free_request(struct Request* req);
void free_response(struct Response* res);

// Main entry
int ocean_loop(struct Rule* table, struct Middleware* mws, char* static_root, int port);

// debugger
void print_headers(struct Header* header);

// Default handlers
struct Response* handle_static(struct Request* request);

#endif /* ocean_h */
