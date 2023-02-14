#include <cstring> // memset
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "config.h"

class Client {
public:
  /**
   * @brief Construct a new Client object
   */
  Client() = default;

  /****
   * @brief TCP Init
   * Include
   * 1. init the address of main server
   * 2. init the socket of client
   */
  void tcpInit() {
    // 1. init the socket file descriptor of client
    client_tcp_sock_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (client_tcp_sock_fd_ == -1) {
      std::cout << "Client start tcp socket error!" << std::endl;
      exit(1);
    }
    // 2. init the address of server main
    memset(&server_tcp_sock_addr_, 0, sizeof(server_tcp_sock_addr_));
    server_tcp_sock_addr_.sin_family = AF_INET;
    server_tcp_sock_addr_.sin_addr.s_addr = inet_addr(localhost);
    server_tcp_sock_addr_.sin_port = htons(SERVERMAIN_PORT);
  }

  void bootUp() {
    int ret =
        connect(client_tcp_sock_fd_, (struct sockaddr *)&server_tcp_sock_addr_,
                sizeof(server_tcp_sock_addr_));
    if (ret == -1) {
      std::cout << "Client connect tcp socket error" << std::endl;
      bootDown();
      exit(1);
    }

    // get dynamic tcp port
    struct sockaddr_in client_tcp_sock_addr;
    socklen_t client_tcp_sock_len;
    memset(&client_tcp_sock_addr, 0, sizeof(client_tcp_sock_addr));
    int getsock_check = getsockname(client_tcp_sock_fd_,
                                    (struct sockaddr *)&client_tcp_sock_addr,
                                    (socklen_t *)&client_tcp_sock_len);
    if (getsock_check == -1) {
      std::cout << "Client getsockname error" << std::endl;
      bootDown();
      exit(1);
    }
    dynamic_port_ = ntohs(client_tcp_sock_addr.sin_port);

    std::cout << "Client is up and running." << std::endl;
  }

  /***
   * @brief Send department name query information to main server
   */
  void query() {
    ClientRequst request;
    ClientResponse response;
    char readbuf[BUFSIZE];
    while (true) {
      std::cout << "Enter Department Name: ";
      memset(&request, 0, sizeof(request));
      memset(&response, 0, sizeof(response));
      memset(readbuf, 0, BUFSIZE);
      std::cin >> request.msg;
      send(client_tcp_sock_fd_, &request, sizeof(request), 0);
      std::cout << "Client has sent Department " << request.msg
                << " to Main Server using TCP." << std::endl;
      int size = recv(client_tcp_sock_fd_, readbuf, BUFSIZE, 0);
      if (size <= 0) {
        std::cout << "connection link error!" << std::endl;
        break;
      }
      memcpy(&response, readbuf, sizeof(response));
      if (response.server_id == -1) {
        std::cout << request.msg << " not found." << std::endl;
      } else {
        std::cout << "Client has received results from Main Server: " << std::endl
                  << request.msg << " is associated with backend server "
                  << response.server_id << "." << std::endl;
      }
      std::cout << "-----Start a new query-----" << std::endl;
    }
  }

  void run() {
    tcpInit();
    bootUp();
    query();
  }

  /***
   * @brief release the tcp client resource
   */
  void bootDown() {
    if (client_tcp_sock_fd_ > 0) {
      close(client_tcp_sock_fd_);
    }
  }

  ~Client() { bootDown(); }

private:
  // The socket file descriptor of client
  int client_tcp_sock_fd_{-1};
  // The dynamic port of TCP client
  unsigned int dynamic_port_{0};
  // The address of main server
  struct sockaddr_in server_tcp_sock_addr_;
};


int main() {
  Client client;
  client.run();
  return 0;
}