#pragma once

#define SERVERMAIN_PORT 33319
#define localhost "127.0.0.1"
#define BUFSIZE 128 
#define MAXCONN 5

struct ClientRequst {
  char msg[32];
};

struct ClientResponse {
  int server_id; // -1 represents not found
};
