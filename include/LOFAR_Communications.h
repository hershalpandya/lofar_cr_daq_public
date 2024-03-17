#pragma once
#include <string>
#include <memory>
#include <sys/socket.h> // ip protocols for eg int values for AF_INET defined here.
#include <netdb.h> //has getaddrinfo(),struct addrinfo,etc.
#include <sys/fcntl.h> // fcntl(), O_NONBLOCK,etc
#include <sys/select.h> //select(), FD_SET(), etc.
#include <arpa/inet.h>//struct in_addr
#include <netinet/in.h>
#include "Structs.h"
#include "functions_library.h"

//Just cleanup of old code. Not using socket calls since this is UDP socket not
//streaming socket.

class LOFAR_COMMS
{
public:
  void Init(const STATION_INFO&, const DETECTOR_CONFIG&);
  void Open();
  void Close();
  void Send_Trigger(const unsigned int&, const unsigned int&, unsigned int&);
private:
  std::string host_ip; //ip address of LORA main PC.
  int port_number; // port numbers, SOCKET_CALLS needs string.%s
  struct sockaddr_in Host_Addr_out ;
  int sockfd;
  tchrono most_recent_radio_trig_time;
  int first_trigg=1;
  int tbb_dump_wait ; //do not send radio trig within x mins of last trig.
};
