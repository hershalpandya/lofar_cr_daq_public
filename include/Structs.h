#pragma once

#include<string>
#include <vector>
#include <utility>
#include <chrono>

/*
Definition for some basic data holding structs.
*/
struct ONE_SEC_STRUCTURE
{
    //copied as is... from the old code.
    //One sec message structure to be stored in ROOT file
	unsigned int Lasa = (unsigned int) 0;
	unsigned int Master_or_Slave = (unsigned int) 0;
	//GPS Time stamp in secs since the Epoch (00:00:00 UTC, Jan 1 1970)
	unsigned int GPS_time_stamp = (unsigned int) 0;
	unsigned int sync = (unsigned int) 0;
	unsigned int CTP = (unsigned int) 0;
	float quant = (float) 0;
	unsigned short Channel_1_Thres_count_high = (unsigned short) 0;
	unsigned short Channel_1_Thres_count_low  = (unsigned short) 0;
	unsigned short Channel_2_Thres_count_high  = (unsigned short) 0;
	unsigned short Channel_2_Thres_count_low  = (unsigned short) 0;
	unsigned char Satellite_info[61] = {0};

	unsigned int UTC_offset = (unsigned int) 0; //katie, V2, offset GPS -> UTC in leap seconds
  unsigned short trigger_rate = (unsigned short) 0; //katie, V2, trigger rate from all channels / last second
};

struct EVENT_DATA_STRUCTURE	//data structure to be stored in ROOT file - per channel - 4 per station
{
	unsigned int Station = (unsigned int)0;		//station number
	unsigned int detector = (unsigned int)0;		//Detector number
	unsigned int GPS_time_stamp = (unsigned int)0;	//Event time stamp in secs since the Epoch (00:00:00 UTC, Jan 1 1970)
	unsigned long int Time_Msg_Rcvd_ms = (unsigned long int)0;	//Event time stamp in secs since the Epoch (00:00:00 UTC, Jan 1 1970)
	unsigned int CTD = (unsigned int)0;
	unsigned int nsec = (unsigned int)0;		//An apprx. value
	unsigned int Event_id=  (unsigned int)0;
	unsigned long int Run_id=  (long int)0;
	unsigned int Trigg_condition = (unsigned int)0;	//Trigger condition
	unsigned int Trigg_pattern = (unsigned int)0;	//Trigger pattern
	unsigned int Has_trigg = (unsigned int)0;	//raw peak > threshold
	unsigned int Threshold_ADC= (unsigned int)0;
	unsigned int LOFAR_trigg = (unsigned int)0;	//
	float Total_counts  = -99.0;	//Total ADC counts of the signal
	float Corrected_peak = -99.0;	//Pulse height of the signal (in ADC counts)
	unsigned int Raw_peak = (unsigned int)0;	//(in ADC counts)
	int counts[4000] = {0} ;	//Traces (ADC counts)
};
typedef std::vector<EVENT_DATA_STRUCTURE> tEvent_Data_Station;

struct EVENT_HEADER_STRUCTURE	//data structure to be stored in ROOT file - per channel - 4 per station
{
	unsigned int GPS_time_stamp_firsthit = (unsigned int)0;	//Event time stamp in secs since the Epoch (00:00:00 UTC, Jan 1 1970)
	unsigned long int Time_Msg_Rcvd_ms_firsthit = (unsigned long int)0;	//Event time stamp in secs since the Epoch (00:00:00 UTC, Jan 1 1970)
	unsigned int nsec_firsthit = (unsigned int)0;		//An apprx. value
	unsigned int Event_id=  (unsigned int)0;
	unsigned long int Run_id=  (unsigned int)0;
	unsigned int LOFAR_trigg = (unsigned int)0;	//
	unsigned int Event_tree_index=  (unsigned int)0;
	unsigned int Event_size=  (unsigned int)0;
};


struct CONTROL_STRUCTURE
{
    //Structure of control parameters (returned by the Hisparc units) + headers to be stored in the log book
    //copied as is... from the old code.
	unsigned int Station = (unsigned int)0;		//Detector no
	unsigned int Detector = (unsigned int)0;		//Detector no
	unsigned int Time_stamp = (unsigned int)0;	//Time stamp in secs since the Epoch (00:00:00 UTC, Jan 1 1970)
	unsigned short Channel_offset_pos = (unsigned short)0;
	unsigned short Channel_offset_neg = (unsigned short)0;
	unsigned short Channel_gain_pos = (unsigned short)0;
	unsigned short Channel_gain_neg = (unsigned short)0;
	unsigned short Common_offset_adj = (unsigned short)0;
	unsigned short Full_scale_adj = (unsigned short)0;
	unsigned short Channel_inte_time = (unsigned short)0;
	unsigned short Comp_thres_low = (unsigned short)0;
	unsigned short Comp_thres_high = (unsigned short)0;
	unsigned short Channel_HV = (unsigned short)0;
	unsigned short Channel_thres_low = (unsigned short)0;
	unsigned short Channel_thres_high = (unsigned short)0;
	unsigned short Trigg_condition = (unsigned short)0;
	unsigned short Pre_coin_time = (unsigned short)0;	//in ns
	unsigned short Coin_time = (unsigned short)0;	//in ns
	unsigned short Post_coin_time = (unsigned short)0;	//in ns
};
typedef std::vector<CONTROL_STRUCTURE> tControl_Params_Station;

struct LOG_STRUCTURE			//Noise structure to be stored in ROOT file
{
	unsigned int Station = (unsigned int)0;
	unsigned short Channel_thres_low = (unsigned short)0;
	unsigned int detector = (unsigned int)0;		//Detector number
	unsigned int UTC_time_stamp = (unsigned int)0;	//Event time stamp in secs since the Epoch (00:00:00 UTC, Jan 1 1970)
	float Mean =0;			//Mean value of the noise distribution (100 events)
	float Sigma =0;			//Sigma value of the noise distribution (100 events)
};
typedef std::vector<LOG_STRUCTURE> tLog_Station;

struct STATION_INFO
{
	std::string name=""; // lasa1, lasa2, lasa3,....
	std::string type=""; // v1(old) or v2(new).
	std::string port_1=""; // master(old/v1) or digitizer_write(new/v2) //katie
	std::string port_2=""; // slave(old/v1) or digitizer_Read(new/v2) //katie
	std::string port_3=""; // empty(old/v1) or HV(new/v2) //katie
	std::string IPV4=""; // Self IPV4 address on network
	int no=99; // station number

	//only relevant for old stations. Set to zero for new.
	std::string HISPARC_Serial_m="";
	std::string HISPARC_Serial_s="";

	unsigned short init_control_params_m[40];
	unsigned short init_control_params_s[40];

	unsigned short init_control_params_ch[4][40]; //katie for V2
	unsigned short init_control_params_stn[40]; //katie for V2
};

struct DETECTOR_CONFIG{
	std::vector<std::string> active_stations;
	std::vector<int> list_of_dets_for_calib;
	int lofar_trig_mode=0;
	int lofar_trig_cond=0;
	int lora_trig_cond=0;
	int lora_trig_mode=0;
	int calibration_mode=0;
	int log_interval=0;
	int check_coinc_interval=0;
	int max_iteration_wait_interval=0;
	int reset_thresh_interval=0;
	int init_reset_thresh_interval=0;
	std::string output_path="";
	int coin_window=0;
	int wvfm_process_offtwlen=0;
	int wvfm_process_wpost=0;
	int wvfm_process_wpre=0;
	int wvfm_process_offtwlen_v2=0;  //katie, V2 trace has different sampling
  int wvfm_process_wpost_v2=0;  //katie, V2 trace has different sampling
  int wvfm_process_wpre_v2=0;  //katie, V2 trace has different sampling
	int diagnostics_interval=0;
	float sigma_ovr_thresh=0.0;
	int output_save_hour=23;
	int tbb_dump_wait_min=6;
	int osm_store_interval=600;
	int max_trigg_rate_to_reset_thresh=55; //per 5 minutes.
};

struct EVENT_SPOOL_SUMMARY
{
	std::string station_name="";
	int station_no=99;
	int evtspool_i=99;
	int evtspool_j=99;
	int Has_trigg=99;
	long long unsigned time_stamp=0;
	unsigned int GPS_time_stamp = (unsigned int)0;
	unsigned long int Time_Msg_Rcvd_ms = (unsigned long int)0;
	unsigned int nsec = (unsigned int)0;
	float charge=0.0;
	float corrected_peak=0.0;
};
typedef std::vector<EVENT_SPOOL_SUMMARY> tvec_EVENT_SPOOL_SUMMARY;
bool compare_by_time_stamp(const EVENT_SPOOL_SUMMARY &a,
													 const EVENT_SPOOL_SUMMARY &b);


std::string verify_detector_config(const DETECTOR_CONFIG&);

std::string verify_network_config(const std::vector<STATION_INFO>&);

std::string verify_init_control_params(const std::vector<STATION_INFO>&);

typedef std::vector<std::pair <unsigned char, int>> vec_pair_unsgnchar_int;

// typedef std::vector< std::pair<long long unsigned, std::string> > tvec_pair_lnglngunsgn_str;

typedef std::chrono::time_point<std::chrono::high_resolution_clock> tchrono;
