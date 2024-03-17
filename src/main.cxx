#include "OPERATIONS.h"
#include <iostream>
#include "functions_library.h"

int main()
{
  OPERATIONS lora_daq;

  lora_daq.Init("../input/network_config.txt",
           "../input/detector_config.txt",
           "../input/init_control_params.txt",
           "../input/init_control_params_V2_det.txt", //katie
           "../input/init_control_params_V2_stn.txt", //katie
           "../daq_managers/daq_execution_status_file.txt",
           "../input/new_detector_coord.txt");

  // std::cout << __LINE__ << " " << __FILE__ << "\n";
  lora_daq.Connect_To_Stations();

  std::cout << "************** WARNINGS********************"<< std::endl;
  std::cout << "FIXIT in OPERATIONS.cxx if (network_config[i].type!=...clientv1 ) continue; "<< std::endl;
  std::cout << "uncomment Run_Detectors_Diagnostics() exceeding_trigger_rate=true;" << std::endl;
  std::cout << "**********************************"<< std::endl;

  sleep(1);

  lora_daq.Accept_Connections_From_Stations();

  lora_daq.Send_Control_Params();

  bool stay_in_loop=lora_daq.Get_Execution_Status();
  while(stay_in_loop)
  {
    lora_daq.Listen_To_Stations();

    lora_daq.Interpret_And_Store_Incoming_Msgs();

    lora_daq.Check_Coinc_Store_Event_Send_LOFAR_Trigger(!stay_in_loop);

    lora_daq.Reset_Thresh_Store_Log();

    lora_daq.Store_OSM();

    lora_daq.Periodic_Store_Log();

    lora_daq.Run_Detectors_Diagnostics();

    lora_daq.Save_Output_Files_At_Designated_Hour();

    lora_daq.Status_Monitoring();

    if (!lora_daq.Is_DAQ_Execution_Status_True())
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
  while (n_msgs_unsaved>0 && count<100)
  {
    std::cout <<".";
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

  return 0;
}
