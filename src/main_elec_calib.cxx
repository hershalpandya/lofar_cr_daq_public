#include "OPERATIONS.h"
#include <iostream>
#include "functions_library.h"

int main()
{
  OPERATIONS lora_daq;

  std::cout
  << "ELECTRONICS CALIB STARTING..." << std::endl;

  lora_daq.Init("../input/network_config.txt",
           "../input/detector_config_elec_calib.txt",
           "../input/init_control_params.txt",
           "../input/init_control_params_V2_det.txt", //dummy input
           "../input/init_control_params_V2_stn.txt", //dummy input 
           "../daq_managers/daq_execution_status_file.txt",
           "../input/detector_coord.txt");

  lora_daq.Connect_To_Stations();

  sleep(1);

  lora_daq.Accept_Connections_From_Stations();

  lora_daq.Send_Control_Params();

  bool stay_in_loop=true;
  while(stay_in_loop)
  {
    lora_daq.Listen_To_Stations();

    lora_daq.Interpret_And_Store_Incoming_Msgs();

    if (!lora_daq.Is_DAQ_Execution_Status_True())
    {
      stay_in_loop=false;
      std::cout << "Ending DAQ...\n";
      sleep(1);//required to wait for any running processes.
    }
  }

  lora_daq.End();

  return 0;
}
