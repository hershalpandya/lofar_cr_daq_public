#include "OPERATIONS.h"
#include <iostream>
#include "functions_library.h"
#include <sstream>

//This is basically the same script as main.cxx
//But with modules not needed for detector calib removed.
//And looping to start and stop daq for 20 panels.

int main()
{
  std::vector<std::pair<float,float>> sigma_range;
  sigma_range.push_back(std::make_pair(2.2, 3.4));//Det 1
  sigma_range.push_back(std::make_pair(2.2, 3.3));//Det 2
  sigma_range.push_back(std::make_pair(2.8, 4.1));//Det 3
  sigma_range.push_back(std::make_pair(2.8, 4.1));//Det 4

  sigma_range.push_back(std::make_pair(3.4, 4.6));//Det 5
  sigma_range.push_back(std::make_pair(3.6, 5.0));//Det 6
  sigma_range.push_back(std::make_pair(3.2, 4.6));//Det 7
  sigma_range.push_back(std::make_pair(3.9, 5.2));//Det 8

  sigma_range.push_back(std::make_pair(3.0, 4.0));//Det 9
  sigma_range.push_back(std::make_pair(3.2, 4.1));//Det 10
  // sigma_range.push_back(std::make_pair(2.8, 4.0));//Det 11
  sigma_range.push_back(std::make_pair(3.6, 4.0));//Det 11
  sigma_range.push_back(std::make_pair(2.8, 4.1));//Det 12

  sigma_range.push_back(std::make_pair(2.0, 3.2));//Det 13
  sigma_range.push_back(std::make_pair(2.3, 3.6));//Det 14
  sigma_range.push_back(std::make_pair(4.2, 6.0));//Det 15
  sigma_range.push_back(std::make_pair(5.4, 7.2));//Det 16

  //sigma_range.push_back(std::make_pair(2.4, 7.0));//Det 17
  sigma_range.push_back(std::make_pair(4.1, 7.0));//Det 17
  sigma_range.push_back(std::make_pair(3.4, 4.7));//Det 18
  sigma_range.push_back(std::make_pair(2.1, 3.2));//Det 19
  sigma_range.push_back(std::make_pair(2.9, 4.1));//Det 20



  int run_time = 7;//in minutes
  //this whole run will take: 20 (sig) * 5 (sta) * 4 (ch) * 7 mins = 46.67 hrs
  for (int calib_ch=3; calib_ch<=4; ++calib_ch)
  {
  for (int sta=1; sta<=5; ++sta)
  {
  //
  if (calib_ch==3 && sta<3) continue;
  int sigma_index=Get_Detector_Number(sta,calib_ch-1)-1;
  float sigma_l = sigma_range[sigma_index].first;
  float sigma_h = sigma_range[sigma_index].second;
  for (float sigma_ovr_thresh=sigma_l; sigma_ovr_thresh<=sigma_h; sigma_ovr_thresh+=0.1)
  {

  std::cout << "------------------------x x x x x-----------------" << std::endl;
  std::cout << "Calibrating ch: " << calib_ch << " sta: " << sta << " sigma: "
  << sigma_ovr_thresh << std::endl;
  std::stringstream this_detconfig_file, this_inticp_file;
  this_detconfig_file << "../input/detector_config_calib_lasa" << sta << ".txt";
  this_inticp_file << "../input/init_control_params_muon_calib_ch"
  << calib_ch << ".txt";
  std::vector<int> list_of_dets_for_calib;
  for (int i=0; i<4; ++i)
  {
    if (i==calib_ch-1) continue; //don't add the ch to be calibrated.
    list_of_dets_for_calib.push_back(Get_Detector_Number(sta,i));
  }

  OPERATIONS lora_daq;

  tchrono start_time = Get_Current_Time_chrono();

  lora_daq.Init("../input/network_config.txt",
           this_detconfig_file.str(),
           this_inticp_file.str(),
           "../daq_managers/daq_execution_status_file.txt",
           "../input/detector_coord.txt");

  lora_daq.Fiddle_With_Det_Config(sigma_ovr_thresh,list_of_dets_for_calib);

  sleep(1);

  lora_daq.Connect_To_Stations();

  sleep(1);

  lora_daq.Accept_Connections_From_Stations();

  lora_daq.Send_Control_Params();

  bool stay_in_loop=true;
  while(stay_in_loop)
  {
    lora_daq.Listen_To_Stations();

    lora_daq.Interpret_And_Store_Incoming_Msgs();

    lora_daq.Check_Coinc_Store_Event_Send_LOFAR_Trigger(!stay_in_loop);

    lora_daq.Reset_Thresh_Store_Log();

    lora_daq.Store_OSM();

    lora_daq.Run_Detectors_Diagnostics();

    lora_daq.Status_Monitoring();

    int dt_ms = Get_ms_Elapsed_Since(start_time);
    if (!lora_daq.Is_DAQ_Execution_Status_True() || dt_ms>run_time*60*1000)
    {
      stay_in_loop=false;
      std::cout << "Ending DAQ...\n";
      sleep(1);//required to wait for any running processes.
    }
  }

  int n_msgs_unsaved = lora_daq.Get_Sum_Size_of_Spools();
  std::cout << std::endl << "Shutting the code... "
  << "emptying all recvd msgs from spools... " << std::endl;
  int count=0;
  while (n_msgs_unsaved>0 && count<2000)
  {
    if (count==400 || count==800 || count==1200) std::cout << "." ;
    lora_daq.Interpret_And_Store_Incoming_Msgs();
    lora_daq.Check_Coinc_Store_Event_Send_LOFAR_Trigger(!stay_in_loop);
    lora_daq.Store_OSM();
    lora_daq.Periodic_Store_Log();
    lora_daq.Run_Detectors_Diagnostics();
    int new_value = lora_daq.Get_Sum_Size_of_Spools();
    if (new_value==n_msgs_unsaved)
      count++;
    else count=0;
    n_msgs_unsaved = new_value;
  }
  std::cout << std::endl<< "All spools are empty... have a good day!" << std::endl;

  lora_daq.End();
  }
  }
  }
  return 0;
}
