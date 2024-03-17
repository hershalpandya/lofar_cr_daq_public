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

class LORA_STATION_V1: public LORA_STATION
{
public:

  //define a destructor
  ~LORA_STATION_V1() {}

  //define a constructor
  LORA_STATION_V1() {}

  //Initializes all private members. get info from ops class.
  virtual void Init(const STATION_INFO&, const std::string&) override;

  //Opens open/bind/accept all sockets for this station
  virtual void Open() override;

  //Sends initial control params to digitizer via spare socket.
  virtual void Send_Control_Params() override;

  //Sends a small msg that tells lasa-client / digitzer to perform Electronics calibration
  virtual void Send_Electronics_Calib_Msg() override;

  //Returns no of detectors for which electronics msg was received
  virtual void Receive_Electronics_Calib_Msg(int&, const std::string&,
                                            const STATION_INFO&) override;

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
  // same as above for cp - we don't save complete cp msgs since all info is same except threshold.
  //virtual void Send_CP_Delete_From_Spool(tControl_Params_Station&) override;
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

  virtual void Run_Detectors_Diagnostics(const std::string&, const DETECTOR_CONFIG&, const tm&, bool&) override;

  virtual void Calculate_New_Threshold(DETECTOR_CONFIG&,const tm&,int&) override;

  virtual void Set_New_Threshold() override;

  virtual void Status_Monitoring(bool&, std::string&) override;

  virtual int Get_Sum_Size_of_Spools() override;
private:

  //structure for arrays:
  //2 dims: 0: master , 1: slave
  //4 dims: 0: master ch1 , 1: master ch2 , 2: slave ch1, 3: slave ch2

  //some constants
  const unsigned char msg_header_bit = 0x99 ;
  const unsigned char msg_tail_bit = 0x66 ;

  const unsigned int event_msg_size=12023;
  const unsigned char event_msg_bit = 0xA0;

  const unsigned int onesec_msg_size=87;
  const unsigned char onesec_msg_bit=0xA4;

  const unsigned int cp_msg_size=79;
  const unsigned char cp_msg_bit=0x55;

  const unsigned int min_msg_size=cp_msg_size;
  vec_pair_unsgnchar_int idbit_and_msgsize;

  std::string name;
  int station_no;
  std::string type;
  std::string HISPARC_Serial[2]; // for master 0, slave 1
  std::string host_ip; //ip address of LORA main PC.
  std::string port[2]; // port numbers, SOCKET_CALLS needs string.%s
  //Unsigned char is the format in which messages are sent over socket.
  unsigned char init_control_params[2][40];
  unsigned short current_threshold_ADC[4];//M-Ch1 M-CH2 S-Ch1 S-Ch2

  std::unique_ptr<SOCKET_CALLS> socket[2];
  std::unique_ptr<BUFFER> buf_socket[2];//initialize in init to 10mb: FIXIT
  //std::unique_ptr<BUFFER> buf_socket_spare[2];//initialize in init to 10mb: FIXIT

  //a vector of Event_Data, TimeStamp, pairs for each channel.
  //event_spool 0 = Master Ch1 , 1 = Master Ch2 , 2 = Slave Ch1, 3 = Slave ch2
  //stored in pair of event_data_structure and time. so that only time is pulled out
  //at first to see if this is part of an event.
  //on later request, data_structure is passed on to the OPERATIONS class
  //and removed from spool.
  std::vector<std::pair<EVENT_DATA_STRUCTURE,long long unsigned>> event_spool[4];
  std::vector<ONE_SEC_STRUCTURE> osm_spool[2];//a vector for master / slave
  //cp_spool is not used anymore.
  //std::vector<CONTROL_STRUCTURE> cp_spool[4];//a vector for each channel.
  unsigned int current_CTP[2]; // master and slave
  int request_OPS_to_wait_another_iteration=0;

  // Send Stop LORA signal as per old code. Called inside Close().
  // On lasa-client side, just leads to exit from daq code.
  //void Send_Stop_LORA();

  //runtime is same for all stations. its based on the very first osm received.
  //variable is owned and managed by Ops class.
  void Unpack_Event_Msg_Store_To_Spool(const std::vector<unsigned char>&,
                                       const std::string&,
                                       const int&,
                                       const tm&);

  void Unpack_OSM_Msg_Store_To_Spool(const std::vector<unsigned char>&,
                                     const int&,
                                     tm& rs_time);
  void Update_current_CTP(const int&);

  void Unpack_CP_Msg_Update_Threshold(const std::vector<unsigned char>&,
                                    const std::string&);


  void Reset_Electronics();
  //Based on the events in the event spool.
  //Calculate a running mean of means and a running mean of sigmas.
  boost::circular_buffer<float> hundred_means[4];
  boost::circular_buffer<float> hundred_sigmas[4];
  boost::circular_buffer<long long unsigned> hundred_timestamps[4];
  boost::circular_buffer<long long unsigned> hundred_thresh_crossing_timestamps[4];
  // boost::circular_buffer<long long unsigned> hundred_station_event_timestamps;

  tchrono most_recent_msg_rcvd_time[2];
  tchrono most_recent_event_msg_rcvd_time[2];
};
