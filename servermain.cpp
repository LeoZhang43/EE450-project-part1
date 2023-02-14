#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <thread>
#include <unistd.h>
#include <unordered_map>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "config.h"

class ServerMain {
public:
  ServerMain() = default;

  /**
   * @brief Init TCP Socket Network
   */
  void tcpInit() {
    // 1. Init the socket file descriptor of network
    server_tcp_sock_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_tcp_sock_fd_ == -1) {
      std::cout << "ServerMain start TCP socket error!" << std::endl;
      exit(1);
    }
    // 2. Init the network address of this main server
    memset(&server_tcp_addr_, 0, sizeof(server_tcp_addr_));
    server_tcp_addr_.sin_family = AF_INET;
    server_tcp_addr_.sin_port = htons(SERVERMAIN_PORT);
    server_tcp_addr_.sin_addr.s_addr = inet_addr(localhost);
    // 3. Bind the socket file descriptor with network address
    if (bind(server_tcp_sock_fd_, (struct sockaddr *)&server_tcp_addr_,
             sizeof(struct sockaddr)) == -1) {
      std::cout << "ServerMain bind TCP socket error!" << std::endl;
      bootDown();
      exit(1);
    }
    // 4. listen for client to accept
    if (listen(server_tcp_sock_fd_, MAXCONN) == -1) {
      std::cout << "ServerMain listen TCP socket error!" << std::endl;
      bootDown();
      exit(1);
    }
    std::cout << "Main server is up and running." << std::endl;
  }

  /**
   * @brief Load department information from list.txt
   */
  void loadFile() {
    std::ifstream infile("./list.txt", std::ios::in | std::ios::binary);
    if (!infile.is_open()) {
      std::cout << "can't open list.txt" << std::endl;
      exit(1);
    }
    int cnt = 0;
    int serverid = -1;
    std::string line;
    while (safeGetline(infile, line)) {
      // std::cout << line << std::endl;
      cnt += 1;
      if (cnt & 1) {
        serverid = std::atoi(line.c_str());
      } else {
        size_t pos = -1;
        while (!line.empty()) {
          pos = line.find(",");
          std::string dept_name;
          if (pos == std::string::npos) {
            dept_name = line.substr(0);
            line.clear();
          } else {
            dept_name = line.substr(0, pos);
            line = line.substr(pos + 1);
          }
          server_info_[serverid].insert(dept_name);
          table_[dept_name] = serverid;
        }
      }
    }
    infile.close();
    std::cout << "Main server has read the department list from list.txt."
              << std::endl;
    std::cout << "Total num of Backend Servers: " << server_info_.size()
              << std::endl;
    for (const auto &iter : server_info_) {
      std::cout << "Backend Servers " << iter.first << " contains "
                << iter.second.size() << " departments." << std::endl;
    }
  }

  /**
   * @brief Wait connection from Client
   */
  void bootUp() {
    struct sockaddr_in client_tcp_addr;
    socklen_t client_tcp_len = sizeof(client_tcp_addr);
    while (true) {
      int client_tcp_sock_fd =
          accept(server_tcp_sock_fd_, (struct sockaddr *)&client_tcp_addr,
                 &client_tcp_len);
      client_num_ += 1;
      int client_id = client_num_;
      std::thread client_thread{std::mem_fn(&ServerMain::reply), this,
                                client_tcp_sock_fd, client_tcp_addr, client_id};
      client_thread.detach();
    }
  }

  /**
   * @brief Reply the Request of Client
   * 
   * @param client_tcp_sock_fd 
   * @param client_tcp_addr 
   * @param client_id 
   */
  void reply(int client_tcp_sock_fd, struct sockaddr_in client_tcp_addr,
             int client_id) {
    ClientRequst request;
    ClientResponse response;
    char readbuf[BUFSIZE];
    std::string dept_name;
    while (true) {
      memset(&request, 0, sizeof(request));
      memset(&response, 0, sizeof(response));
      memset(readbuf, 0, sizeof(readbuf));
      dept_name.clear();

      int readsize = recv(client_tcp_sock_fd, readbuf, sizeof(readbuf), 0);
      if (readsize <= 0) {
        // std::cout << "client is logout!" << std::endl;
        client_num_ -= 1;
        break;
      }
      memcpy(&request, readbuf, sizeof(request));
      dept_name = request.msg;
      std::cout << "Main server has received the request on Department "
                << dept_name << " from client" << client_id
                << " using TCP over port " << ntohs(server_tcp_addr_.sin_port)
                << std::endl;

      auto iter = table_.find(dept_name);
      if (iter == table_.end()) {
        std::cout << dept_name << " does not show up in backend server ";
        std::string out;
        for (const auto &iter : server_info_) {
          out += std::to_string(iter.first) + ","; 
        }
        out.pop_back();
        std::cout << out << std::endl;
        response.server_id = -1;
      } else {
        std::cout << dept_name << " shows up in backend server " << iter->second
                  << std::endl;
        response.server_id = iter->second;
      }
      send(client_tcp_sock_fd, &response, sizeof(response), 0);
      if (iter == table_.end()) {
        std::cout << "The Main Server has sent \"Department Name: Not found\" "
                     "to client"
                  << client_id << " using TCP over port "
                  << ntohs(server_tcp_addr_.sin_port) << std::endl;
      } else {
        std::cout << "Main Server has sent searching result to client"
                  << client_id << " using TCP over port "
                  << ntohs(server_tcp_addr_.sin_port) << std::endl;
      }
    }
  }

  void run() {
    tcpInit();
    loadFile();
    bootUp();
  }

  /**
   * @brief Release socket file descripor
   */
  void bootDown() {
    if (server_tcp_sock_fd_ > 0) {
      close(server_tcp_sock_fd_);
    }
  }

private:
  // cite:
  // https://stackoverflow.com/questions/6089231/getting-std-ifstream-to-handle-lf-cr-and-crlf
  std::istream &safeGetline(std::istream &is, std::string &line) {
    line.clear();
    std::string myline;
    if (getline(is, myline)) {
      if (myline.size() && myline.back() == '\r') {
        line = myline.substr(0, myline.size() - 1);
      } else {
        line = myline;
      }
    }
    return is;
  }

private:
  // server network address and socket id for client via TCP connection
  int server_tcp_sock_fd_{-1};
  struct sockaddr_in server_tcp_addr_;
  int client_num_{0};
  // store the department information
  std::map<int, std::set<std::string>> server_info_;
  std::unordered_map<std::string, int> table_;
};

int main() {
  ServerMain servermain;
  servermain.run();
  return 0;
}