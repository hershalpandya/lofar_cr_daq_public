#pragma once

#include <vector>
#include <memory> //unique_ptr import
#include <string>
#include "Structs.h"
#include "LORA_STATION.h"
#include <TFile.h>
#include <TTree.h>
#include <csignal>
#include <ctime> //struct tm
#include <chrono>
#include "LOFAR_Communications.h"

class LORA_STATION;

class OPERATIONS
{
// all members of this class will be public.
// stores all the basic information + functions
// calls other classes when needed.
public:
  // pass names of network , config, init control param files.
  void Init(const std::string, const std::string,
            const std::string, const std::string,
            const std::string, const std::string,//katie (added 2 more V2 files)
            const std::string);

  //calls open method for LORA_STATION's
  void Connect_To_Stations();

  //calls Send_Control_Params() method for LORA_STATIONs
  void Send_Control_Params();

  //initialize all the most_recent_*time variables to current time.
  void Initialize_All_TimeKeepers();

  //calls listen method for LORA_STATION's
  void Listen_To_Stations();

  //calls accept method for LORA_STATION's
  void Accept_Connections_From_Stations();

  //calls interpret method for LORA_STATION's
  void Interpret_And_Store_Incoming_Msgs();

  // Asks LORA_STATIONs to calculate noise and store it.
  void Process_Event_Spool_Before_Coinc_Check(int&);

  //Based on noise, reset threshold for Digitizers
  //Calls reset method of LORA_STATIONs
  void Reset_Thresh_Store_Log();

  //Pulls timestamp and unique identifier for each EventMsg
  //from every station and organizes them into an event.
  //if lora trigger condition is satisfied, this event info is stored.
  //if lofar and lora trigger condition is satisfied, this event info is stored.
  //if lofar and lora trigger condition is satisfied, a trigger is sent to LOFAR
  void Check_Coinc_Store_Event_Send_LOFAR_Trigger(const bool&);

  //Pulls event data structures based on the event info stored by
  //Check_Coinc_Store_Event_Send_LOFAR_Trigger()
  void Store_Event(const tvec_EVENT_SPOOL_SUMMARY&,
                   const std::vector<std::string>&,
                   const int&);


  //Pulls OSM from station spools and stores them to ROOT file.
  //FIXIT: is this done only on sec-spy interval or for every OSM received?
  void Store_OSM();

  void Periodic_Store_Log();

  void Create_DAQ_Execution_Status_File();

  bool Is_DAQ_Execution_Status_True();

  //calls Close() method for LORA_STATION's and writes out remaining file And
  // so on... all lose ends tied up.
  void End();

  void Run_Detectors_Diagnostics();

  void Print_Array_Diagnostics(const tvec_EVENT_SPOOL_SUMMARY&,
                          const int&, const int&, const int&, const int&,
                          const unsigned int&, const unsigned int&, const int&);

  //Everyday at a fixed time, save current output files
  //And open a new one for new day.
  void Save_Output_Files_At_Designated_Hour();

  void Status_Monitoring();

  int Get_Sum_Size_of_Spools();

  void Fiddle_With_Det_Config(const float&, const std::vector<int>&);

  bool Get_Execution_Status();

private:

  //this file should have code run status:  0 for OFF and 1 for ON.
  //# (hash) for comment lines.
  std::string daq_on_off_filename;
  std::string output_root_filename;
  std::string output_detectors_log_filename;
  std::string output_array_log_filename;
  std::string output_elec_calib_filename;
  unsigned long int run_id;
  unsigned int evt_tree_index;
  bool self_execution_status;

  //vector of pointers to stations
  //use lora_array_ptrs.emplace_back(std::make_unique<LORA_STATION_V1>)
  //to add a lora station old/v1 to it.
  std::vector<std::unique_ptr<LORA_STATION>> lora_array_ptrs;
  // holds network info read by init. includes init control params.
  std::vector<STATION_INFO> network_config;
  //holds detector configuration
  DETECTOR_CONFIG detector_config;
  std::vector<float> det_coord_x,det_coord_y;

  // These platforms host the event/log/osm/header data
  // And then immediately Tree->Fill(); should be called to push
  // the data onto Tree structure. Repeat when new data is available.
  EVENT_DATA_STRUCTURE rootfile_event_platform;
  LOG_STRUCTURE rootfile_log_platform;
  ONE_SEC_STRUCTURE rootfile_osm_platform;
  EVENT_HEADER_STRUCTURE rootfile_event_header_platform;

  //event coincidence flags
  bool lofar_trigger=false;
  bool lora_trigger=false;

  //pointers to ROOT objects
  //the obj they point to will be created when
  //Create_ROOT_File() is called.
  TFile *f=nullptr;
  TTree *tree_event=nullptr;
  TTree *tree_log=nullptr;
  TTree *tree_osm_hisparc=nullptr;
  TTree *tree_osm_aera=nullptr;
  TTree *tree_event_header=nullptr;
  TTree *tree_cp=nullptr;
  TTree *tree_det_config=nullptr;
  TTree *tree_cp_v2_stn=nullptr;
  TTree *tree_cp_v2_det=nullptr;

  int n_channels = 4; // 1,2 for master, 1,2, for slave in Old.

  tm run_start_time;
  int detectors_calibrated=0;

  int current_reset_thresh_interval;
  int max_iteration_wait_interval;
  int current_check_coinc_interval;
  tchrono most_recent_event_check_time;
  tchrono most_recent_iteration_wait_time;
  tchrono most_recent_reset_thresh_time;
  tchrono most_recent_osm_store_time;
  tchrono most_recent_cp_store_time;
  tchrono most_recent_daq_switch_check_time;
  tchrono most_recent_print_diagnostics_time;
  tchrono most_recent_hourly_check_time;
  tchrono most_recent_status_monitoring_time;
  unsigned int most_recent_event_id;
  unsigned int most_recent_trigg_GPS=0;

  std::unique_ptr<LOFAR_COMMS> lofar_radio_comm;

  void Read_Detector_File(const std::string);
  void Read_Det_Coord_File(const std::string);

  void Read_Network_File(const std::string);
  void Read_ControlParam_File(const std::string);
  void Read_ControlParam_File_V2_det(const std::string); //katie
  void Read_ControlParam_File_V2_stn(const std::string); //katie

  void Init_LORA_Array();

  void Init_Final_Containers();
  //name it, open it, intiate branch structure.
  void Open_ROOT_File();
  //close it, move it from temp to final location.
  void Close_ROOT_File();

  void Generate_Calib_File();

  void Set_Output_File_Names_And_RunId();

  void Store_Log();

  void Return_Trigger_Decision(int& , int& , const int&, const int&);

};
