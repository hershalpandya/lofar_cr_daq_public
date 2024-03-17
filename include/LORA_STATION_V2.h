#pragma once
#include <string>
#include <vector>
#include <memory>
#include <utility>
#include "LORA_STATION.h"
#include "Buffer.h"
#include "Structs.h"
#include "Socket_Calls.h"
#include <boost/circular_buffer.hpp>

/* TODO 2/12/2021:
1. In operations class, save the station config for new stations in root file.
I think you will have to make sure struct STATION_INFO is then dumped into root file
When root file is newly made.
2. Improve trigger conditions. Now say you want SUPERTERP + 1 new station. ? Or at least 8 from superterp + 1 from new? etc. Brainstorm.
3. Please compare all methods of V1 and V2. Go through every method in OPERATIONS to make sure you implement every small thing in V2.
Because like the station config , other things might have not been implemented .
4. Also need to revisit  - request_OPS_to_wait_another_iteration/ wait_another_iteration to make sure doesnt introduce 30s delays in radio trigger decisions.

*/

class LORA_STATION_V2: public LORA_STATION
{
public:

  //define a destructor
  ~LORA_STATION_V2() {}

  //define a constructor
  LORA_STATION_V2() {}

  //Initializes all private members. get info from ops class.
  virtual void Init(const STATION_INFO&, const std::string&) override;

  //Opens open/bind/accept all sockets for this station
  virtual void Open() override;

  //Sends initial control params to digitizer via spare socket.
  virtual void Send_Control_Params() override;

  //Sends a small msg that tells lasa-client / digitzer to perform Electronics calibration
  virtual void Send_Electronics_Calib_Msg() override; //katie (not needed)

  //Returns no of detectors for which electronics msg was received
  virtual void Receive_Electronics_Calib_Msg(int&, const std::string&,
                                            const STATION_INFO&) override; //katie (not needed)

  //each station should add using FD_SET to active_stations_fds
  //and replace value in max_fd_val if one of its fds has a higher value.
  virtual void Add_readfds_To_List(fd_set&, int&) override;

  //Listens to incoming packets from the client
  virtual void Listen(fd_set&) override;

  //Accepts main and spare connections to the client
  virtual int Accept(fd_set&) override;

  //When the readin buffer is full, interprets the type of
  //msg and moves packet to appropriate spool.
  //osm, control param, dont care if old one has not been emptied
  //event, make sure you never overwrite. Push_??()
  //should always keep on emptying it.
  virtual void Interpret_And_Store_Incoming_Msgs(tm&) override;

  //sends summary of events in the event spool of this station.
  //so that ops class can construct an event out of it.
  virtual void Send_Event_Spool_Info(tvec_EVENT_SPOOL_SUMMARY&) override;

  // gets a vector of <time, str> and deletes the hit at
  // event_spool[i][j] if time of the event matches
  virtual void Discard_Events_From_Spool(const tvec_EVENT_SPOOL_SUMMARY&) override;

  // create a new vec of EVENT_DATA_STRUCTURE size 4
  // this vector is typedefed as tEvent_Data_Station
  // fill it and push to tEvent_Data_Station reference
  // given by the Ops class.
  // called by Ops Pull_Data_From_Stations()
  // should delete the pushed data from buffer.
  virtual void Send_Event_Data(tEvent_Data_Station&,
                               const tvec_EVENT_SPOOL_SUMMARY&) override;

  //same as above for osm
  virtual void Send_OSM_Delete_From_Spool(ONE_SEC_STRUCTURE&,
                                          ONE_SEC_STRUCTURE&) override;
  //calculate and push noise. will need to add private
  //members to hold running noise mean and sigma.
  virtual void Send_Log(tLog_Station&) override;

  // Closes connections. Call Stop and close socket.
  virtual void Close() override;

  virtual std::string Send_Name() override;

  virtual void Process_Event_Spool_Before_Coinc_Check(DETECTOR_CONFIG&,int&) override;

  virtual void Run_Detectors_Diagnostics(const std::string&,
                                         const DETECTOR_CONFIG&,
                                         const tm&, bool&) override;

  virtual void Calculate_New_Threshold(DETECTOR_CONFIG&,const tm&,int&) override;

  virtual void Set_New_Threshold() override;

  virtual void Status_Monitoring(bool&, std::string&) override;

  virtual int Get_Sum_Size_of_Spools() override;
private:

  //structure for arrays:
  //2 dims: digitizer_write(0) and digitizer_read(1),
  //4 dims: 0:  ch1 , 1: ch2 , 2: ch3, 3: ch4

  //some constants
  const unsigned char msg_header_bit = 0x99 ;
  const unsigned char msg_tail_bit = 0x66 ;

  const unsigned int event_msg_size=14024;//1672;//16072;//1576;
  const unsigned char event_msg_bit = 0xC0;

  const unsigned int onesec_msg_size=320;
  const unsigned char onesec_msg_bit=0xC4;

  const unsigned int min_msg_size=onesec_msg_size;
  vec_pair_unsgnchar_int idbit_and_msgsize;

  std::string name;
  int station_no;
  std::string type;

   //ip address of LORA main PC.
  std::string host_ip;
  //ip address of the HV unit
  std::string hv_ip;

  // port numbers :digitizer_write(0) and digitizer_read(1), SOCKET_CALLS needs string.%s
  // i.e. 0 port is where LORA_DAQ gets events/OSM. 1 port is for sending msgs to station.
  std::string port[2];

  //Unsigned char is the format in which messages are sent over socket.
  unsigned short current_threshold_ADC[4];//ch1, ch2, ch3, ch4 //katie
  unsigned int HV[4];//ch1, ch2, ch3, ch4
  unsigned char Control_Messages[4][40] ; //katie
  unsigned char Control_Mode_Messages[40] ; //katie

  std::unique_ptr<SOCKET_CALLS> socket[2];//digitizer_write(0) and digitizer_read(1)
  std::unique_ptr<BUFFER> buf_socket;//only one needed for the reading socket.

  //a vector of Event_Data, TimeStamp, pairs for each channel.
  //event_spool 0 = Master Ch1 , 1 = Master Ch2 , 2 = Slave Ch1, 3 = Slave ch2
  //stored in pair of event_data_structure and time. so that only time is pulled out
  //at first to see if this is part of an event.
  //on later request, data_structure is passed on to the OPERATIONS class
  //and removed from spool.
  std::vector<std::pair<EVENT_DATA_STRUCTURE,long long unsigned>> event_spool[4];
  std::vector<ONE_SEC_STRUCTURE> osm_spool;

  unsigned int current_CTP;
  unsigned int current_UTC_offset;

  int request_OPS_to_wait_another_iteration=0;

  // Send Stop LORA signal as per old code. Called inside Close().
  // On lasa-client side, just leads to exit from daq code.
  //void Send_Stop_LORA();

  //runtime is same for all stations. its based on the very first osm received.
  //variable is owned and managed by Ops class.
  void Unpack_Event_Msg_Store_To_Spool(const std::vector<unsigned char>&,
                                       const tm&);

  void Unpack_OSM_Msg_Store_To_Spool(const std::vector<unsigned char>&,
                                     tm& rs_time);
  void Update_current_CTP_and_UTC_offset();

  //Based on the events in the event spool.
  //Calculate a running mean of means and a running mean of sigmas.
  boost::circular_buffer<float> hundred_means[4];
  boost::circular_buffer<float> hundred_sigmas[4];
  boost::circular_buffer<long long unsigned> hundred_timestamps[4];
  boost::circular_buffer<long long unsigned> hundred_thresh_crossing_timestamps[4];
  // boost::circular_buffer<long long unsigned> hundred_station_event_timestamps;
  tchrono most_recent_msg_rcvd_time;

};
