/**
 * Example Ocean application
 * @author Patrick Kage
 */

#include "../src/web.h"
#include <time.h>

// test handler - returns the requested path
struct Response* test_handler(struct Request* req) {
    struct Response* res = create_response();
    
    // custom resonse packing
    set_response_status(res, 200, "OK");
    add_response_header(res, "Content-Type", "application/json");
    
    // just echo the path
	char* body = malloc(256);
	memset(body, 0, 256);
	sprintf(body, "{\"time\": %ld}", time(0));

    res->body_len = strlen(body);
    res->body = malloc(res->body_len);
    strcpy(res->body, body);

	free(body);
    return res;
}

struct Response* middleware_example(struct Response* res) {
    add_response_header(res, "X-Powered-By", "Ocean");
    return res;
}

void setup(char* static_root, int port) {
	
    struct Rule* table = create_route_table();
	add_route(table, "/api/time", test_handler);
    add_route(table, "/*", handle_static);
    
	struct Middleware* mws = create_middlewares();
    register_middleware(mws, NULL, middleware_example);

	ocean_loop(table, mws, static_root, port);
}
