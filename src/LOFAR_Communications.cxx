#include "LOFAR_Communications.h"
#include <string> // std::string
#include <stdexcept>
#include <iostream>
#include <cerrno>
#include "Structs.h"
#include <cstdlib>//for atoi()
#include <cstring> //memset, strcpy, etc.
#include <unistd.h> //for close(), read(), etc.



void LOFAR_COMMS::Init(const STATION_INFO& nc_LOFAR, const DETECTOR_CONFIG& dc)//nc stands for network config
{
  if (nc_LOFAR.type!="superhost" || nc_LOFAR.name!="lofar")
  {
    std::string errormsg="wrong network config given to LOFAR_COMMS Init()\n";
    errormsg+= "type('superhost' allowed): ";
    errormsg+= nc_LOFAR.type;
    errormsg+= "name('lofar' allowed): ";
    errormsg+= nc_LOFAR.name;
    throw std::runtime_error(errormsg);
  }
  host_ip=nc_LOFAR.IPV4;
  port_number=std::atoi(nc_LOFAR.port_1.c_str());
  tbb_dump_wait = dc.tbb_dump_wait_min*60*1000;
}

void LOFAR_COMMS::Open()
{
  Host_Addr_out.sin_family=AF_INET;
	Host_Addr_out.sin_port=htons(port_number) ;
	Host_Addr_out.sin_addr.s_addr=inet_addr(host_ip.c_str());
  std::memset(Host_Addr_out.sin_zero,'\0',sizeof Host_Addr_out.sin_zero);
  sockfd=socket(AF_INET,SOCK_DGRAM,0);
  if (sockfd==-1)
  {
    std::string errormsg="socket() fails in LOFAR_COMMS.";
    errormsg+= std::string(std::strerror(errno));
    throw std::runtime_error(errormsg);
  }
  std::cout << "UDP Socket to LOFAR created at" << host_ip.c_str()
  << " port: " << port_number << std::endl;
  sleep(5);
}

void LOFAR_COMMS::Close()
{
  close(sockfd);
}

void LOFAR_COMMS::Send_Trigger(const unsigned int& GPS_time_stamp,
                               const unsigned int& nsec,
                               unsigned int& trigg_sent)
{
  int dt_ms = Get_ms_Elapsed_Since(most_recent_radio_trig_time);
  if (dt_ms <= tbb_dump_wait && first_trigg==0)
  {
    std::cout << dt_ms/1000 << " seconds since last radio trig." << std::endl;
    std::cout << "Event trigger not sent to astron." << std::endl;
    trigg_sent=0;
    return;
  }

  unsigned char data[11] ;
	data[0]=0x99 ;	//Header
	data[1]=0xA0 ;	//Identifier
	data[2]=(GPS_time_stamp & 0xff000000)>>24 ;
	data[3]=(GPS_time_stamp & 0x00ff0000)>>16 ;
	data[4]=(GPS_time_stamp & 0x0000ff00)>>8 ;
	data[5]=GPS_time_stamp & 0x000000ff ;
	data[6]=(nsec & 0xff000000)>>24 ;
	data[7]=(nsec & 0x00ff0000)>>16 ;
	data[8]=(nsec & 0x0000ff00)>>8 ;
	data[9]=nsec & 0x000000ff ;
	data[10]=0x66 ;	//End

	int bytes_sent=sendto(sockfd,data,sizeof data, 0 ,
                   (struct sockaddr *)&Host_Addr_out,
                   sizeof Host_Addr_out);

  std::cout << "LOFAR Trig Bytes sent:" << bytes_sent << std::endl;
  if (bytes_sent>0)
  {
    most_recent_radio_trig_time=Get_Current_Time_chrono();
    trigg_sent=1;
    first_trigg=0;
  }
  return;
}
