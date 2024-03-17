#include "OPERATIONS.h"
#include "Structs.h"
#include "LORA_STATION_V1.h"
#include "LORA_STATION_V2.h"//katie
#include "functions_library.h"
#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <memory>
#include <ctime> //to load current time. std::gmtime//struct tm
#include <cerrno>
#include <cstring>
#include <TFile.h>
#include <TTree.h>
#include <algorithm> //std::find(), std::sort
#include <csignal>
#include <chrono> //to measure time durations
#include "LOFAR_Communications.h"

//----------------public methods:

void OPERATIONS::Fiddle_With_Det_Config(const float& sigma_ovr_thresh,
                                        const std::vector<int>& list_of_dets_for_calib)
{
  //This function is for main_muon_calib.cxx
  std::cout << "Resetting Detector Config: " << std::endl;
  // std::cout << "sigma_ovr_thresh (old)= " << detector_config.sigma_ovr_thresh << std::endl;
  // std::cout << "list_of_dets_for_calib(old)= ";
  // for (int i=0; i<detector_config.list_of_dets_for_calib.size(); ++i)
  // std::cout << detector_config.list_of_dets_for_calib[i] << " " ;
  // std::cout << std::endl;

  //reset
  detector_config.list_of_dets_for_calib=list_of_dets_for_calib;
  detector_config.sigma_ovr_thresh=sigma_ovr_thresh;

  std::cout << "sigma_ovr_thresh (new)= " << detector_config.sigma_ovr_thresh << std::endl;
  std::cout << "list_of_dets_for_calib(new)= ";
  for (int i=0; i<detector_config.list_of_dets_for_calib.size(); ++i)
  std::cout << detector_config.list_of_dets_for_calib[i] << " " ;
  std::cout << std::endl;

  //The old file will be overwritten since this happens immediately after startup
  //filenames have time tags upto a minute. no two files within same minute will be
  //accommodated.
  Close_ROOT_File();
  Open_ROOT_File();

  return;
}

void OPERATIONS::Create_DAQ_Execution_Status_File()
{
  std::ofstream outfile;
  std::tm *t = Get_Current_Time();

  outfile.open(daq_on_off_filename);
  outfile << "# DAQ Code Run Status Set at: " ;

  outfile << t->tm_year+1900
  << ":" << t->tm_mon+1
  << ":" << t->tm_mday
  << " " << t->tm_hour
  << ":" << t->tm_sec << std::endl;

  int switch_on= 1;
  outfile << switch_on  << std::endl;

  outfile << "EOF" << std::endl;

  outfile.close();

  return;
}

bool OPERATIONS::Is_DAQ_Execution_Status_True()
{
  int dt_ms = Get_ms_Elapsed_Since(most_recent_daq_switch_check_time);
  if (dt_ms <= 500) return true;//check every 500ms.
  most_recent_daq_switch_check_time= Get_Current_Time_chrono();

  bool execution_status_from_file=false;//will become true based on switch_value
  std::string line;

  std::ifstream infile;
  infile.open(daq_on_off_filename);

  if (infile.fail())
  {
    std::cout << __LINE__ << " " << __FILE__ << std::endl;
    throw std::runtime_error(daq_on_off_filename+" : file open failed.");
  }

  while (std::getline(infile, line))
  {
    if (line=="EOF") break;
    if (line.size()==0) continue; // empty line
    if (line.at(0)=='#') continue; // comment line

    auto switch_value= std::atoi(line.c_str());

    if (switch_value==1)
    {
      infile.close();
      execution_status_from_file=true;
      break;
    }
  }

  infile.close();

  return execution_status_from_file && self_execution_status;
}

void OPERATIONS::Initialize_All_TimeKeepers()
{
  most_recent_event_check_time = Get_Current_Time_chrono();
  most_recent_iteration_wait_time = Get_Current_Time_chrono();
  most_recent_reset_thresh_time = Get_Current_Time_chrono();
  most_recent_osm_store_time = Get_Current_Time_chrono();
  most_recent_cp_store_time = Get_Current_Time_chrono();
  most_recent_daq_switch_check_time = Get_Current_Time_chrono();
  most_recent_print_diagnostics_time = Get_Current_Time_chrono();
  most_recent_hourly_check_time = Get_Current_Time_chrono();
  most_recent_status_monitoring_time = Get_Current_Time_chrono();
}

void OPERATIONS::Init(const std::string net_file,
                      const std::string det_file,
                      const std::string cp_file,
                      const std::string cp_file_V2_det, //katie
                      const std::string cp_file_V2_stn, //katie
                      const std::string daq_on_off_file,
                      const std::string det_coord_file)
{
  //initialize time variables
  Initialize_All_TimeKeepers();
  most_recent_event_id=0;

  self_execution_status=true;

  run_start_time.tm_year =0; run_start_time.tm_mon =0;
  run_start_time.tm_hour =0; run_start_time.tm_min =0;
  run_start_time.tm_min =0; run_start_time.tm_sec =0;

  //Create the file and set code run status to 1.
  daq_on_off_filename=daq_on_off_file;
  Create_DAQ_Execution_Status_File();

  Read_Network_File(net_file);

  Read_Detector_File(det_file);

  current_reset_thresh_interval=detector_config.init_reset_thresh_interval;
  current_check_coinc_interval=detector_config.check_coinc_interval;
  max_iteration_wait_interval=detector_config.max_iteration_wait_interval;

  Read_ControlParam_File(cp_file);
  Read_ControlParam_File_V2_det(cp_file_V2_det); //katie
  Read_ControlParam_File_V2_stn(cp_file_V2_stn); //katie

  Read_Det_Coord_File(det_coord_file);

  //fix output filenames
  Set_Output_File_Names_And_RunId();

  Init_LORA_Array();

  Init_Final_Containers();

  Open_ROOT_File();

  return;
}

void OPERATIONS::Connect_To_Stations()
{
  for (int i=0; i<lora_array_ptrs.size();i++)
  {
    lora_array_ptrs[i]->Open(); // will in turn do open/bind/Accept for all sockets.
  }

  int lofar_nc_index;
  for (lofar_nc_index=0; lofar_nc_index<network_config.size();++lofar_nc_index)
  {if (network_config[lofar_nc_index].type=="superhost") break;}

  std::unique_ptr<LOFAR_COMMS> temp_ptr(new LOFAR_COMMS);
  lofar_radio_comm = std::move(temp_ptr);
  lofar_radio_comm->Init(network_config[lofar_nc_index],detector_config);
  lofar_radio_comm->Open();
  return;
}

void OPERATIONS::Send_Control_Params()
{
  if (!self_execution_status) return;

  if (detector_config.calibration_mode==1)
  {
    for (int i=0; i<lora_array_ptrs.size();i++)
      lora_array_ptrs[i]->Send_Electronics_Calib_Msg();
  }
  else
  {
    for (int i=0; i<lora_array_ptrs.size();i++)
      lora_array_ptrs[i]->Send_Control_Params();
  }

  return;
}

void OPERATIONS::Accept_Connections_From_Stations()
{
  int n_connections = 0 ;
  for (int i=0; i<network_config.size();i++)
  {
    //check if this station
    //was present in list of active_stations in detector_config
    auto vec = detector_config.active_stations;
    auto station_is_active = std::find(vec.begin(), vec.end(), network_config[i].name)!= vec.end();
    if (!station_is_active) continue;//if not , skip it.

    if (network_config[i].type=="clientv1")
    {
      n_connections+=4;
    }
    else if (network_config[i].type=="clientv2")
    {
      n_connections+=2;
    }
  }

  int n_accepted_connections=0;
  int n_iterations=0;

  while(n_accepted_connections<n_connections)
  {
    n_iterations+=1;
    if (!Is_DAQ_Execution_Status_True() || n_iterations>180)
    {
      self_execution_status=false;
      std::cout << "Ending DAQ... from within Accept_Connections_From_Stations()\n";
      break;
    }

    sleep(1);
    int max_fd_val=0;
    int n_fds_ready=0;
    fd_set active_stations_fds;
    struct timeval timeout, waittime;
    timeout.tv_sec =0; timeout.tv_usec = 10*1000; // 10 ms

    //Collect list of file descriptors for active stations
    FD_ZERO(&active_stations_fds);
    for (int i=0; i<lora_array_ptrs.size(); ++i)
    {
      lora_array_ptrs[i]->Add_readfds_To_List(active_stations_fds,max_fd_val);
    }

    // wait till one of them is ready for reading or until timeout.
    n_fds_ready = select(max_fd_val+1, &active_stations_fds, NULL, NULL, &timeout);

    if (n_fds_ready<0)
    {//if select() throws an error:
      //printf("line: %d in: %s\n", __LINE__, __FILE__);
      std::string errormsg = "Error from select() under";
      errormsg+= " OPERATIONS::Listen_To_Stations(). ";
      errormsg+= std::string(std::strerror(errno));
      std::cout << errormsg << std::endl;
      n_iterations=100;
    }

    if (n_fds_ready==0) continue;

    for (int i=0; i<lora_array_ptrs.size(); ++i)
    {
      n_accepted_connections+= lora_array_ptrs[i]->Accept(active_stations_fds);
    }

    std::cout << "accepted # connections: " << n_accepted_connections << std::endl;
  }

  //have to reinitialize because the new HV units take so much frickin' time
  //to get setup.
  Initialize_All_TimeKeepers();
  return;
}

void OPERATIONS::Listen_To_Stations()
{
  int max_fd_val=0;
  int n_fds_ready=0;
  fd_set active_stations_fds;
  struct timeval timeout, waittime;
  timeout.tv_sec =0; timeout.tv_usec = 10*1000; // 10 ms
  waittime.tv_sec =0; waittime.tv_usec = 1*1000; // 1 ms

  //simple wait
  select(1, NULL, NULL, NULL, &waittime);
  //FIXIT: replace wait time value with something from the detector config.

  //Collect list of file descriptors for active stations
  FD_ZERO(&active_stations_fds);
  for (int i=0; i<lora_array_ptrs.size(); ++i)
  {
    lora_array_ptrs[i]->Add_readfds_To_List(active_stations_fds,max_fd_val);
  }

  // wait till one of them is ready for reading or until timeout.
  n_fds_ready = select(max_fd_val+1, &active_stations_fds, NULL, NULL, &timeout);
  //https://stackoverflow.com/questions/589783/not-able-to-catch-sigint-signal-while-using-select

  if (n_fds_ready<0)
  {//if select() throws an error:
    //printf("line: %d in: %s\n", __LINE__, __FILE__);
    std::string errormsg = "Error from select() under";
    errormsg+= " OPERATIONS::Listen_To_Stations(). ";
    errormsg+= std::string(std::strerror(errno));
    std::cout << errormsg << std::endl;
    self_execution_status=false;
  }

  if (n_fds_ready==0) return;

  for (int i=0; i<lora_array_ptrs.size(); ++i)
    lora_array_ptrs[i]->Listen(active_stations_fds);

  return;
}

void OPERATIONS::Interpret_And_Store_Incoming_Msgs()
{
  if (detector_config.calibration_mode==1)
  {
    sleep(15);
    for (int i=0; i<lora_array_ptrs.size();i++)
    {
      int j;
      for (j=0; j<network_config.size();++j)
      {
        if (network_config[j].name==lora_array_ptrs[i]->Send_Name())
        break;
      }
      lora_array_ptrs[i]->Receive_Electronics_Calib_Msg(detectors_calibrated,
                                                      output_elec_calib_filename,
                                                      network_config[j]);
      if (detectors_calibrated>0)
      {
        std::cout << lora_array_ptrs[i]->Send_Name() << " " << detectors_calibrated
        << " / " << 2*detector_config.active_stations.size() << std::endl;
      }
    }

    if (detectors_calibrated==2*detector_config.active_stations.size())
      self_execution_status=false;
  }
  else
  {
    for (int i=0; i<lora_array_ptrs.size(); ++i)
      lora_array_ptrs[i]->Interpret_And_Store_Incoming_Msgs(run_start_time);
  }
  return;
}

void OPERATIONS::Check_Coinc_Store_Event_Send_LOFAR_Trigger(const bool& daq_ending)
{ //check for coincidences and construct an event.
  //time elapsed since last check.
  if (detector_config.calibration_mode==1) return;

  int dt_ms = Get_ms_Elapsed_Since(most_recent_event_check_time);
  if (dt_ms <= current_check_coinc_interval) return;

  int wait_another_iteration=0;
  //Get noise information from waveforms before doing anything with them
  Process_Event_Spool_Before_Coinc_Check(wait_another_iteration);

  int dt_ms2 = Get_ms_Elapsed_Since(most_recent_iteration_wait_time);

  if (wait_another_iteration==1 && !daq_ending && dt_ms2 <= max_iteration_wait_interval)
  {
    //i.e. Process_Event_Spool_Before_Coinc_Check() has altered "wait_another_iteration"
    std::cout <<" ---wait one more iteration..upto a limit of----"<< max_iteration_wait_interval/1000<< "s."<<std::endl;
    most_recent_iteration_wait_time=Get_Current_Time_chrono();
    return;
  }

  // set the interval to default value again.
  current_check_coinc_interval=detector_config.check_coinc_interval;
  most_recent_event_check_time= Get_Current_Time_chrono();
  // std::cout << "dt_check_coinc_ms:" << dt_ms << std::endl;

  //Collect all hit information from all stations.
  tvec_EVENT_SPOOL_SUMMARY all_hits;
  for (int i=0; i<lora_array_ptrs.size(); ++i)
    lora_array_ptrs[i]->Send_Event_Spool_Info(all_hits);
  int all_hits_size=all_hits.size();

  // std::cout << "all_hits_size:" << all_hits_size << std::endl;

  while (all_hits_size>0)
  {
    //sort all hits by time. event = first hit + rest within a timewindow
    std::sort(all_hits.begin(), all_hits.end(), compare_by_time_stamp);

    //check that the last hit in the all_hits[] is not within coin window
    int last_index = all_hits.size() -1 ;
    long long unsigned delta_ns1 = all_hits[last_index].time_stamp - all_hits[0].time_stamp;

    //Implement Pre Emptive Trigger:
    if (delta_ns1<=detector_config.coin_window && !daq_ending && detector_config.calibration_mode==0)
    {
      //stop recurrence of the same pre emptive trigger.
      if (most_recent_trigg_GPS==all_hits[0].GPS_time_stamp) break;

      //--------------------------------------------------------
      //Implement Pre Emptive Trigger:
      //--------------------------------------------------------
      //special case: when daq is ending, pls go ahead with event formation.
      //in case the timenow-GPS_time_stamp_of_last_hit>2
      //check whether trig condition has been satisfied.
      //send radio trigg. move on with rest of the code as is.
      //event will be formed when delta_ns1>detector_config.coin_window
      //this was implemented because triggers to astron
      //have to be sent ASAP - at least within 5s. for tbb dumps.
      //the actual LORA event forming is delayed for when the next hit outside coinc window
      //arrives.
      //FIXIT: hardcoded radio trigger for more than 4 detectors ! !!
      //--------------------------------------------------------

      std::tm *tnow = Get_Current_Time();
      unsigned int GPS_t_now = (unsigned int)timegm(tnow);
      if (GPS_t_now - all_hits[last_index].GPS_time_stamp >= 1 && all_hits.size()>4)
      {
        int fn_detectors=0;
        std::vector<std::string> funiq_statns;
        tvec_EVENT_SPOOL_SUMMARY fevent;

        fevent.push_back(all_hits[0]);
        fn_detectors+= all_hits[0].Has_trigg;
        //include all subsequent hits in event since they are within delta_ns1 defined above.
        for (int i=1; i<all_hits.size();++i)
        {
          fevent.push_back(all_hits[i]);
          auto temp_it = std::find(funiq_statns.begin(),
                                   funiq_statns.end(),
                                   all_hits[i].station_name);
          if (temp_it== funiq_statns.end())
              funiq_statns.push_back(all_hits[i].station_name);

          fn_detectors+= all_hits[i].Has_trigg;
        }

        int flora_trig_satisfied, flofar_trig_satisfied;
        int fn_stations=funiq_statns.size();

        Return_Trigger_Decision(flora_trig_satisfied, flofar_trig_satisfied,
                                fn_detectors, fn_stations);

        if (flofar_trig_satisfied==1)
        {
          //send time for the first hit of the event.
          unsigned int ftrigg_sent;
          most_recent_trigg_GPS=all_hits[0].GPS_time_stamp;
          lofar_radio_comm->Send_Trigger(all_hits[0].GPS_time_stamp, all_hits[0].nsec,ftrigg_sent);
          std::cout << std::endl;
          std::cout << "------ <<<< Radio trigger >>>> ------  " << std::endl;
          std::cout << std::endl;

          int pre_emptive_trigg=1;
          Print_Array_Diagnostics(fevent,fn_detectors,fn_stations,
                                  flora_trig_satisfied, flofar_trig_satisfied, ftrigg_sent,
                                  GPS_t_now, pre_emptive_trigg);
        }
      }
      //the code should wait before event forming.
      break;
    }

    //construct the event & list of unique stations.
    tvec_EVENT_SPOOL_SUMMARY event;
    std::vector<std::string> uniq_statns;
    int n_detectors=0;

    //include the earliest hit in the event
    event.push_back(all_hits[0]);
    uniq_statns.push_back(all_hits[0].station_name);
    n_detectors+= all_hits[0].Has_trigg;

    //include all subsequent hits in event if they are within
    // coin_window entered in the detector_config file.
    for (int i=1; i<all_hits.size();++i)
    {
      long long unsigned delta_ns = all_hits[i].time_stamp - all_hits[0].time_stamp;

      if (delta_ns<detector_config.coin_window)
      {
        event.push_back(all_hits[i]);
        auto temp_it = std::find(uniq_statns.begin(),
                                 uniq_statns.end(),
                                 all_hits[i].station_name);

        if (temp_it== uniq_statns.end())
            uniq_statns.push_back(all_hits[i].station_name);
        n_detectors+= all_hits[i].Has_trigg;
      }
      else
      {
        break;
      }
    }

    // std::cout << "event.size():" << event.size() << std::endl;
    std::cout << " event with " << n_detectors << " detectors: " << std::endl ;
    for (int kk=0;kk<uniq_statns.size();++kk) std::cout << uniq_statns[kk] << " ";
    std::cout << std::endl;

    int lora_trig_satisfied=0;
    int lofar_trig_satisfied=0;
    int n_stations=uniq_statns.size();

    Return_Trigger_Decision(lora_trig_satisfied, lofar_trig_satisfied,
                            n_detectors, n_stations);

    std::tm *t = Get_Current_Time();
    unsigned int event_reporting_GPS = (unsigned int)timegm(t);

    // won't be sent if this trig is within 6 mins of previous one.
    // take care of inside LOFAR_COMMS class.
    unsigned int trigg_sent=0;
    if (lofar_trig_satisfied==1)
    {
      //send time for the first hit of the event.
      most_recent_trigg_GPS=event[0].GPS_time_stamp;
      lofar_radio_comm->Send_Trigger(event[0].GPS_time_stamp,event[0].nsec,trigg_sent);
      std::cout << std::endl;
      std::cout << "------ <<<< Radio trigger >>>> ------  " << std::endl;
      std::cout << std::endl;
    }

    //Store event in ROOT file, ask LORA_STATIONs
    //to discard it from the spools
    if (lora_trig_satisfied==1)
      Store_Event(event, uniq_statns, lofar_trig_satisfied);

    int pre_emptive_trigg=0;
    Print_Array_Diagnostics(event,n_detectors,uniq_statns.size(),
                            lora_trig_satisfied, lofar_trig_satisfied, trigg_sent,
                            event_reporting_GPS,pre_emptive_trigg);

    for (int i=0; i<lora_array_ptrs.size(); ++i)
     lora_array_ptrs[i]->Discard_Events_From_Spool(event);

    //Collect all hit information from all stations.
    all_hits.clear();
    for (int i=0; i<lora_array_ptrs.size(); ++i)
      lora_array_ptrs[i]->Send_Event_Spool_Info(all_hits);
    all_hits_size=all_hits.size();
   }
  return;
}

void OPERATIONS::Print_Array_Diagnostics(const tvec_EVENT_SPOOL_SUMMARY& event,
                                         const int& n_detectors,
                                         const int& n_stations,
                                         const int& lora_trig_satisfied,
                                         const int& lofar_trig_satisfied,
                                         const unsigned int& trigg_sent,
                                         const unsigned int& event_reporting_GPS,
                                         const int& pre_emptive_trigg)
{
  std::ofstream outfile;
  outfile.open(output_array_log_filename,std::fstream::app);

  float total_charge=0;
  float core_position_x=0;
  float core_position_y=0;
  std::vector<float> charges;
  std::vector<float> peaks;
  for (int i=0; i<network_config.size(); ++i)
  {
    if (network_config[i].type=="host" || network_config[i].type=="superhost" )
    continue;
    for (int j=0;j<n_channels;++j)
    {
      charges.push_back(0);
      peaks.push_back(0);
    }
  }

  for (int i=0; i<event.size(); ++i)
  {
    total_charge+=event[i].charge;
    int j=Get_Detector_Number(event[i].station_no,event[i].evtspool_i);

    charges[j-1]=event[i].charge;
    peaks[j-1]=event[i].corrected_peak;
    core_position_x+=det_coord_x[j-1]*event[i].charge;
    core_position_y+=det_coord_y[j-1]*event[i].charge;
  }

  core_position_x /= total_charge;
  core_position_y /= total_charge;

  outfile << event[0].GPS_time_stamp
  << "\t" << event[0].nsec
  << "\t" << n_detectors
  << "\t" << n_stations
  << "\t" << total_charge
  << "\t" << core_position_x
  << "\t" << core_position_y
  << "\t" << lora_trig_satisfied
  << "\t" << lofar_trig_satisfied
  << "\t" << trigg_sent;

  for (int i=0; i<charges.size(); ++i)
  {
    outfile << "\t" << charges[i];
  }

  for (int i=0; i<peaks.size(); ++i)
  {
    outfile << "\t" << peaks[i];
  }

  outfile << "\t" << event_reporting_GPS
  << "\t" << pre_emptive_trigg
  << std::endl;

  outfile.close();
}

void OPERATIONS::Store_Event(const tvec_EVENT_SPOOL_SUMMARY& event,
                             const std::vector<std::string>& uniq_statns,
                             const int& lofar_trig_satisfied)
{
  //assign a common id to all traces in this event
  unsigned int evt_id = most_recent_event_id+ 1;
  most_recent_event_id=evt_id;
  std::cout <<" ----> event id: " << evt_id << std::endl;

  EVENT_HEADER_STRUCTURE this_evt_header;
  this_evt_header.Run_id = run_id;
  this_evt_header.Event_id = evt_id;
  this_evt_header.LOFAR_trigg = (unsigned int) lofar_trig_satisfied;
  this_evt_header.Event_tree_index = evt_tree_index;
  this_evt_header.Event_size = (unsigned int) event.size();
  evt_tree_index+=event.size();
  this_evt_header.GPS_time_stamp_firsthit = event[0].GPS_time_stamp;
  this_evt_header.Time_Msg_Rcvd_ms_firsthit = event[0].Time_Msg_Rcvd_ms;
  this_evt_header.nsec_firsthit = event[0].nsec;

  rootfile_event_header_platform=this_evt_header;
  tree_event_header->Fill();

  for (int i=0;i<lora_array_ptrs.size();++i)
  {
    auto station_in_event = std::find(uniq_statns.begin(), uniq_statns.end(),
                             lora_array_ptrs[i]->Send_Name())!=uniq_statns.end();
    if (!station_in_event) continue;

    //initiate a station container.
    tEvent_Data_Station temp_station_data;

    //collect event data from individual stations
    lora_array_ptrs[i]->Send_Event_Data(temp_station_data,event);

    if (temp_station_data.size()!=4)
    {
      std::stringstream errormsg;
      errormsg << "line:" << __LINE__ << "in " << __FILE__ << std::endl;
      errormsg << "Statn with !=4 Dets is being sent to root file." << std::endl;
      //throw std::runtime_error(errormsg.str());
      std::cout << errormsg.str() << std::endl;
    }

    for (int k=0;k<n_channels;++k)
    {
      temp_station_data[k].Event_id = evt_id;
      temp_station_data[k].LOFAR_trigg = (unsigned int) lofar_trig_satisfied;
      temp_station_data[k].Run_id = run_id;
    }

    for (int k=0; k<temp_station_data.size(); ++k)
    {
      rootfile_event_platform=temp_station_data[k];
      tree_event->Fill();
    }
  }
  return;
}

void OPERATIONS::Process_Event_Spool_Before_Coinc_Check(int& wait_another_iteration)
{
  for (int i=0; i<lora_array_ptrs.size(); ++i)
  {
    lora_array_ptrs[i]->Process_Event_Spool_Before_Coinc_Check(detector_config, wait_another_iteration);
  }
  return;
}

void OPERATIONS::Reset_Thresh_Store_Log()
{
  if (detector_config.calibration_mode==1) return;

  int dt_ms = Get_ms_Elapsed_Since(most_recent_reset_thresh_time);
  if (dt_ms <= current_reset_thresh_interval) return;
  most_recent_reset_thresh_time= Get_Current_Time_chrono();

  //increasing to standard value only after first full span of reset_thresh_interval
  // till then it stays at init_reset_thresh_interval
  if (current_reset_thresh_interval!=detector_config.reset_thresh_interval)
  {
    std::tm *t = Get_Current_Time();
    short unsigned int year = t->tm_year + 1900;
    short unsigned int month = t->tm_mon + 1;
    short unsigned int day = t->tm_mday;
    short unsigned int hour = t->tm_hour;
    short unsigned int min = t->tm_min;
    short unsigned int sec = t->tm_sec;

    auto time_stamp_now=(long long unsigned)((year-run_start_time.tm_year)*365*24*60*60);//to s
    time_stamp_now+=(long long unsigned)((month-run_start_time.tm_mon)*30*24*60*60);//to s
    time_stamp_now+=(long long unsigned)((day-run_start_time.tm_mday)*24*60*60);//to s
    time_stamp_now+=(long long unsigned)((hour-run_start_time.tm_hour)*60*60);//to s
    time_stamp_now+=(long long unsigned)((min-run_start_time.tm_min)*60);//to s
    time_stamp_now+=(long long unsigned)(sec-run_start_time.tm_sec);
    time_stamp_now*= 1000 ; //to ms.

    if (time_stamp_now>=detector_config.reset_thresh_interval)
      current_reset_thresh_interval=detector_config.reset_thresh_interval;
  }

  std::cout << "Reseting Threshold.... ..." << std::endl;
  for (int i=0; i<lora_array_ptrs.size();++i)
  {
    lora_array_ptrs[i]->Calculate_New_Threshold(detector_config,
                                                run_start_time,
                                                current_reset_thresh_interval);
    lora_array_ptrs[i]->Set_New_Threshold();
  }
  Store_Log();
}

void OPERATIONS::Store_OSM()
{
  if (detector_config.calibration_mode==1) return;
  int dt_ms = Get_ms_Elapsed_Since(most_recent_osm_store_time);
  //try to store OSM a.k.a. one second message every 500 ms
  if (dt_ms <= detector_config.osm_store_interval) return;
  most_recent_osm_store_time= Get_Current_Time_chrono();

  for (int i=0; i<lora_array_ptrs.size();i++)
  {
    //initialize empty osm containers
    //master and slave for V1
    //only first one used for V2.
    ONE_SEC_STRUCTURE temp_osm[2];

    //ask station to send its noise info.
    lora_array_ptrs[i]->Send_OSM_Delete_From_Spool(temp_osm[0],temp_osm[1]);

    if (temp_osm[0].Lasa>=1 && temp_osm[0].Lasa<=5)
    {
      rootfile_osm_platform=temp_osm[0];
      tree_osm_hisparc->Fill();
    }
    else if (temp_osm[0].Lasa>=6 && temp_osm[0].Lasa<=10)
    {
      rootfile_osm_platform=temp_osm[0];
      tree_osm_aera->Fill();
    }

    if (temp_osm[1].Lasa>=1 && temp_osm[1].Lasa<=5)
    {
      rootfile_osm_platform=temp_osm[1];
      tree_osm_hisparc->Fill();
    }
  }
}

void OPERATIONS::Periodic_Store_Log()
{
  if (detector_config.calibration_mode==1) return;
  int dt_ms = Get_ms_Elapsed_Since(most_recent_cp_store_time);
  //store log = threshold + noise info every log_interval
  if (dt_ms <= detector_config.log_interval) return;
  most_recent_cp_store_time= Get_Current_Time_chrono();
  Store_Log();
}

void OPERATIONS::Run_Detectors_Diagnostics()
{
  if (detector_config.calibration_mode!=0) return;
  int dt_ms = Get_ms_Elapsed_Since(most_recent_print_diagnostics_time);
  if (dt_ms <= detector_config.diagnostics_interval) return;
  most_recent_print_diagnostics_time= Get_Current_Time_chrono();

  bool exceeding_trigger_rate=false;
  for (int i=0;i<lora_array_ptrs.size();++i)
  lora_array_ptrs[i]->Run_Detectors_Diagnostics(output_detectors_log_filename,
                                        detector_config, run_start_time, exceeding_trigger_rate);

  if (exceeding_trigger_rate==true)
    current_reset_thresh_interval=detector_config.init_reset_thresh_interval;
}

void OPERATIONS::End()
{
  Close_ROOT_File();
  for (int i=0; i<lora_array_ptrs.size(); i++)
  {
    lora_array_ptrs[i]->Close();
  }
  lofar_radio_comm->Close();
}

bool OPERATIONS::Get_Execution_Status()
{
  return self_execution_status;
}

int OPERATIONS::Get_Sum_Size_of_Spools()
{
  int sum=0;
  for (int i=0; i<lora_array_ptrs.size(); ++i)
    sum+=lora_array_ptrs[i]->Get_Sum_Size_of_Spools();
  return sum;
}

//-----------------------private methods:
//----------------------------------------

void OPERATIONS::Read_Det_Coord_File(const std::string fname)
{
  /*
  Network File Structure:
  # COMMENT CHAR IS hash sign
  #
  fills out det_coord_x,det_coord_y vectors
  */
  std::string line;
  std::ifstream infile;

  infile.open(fname);

  if (infile.fail())
  {
    throw std::runtime_error(fname+" : file open failed.");
  }

  while (std::getline(infile, line))
  {
    if (line=="EOF") break;
    if (line.size()==0) continue; // empty line
    if (line.at(0)=='#') continue; // comment line
    float temp1,temp2, temp3;
    std::stringstream ss(line);
    ss >> temp3 >> temp1 >> temp2;

    det_coord_x.push_back(temp1);
    det_coord_y.push_back(temp2);
  }

  infile.close();

  if (det_coord_x.size()!=40 || det_coord_y.size()!=40)
  {
    throw std::runtime_error("det coordinates are not 20.");
  }
}

void OPERATIONS::Read_Network_File(const std::string fname)
{
  /*
  Network File Structure:
  # COMMENT CHAR IS hash sign
  # IPV4 is machine add on network
  # port numbers are for port number on the server machine i.e. main LORA PC
  # StationName   IPV4    port_1    port_2    spare_port    HISPARC_Serial_m    HISPARC_Serial_s
  lasa1   192.168.88.1    3301    3302    3303    FTQZTB7R    FTQZTB7R
  ....

  stations_info struct vector is filled out.
  */
  std::string line;
  std::ifstream infile;

  infile.open(fname);

  if (infile.fail())
  {
    throw std::runtime_error(fname+" : file open failed.");
  }

  while (std::getline(infile, line))
  {
    if (line=="EOF") break;
    if (line.size()==0) continue; // empty line
    if (line.at(0)=='#') continue; // comment line
    STATION_INFO temp;
    std::stringstream ss(line);
    ss >> temp.name  >> temp.type >> temp.IPV4 >> temp.port_1 >> temp.port_2
     >>  temp.HISPARC_Serial_m >> temp.HISPARC_Serial_s;

    if (temp.type=="clientv1" || temp.type=="clientv2")
      temp.no = std::stoi(temp.name.substr(4,2));
    // 2 because lasa count can go upto 10 which has 2 digits
    network_config.push_back(temp);
  }

  infile.close();

  //SANITY CHECK:check that active_stations have "lasa" in their names
  std::string retnval = verify_network_config(network_config);
  if (retnval!="") throw std::runtime_error(retnval);

}

void OPERATIONS::Read_Detector_File(const std::string fname)
{
  /*
  #contains configuration of the detector and this run.
  #keyword    value0    value1    value2    ...
  # active_stations   list all active stations with 2 tabs in between them.
  # complete list: lasa1 , lasa2, lasa3, lasa4, lasa5
  # lofar_trig_mode : 1: LORA stations 2: LORA detectors - > FOR LOFAR. Not LORA trigger.
  # lofar_trig_condition : Min no. of LORA Stations/Detectors required to send trigger to LOFAR
  # lora trig condition : Minimum no. of detectors to be triggered to accept an event
  */
  std::string line;
  std::ifstream infile;

  infile.open(fname);

  if (infile.fail())
  {
    throw std::runtime_error(fname+" : file open failed.");
  }

  //loop over lines
  while (std::getline(infile, line))
  {
    if (line=="EOF") break;
    if (line.size()==0) continue; // empty line
    if (line.at(0)=='#') continue; //commented line
    std::stringstream ss(line);
    std::string k;
    ss>>k;

    if (k=="active_stations")
    {
      //push rest of elements to active station vec
      std::string stationname_;
      while (ss>>stationname_)
      {
        detector_config.active_stations.push_back(stationname_);
      }
    }
    else if (k=="list_of_dets_for_calib")
    {
      //push rest of elements to active station vec
      int tdetno_;
      while (ss>>tdetno_)
      {
        detector_config.list_of_dets_for_calib.push_back(tdetno_);
      }
    }
    else if (k=="lofar_trig_mode")
      ss>> detector_config.lofar_trig_mode;
    else if (k=="lofar_trig_cond")
      ss>> detector_config.lofar_trig_cond;
    else if (k=="lora_trig_cond")
      ss>> detector_config.lora_trig_cond;
    else if (k=="lora_trig_mode")
      ss>> detector_config.lora_trig_mode;
    else if (k=="calibration_mode")
      ss>> detector_config.calibration_mode;
    else if (k=="log_interval")
      ss>> detector_config.log_interval;
    else if (k=="check_coinc_interval")
      ss>> detector_config.check_coinc_interval;
    else if (k=="max_iteration_wait_interval")
      ss>> detector_config.max_iteration_wait_interval;
    else if (k=="reset_thresh_interval")
        ss>> detector_config.reset_thresh_interval;
    else if (k=="init_reset_thresh_interval")
        ss>> detector_config.init_reset_thresh_interval;
    else if (k=="output_path")
      ss>> detector_config.output_path;
    else if (k=="coin_window")
      ss>> detector_config.coin_window;
    else if (k=="wvfm_process_wpre")
        ss>> detector_config.wvfm_process_wpre;
    else if (k=="wvfm_process_wpost")
        ss>> detector_config.wvfm_process_wpost;
    else if (k=="wvfm_process_offtwlen")
        ss>> detector_config.wvfm_process_offtwlen;
    else if (k=="diagnostics_interval")
        ss>> detector_config.diagnostics_interval;
    else if (k=="sigma_ovr_thresh")
        ss>> detector_config.sigma_ovr_thresh;
    else if (k=="output_save_hour")
        ss>> detector_config.output_save_hour;
    else if (k=="tbb_dump_wait_min")
        ss>> detector_config.tbb_dump_wait_min;
    else if (k=="osm_store_interval")
        ss>> detector_config.osm_store_interval;
    else if (k=="max_trigg_rate_to_reset_thresh")
        ss>> detector_config.max_trigg_rate_to_reset_thresh;
    else if (k=="wvfm_process_wpre_v2") //katie
      ss>> detector_config.wvfm_process_wpre_v2; //katie
    else if (k=="wvfm_process_wpost_v2") //katie
        ss>> detector_config.wvfm_process_wpost_v2; //katie
    else if (k=="wvfm_process_offtwlen_v2") //katie
        ss>> detector_config.wvfm_process_offtwlen_v2; //katie
    else
    {
      std::stringstream ss1;
      ss1 << "found unknown keyword: " << k << ", in file: "<< fname;
      throw std::runtime_error(ss1.str());
    }
  }//end while()

  //SANITY CHECK:check that active_stations have "lasa" in their names
  std::string retnval = verify_detector_config(detector_config);
  if (retnval!="") throw std::runtime_error(retnval);

  infile.close();
}

void OPERATIONS::Read_ControlParam_File(const std::string fname)
{
  /*
  Control Param File Structure:
  # COMMENT CHAR IS hash sign
  # NAME Digitizer  <line contd...>
  (0-3) CHANNEL_1_OFFSET+ CHANNEL_1_OFFSET- CHANNEL_2_OFFSET+ CHANNEL_2_OFFSET-
  (4-7)  CHANNEL_1_GAIN+ CHANNEL_1_GAIN- CHANNEL_2_GAIN+ CHANNEL_2_GAIN-
  (8-9) COMMON_OFFSET_ADJ FULL_SCALE_ADJ
  (10-11) CHANNEL_1_INTE_TIME CHANNEL_2_INTE_TIME
  (12-13) COMP_THRES_LOW COMP_THRES_HIGH
  (14-15) CHANNEL_1_HV CHANNEL_2_HV
  (16-19) CHANNEL_1_THRES_LOW CHANNEL_1_THRES_HIGH CHANNEL_2_THRES_LOW CHANNEL_2_THRES_HIGH
  (20-23) TRIGG_CONDITION PRE_COIN_TIME COIN_TIME POST_COIN_TIME
  (24) LOG_BOOK
  ....

  */
  std::string line;
  std::ifstream infile;

  infile.open(fname);

  if (infile.fail())
  {
    throw std::runtime_error(fname+" : file open failed.");
  }

  while (std::getline(infile, line))
  {
    if (line=="EOF") break;
    if (line.size()==0) continue; // empty line
    if (line.at(0)=='#') continue; // comment line
    unsigned short temp[40]; // for loading from stringstream
    std::string lasa_name, m_or_s;
    unsigned int det_no;
    std::stringstream ss(line);
    ss >> lasa_name >> m_or_s;
    ss >> temp[0] >> temp[1] >> temp[2] >> temp[3] ;
    ss >> temp[4] >> temp[5] >> temp[6] >> temp[7] ;
    ss >> temp[8] >> temp[9] >> temp[10] >> temp[11];
    ss >> temp[12] >> temp[13] >> temp[14] >> temp[15];
    ss >> temp[16] >> temp[17] >> temp[18] >> temp[19];
    ss >> temp[20] >> temp[21] >> temp[22] >> temp[23];
    ss >> temp[24];

    int found_in_network_config=0;
    for (int i=0;i<network_config.size();i++)
    {
      if (network_config[i].name==lasa_name)
      {
        found_in_network_config=1;
        if (m_or_s=="Master")
        {
          memcpy(network_config[i].init_control_params_m,temp,
            sizeof(network_config[i].init_control_params_m));
        }
        else if (m_or_s=="Slave")
        {
          memcpy(network_config[i].init_control_params_s,temp,
            sizeof(network_config[i].init_control_params_s));
        }
        else
        {
          std::stringstream ss1;
          ss1 << "Init Control Param File: " << fname;
          ss1 << "Has a Digitizer name other than Master/Slave.";
          ss1 << "Got ' " << m_or_s << "' for " << lasa_name;
          throw std::runtime_error(ss1.str());
        }
        break;
      }
    }

    if (found_in_network_config==0)
    {
      std::stringstream ss1;
      ss1 << lasa_name << "not found in network_config" ;
      ss1 << " which should have been already loaded using Read_Network_File()" ;
      throw std::runtime_error(ss1.str());
    }
  }

  infile.close();

  //SANITY CHECK:check that HV does not exceed 1800
  std::string retnval = verify_init_control_params(network_config);
  if (retnval!="") throw std::runtime_error(retnval);
}

void OPERATIONS::Read_ControlParam_File_V2_det(const std::string fname) //katie
{
    /*
     Control Param File Structure:
     # COMMENT CHAR IS hash sign
     # NAME Digitizer  <line contd...>
     lasa, det#, HV, pre_coin_time, post_coin_time,gain_correction,
     offset_correction, integration_time, base_max, base_min, sig_T1,
     sig_T2, Tprev, Tper, TCmax, NCmax, NCmin, Qmax,Qmin
     ....

     */
    std::string line;
    std::ifstream infile;

    infile.open(fname);

    if (infile.fail())
    {
        throw std::runtime_error(fname+" : file open failed.");
    }

    while (std::getline(infile, line))
    {
        if (line=="EOF") break;
        if (line.size()==0) continue; // empty line
        if (line.at(0)=='#') continue; // comment line

        unsigned short temp[40]; // for loading from stringstream
        std::string lasa_name,det;

        unsigned int det_no;
        std::stringstream ss(line);

        ss >> lasa_name >>det;
        //std::cout<<lasa_name<<"  "<<det<<"\n";

        ss >> temp[0] >> temp[1] >> temp[2] >> temp[3] ;
        ss >> temp[4] >> temp[5] >> temp[6] >> temp[7] ;
        ss >> temp[8] >> temp[9] >> temp[10] >> temp[11];
        ss >> temp[12] >> temp[13] >> temp[14] >> temp[15];
        ss >> temp[16];

        int found_in_network_config=0;

        for (int i=0;i<network_config.size();i++)
        {
        if (network_config[i].name==lasa_name)
        {
          found_in_network_config=1;
          if (det=="1")
          {
            memcpy(network_config[i].init_control_params_ch[0],temp,
                   sizeof(network_config[i].init_control_params_ch[0]));
          }
          if (det=="2")
          {
            memcpy(network_config[i].init_control_params_ch[1],temp,
                   sizeof(network_config[i].init_control_params_ch[1]));
          }
          if (det=="3")
          {
            memcpy(network_config[i].init_control_params_ch[2],temp,
                   sizeof(network_config[i].init_control_params_ch[2]));
          }
          if (det=="4")
          {
            memcpy(network_config[i].init_control_params_ch[3],temp,
                     sizeof(network_config[i].init_control_params_ch[3]));
          }
          break;
        }
        }
    }
    infile.close();
}

void OPERATIONS::Read_ControlParam_File_V2_stn(const std::string fname) //katie
{
  /*
  -- in file, for each line, the value in very first column is  :
  lasa,
  -- using 'temp' array numbers below ... column number in file would be +1:
  0 triger condition,
  1 full scale enable,
  2 pps enable,
  3 daq enable,
  4 ch1 en,
  5 ch2 en,
  6 ch3 en,
  7 ch4 en,
  8 cal en,
  9 10 sec en,
  10 ext en,
  11 ch1 readout en,
  12 ch2 readout en,
  13 ch3 readout en,
  14 ch4 readout en,
  15 triger rate divider,
  16 coincidence readout time
  */
    std::string line;
    std::ifstream infile;

    infile.open(fname);

    if (infile.fail())
    {
        throw std::runtime_error(fname+" : file open failed.");
    }

    while (std::getline(infile, line))
    {
        if (line=="EOF") break;
        if (line.size()==0) continue; // empty line
        if (line.at(0)=='#') continue; // comment line

        unsigned short temp[40]; // for loading from stringstream
        std::string lasa_name,det;

        unsigned int det_no;
        std::stringstream ss(line);

        ss >> lasa_name;
        //std::cout<<lasa_name<<"\n";

        ss >> temp[0] >> temp[1] >> temp[2] >> temp[3] ;
        ss >> temp[4] >> temp[5] >> temp[6] >> temp[7] ;
        ss >> temp[8] >> temp[9] >> temp[10] >> temp[11];
        ss >> temp[12] >> temp[13] >> temp[14] >> temp[15] >> temp[16];

        int found_in_network_config=0;

        for (int i=0;i<network_config.size();i++)
        {

            if (network_config[i].name==lasa_name)
            {
                found_in_network_config=1;
                memcpy(network_config[i].init_control_params_stn,temp,
                           sizeof(network_config[i].init_control_params_stn));

                break;

            }
        }

        if (found_in_network_config==0)
        {
          std::stringstream ss1;
          ss1 << lasa_name << "not found in network_config" ;
          ss1 << " which should have been already loaded using Read_Network_File()" ;
          throw std::runtime_error(ss1.str());
        }

    }
    infile.close();
}

void OPERATIONS::Init_LORA_Array()
{
  //create the array.
  if (network_config.size()==0)
  {
    throw std::runtime_error("struct loaded from network_config file empty.");
  }

  //find ip address of host a.k.a. server.
  std::string server_ip="";
  for (int i=0; i<network_config.size();++i)
  {
    if (network_config[i].type=="host")
    {
      server_ip = network_config[i].IPV4;
      break;
    }
  }
  //throw error if host ip/server ip not found.
  if (server_ip=="")
  {
    std::stringstream ss;
    ss << "No network member of type 'host' ";
    ss << "defined in network_config file";
    throw std::runtime_error(ss.str());
  }

  for (int i=0; i<network_config.size();i++)
  {
    //check if this station
    //was present in list of active_stations in detector_config
    auto vec = detector_config.active_stations;
    auto station_is_active = std::find(vec.begin(), vec.end(), network_config[i].name)!= vec.end();
    if (!station_is_active) continue;//if not , skip it.

    if (network_config[i].type=="clientv1")
    {
      //add a station of type V1.
      //lora_array_ptrs.push_back(std::make_unique<LORA_STATION_V1>());
      std::unique_ptr<LORA_STATION_V1> temp_ptr(new LORA_STATION_V1);
      lora_array_ptrs.push_back(std::move(temp_ptr));
      //initialize the added member.
      (lora_array_ptrs.back())->Init(network_config[i],server_ip);
    }
    else if (network_config[i].type=="clientv2")
    {
      //add a station of type V2.
      std::unique_ptr<LORA_STATION_V2> temp_ptr(new LORA_STATION_V2);
      lora_array_ptrs.push_back(std::move(temp_ptr));
      //initialize the added member.
      (lora_array_ptrs.back())->Init(network_config[i],server_ip);
    }
    //if STATION_INFO.type is neither of these two, do nothing.
  }

  //if no stations were found, throw error.
  if (lora_array_ptrs.size()==0)
  {
    std::stringstream ss;
    ss << "Check network_config (name, type) and detector_config(active_stations)";
    ss << "No active stations found";
    throw std::runtime_error(ss.str());
  }
}

void OPERATIONS::Init_Final_Containers()
{
  rootfile_event_platform = EVENT_DATA_STRUCTURE();
  rootfile_log_platform = LOG_STRUCTURE();
  rootfile_osm_platform = ONE_SEC_STRUCTURE();
  rootfile_event_header_platform= EVENT_HEADER_STRUCTURE();
}

void OPERATIONS::Open_ROOT_File()
{
  f= new TFile(output_root_filename.c_str(),"RECREATE") ; //over-write if file exists.

  int autosave_intrval = 100 * 1000;
  //100 kb, previous value of 10kb or 100b seemed too small
  //- will lead to very frequent writing operations.
  //suggested value by ROOT is several MB.

  //ROOT data types
  // C: a character string terminated by the 0 character
  // B: an 8 bit signed integer
  // b: an 8 bit unsigned integer
  // S: a 16 bit signed integer
  // s: a 16 bit unsigned integer
  // I: a 32 bit signed integer
  // i: a 32 bit unsigned integer
  // L: a 64 bit signed integer
  // l: a 64 bit unsigned integer
  // F: a 32 bit floating point
  // D: a 64 bit floating point
  // O: [the letter ‘o’, not a zero] a boolean (Bool_t)

  	//One sec tree
	tree_osm_hisparc= new TTree("Tree_OSM_HiSparc","Tree_OSM_HiSparc");
	tree_osm_hisparc->SetAutoSave(autosave_intrval) ;
  tree_osm_hisparc->Branch("Station", &rootfile_osm_platform.Lasa, "Station/i");
  tree_osm_hisparc->Branch("Master_Or_Slave",
                           &rootfile_osm_platform.Master_or_Slave,
                           "Master_Or_Slave/i");
  tree_osm_hisparc->Branch("GPS_Time_Stamp", &rootfile_osm_platform.GPS_time_stamp,
                           "GPS_Time_Stamp/i");
  tree_osm_hisparc->Branch("Sync_Error", &rootfile_osm_platform.sync,
                           "Sync_Error/i");
  tree_osm_hisparc->Branch("Quant_Error", &rootfile_osm_platform.quant,
                           "Quant_Error/F");
  tree_osm_hisparc->Branch("CTP", &rootfile_osm_platform.CTP, "CTP/i");
  tree_osm_hisparc->Branch("Channel_1_Thres_Count_High",
                  &rootfile_osm_platform.Channel_1_Thres_count_high,
                  "Channel_1_Thres_Count_High/s");
  tree_osm_hisparc->Branch("Channel_1_Thres_Count_Low",
                  &rootfile_osm_platform.Channel_1_Thres_count_low,
                  "Channel_1_Thres_Count_Low/s");
  tree_osm_hisparc->Branch("Channel_2_Thres_Count_High",
                  &rootfile_osm_platform.Channel_2_Thres_count_high,
                  "Channel_2_Thres_Count_High/s");
  tree_osm_hisparc->Branch("Channel_2_Thres_Count_Low",
                  &rootfile_osm_platform.Channel_2_Thres_count_low,
                  "Channel_2_Thres_Count_Low/s");
  tree_osm_hisparc->Branch("Satellite_Info",
                  rootfile_osm_platform.Satellite_info,
                  "Satellite_Info[61]/s");

	tree_osm_aera= new TTree("Tree_OSM_AERA","Tree_OSM_AERA");	//One sec tree
	tree_osm_aera->SetAutoSave(autosave_intrval) ;
  tree_osm_aera->Branch("Station", &rootfile_osm_platform.Lasa, "Station/i");
  tree_osm_aera->Branch("GPS_Time_Stamp",
                        &rootfile_osm_platform.GPS_time_stamp,
                        "GPS_Time_Stamp/i");
  tree_osm_aera->Branch("Sync_Error", &rootfile_osm_platform.sync,
                        "Sync_Error/i");
  tree_osm_aera->Branch("Quant_Error", &rootfile_osm_platform.quant,
                        "Quant_Error/F");
  tree_osm_aera->Branch("CTP", &rootfile_osm_platform.CTP, "CTP/i");
  tree_osm_aera->Branch("UTC_offset", &rootfile_osm_platform.UTC_offset,
                        "UTC_offset/i");
  tree_osm_aera->Branch("trigger_rate", &rootfile_osm_platform.trigger_rate,
                        "trigger_rate/s");

  tree_log=new TTree("Tree_Log","Tree_Log") ; // Log Tree + Noise Tree
  tree_log->SetAutoSave(autosave_intrval) ;
  tree_log->Branch("Station", &rootfile_log_platform.Station, "Station/i");
  tree_log->Branch("Detector", &rootfile_log_platform.detector, "Detector/i");
  tree_log->Branch("UTC_Time_Stamp", &rootfile_log_platform.UTC_time_stamp,
                   "UTC_Time_Stamp/i");
  tree_log->Branch("Channel_Thres_Low",
                   &rootfile_log_platform.Channel_thres_low,
                   "Channel_Thres_Low/s");
  tree_log->Branch("Mean_Baseline", &rootfile_log_platform.Mean,
                  "Mean_Baseline/F");
  tree_log->Branch("Mean_Sigma", &rootfile_log_platform.Sigma,
                   "Mean_Sigma/F");

  tree_event=new TTree("Tree_Event","Tree_Event");	//Event tree
  tree_event->SetAutoSave(autosave_intrval);
  tree_event->Branch("Station", &rootfile_event_platform.Station,"Station/i");
  tree_event->Branch("Detector", &rootfile_event_platform.detector,"Detector/i");
  tree_event->Branch("GPS_Time_Stamp", &rootfile_event_platform.GPS_time_stamp,
                     "GPS_Time_Stamp/i");
  tree_event->Branch("CTD", &rootfile_event_platform.CTD,
                     "CTD/i");
  tree_event->Branch("nsec_Online", &rootfile_event_platform.nsec,
                     "nsec_Online/i");
  tree_event->Branch("Event_Id", &rootfile_event_platform.Event_id,
                     "Event_Id/i");
  tree_event->Branch("Run_Id", &rootfile_event_platform.Run_id,
                     "Run_Id/l");
  tree_event->Branch("HiSparc_Trigg_Pattern",
                     &rootfile_event_platform.Trigg_pattern,
                     "HiSparc_Trigg_Pattern/i");
  tree_event->Branch("HiSparc_Trigg_Condition",
                     &rootfile_event_platform.Trigg_condition,
                     "HiSparc_Trigg_Condition/i");
  tree_event->Branch("Channel_Passed_Threshold",
                     &rootfile_event_platform.Has_trigg,
                     "Channel_Passed_Threshold/i");
  tree_event->Branch("Trigg_Threshold",
                     &rootfile_event_platform.Threshold_ADC,
                     "Trigg_Threshold/i");
  tree_event->Branch("LOFAR_trigg",
                    &rootfile_event_platform.LOFAR_trigg,
                    "LOFAR_trigg/i");
  tree_event->Branch("Charge_Corrected",
                     &rootfile_event_platform.Total_counts,
                     "Charge_Corrected/F");
  tree_event->Branch("Peak_Height_Corrected",
                     &rootfile_event_platform.Corrected_peak,
                     "Peak_Height_Corrected/F");
  tree_event->Branch("Peak_Height_Raw",
                     &rootfile_event_platform.Raw_peak,
                     "Peak_Height_Raw/i");
  tree_event->Branch("Waveform_Raw", rootfile_event_platform.counts,
  "Waveform_Raw[4000]/I");

  tree_event_header=new TTree("Tree_Event_Header","Tree_Event_Header");	//Event tree
  tree_event_header->SetAutoSave(autosave_intrval);
  tree_event_header->Branch("GPS_Time_Stamp_FirstHit",
                             &rootfile_event_header_platform.GPS_time_stamp_firsthit,
                             "GPS_Time_Stamp_FirstHit/i");
  tree_event_header->Branch("nsec_Online_FirstHit",
                            &rootfile_event_header_platform.nsec_firsthit,
                            "nsec_Online_FirstHit/i");
  tree_event_header->Branch("Time_Msg_Rcvd_ms_firsthit",
                            &rootfile_event_header_platform.Time_Msg_Rcvd_ms_firsthit,
                            "Time_Msg_Rcvd_ms_firsthit/l");
  tree_event_header->Branch("Event_Id",
                             &rootfile_event_header_platform.Event_id,
                             "Event_Id/i");
  tree_event_header->Branch("Run_Id",
                             &rootfile_event_header_platform.Run_id,
                             "Run_Id/l");
  tree_event_header->Branch("LOFAR_Trigg",
                             &rootfile_event_header_platform.LOFAR_trigg,
                             "LOFAR_Trigg/i");
  tree_event_header->Branch("Event_Tree_Index",
                             &rootfile_event_header_platform.Event_tree_index,
                             "Event_Tree_Index/i");
  tree_event_header->Branch("Event_Size",
                             &rootfile_event_header_platform.Event_size ,
                             "Event_Size/i");

  tree_det_config=new TTree("Tree_Detector_Config","Tree_Detector_Config");	//Event tree
  tree_det_config->SetAutoSave(autosave_intrval);
  unsigned int rootfile_sta_is_active_arr[10] = {0};
  unsigned int rootfile_sta_ver_arr[10] = {0};
  unsigned int j = 0;
  for (int i=0;i<network_config.size();++i)
  {
    if (network_config[i].type=="host"
        || network_config[i].type=="superhost") continue;

    auto vec = detector_config.active_stations;
    auto station_is_active = std::find(vec.begin(),
                                       vec.end(),
                                       network_config[i].name)!= vec.end();

    if (station_is_active) rootfile_sta_is_active_arr[j]=1;

    std::string br_name = network_config[i].name + "_is_active";
    std::string br_descrip = br_name +"/i";

    std::cout << br_name << " " << br_descrip << " " << rootfile_sta_is_active_arr[j] << std::endl;

    tree_det_config->Branch(br_name.c_str(),
                        &rootfile_sta_is_active_arr[j],
                        br_descrip.c_str()
                       );

    br_name = network_config[i].name + "_version";
    br_descrip = br_name +"/i";

    if (network_config[i].type=="clientv1") rootfile_sta_ver_arr[j]=1;
    if (network_config[i].type=="clientv2") rootfile_sta_ver_arr[j]=2;
    tree_det_config->Branch(br_name.c_str(),
                        &rootfile_sta_ver_arr[j],
                        br_descrip.c_str()
                      );
    ++j;
  }
  tree_det_config->Branch("lofar_trig_mode",&detector_config.lofar_trig_mode,
                      "lofar_trig_mode/I");
  tree_det_config->Branch("lofar_trig_cond",&detector_config.lofar_trig_cond,
                      "lofar_trig_cond/I");
  tree_det_config->Branch("lora_trig_cond",&detector_config.lora_trig_cond, "lora_trig_cond/I");
  tree_det_config->Branch("lora_trig_mode",&detector_config.lora_trig_mode, "lora_trig_mode/I");
  tree_det_config->Branch("calibration_mode",&detector_config.calibration_mode, "calibration_mode/I");
  tree_det_config->Branch("log_interval",&detector_config.log_interval, "log_interval/I");
  tree_det_config->Branch("check_coinc_interval",&detector_config.check_coinc_interval, "check_coinc_interval/I");
  tree_det_config->Branch("reset_thresh_interval",&detector_config.reset_thresh_interval, "reset_thresh_interval/I");
  tree_det_config->Branch("init_reset_thresh_interval",&detector_config.init_reset_thresh_interval, "init_reset_thresh_interval/I");
  tree_det_config->Branch("coin_window",&detector_config.coin_window, "coin_window/I");
  tree_det_config->Branch("wvfm_process_offtwlen",&detector_config.wvfm_process_offtwlen, "wvfm_process_offtwlen/I");
  tree_det_config->Branch("wvfm_process_wpost",&detector_config.wvfm_process_wpost, "wvfm_process_wpost/I");
  tree_det_config->Branch("wvfm_process_wpre",&detector_config.wvfm_process_wpre, "wvfm_process_wpre/I");
  tree_det_config->Branch("diagnostics_interval",&detector_config.diagnostics_interval, "diagnostics_interval/I");
  tree_det_config->Branch("sigma_ovr_thresh",&detector_config.sigma_ovr_thresh, "sigma_ovr_thresh/F");
  tree_det_config->Branch("output_save_hour",&detector_config.output_save_hour, "output_save_hour/I");
  tree_det_config->Branch("tbb_dump_wait_min",&detector_config.tbb_dump_wait_min, "tbb_dump_wait_min/I");
  tree_det_config->Branch("osm_store_interval",&detector_config.osm_store_interval, "osm_store_interval/I");
  tree_det_config->Fill();

  tree_cp=new TTree("Tree_Control_Parameters_HiSparc","Tree_Control_Parameters_HiSparc");	//Event tree
  tree_cp->SetAutoSave(autosave_intrval);
  unsigned short temp_cp[40]={0};
  unsigned short temp_m_or_s;
  unsigned short temp_station_no;
  tree_cp->Branch("Station",&temp_station_no, "Station/s");
  tree_cp->Branch("Master_Or_Slave",&temp_m_or_s, "Master_Or_Slave/s");
  tree_cp->Branch("CHANNEL_1_OFFSET+",&temp_cp[0], "CHANNEL_1_OFFSET+/s");
  tree_cp->Branch("CHANNEL_1_OFFSET-",&temp_cp[1], "CHANNEL_1_OFFSET-/s");
  tree_cp->Branch("CHANNEL_2_OFFSET+",&temp_cp[2], "CHANNEL_2_OFFSET+/s");
  tree_cp->Branch("CHANNEL_2_OFFSET-",&temp_cp[3], "CHANNEL_2_OFFSET-/s");
  tree_cp->Branch("CHANNEL_1_GAIN+",&temp_cp[4], "CHANNEL_1_GAIN+/s");
  tree_cp->Branch("CHANNEL_1_GAIN-",&temp_cp[5], "CHANNEL_1_GAIN-/s");
  tree_cp->Branch("CHANNEL_2_GAIN+",&temp_cp[6], "CHANNEL_2_GAIN+/s");
  tree_cp->Branch("CHANNEL_2_GAIN-",&temp_cp[7], "CHANNEL_2_GAIN-/s");
  tree_cp->Branch("COMMON_OFFSET_ADJ",&temp_cp[8], "COMMON_OFFSET_ADJ/s");
  tree_cp->Branch("FULL_SCALE_ADJ",&temp_cp[9], "FULL_SCALE_ADJ/s");
  tree_cp->Branch("CHANNEL_1_INTE_TIME",&temp_cp[10], "CHANNEL_1_INTE_TIME/s");
  tree_cp->Branch("CHANNEL_2_INTE_TIME",&temp_cp[11], "CHANNEL_2_INTE_TIME/s");
  tree_cp->Branch("COMP_THRES_LOW",&temp_cp[12], "COMP_THRES_LOW/s");
  tree_cp->Branch("COMP_THRES_HIGH",&temp_cp[13], "COMP_THRES_HIGH/s");
  tree_cp->Branch("CHANNEL_1_HV",&temp_cp[14], "CHANNEL_1_HV/s");
  tree_cp->Branch("CHANNEL_2_HV",&temp_cp[15], "CHANNEL_2_HV/s");
  tree_cp->Branch("CHANNEL_1_THRES_LOW",&temp_cp[16], "CHANNEL_1_THRES_LOW/s");
  tree_cp->Branch("CHANNEL_1_THRES_HIGH",&temp_cp[17], "CHANNEL_1_THRES_HIGH/s");
  tree_cp->Branch("CHANNEL_2_THRES_LOW",&temp_cp[18], "CHANNEL_2_THRES_LOW/s");
  tree_cp->Branch("CHANNEL_2_THRES_HIGH",&temp_cp[19], "CHANNEL_2_THRES_HIGH/s");
  tree_cp->Branch("TRIGG_CONDITION",&temp_cp[20], "TRIGG_CONDITION/s");
  tree_cp->Branch("PRE_COIN_TIME",&temp_cp[21], "PRE_COIN_TIME/s");
  tree_cp->Branch("COIN_TIME",&temp_cp[22], "COIN_TIME/s");
  tree_cp->Branch("POST_COIN_TIME",&temp_cp[23], "POST_COIN_TIME/s");
  tree_cp->Branch("LOG_BOOK",&temp_cp[24], "LOG_BOOK/s");
  for (int i=0;i<network_config.size();++i)
  {
    if (network_config[i].type!="clientv1") continue;
    std::stringstream ss22(network_config[i].name.substr(4,2));
    //2 because lasa10 will have 2 digits
    ss22 >> temp_station_no;

    memcpy(temp_cp,network_config[i].init_control_params_m,
      sizeof(network_config[i].init_control_params_m));
    temp_m_or_s=0;
    tree_cp->Fill();

    memcpy(temp_cp,network_config[i].init_control_params_s,
      sizeof(network_config[i].init_control_params_s));
    temp_m_or_s=1;
    tree_cp->Fill();
  }

  //Tree storing init_control_params_v2_stn
  tree_cp_v2_stn= new TTree("Tree_Control_Parameters_AERA_Station","Tree_Control_Parameters_AERA_Station");
  tree_cp_v2_stn->SetAutoSave(autosave_intrval);
  // Using the 'temp_station_no' and 'temp_cp' containers from above.
  tree_cp_v2_stn->Branch("Station",&temp_station_no, "Station/s");
  tree_cp_v2_stn->Branch("Trigger_Condition",&temp_cp[0], "Trigger_Condition/s");
  tree_cp_v2_stn->Branch("Full_Scale_Enable",&temp_cp[1], "Full_Scale_Enable/s");
  tree_cp_v2_stn->Branch("PPS_Enable",&temp_cp[2], "PPS_Enable/s");
  tree_cp_v2_stn->Branch("DAQ_Enable",&temp_cp[3], "DAQ_Enable/s");
  tree_cp_v2_stn->Branch("Ch1_Enable",&temp_cp[4], "Ch1_Enable/s");
  tree_cp_v2_stn->Branch("Ch2_Enable",&temp_cp[5], "Ch2_Enable/s");
  tree_cp_v2_stn->Branch("Ch3_Enable",&temp_cp[6], "Ch3_Enable/s");
  tree_cp_v2_stn->Branch("Ch4_Enable",&temp_cp[7], "Ch4_Enable/s");
  tree_cp_v2_stn->Branch("Calib_Enable",&temp_cp[8], "Calib_Enable/s");
  tree_cp_v2_stn->Branch("TenSec_ForcedTrig_Enable",&temp_cp[9], "TenSec_ForcedTrig_Enable/s");
  tree_cp_v2_stn->Branch("ExternalTrig_Enable",&temp_cp[10], "ExternalTrig_Enable/s");
  tree_cp_v2_stn->Branch("Ch1_Readout_Enable",&temp_cp[11], "Ch1_Readout_Enable/s");
  tree_cp_v2_stn->Branch("Ch2_Readout_Enable",&temp_cp[12], "Ch2_Readout_Enable/s");
  tree_cp_v2_stn->Branch("Ch3_Readout_Enable",&temp_cp[13], "Ch3_Readout_Enable/s");
  tree_cp_v2_stn->Branch("Ch4_Readout_Enable",&temp_cp[14], "Ch4_Readout_Enable/s");
  tree_cp_v2_stn->Branch("Trig_Rate_Divider",&temp_cp[15], "Trig_Rate_Divider/s");
  tree_cp_v2_stn->Branch("Coinc_Readout_Time",&temp_cp[16], "Coinc_Readout_Time/s");
  for (int i=0;i<network_config.size();++i)
  {
    if (network_config[i].type!="clientv2") continue;
    std::stringstream ss22(network_config[i].name.substr(4,2));
    //2 because lasa10 will have 2 digits
    ss22 >> temp_station_no;

    memcpy(temp_cp,network_config[i].init_control_params_stn,
      sizeof(network_config[i].init_control_params_stn));
    tree_cp_v2_stn->Fill();
  }

  //Tree storing init_control_params_v2_det
  tree_cp_v2_det= new TTree("Tree_Control_Parameters_AERA_Detector","Tree_Control_Parameters_AERA_Detector");
  tree_cp_v2_det->SetAutoSave(autosave_intrval);
  unsigned short temp_det_no;
  // Using the 'temp_station_no' and 'temp_cp' containers from above.
  tree_cp_v2_det->Branch("Station",&temp_station_no, "Station/s");
  tree_cp_v2_det->Branch("Detector",&temp_det_no, "Detector/s");
  tree_cp_v2_det->Branch("HV",&temp_cp[0], "HV/s");
  tree_cp_v2_det->Branch("Pre_Coin_Time",&temp_cp[1], "Pre_Coin_Time/s");
  tree_cp_v2_det->Branch("Post_Coin_Time",&temp_cp[2], "Post_Coin_Time/s");
  tree_cp_v2_det->Branch("Gain_Correction",&temp_cp[3], "Gain_Correction/s");
  tree_cp_v2_det->Branch("Offset_Correction",&temp_cp[4], "Offset_Correction/s");
  tree_cp_v2_det->Branch("Integration_Time",&temp_cp[5], "Integration_Time/s");
  tree_cp_v2_det->Branch("Base_Max",&temp_cp[6], "Base_Max/s");
  tree_cp_v2_det->Branch("Base_Min",&temp_cp[7], "Base_Min/s");
  tree_cp_v2_det->Branch("Sig_T1",&temp_cp[8], "Sig_T1/s");
  tree_cp_v2_det->Branch("Sig_T2",&temp_cp[9], "Sig_T2/s");
  tree_cp_v2_det->Branch("Tprev",&temp_cp[10], "Tprev/s");
  tree_cp_v2_det->Branch("Tper",&temp_cp[11], "Tper/s");
  tree_cp_v2_det->Branch("TC_Max",&temp_cp[12], "TC_Max/s");
  tree_cp_v2_det->Branch("NC_Max",&temp_cp[13], "NC_Max/s");
  tree_cp_v2_det->Branch("NC_Min",&temp_cp[14], "NC_Min/s");
  tree_cp_v2_det->Branch("Q_Max",&temp_cp[15], "Q_Max/s");
  tree_cp_v2_det->Branch("Q_Min",&temp_cp[16], "Q_Min/s");

  for (int i=0;i<network_config.size();++i)
  {
    if (network_config[i].type!="clientv2") continue;
    std::stringstream ss22(network_config[i].name.substr(4,2));
    //2 because lasa10 will have 2 digits
    ss22 >> temp_station_no;
    // std::cout << "inside tree cp v2 det filling loop " << std::endl;
    // std::cout << temp_station_no << std::endl;

    for (int j=0;j<4;++j)
    {
      auto temp33 = Get_Detector_Number(network_config[i].no, j);
      temp_det_no = (unsigned short) temp33;
      std::cout << temp_det_no << std::endl;
      memcpy(temp_cp,network_config[i].init_control_params_ch[j],
        sizeof(network_config[i].init_control_params_ch[j]));
      tree_cp_v2_det->Fill();
    }

  }




}

void OPERATIONS::Close_ROOT_File()
{
  //see this when you want to auto make new root files based on size
  //https://root.cern.ch/doc/master/classTTree.html#a4680b0dfd17292acc29ba9a8f33788a3
  //https://root.cern.ch/doc/master/classTTree.html#adbb8b5203ce3fca8a4223ab9f19f68fe
  //https://root.cern.ch/doc/master/classTTree.html#a3ab4006402ddbc3950e1ddab5e3aadd6
  f->Write();
  //closing the file also deletes the TTree object
  //https://root-forum.cern.ch/t/tfile-close-and-ttree/9443
  // so no need to call on delete keyword to accompany the new declaration used in Open_ROOT_File()
  f->Close() ;
}

void OPERATIONS::Set_Output_File_Names_And_RunId()
{
  std::tm *t = Get_Current_Time();
  unsigned int YMD=(t->tm_year+1900)*10000+(t->tm_mon+1)*100+t->tm_mday ;
  unsigned int HM=t->tm_hour*100+t->tm_min;

  std::stringstream ss;
  ss << detector_config.output_path ;
  if(t->tm_hour<1) ss << YMD << "_00" << HM ;
  else if(t->tm_hour<10) ss << YMD << "_0" << HM ;
  else ss<< YMD << "_" << HM ;


  output_root_filename = ss.str() + ".root" ;
  output_detectors_log_filename = ss.str() + "_detectors.log" ;
  output_array_log_filename = ss.str() + "_array.log" ;
  output_elec_calib_filename= ss.str() + "_calib_control_params.txt";

  evt_tree_index=0;
  run_id= ((unsigned long int)(YMD-(2000*10000))*10000) + ((unsigned long int)HM);
  std::cout << "RunId: " << run_id << std::endl;
  std::cout << "output file prefix: " << ss.str() << std::endl;
}

void OPERATIONS::Save_Output_Files_At_Designated_Hour()
{
  int dt_ms = Get_ms_Elapsed_Since(most_recent_hourly_check_time);
  if (dt_ms <= 60*60*1000) return; //1 hour
  most_recent_hourly_check_time= Get_Current_Time_chrono();

  std::tm *t = Get_Current_Time();
  if (t->tm_hour!=detector_config.output_save_hour) return;

  Close_ROOT_File();
  Set_Output_File_Names_And_RunId();
  Open_ROOT_File();
}

void OPERATIONS::Store_Log()
{
  std::cout << "Storing log.... ..." << std::endl;
  for (int i=0;i<lora_array_ptrs.size();++i)
  {
    //initialize empty station container
    tLog_Station temp_station_noise;

    //get station to send its noise info.
    lora_array_ptrs[i]->Send_Log(temp_station_noise);

    //send the info to the root file.
    for (int jj=0; jj<temp_station_noise.size(); ++jj)
    {
      rootfile_log_platform=temp_station_noise[jj];
      tree_log->Fill();
    }
  }
}

void OPERATIONS::Status_Monitoring()
{
  if (detector_config.calibration_mode==1) return;
  int dt_ms = Get_ms_Elapsed_Since(most_recent_status_monitoring_time);
  if (dt_ms <= 1000) return;
  most_recent_status_monitoring_time= Get_Current_Time_chrono();

  bool fatal_error=false;
  std::string error_msg="";
  for (int i=0; i<lora_array_ptrs.size(); ++i)
  {
    lora_array_ptrs[i]->Status_Monitoring(fatal_error,error_msg);
  }

  if (fatal_error)
  {
    std::cout << "Fatal Error.... Ending DAQ" << std::endl;
    std::cout << error_msg << std::endl;
    sleep(1);
    self_execution_status=false;
  }
}

void OPERATIONS::Return_Trigger_Decision(int& lora_trig_satisfied,
                             int& lofar_trig_satisfied,
                             const int& n_detectors,
                             const int& n_stations)
{
  lora_trig_satisfied=0;
  lofar_trig_satisfied=0;

  // n_stations based.
  if (detector_config.lora_trig_mode==1
      && n_stations>= detector_config.lora_trig_cond )
      lora_trig_satisfied=1;
  // n_detectors based
  if (detector_config.lora_trig_mode==2
      && n_detectors>= detector_config.lora_trig_cond )
      lora_trig_satisfied=1;

  // n_stations based.
  if (detector_config.lofar_trig_mode==1
      && n_stations>= detector_config.lofar_trig_cond
      && lora_trig_satisfied==1)
      lofar_trig_satisfied=1;
  // n_detectors based
  if (detector_config.lofar_trig_mode==2
      && n_detectors>= detector_config.lofar_trig_cond
      && lora_trig_satisfied==1)
      lofar_trig_satisfied=1;
}
