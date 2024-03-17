#include "LORA_STATION_V1.h"
#include "Structs.h"
#include "functions_library.h"
#include "Buffer.h"
#include <string>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <memory> //unique_ptr
#include <algorithm>
#include <numeric>// std accumulate.
#include <cerrno> //strerror
#include <cstring> //strerror
#include <ctime> //struct tm
#include <utility> //std::pair

void LORA_STATION_V1::Init(const STATION_INFO& sta_info, const std::string& server_ip)
{
  if (sta_info.type!="clientv1")
  {
    std::stringstream ss;
    ss << "Found STATION_INFO.type: "<< sta_info.type;
    ss << ". LORA_STATION_V1 class applied to a station of type other than clientv1.";
    throw std::runtime_error(ss.str());
  }

  most_recent_msg_rcvd_time[0]=Get_Current_Time_chrono();
  most_recent_msg_rcvd_time[1]=Get_Current_Time_chrono();

  most_recent_event_msg_rcvd_time[0]=Get_Current_Time_chrono();
  most_recent_event_msg_rcvd_time[1]=Get_Current_Time_chrono();

  port[0] = sta_info.port_1;
  port[1] = sta_info.port_2;

  name = sta_info.name;
  station_no = sta_info.no;
  type = sta_info.type;

  HISPARC_Serial[0] = sta_info.HISPARC_Serial_m;
  HISPARC_Serial[1] = sta_info.HISPARC_Serial_s;

  host_ip = server_ip;

  //load init control param to the container which will be sent to client
  //via the socket. Build_Hisparc_Messages converts unsigned short array
  //to unsigned char array , also doing transformations required to make
  //message readable by Hisparc Digitizers.
  Build_Hisparc_Messages(sta_info.init_control_params_m, init_control_params[0]);
  Build_Hisparc_Messages(sta_info.init_control_params_s, init_control_params[1]);

  //mVolts2ADC is 2

  current_threshold_ADC[0]= (unsigned short) (sta_info.init_control_params_m[16] * 2);
  current_threshold_ADC[1]= (unsigned short) (sta_info.init_control_params_m[18] * 2);

  current_threshold_ADC[2]= (unsigned short) (sta_info.init_control_params_s[16] * 2);
  current_threshold_ADC[3]= (unsigned short) (sta_info.init_control_params_s[18] * 2);

  //socket buffer: holds all incoming msgs.
  // it is frequently checked for a complete msg and msg is moved
  // from here to corresponding spool
  unsigned int size_buf_sock= 10*event_msg_size + 10*onesec_msg_size + 1*cp_msg_size ;
  // buffer size = f(how fast you empty it, whats the event rate.)

  for (int i=0;i<2;i++)
  {
    std::unique_ptr<BUFFER> temp_ptr1(new BUFFER());
    buf_socket[i] = std::move(temp_ptr1);
    buf_socket[i]->Init(size_buf_sock);
  }

  for (int i=0;i<4;++i)
  {
    hundred_means[i].set_capacity(1000);
    hundred_sigmas[i].set_capacity(1000);
    hundred_thresh_crossing_timestamps[i].set_capacity(1000);
    hundred_timestamps[i].set_capacity(1000);
  }
  // hundred_station_event_timestamps.set_capacity(1000);

  idbit_and_msgsize.push_back(std::make_pair(onesec_msg_bit,onesec_msg_size)); //one sec msg
  idbit_and_msgsize.push_back(std::make_pair(event_msg_bit,event_msg_size));//event msg
  idbit_and_msgsize.push_back(std::make_pair(cp_msg_bit,cp_msg_size));//control parameters
}

void LORA_STATION_V1::Open()
{
  std::stringstream command, command2;
  //https://serverfault.com/questions/104668/create-screen-and-run-command-without-attaching
  //create a detached screen session:
  command2 << "screen -dmS LORA" << station_no;
  system(command2.str().c_str());

  //run the command on the detached screen session:
	command << "screen -S LORA" << station_no
          << " -X stuff 'sh ../daq_managers/start_stations.sh " << station_no << " \n'";

	std::cout << "Now starting LORA" << station_no << std::endl;
	system(command.str().c_str());

  for (int i=0;i<2;i++) //loop for master, slave.
  {
    bool set_fd_to_nonblock = true;
    // so that we can read repeatedly from it, waiting for EWOULDBLOCK to stop reading.
    //https://eklitzke.org/blocking-io-nonblocking-io-and-epoll
    std::unique_ptr<SOCKET_CALLS> temp_ptr(new SOCKET_CALLS(host_ip, port[i],
                                              set_fd_to_nonblock));
    socket[i] = std::move(temp_ptr);
    socket[i]->Open();
    socket[i]->Bind();
    socket[i]->Listen();
    //socket[i]->Accept();
    // // a socket is defined by {SRC-IP, SRC-PORT, DEST-IP, DEST-PORT, PROTOCOL}
    // // src is client(lasa-client) and dest is host (lora main pc that hosts this code.)
    // // as soon as connection is accepted by a socket, a diff sockfd is assigned.
    // // the orig socket connection can Accept() again and create a second sockfd
    // // for this second sockfd, its as good as a second port on DEST side because
    // // on the client side local port is different. i.e. SRC-PORT is diff
    // // this happens automatically because when Connect() is called a client
    // // a local ip and port is assigned to the socket internally.
    //socket[i]->Accept_Spare();
    //FIXIT: accept spare - how to do it under new framework when we do
    //select() on listening sockfd and then accept it. check. relay server doesn't
    //give spare socket as of now. so come to it later.
  }
}

void LORA_STATION_V1::Send_Control_Params()
{
  for (int i=0;i<2;i++)
  {
    auto lenofbuffer = sizeof(init_control_params[i])/sizeof(init_control_params[i][0]);
    int bytes_sent = 0;
    bool use_spare_socket=false;
    socket[i]->Send(init_control_params[i],lenofbuffer,bytes_sent,use_spare_socket);
    std::cout << "Control Params bytes sent: " << bytes_sent << std::endl;
  }
}

void LORA_STATION_V1::Send_Electronics_Calib_Msg()
{
  unsigned char dat[4] ;
	dat[0]=0x99 ;
	dat[1]=0x30 ;
	dat[2]=0x80 ;
	dat[3]=0x66 ;

  for (int i=0;i<2;i++)
  {
    auto lenofbuffer = sizeof(dat)/sizeof(dat[0]);
    int bytes_sent = 0;
    bool use_spare_socket=false;
    socket[i]->Send(dat,lenofbuffer,bytes_sent,use_spare_socket);
    std::cout << "Electronics calib msg bytes sent: " << bytes_sent << std::endl;
  }
}

void LORA_STATION_V1::Receive_Electronics_Calib_Msg(int& detectors_calibrated,
                                                    const std::string& ecalib_fname,
                                                    const STATION_INFO& sta_info)
{
  for (int i=0;i<2;i++)
  {
    std::string m_or_s = (i==0) ? "Master" : "Slave" ;
    std::vector<unsigned char> msg;
    msg = buf_socket[i]->extract_electronics_calib_msg_in_buf();
    if (msg.size()==9)
    {
      std::cout << name << " " << i << msg.size() << std::endl;
      detectors_calibrated++;
      std::ofstream outfile;
      outfile.open(ecalib_fname,std::fstream::app);
      outfile << name //e.g. lasa1
      << "\t" << m_or_s //e.g. Master
      << "\t" << (unsigned int) msg[0] // CHANNEL_1_OFFSET+
      << "\t" << (unsigned int) msg[1] // CHANNEL_1_OFFSET-
      << "\t" << (unsigned int) msg[2] // CHANNEL_2_OFFSET+
      << "\t" << (unsigned int) msg[3] // CHANNEL_2_OFFSET-
      << "\t" << (unsigned int) msg[4] // CHANNEL_1_GAIN+
      << "\t" << (unsigned int) msg[5] // CHANNEL_1_GAIN-
      << "\t" << (unsigned int) msg[6] // CHANNEL_2_GAIN+
      << "\t" << (unsigned int) msg[7] // CHANNEL_2_GAIN-
      << "\t" << (unsigned int) msg[8] ; // COMMON_OFFSET_ADJ
      if (i==0)
      {
        outfile
        << "\t" << sta_info.init_control_params_m[9] // FULL_SCALE_ADJ
        << "\t" << sta_info.init_control_params_m[10] // CHANNEL_1_INTE_TIME
        << "\t" << sta_info.init_control_params_m[11] // CHANNEL_2_INTE_TIME
        << "\t" << sta_info.init_control_params_m[12] // COMP_THRES_LOW
        << "\t" << sta_info.init_control_params_m[13] // COMP_THRES_HIGH
        << "\t" << sta_info.init_control_params_m[14] // CHANNEL_1_HV
        << "\t" << sta_info.init_control_params_m[15] // CHANNEL_2_HV
        << "\t" << sta_info.init_control_params_m[16] // CHANNEL_1_THRES_LOW
        << "\t" << sta_info.init_control_params_m[17] // CHANNEL_1_THRES_HIGH
        << "\t" << sta_info.init_control_params_m[18] // CHANNEL_2_THRES_LOW
        << "\t" << sta_info.init_control_params_m[19] // CHANNEL_2_THRES_HIGH
        << "\t" << sta_info.init_control_params_m[20] // TRIGG_CONDITION
        << "\t" << sta_info.init_control_params_m[21] // PRE_COIN_TIME
        << "\t" << sta_info.init_control_params_m[22] // COIN_TIME
        << "\t" << sta_info.init_control_params_m[23] // POST_COIN_TIME
        << "\t" << sta_info.init_control_params_m[24] // LOG_BOOK
        << "\n" ;
      }
      else if (i==1)
      {
        outfile
        << "\t" << sta_info.init_control_params_s[9] // FULL_SCALE_ADJ
        << "\t" << sta_info.init_control_params_s[10] // CHANNEL_1_INTE_TIME
        << "\t" << sta_info.init_control_params_s[11] // CHANNEL_2_INTE_TIME
        << "\t" << sta_info.init_control_params_s[12] // COMP_THRES_LOW
        << "\t" << sta_info.init_control_params_s[13] // COMP_THRES_HIGH
        << "\t" << sta_info.init_control_params_s[14] // CHANNEL_1_HV
        << "\t" << sta_info.init_control_params_s[15] // CHANNEL_2_HV
        << "\t" << sta_info.init_control_params_s[16] // CHANNEL_1_THRES_LOW
        << "\t" << sta_info.init_control_params_s[17] // CHANNEL_1_THRES_HIGH
        << "\t" << sta_info.init_control_params_s[18] // CHANNEL_2_THRES_LOW
        << "\t" << sta_info.init_control_params_s[19] // CHANNEL_2_THRES_HIGH
        << "\t" << sta_info.init_control_params_s[20] // TRIGG_CONDITION
        << "\t" << sta_info.init_control_params_s[21] // PRE_COIN_TIME
        << "\t" << sta_info.init_control_params_s[22] // COIN_TIME
        << "\t" << sta_info.init_control_params_s[23] // POST_COIN_TIME
        << "\t" << sta_info.init_control_params_s[24] // LOG_BOOK
        << "\n" ;
      }
      outfile.close();
    }
  }
  return;
}

void LORA_STATION_V1::Add_readfds_To_List(fd_set& active_read_fds, int& max_fd_val)
{
  //each station should add using FD_SET to active_stations_fds
  //and replace value in max_fd_val if one of its fds has a higher value.
  for (int i=0;i<2;i++)
  {
    FD_SET(socket[i]->sc_active_sockfd, &active_read_fds);
    max_fd_val = std::max({max_fd_val,
                          socket[i]->sc_active_sockfd});

    //you are not reading from spare socket - so these have been commented out:
    // if (socket[i]->Get_Accept_Spare_Status())
    // {
    //   FD_SET(socket[i]->sc_active_spare_sockfd, &active_read_fds);
    //   max_fd_val = std::max({max_fd_val,
    //                         socket[i]->sc_active_spare_sockfd});
    // }
  }
}

int LORA_STATION_V1::Accept(fd_set& fd_list)
{
  int n_accepted_connections = 0;
  for (int i=0;i<2;i++) // master, slave. 2 sockets.
  {
    if (!FD_ISSET(socket[i]->sc_active_sockfd, &fd_list)) continue;

    if (!socket[i]->Get_Accept_Status())
    {
      //when its ready for the first time
      //accept the connection. internally socket fd will be
      //replaced by the accepted connection fd.
      socket[i]->Accept();
      ++n_accepted_connections;
      std::cout << "Accepted 1 connection from:" << station_no << std::endl;
      most_recent_msg_rcvd_time[i]=Get_Current_Time_chrono();
    }

    if (!socket[i]->Get_Accept_Spare_Status())
    {
      socket[i]->Accept_Spare();
      ++n_accepted_connections;
      most_recent_msg_rcvd_time[i]=Get_Current_Time_chrono();
    }
  }
  return n_accepted_connections;
}

void LORA_STATION_V1::Listen(fd_set& fd_list)
{
  for (int m_or_s=0; m_or_s<2; m_or_s++) // master, slave. 2 sockets.
  {
    if (!FD_ISSET(socket[m_or_s]->sc_active_sockfd, &fd_list)) continue;

    unsigned char buffer[4096];
    //https://stackoverflow.com/questions/13433286/optimal-buffer-size-for-reading-file-in-c
    size_t sizeofbuffer=sizeof(buffer);//len_array == size_array for unsigned char.
    int read_bytes;
    bool use_spare=false;
    errno= socket[m_or_s]->Receive(buffer, sizeofbuffer, read_bytes, use_spare);

    if (read_bytes>0) buf_socket[m_or_s]->char_pushback(buffer,read_bytes);

    if (read_bytes<0)
    {
      std::string errormsg ;
      errormsg= std::string(std::strerror(errno));
      std::cout << port[m_or_s] << ":  " << errormsg << std::endl;
    }
  }
}

void LORA_STATION_V1::Interpret_And_Store_Incoming_Msgs(tm& rs_time)
{
  /*printf("Value of header_bit: Hex: %X, Decimal: %d\n",header_bit,header_bit);*/
  for (int im_or_s=0; im_or_s<2; ++im_or_s) //master and slave loop.
  {
    std::string m_or_s = (im_or_s==0) ? "Master" : "Slave" ;
    int bufsize=buf_socket[im_or_s]->Get_Buffer_Size();
    //unpack all the msgs in this buffer.
    //earlier we only unpacked one msg in single looop run of main()
    while (bufsize>=min_msg_size)
    {
      std::vector<unsigned char> msg;
      msg = buf_socket[im_or_s]->extract_first_msg_in_buf(msg_header_bit,
                                                    msg_tail_bit,
                                                    idbit_and_msgsize);
      if (msg.size()==0) break; // break while() if no msg was found in buffer.

      most_recent_msg_rcvd_time[im_or_s]=Get_Current_Time_chrono();

      if (msg[1]==onesec_msg_bit)//Interpret
      {
        Unpack_OSM_Msg_Store_To_Spool(msg, im_or_s, rs_time); //Store
        Update_current_CTP(im_or_s); //after each OSM. Clock is separate for Master and Slave.
      }
      else if (msg[1]==event_msg_bit) //Interpret
      {
        Unpack_Event_Msg_Store_To_Spool(msg, m_or_s, im_or_s, rs_time); //Store
        most_recent_event_msg_rcvd_time[im_or_s]=Get_Current_Time_chrono();
      }
      else if (msg[1]==cp_msg_bit)
      {
        std::cout <<"...log recvd..."<< name <<" " <<im_or_s << std::endl;
        Unpack_CP_Msg_Update_Threshold(msg, m_or_s); //Store
      }
      bufsize=buf_socket[im_or_s]->Get_Buffer_Size();
    }
  }
}

void LORA_STATION_V1::Send_Event_Spool_Info(tvec_EVENT_SPOOL_SUMMARY& v)
{
  bool all_spools_empty=true;
  for (int i=0; i<4; ++i)
  {
    if (event_spool[i].size()!=0)
    {
      all_spools_empty=false;
      break;
    }
  }
  if (all_spools_empty) return;

  for (int i=0; i<4; ++i)
  {
    for (int j=0; j<event_spool[i].size(); ++j)
    {
      if (event_spool[i][j].first.Total_counts==-99.0)
      {
        std::cout
        << "> > >  .... Process_Event_Spool_Before_Coinc_Check() not run. ........ < < <"
        << std::endl;
      }

      EVENT_SPOOL_SUMMARY temp;
      temp.station_name = name;
      temp.station_no = station_no;
      temp.evtspool_i = i;
      temp.evtspool_j = j;
      temp.Has_trigg = event_spool[i][j].first.Has_trigg;
      temp.time_stamp = event_spool[i][j].second;
      temp.GPS_time_stamp= event_spool[i][j].first.GPS_time_stamp;
      temp.Time_Msg_Rcvd_ms= event_spool[i][j].first.Time_Msg_Rcvd_ms;
      temp.nsec= event_spool[i][j].first.nsec;
      temp.charge = event_spool[i][j].first.Total_counts;
      temp.corrected_peak = event_spool[i][j].first.Corrected_peak;
      v.push_back(temp);
    }
  }
}

void LORA_STATION_V1::Discard_Events_From_Spool(const tvec_EVENT_SPOOL_SUMMARY& event_vec)
{
  for (int k=0; k<event_vec.size(); ++k)
  {
    if (name!=event_vec[k].station_name) continue;

    int i = event_vec[k].evtspool_i;
    int j = event_vec[k].evtspool_j;

    if (event_spool[i][j].second != event_vec[k].time_stamp) continue;

    event_spool[i].erase(event_spool[i].begin() + j);
  }
}

void LORA_STATION_V1::Send_Event_Data(tEvent_Data_Station& station_data,
                                      const tvec_EVENT_SPOOL_SUMMARY& event_vec)
{
  //Will send event_spool contents if this station is listed in event_vec
  //otherwise sends an empty station back.
  //this is called only if a station is active.
  //(obvious because inactive station class instance of LORA_STATION_V1 won't even be generated.)

  for (int i=0; i<4; ++i)
  {
    station_data.push_back(EVENT_DATA_STRUCTURE());
    station_data[i].detector = Get_Detector_Number(station_no,i);
  }

  int n_replaced=0;

  //if a channel is in the events list then its added to empty station
  //all 4 channels will be added mostly since even zero signal channel
  //should have a time stamp within 500ns. for 3 of 4 channels trig condition

  for (int k=0; k<event_vec.size(); ++k)
  {
    if (name!=event_vec[k].station_name) continue;

    int i = event_vec[k].evtspool_i;
    int j = event_vec[k].evtspool_j;

    if (event_spool[i][j].second != event_vec[k].time_stamp) continue;

    station_data[i]=event_spool[i][j].first;
    n_replaced+=1;
  }

  if (n_replaced!=4)
  {
    std::cout << "station did not contribute 4 ch's to event. " ;
    std::cout << n_replaced << " ch's were provided." << std::endl;
  }
}

void LORA_STATION_V1::Send_OSM_Delete_From_Spool(ONE_SEC_STRUCTURE& osm_master,
                                                 ONE_SEC_STRUCTURE& osm_slave)
{
  if (osm_spool[0].size()>0)
  {
    osm_master = osm_spool[0][0];
    osm_spool[0].erase(osm_spool[0].begin());
  }
  if (osm_spool[1].size()>0)
  {
    osm_slave = osm_spool[1][0];
    osm_spool[1].erase(osm_spool[1].begin());
  }
}

void LORA_STATION_V1::Send_Log(tLog_Station& noise_station_vec)
{
  for (int i=0; i<4; ++i)
  {
    float avg_mean = std::accumulate(hundred_means[i].begin(),
                                      hundred_means[i].end(),0.0);
    avg_mean /= hundred_means[i].size();

    float avg_sigma = std::accumulate(hundred_sigmas[i].begin(),
                                      hundred_sigmas[i].end(),0.0);
    avg_sigma /= hundred_sigmas[i].size();

    std::tm *t = Get_Current_Time();

    LOG_STRUCTURE temp;
    temp.detector = Get_Detector_Number(station_no,i);
    temp.UTC_time_stamp = (unsigned int)timegm(t);
    temp.Mean = avg_mean;
    temp.Sigma = avg_sigma;
    temp.Station = station_no;
    temp.Channel_thres_low = current_threshold_ADC[i];

    noise_station_vec.push_back(temp);
  }
}

void LORA_STATION_V1::Calculate_New_Threshold(DETECTOR_CONFIG& dc,
                                              const tm& rs_time,
                                              int& current_reset_thresh_interval)
{
  int avg_over_dt= 60*60*1000; // ms

  std::tm *t = Get_Current_Time();
  short unsigned int year = t->tm_year + 1900;
  short unsigned int month = t->tm_mon + 1;
  short unsigned int day = t->tm_mday;
  short unsigned int hour = t->tm_hour;
  short unsigned int min = t->tm_min;
  short unsigned int sec = t->tm_sec;

  auto time_stamp_now=(long long unsigned)((year-rs_time.tm_year)*365*24*60*60);//to s
  time_stamp_now+=(long long unsigned)((month-rs_time.tm_mon)*30*24*60*60);//to s
  time_stamp_now+=(long long unsigned)((day-rs_time.tm_mday)*24*60*60);//to s
  time_stamp_now+=(long long unsigned)((hour-rs_time.tm_hour)*60*60);//to s
  time_stamp_now+=(long long unsigned)((min-rs_time.tm_min)*60);//to s
  time_stamp_now+=(long long unsigned)(sec-rs_time.tm_sec);
  time_stamp_now*=1000000000; //to ns

  for (int i=0; i<4; ++i)
  {
    int temp_detno=Get_Detector_Number(station_no,i);
    if (dc.calibration_mode==2)
    {
      auto vec= dc.list_of_dets_for_calib;
      auto det_in_list = std::find(vec.begin(), vec.end(), temp_detno)!= vec.end();
      if (det_in_list) continue;//do not alter threshold. this det is in calib mode.
    }

    //its 5 mins actually. not renaming vars again.
    float sixty_min_mean=0;
    float sixty_min_sigma=0;
    int sixty_min_n=0;

    Time_Average_From_Circ_Buffer(hundred_means[i],
                                  hundred_timestamps[i],
                                  time_stamp_now,
                                  avg_over_dt,
                                  sixty_min_mean, sixty_min_n);

    Time_Average_From_Circ_Buffer(hundred_sigmas[i],
                                  hundred_timestamps[i],
                                  time_stamp_now,
                                  avg_over_dt,
                                  sixty_min_sigma, sixty_min_n);

    if (sixty_min_n>0 && sixty_min_mean>0)
    {
      float temp = sixty_min_mean
                  + dc.sigma_ovr_thresh * sixty_min_sigma ;
      current_threshold_ADC[i]= static_cast<unsigned short>(temp);
    }
    else
    {
      current_threshold_ADC[i]-=2;
      current_reset_thresh_interval=dc.init_reset_thresh_interval;
      std::cout << "reducing thresh for " << name << " " << i << std::endl;
    }
  }
}

void LORA_STATION_V1::Close()
{
	unsigned char dat[3] ;
	dat[0]=0x99 ;
	dat[1]=0xAA ;		//We have used identifier 'AA' for stopping DAQ in LORA
	dat[2]=0x66 ;
  auto lenofbuffer = sizeof(dat)/sizeof(dat[0]);

  for (int i=0;i<2;i++)
  {
    int bytes_sent = 0;
    bool use_spare_socket=true;
    socket[i]->Send(dat,lenofbuffer,bytes_sent,use_spare_socket);
    socket[i]->Close();
  }

  sleep(2);

  std::stringstream command;
  //run the command on the detached screen session:
	command << "screen -S LORA" << station_no
          << " -X quit" ;
  std::cout << "Ending screen session: LORA" << station_no << std::endl;
  std::cout << command.str() << std::endl;
	system(command.str().c_str());
  sleep(2);
}

void LORA_STATION_V1::Unpack_Event_Msg_Store_To_Spool(const std::vector<unsigned char>& msg,
                                                      const std::string& m_or_s,
                                                      const int& m_or_s_int,
                                                      const tm& rs_time)
{

  if (m_or_s!="Master" && m_or_s!="Slave")
  {
    std::stringstream ss;
    ss << "Unpack_Event_Msg_Store_To_Spool() got invalid input for m_or_s. 'Master' or 'Slave' allowed. ";
    ss << "\n . Received: " << m_or_s;
    throw std::runtime_error(ss.str());
  }

  unsigned long int Time_Msg_Rcvd_ms= Get_Current_Time_ms();

  EVENT_DATA_STRUCTURE event[2];

  unsigned char header = (unsigned char) msg[0];
  unsigned char identifier_bit = (unsigned char) msg[1] ;
  unsigned int Trigg_condition= (unsigned int) msg[2] ;
  unsigned int Trigg_pattern= (unsigned int) ((msg[3]<<8)+msg[4]) ;
  // 5,6, are pre coinc window - not loaded
  // 7,8 are coinc window - not loaded
  // 9,10 are post coinc window - not loaded
  short unsigned int day=msg[11] ;
  short unsigned int month=msg[12] ;
  short unsigned int year=(msg[13]<<8)+msg[14] ;
  short unsigned int hour=msg[15] ;
  short unsigned int min=msg[16] ;
  short unsigned int sec=msg[17] ;
	unsigned int CTD=(msg[18]<<24)+(msg[19]<<16)+(msg[20]<<8)+msg[21] ;

  //current_CTP would have been set by the previous call
  //to unpack OSM for this station.
  //its a LORA_STATION_V1 member.
	unsigned int nsec=(unsigned int)((1.0*CTD/current_CTP[m_or_s_int])*(1000000000)) ;
  //This relation is not exact.

  //-----------xxx--------------
  tm t; t.tm_sec=sec ; t.tm_min=min ;
	t.tm_hour=hour ; t.tm_mday=day ;
	t.tm_mon=month -1 ; t.tm_year=year-1900 ;
  //Because GPS month starts from 1 while in 'tm' struct, it starts from 0.
	unsigned int GPS_time_stamp=(unsigned int)timegm(&t) ;
  GPS_time_stamp+=1;

  std::time_t tempGPS= (std::time_t) GPS_time_stamp;
  std::tm *ggt = std::gmtime(&tempGPS) ;
  year = ggt->tm_year + 1900;
  month = ggt->tm_mon + 1;
  day = ggt->tm_mday;
  hour = ggt->tm_hour;
  min = ggt->tm_min;
  sec = ggt->tm_sec;


  for (int i=0;i<2;++i)
  {
    auto detno = Get_Detector_Number(name,m_or_s,i);
    event[i].Station = station_no ;
    event[i].detector = detno ;
    //GPS_time_stamp + 1 because LORA clock is behind 1 second. see old daq.
  	event[i].GPS_time_stamp = GPS_time_stamp ;
    event[i].Time_Msg_Rcvd_ms = Time_Msg_Rcvd_ms;
  	event[i].CTD = CTD;
  	event[i].nsec = nsec;
  	event[i].Trigg_condition = Trigg_condition;
  	event[i].Trigg_pattern = Trigg_pattern;
  	event[i].Total_counts  = -99.0;
  	event[i].Corrected_peak = -99.0;
  	event[i].Raw_peak = 0;
  }

	short unsigned a1,b1,a2,b2,a3,b3,ch1_bin1,ch1_bin2 ;	//For channel 1
	short unsigned A1,B1,A2,B2,A3,B3,ch2_bin1,ch2_bin2 ;	//For channel 2
	int k=0 ;

	for(int j=22;j<=5998+22;j+=3)
  //it seems content of 2 bins is spread in 3 bits
  //of the event msg from the lasa-clients
  //after 6000 is second channel. being loaded simultaneously
	{
		a1=(msg[j]>>4)&0xf ;
		b1=msg[j]&0xf ;
		a2=(msg[j+1]>>4)&0xf ;
		ch1_bin1=(a1<<8)+(b1<<4)+a2 ;

    A1=(msg[j+6000]>>4)&0xf ;
		B1=msg[j+6000]&0xf ;
		A2=(msg[j+6000+1]>>4)&0xf ;
		ch2_bin1=(A1<<8)+(B1<<4)+A2 ;

		event[0].counts[k]=(int)ch1_bin1 ;
		event[1].counts[k]=(int)ch2_bin1 ;
		k++ ;

    b2=msg[j+1]&0xf ;
		a3=(msg[j+2]>>4)&0xf ;
		b3=msg[j+2]&0xf ;
		ch1_bin2=(b2<<8)+(a3<<4)+b3 ;

		B2=msg[j+6000+1]&0xf ;
		A3=(msg[j+6000+2]>>4)&0xf ;
		B3=msg[j+6000+2]&0xf ;
		ch2_bin2=(B2<<8)+(A3<<4)+B3 ;

		event[0].counts[k]=(int)ch1_bin2 ;
		event[1].counts[k]=(int)ch2_bin2 ;
    k++ ;
  }

  // //--------Event time stamp in nanosecs (w.r.t run_start_time and is used
  //"only" for checking the coincidences)--------
  // The calculation below assumes:-> 1 year=365 days; 1 month=30 days;
  // 1 day=24 hrs; 1 hour=60 mins; 1 min=60 sec
  auto time_stamp=(long long unsigned)((year-rs_time.tm_year)*365*24*60*60);//to s
  time_stamp+=(long long unsigned)((month-rs_time.tm_mon)*30*24*60*60);//to s
  time_stamp+=(long long unsigned)((day-rs_time.tm_mday)*24*60*60);//to s
  time_stamp+=(long long unsigned)((hour-rs_time.tm_hour)*60*60);//to s
  time_stamp+=(long long unsigned)((min-rs_time.tm_min)*60);//to s
  time_stamp+=(long long unsigned)(sec-rs_time.tm_sec);
  time_stamp*=1000000000; //to ns
  time_stamp+=(long long unsigned)(nsec) ;

  for (int i=0;i<2;i++)
  {
    //add both event containers to event_spool of corresponding
    //channel. m_or_s=0 for Master, m_or_s=1 for Slave.
    if (m_or_s=="Master")
    {
      event_spool[i].push_back(std::make_pair(event[i],time_stamp));
    }
    else if (m_or_s=="Slave")
    {
      event_spool[2+i].push_back(std::make_pair(event[i],time_stamp));
    }
  }
}

void LORA_STATION_V1::Update_current_CTP(const int& m_or_s)
{
  if (osm_spool[m_or_s].size()==0)
  {
    std::string errormsg="No OSM MSG FOUND TO PULL CTP FROM...";
    throw std::runtime_error(errormsg);
  }
  //FIXIT: once i saw this error above being thrown. how? since its only
  //called after Unpack_OSM_Msg_Store_To_Spool is called.
  int last_element = osm_spool[m_or_s].size() -1 ;
  current_CTP[m_or_s] = osm_spool[m_or_s][last_element].CTP;
}

void LORA_STATION_V1::Unpack_OSM_Msg_Store_To_Spool(const std::vector<unsigned char>& msg,
                                   const int& m_or_s,
                                   tm& rs_time)
{
  ONE_SEC_STRUCTURE osm;

  osm.Lasa=station_no;

  unsigned char header = msg[0];
  unsigned char identifier_bit = msg[1] ;
  short unsigned int day=msg[2] ;
	short unsigned int month=msg[3] ;
	short unsigned int year=(msg[4]<<8)+msg[5] ;

  short unsigned int hour=msg[6] ;
  short unsigned int min=msg[7] ;
  short unsigned int sec=msg[8] ;

	short unsigned int sync=(msg[9]>>7)&0x1 ;
  osm.sync = (unsigned int) sync;
  //highest bit of CTP 'indicates' whether 2.5ns synch error offset has to be
  //applied for ns level time assignment. refer to HISPARC manual.
	osm.CTP=((msg[9]&0x7f)<<24)+(msg[10]<<16)+(msg[11]<<8)+msg[12] ;

	unsigned char buf[4] ;
	for(int i=0;i<4;++i) buf[i]=msg[16-i] ;
	osm.quant=*(float*)buf ;
  //FIXIT: check if this is a really valid syntax?!

  tm t; t.tm_sec=sec ; t.tm_min=min ;
  t.tm_hour=hour ; t.tm_mday=day ;
	t.tm_mon=month-1 ; t.tm_year=year-1900 ;
	//Because GPS month starts from 1 while in 'tm' struct, it starts from 0.
	osm.GPS_time_stamp=(unsigned int)timegm(&t) ;
  //GPS_time_stamp + 1 because LORA clock is behind 1 second. see old daq.
  osm.GPS_time_stamp+=1;

	osm.Channel_1_Thres_count_high=(msg[17]<<8)+msg[18] ;
	osm.Channel_1_Thres_count_low=(msg[19]<<8)+msg[20] ;
  osm.Channel_2_Thres_count_high=(msg[21]<<8)+msg[22] ;
 	osm.Channel_2_Thres_count_low=(msg[23]<<8)+msg[24] ;

  for (int i=0;i<61;++i) osm.Satellite_info[i]= msg[i+25] ;

  osm.Master_or_Slave=m_or_s;

  osm_spool[m_or_s].push_back(osm);

  if(rs_time.tm_year==0)
  {
    std::cout << "Setting the RUN START TIME...............!\n";
    if (year==0)
    {
      std::string errormsg="OSM msg has year==0...";
      throw std::runtime_error(errormsg);
    }
    t.tm_mon=month ;
    t.tm_year=year ;
    rs_time = t;
  }

}

std::string LORA_STATION_V1::Send_Name()
{
  return name;
}

void LORA_STATION_V1::Process_Event_Spool_Before_Coinc_Check(DETECTOR_CONFIG& det_config,
                                                            int& wait_another_iteration)
{
  // Calculate properties for all traces present
  // in the event_spool - before they are forwarded to OPERATIONS
  // class for event forming and eventually discarded.

  for (int i=0; i<4; ++i)
  {
    for (int j=0;j<event_spool[i].size();++j)
    {
      //check if this trace had already been processed.
      if (event_spool[i][j].first.Total_counts!=-99.0) continue;

      //redundant but keeping for later reference.
      // the following was being used in the old DAQ for counting
      // n detectors for trigger decision.
      //assign trig bit to each channel by reading ch0's trig_pattern
      // unsigned short chtrig[4];
      // chtrig[0]= (event_spool[0][j].first.Trigg_pattern)&0x1 ;
      // chtrig[1]=(event_spool[0][j].first.Trigg_pattern>>2)&0x1 ;
      // chtrig[2]=(event_spool[0][j].first.Trigg_pattern>>4)&0x1 ;
      // chtrig[3]=(event_spool[0][j].first.Trigg_pattern>>6)&0x1 ;
      // event_spool[i][j].first.Has_trigg = chtrig[i];

      //process waveform to get baseline/peak/counts/etc.
      int *temp = event_spool[i][j].first.counts;
      std::vector<int> trace(temp, temp+4000);
      int wpre= det_config.wvfm_process_wpre;
      int wpost = det_config.wvfm_process_wpost;
      int offwtrace_length= det_config.wvfm_process_offtwlen;
      int raw_peak;
      float baseline, corrected_peak, integrated_counts,offw_std;
      Process_Waveform(trace,wpre,wpost,offwtrace_length,
                       raw_peak, baseline, corrected_peak,
                       integrated_counts, offw_std);

      //use the actual waveform to re-evaulate if it would have triggered.
      //this way, even though the actual ADC count used by the Digitizer
      //for threshold is not known(we supply mV and we roughly know mV2ADC)
      if (raw_peak>=current_threshold_ADC[i])
      {
        event_spool[i][j].first.Has_trigg=1;
      }
      else
      {
        event_spool[i][j].first.Has_trigg=0;
      }

      event_spool[i][j].first.Threshold_ADC= (unsigned int) current_threshold_ADC[i];

      //Store calculated quantities to apt longterm variables.
      hundred_means[i].push_back(baseline);
      hundred_sigmas[i].push_back(offw_std);
      hundred_timestamps[i].push_back(event_spool[i][j].second);

      if (event_spool[i][j].first.Has_trigg==1)
      hundred_thresh_crossing_timestamps[i].push_back(event_spool[i][j].second);

      // if (j==0)
      //   hundred_station_event_timestamps.push_back(event_spool[i][j].second);

      event_spool[i][j].first.Total_counts = integrated_counts;
      event_spool[i][j].first.Corrected_peak = corrected_peak;
      event_spool[i][j].first.Raw_peak = raw_peak;
    }
  }

  if (event_spool[0].size()!=event_spool[1].size() ||
      event_spool[0].size()!=event_spool[2].size() ||
      event_spool[0].size()!=event_spool[3].size() )
    {
      if (event_spool[0].size()<4 && event_spool[0].size()>0)
      {
        std::cout << "Event_Spools sizes,"
        << name << ":" << event_spool[0].size() << ","
        << event_spool[1].size() << "," << event_spool[2].size()
        << "," << event_spool[3].size() << std::endl;

        request_OPS_to_wait_another_iteration+=1;

        if (request_OPS_to_wait_another_iteration<=3)
        {
          wait_another_iteration=1;
          return;
        }
      }
    }

  //if request had been made 3 times already, set counter 0 and move on.
  // or if it has never been made before... set it 0 anyway.
  request_OPS_to_wait_another_iteration=0;

  return;
}

void LORA_STATION_V1::Unpack_CP_Msg_Update_Threshold(const std::vector<unsigned char>& msg,
                                  const std::string& m_or_s)
{

  if (m_or_s!="Master" && m_or_s!="Slave")
  {
    std::stringstream ss;
    ss << "Unpack_Event_Msg_Store_To_Spool() got invalid input for m_or_s. 'Master' or 'Slave' allowed. ";
    ss << "\n . Received: " << m_or_s;
    throw std::runtime_error(ss.str());
  }

  CONTROL_STRUCTURE cp[2];//ch1 . ch2
  // keeping code for unpacking complete cp. but not required at the moment.
  // std::tm *t = Get_Current_Time();
  //
  // cp[0].Detector = Get_Detector_Number(name, m_or_s, 0);
  //
  // cp[1].Detector = Get_Detector_Number(name, m_or_s, 1);
  //
  // cp[0].Time_stamp=(unsigned int)timegm(t) ;
  //
	// cp[1].Time_stamp=(unsigned int)timegm(t) ;
  //
	// cp[0].Channel_offset_pos=msg[2] ;
	// cp[0].Channel_offset_neg=msg[3] ;
  //
	// cp[1].Channel_offset_pos=msg[4] ;
	// cp[1].Channel_offset_neg=msg[5] ;
  //
	// cp[0].Channel_gain_pos=msg[6] ;
	// cp[0].Channel_gain_neg=msg[7] ;
  //
	// cp[1].Channel_gain_pos=msg[8] ;
	// cp[1].Channel_gain_neg=msg[9] ;
  //
	// cp[0].Common_offset_adj=msg[10] ;
	// cp[1].Common_offset_adj=msg[10] ;
  //
	// cp[0].Full_scale_adj=msg[11] ;
	// cp[1].Full_scale_adj=msg[11] ;
  //
	// cp[0].Channel_inte_time=msg[12] ;
	// cp[1].Channel_inte_time=msg[13] ;
  //
	// cp[0].Comp_thres_low=msg[14] ;
	// cp[1].Comp_thres_low=msg[14] ;
  //
	// cp[0].Comp_thres_high=msg[15] ;
	// cp[1].Comp_thres_high=msg[15] ;
  //
	// cp[0].Channel_HV=(unsigned short)(msg[16]*7.93155-34.0714) ;
	// cp[1].Channel_HV=(unsigned short)(msg[17]*7.93155-34.0714) ;

	cp[0].Channel_thres_low=(unsigned short)(((msg[18]<<8)+msg[19])*0.4883) ;
	// cp[0].Channel_thres_high=(unsigned short)(((msg[20]<<8)+msg[21])*0.4883) ;

	cp[1].Channel_thres_low=(unsigned short)(((msg[22]<<8)+msg[23])*0.4883) ;
	// cp[1].Channel_thres_high=(unsigned short)(((msg[24]<<8)+msg[25])*0.4883) ;
  //
	// cp[0].Trigg_condition=msg[26] ;
	// cp[1].Trigg_condition=msg[26] ;
  //
	// cp[0].Pre_coin_time=((msg[27]<<8)+msg[28])*5 ;
	// cp[1].Pre_coin_time=((msg[27]<<8)+msg[28])*5 ;
  //
	// cp[0].Coin_time=(unsigned short)(((msg[29]<<8)+msg[30])*5) ;
	// cp[1].Coin_time=(unsigned short)(((msg[29]<<8)+msg[30])*5) ;
  //
	// cp[0].Post_coin_time=(unsigned short)(((msg[31]<<8)+msg[32])*5) ;
	// cp[1].Post_coin_time=(unsigned short)(((msg[31]<<8)+msg[32])*5) ;
  //
  // cp[0].Station=station_no ;
	// cp[1].Station=station_no ;

  //look for github version where cp_spools were maintained.
  //if you want to go back to storing all cp_ params...
  int temp_i=0;
  if (m_or_s=="Master") {temp_i=0;}
  else if (m_or_s=="Slave") {temp_i=2;}

  // if (cp[0].Channel_thres_low*2!=current_threshold_ADC[temp_i+0])
  // {
  //   std::cout << name <<  " Ch: " << temp_i+0+1
  //   << ", software tracked thresh: " << current_threshold_ADC[temp_i+0]
  //   << " digitizer reported: " << cp[0].Channel_thres_low*2;
  // }
  //
  // if (cp[1].Channel_thres_low*2!=current_threshold_ADC[temp_i+1])
  // {
  //   std::cout << name << " Ch: " << temp_i+1+1
  //   << ", software tracked thresh: " << current_threshold_ADC[temp_i+0]
  //   << " digitizer reported: " << cp[0].Channel_thres_low*2;
  // }

  current_threshold_ADC[temp_i+0]=cp[0].Channel_thres_low*2;
  current_threshold_ADC[temp_i+1]=cp[1].Channel_thres_low*2;
}

void LORA_STATION_V1::Reset_Electronics()
{
	unsigned char dat[3] ;
	dat[0]=0x99 ;
	dat[1]=0xff ;
	dat[2]=0x66 ;
  auto lenofbuffer = sizeof(dat)/sizeof(dat[0]);

  for (int i=0;i<2;i++)
  {
    int bytes_sent = 0;
    bool use_spare_socket=true;
    socket[i]->Send(dat,lenofbuffer,bytes_sent,use_spare_socket);
    std::cout << "Reset Electronics Msg Sent. Size: " << bytes_sent << std::endl;
  }
}

void LORA_STATION_V1::Run_Detectors_Diagnostics(const std::string& diagnostics_fname,
                                        const DETECTOR_CONFIG& dcfg,
                                        const tm& rs_time, bool& exceeding_trigger_rate)
{
  std::ofstream outfile;
  std::tm *t = Get_Current_Time();
  unsigned int GPS_time_stamp=(unsigned int)timegm(t) ;

  short unsigned int year = t->tm_year + 1900;
  short unsigned int month = t->tm_mon + 1;
  short unsigned int day = t->tm_mday;
  short unsigned int hour = t->tm_hour;
  short unsigned int min = t->tm_min;
  short unsigned int sec = t->tm_sec;

  auto time_stamp_now=(long long unsigned)((year-rs_time.tm_year)*365*24*60*60);//to s
  time_stamp_now+=(long long unsigned)((month-rs_time.tm_mon)*30*24*60*60);//to s
  time_stamp_now+=(long long unsigned)((day-rs_time.tm_mday)*24*60*60);//to s
  time_stamp_now+=(long long unsigned)((hour-rs_time.tm_hour)*60*60);//to s
  time_stamp_now+=(long long unsigned)((min-rs_time.tm_min)*60);//to s
  time_stamp_now+=(long long unsigned)(sec-rs_time.tm_sec);
  time_stamp_now*=1000000000; //to ns

  outfile.open(diagnostics_fname,std::fstream::app);
  int avg_over_dt= 5*60*1000; // ms

  for (int i=0; i<4; ++i)
  {
    //its 5 mins actually. not renaming vars again.
    float five_min_mean=0;
    float five_min_sigma=0;
    int five_min_n=0;
    int five_min_n_thresh=0;

    Time_Average_From_Circ_Buffer(hundred_means[i],
                                  hundred_timestamps[i],
                                  time_stamp_now,
                                  avg_over_dt,
                                  five_min_mean, five_min_n);

    Time_Average_From_Circ_Buffer(hundred_sigmas[i],
                                  hundred_timestamps[i],
                                  time_stamp_now,
                                  avg_over_dt,
                                  five_min_sigma, five_min_n);

    Time_Average_From_Circ_Buffer(hundred_thresh_crossing_timestamps[i],
                                  time_stamp_now,
                                  avg_over_dt,
                                  five_min_n_thresh);

    outfile << GPS_time_stamp
    << "\t" << Get_Detector_Number(station_no,i)
    << "\t" << five_min_mean
    << "\t" << five_min_sigma
    << "\t" << five_min_n_thresh
    << "\t" << current_threshold_ADC[i]
    << std::endl;

    if (dcfg.calibration_mode==0 && five_min_n_thresh>=dcfg.max_trigg_rate_to_reset_thresh)
      exceeding_trigger_rate=true;
  }

  outfile.close();

  return;
}

void LORA_STATION_V1::Set_New_Threshold()
{
  for (int i=0; i<2; ++i)
  {
    std::cout << name << ", " << i << ": " << current_threshold_ADC[0+2*i] << " "
    << current_threshold_ADC[1+2*i] << std::endl;
    //mVolts2ADC=2.0 , that is the 2.0 factor below.
    unsigned char dat[7] ;
    dat[0]=0x99 ;
    dat[1]=0x77 ;		//We have use identifier '77' for the thresholds
    dat[2]=((unsigned short)(current_threshold_ADC[0+2*i]/(2.0*0.4883)) & 0xff00)>>8 ;
    dat[3]=((unsigned short)(current_threshold_ADC[0+2*i]/(2.0*0.4883)) & 0x00ff) ;
    dat[4]=((unsigned short)(current_threshold_ADC[1+2*i]/(2.0*0.4883)) & 0xff00)>>8 ;
    dat[5]=((unsigned short)(current_threshold_ADC[1+2*i]/(2.0*0.4883)) & 0x00ff) ;
    dat[6]=0x66 ;
    auto lenofbuffer = sizeof(dat)/sizeof(dat[0]);
    int bytes_sent;
    bool use_spare=true;
    socket[i]->Send(dat,lenofbuffer, bytes_sent, use_spare);
  }
}

void LORA_STATION_V1::Status_Monitoring(bool& fatal_error, std::string& error_msg)
{
  //fatal error is usually false before being passed to this method.
  //can be true if another instance of LORA_STATION_V1 had set it to true
  //before this station gets it. Then this station will provide additional
  //error_msg if any.
  for (int i=0; i<2; ++i)
  {
    if (buf_socket[i]->Is_Buffer_Full())
    {
      fatal_error=true;
      std::stringstream ss;
      ss << name << " buf_socket[" << i <<  "] is full \n" ;
      error_msg += ss.str();
    }

    if (osm_spool[i].size()>300)
    {
      fatal_error=true;
      std::stringstream ss;
      ss << name << " osm_spool[" << i <<  "].size() =" << osm_spool[i].size()  ;
      error_msg += ss.str();
    }

    int dt_ms = Get_ms_Elapsed_Since(most_recent_msg_rcvd_time[i]);
    if (dt_ms >= 1*180*1000)//for one minute. at least 60 OSM msgs are missed!!
    {
      fatal_error=true;
      std::stringstream ss;
      ss << name << " " << i << " : No msgs received from this socket in "
      << dt_ms/1000 << "seconds." ;
      error_msg += ss.str();
    }

    int dt_ms2 = Get_ms_Elapsed_Since(most_recent_event_msg_rcvd_time[i]);
    if (dt_ms2 >= 1*60*45*1000)//if no event triggered for 45 minutes
    {
      fatal_error=true;
      std::stringstream ss;
      ss << name << " " << i << " : No EVENT msgs received from this socket in "
      << dt_ms2/(60*1000) << "minutes." ;
      error_msg += ss.str();
    }
  }

  for (int i=0; i<4; ++i)
  {
    if (event_spool[i].size()>250)
    {
      fatal_error=true;
      std::stringstream ss;
      ss << name << " event_spool[" << i <<  "].size() =" << event_spool[i].size()  ;
      error_msg += ss.str();
    }
  }
}

int LORA_STATION_V1::Get_Sum_Size_of_Spools()
{
  int sum=0;
  for (int i=0; i<4; ++i) sum+=event_spool[i].size();
  for (int i=0; i<2; ++i) sum+=osm_spool[i].size();
  return sum;
}
// Vestigial code.

// void LORA_STATION_V1::Send_CP_Delete_From_Spool(tControl_Params_Station& cp_station)
// {//we don't save complete cp msgs since all info is same except threshold.
//   for (int i=0;i<4;++i)
//   {
//     if (cp_spool[i].size()>0)
//     {
//       cp_station.push_back(cp_spool[i][0]);
//       cp_spool[i].erase(cp_spool[i].begin());
//     }
//     else
//     {
//       cp_station.push_back(CONTROL_STRUCTURE());
//     }
//   }
// }
