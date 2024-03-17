/**
@author hershalpandya@gmail.com
*/
#pragma once

// These are all OS libraries. No equivalent in C++.
#include <sys/socket.h> // ip protocols for eg int values for AF_INET defined here.
#include <netdb.h> //has getaddrinfo(),struct addrinfo,etc.
#include <sys/fcntl.h> // fcntl(), O_NONBLOCK,etc
#include <sys/select.h> //select(), FD_SET(), etc.

/* delete the following library invocations if not used.
#include <sys/types.h> //definition of certain types size_t, useconds_t, etc.
*/

// For the sake of consistency, use C++ libraries only.
#include <string>


/*
* @class SOCKET_CALLS
* @brief manages sockets for TCP/IP Streaming
* This class is called by all the PCs (LORA MAIN / LASA PCs)
* Each PC will instantiate the class as per need:
** For LORA MAIN
*** 1 instance for communicating with LOFAR SERVER
*** 10 instances for communicating with 10 LASA CLIENTS
*** (There is a need to write a separate method for LORA to loop over all LASA PCs. Need to think where it goes in the organization.)
** For LASA PC
*** 1 instance to communicate with LORA server
*** 2/1 (old LASA/new LASA) instances to communicate with digitizer(s)
* General sequence for each instance will follow:
** Step 1: Load SOCKET INFO (machine name, port)
the IPV will be hardcoded as IPV4. and socktype to streaming socktype.
** Step 2:
*/

class SOCKET_CALLS
{
public:
  // sc_active_socfd will store the socket each module should operate on
  // only real use is, on the server side, when accept happens, the sockfd
  // is switched from initial sockfd to listening sockfd
  int sc_active_sockfd=0; // public to check status of fds.
  //to Accept() another connection from the same socket.
  int sc_active_spare_sockfd=0;

  //add a default constructor
  SOCKET_CALLS() {}
  //result: populates sc_addr, sc_machine_name, sc_port_string
  SOCKET_CALLS(const std::string&, const std::string&, bool);

  //Methods For Clients and Servers
  void Open();
  int Receive(unsigned char*, size_t, int&, bool); //give references to data buffer (to store o/p from this method) & size of msg received (to store o/p from this method) & an int to hold errno given by read() method.
  void Send(const unsigned char*, size_t, int&, bool); //give references to data (as i/p to this method) and size of sent msg(to store o/p from method).
  void Close();

  //Methods For Servers
  void Bind();
  void Listen();
  void Accept();
  bool Get_Accept_Status();
  void Accept_Spare();
  bool Get_Accept_Spare_Status();



private:
  //sock file descriptors
  int sc_init_sockfd=0;
  int sc_listening_sockfd=0;

  //hard coded to 200. Not sure why it was 200 in previous version of LORA_DAQ.
  int sc_max_buffer_size = 200;

  // machine name is self for server and server's for the client.
  // port number needs to be passed as string.
  std::string sc_machine_name, sc_port_string;
  //non block flag is set for a socket file descriptor
  //to make sure that read() should not block the current process i.e. rest of the code
  //but it also makes sure that read() returns EWOULDBLOCK when fd is not ready for a read() operation
  //which can be used to determine end of an incoming buffer. i.e. keep reading until errno is set to EWOULDBLOCK by read().
  //if you dont do this, you wait another loop cycle to read remaining buffer(maintained by NIC i.e. Network Interface).
  //nonblock is not required if you use select() to check for active sockfds but good to do it.
  bool sc_nonblock=false;

  // some fixed protocols. defining variables for maintainability in the future.
  // can free up these constants if need be.
  const int sc_ip_family = AF_INET; // fixed to ipv4
  const int sc_sock_type = SOCK_STREAM; // fixed to streaming.

  // will hold the found good addr
  struct addrinfo sc_addr;

};
