/*
Struct verification standalone functions.
*/
#include "Structs.h"
#include "functions_library.h"
#include <sstream>
#include <string>
#include <vector>
#include <iostream>
#include <algorithm> //std::find(), std::sort

//FIXIT: Consider making a namespace for standalone functions.

std::string verify_detector_config(const DETECTOR_CONFIG& dcfg)
{
	std::string final_msg="";

	for (int i=0;i<dcfg.active_stations.size();i++)
	{
		if (dcfg.active_stations[i].find("lasa")==std::string::npos)
		{
			std::stringstream ss1;
			ss1 << "List of active_stations contains bad name: ";
			ss1 << dcfg.active_stations[i];
			final_msg+= ss1.str() + " ;";
		}
	}

	for (int i=0;i<dcfg.list_of_dets_for_calib.size();i++)
	{
		if (dcfg.list_of_dets_for_calib[i]<1 || dcfg.list_of_dets_for_calib[i]>20)
		{
			std::stringstream ss1;
			ss1 << "list_of_dets_for_calib contains bad no: ";
			ss1 << dcfg.list_of_dets_for_calib[i];
			final_msg+= ss1.str() + " ;";
		}
	}

	if (dcfg.lofar_trig_mode!=1 && dcfg.lofar_trig_mode!=2)
	{
		std::stringstream ss1;
		ss1<< "lofar_trig_mode value given: " << dcfg.lofar_trig_mode;
		ss1<< ". allowed values: 1 / 2.";
		final_msg+= ss1.str() + " ;";
	}

	if (dcfg.max_trigg_rate_to_reset_thresh<15 && dcfg.max_trigg_rate_to_reset_thresh>60)
	{
		std::stringstream ss1;
		ss1<< "max_trigg_rate_to_reset_thresh value given: " << dcfg.lofar_trig_mode;
		ss1<< ". allowed values: 15 to 60";
		final_msg+= ss1.str() + " ;";
	}

	if (dcfg.tbb_dump_wait_min<4 && dcfg.tbb_dump_wait_min>61)
	{
		std::stringstream ss1;
		ss1<< "tbb_dump_wait_min value given: " << dcfg.tbb_dump_wait_min;
		ss1<< ". allowed values:>4 or <61.";
		final_msg+= ss1.str() + " ;";
	}

	if (dcfg.lora_trig_mode!=1 && dcfg.lora_trig_mode!=2)
	{
		std::stringstream ss1;
		ss1<< "lora_trig_mode value given: " << dcfg.lora_trig_mode;
		ss1<< ". allowed values: 1 / 2.";
		final_msg+= ss1.str() + " ;";
	}

	if (dcfg.lofar_trig_cond>=40 || dcfg.lofar_trig_cond<3)
	{
		std::stringstream ss1;
		ss1<< "lofar_trig_cond value given: " << dcfg.lofar_trig_cond;
		ss1<< ". allowed values: [2,40) ";
		final_msg+= ss1.str() + " ;";
	}

	if (dcfg.reset_thresh_interval<30000)
	{
		std::stringstream ss1;
		ss1<< "reset_thresh_interval value given: " << dcfg.reset_thresh_interval;
		ss1<< ". allowed values: >=30,000.0 i.e. ? mins in ms. ";
		final_msg+= ss1.str() + " ;";
	}

	if (dcfg.osm_store_interval>900)
	{
		std::stringstream ss1;
		ss1<< "osm_store_interval value given: " << dcfg.osm_store_interval;
		ss1<< ". allowed values: <=900 ms. ";
		final_msg+= ss1.str() + " ;";
	}

	if (dcfg.reset_thresh_interval<=dcfg.init_reset_thresh_interval)
	{
		std::stringstream ss1;
		ss1<< "reset_thresh_interval value given: " << dcfg.reset_thresh_interval;
		ss1<< "init_reset_thresh_interval value given: " << dcfg.init_reset_thresh_interval;
		ss1<< "init has to be less than main value. ";
		final_msg+= ss1.str() + " ;";
	}

	if (dcfg.sigma_ovr_thresh<1 || dcfg.sigma_ovr_thresh>9)
	{
		std::stringstream ss1;
		ss1<< "sigma_ovr_thresh value given: " << dcfg.sigma_ovr_thresh;
		ss1<< ". allowed values: >1 and <9 ";
		final_msg+= ss1.str() + " ;";
	}

	if (dcfg.coin_window<400 || dcfg.coin_window>4000 )
	{
		std::stringstream ss1;
		ss1<< "coin_window value given: " << dcfg.coin_window;
		ss1<< ". allowed values: cw>400, cw<4000 ";
		final_msg+= ss1.str() + " ;";
	}

	if (dcfg.output_save_hour<00 || dcfg.output_save_hour>23 )
	{
		std::stringstream ss1;
		ss1<< "output_save_hour value given: " << dcfg.output_save_hour;
		ss1<< ". allowed values: output_save_hour>=0, output_save_hour<=23 ";
		final_msg+= ss1.str() + " ;";
	}

	if (dcfg.lora_trig_cond<1 || dcfg.lora_trig_cond>40)
	{
		std::stringstream ss1;
		ss1<< "lora_trig_cond value given: " << dcfg.lora_trig_cond;
		ss1<< ". allowed values: [1,40] ";
		final_msg+= ss1.str() + " ;";
	}

	if (dcfg.calibration_mode!=0 && dcfg.calibration_mode!=1 && dcfg.calibration_mode!=2)
	{
		std::stringstream ss1;
		ss1<< "calibration_mode value given: " << dcfg.calibration_mode;
		ss1<< ". allowed values: 0/1/2 ";
		final_msg+= ss1.str() + " ;";
	}

	if (dcfg.log_interval>3600000 || dcfg.log_interval<30000)
	{
		std::stringstream ss1;
		ss1<< "log_interval value given: " << dcfg.log_interval;
		ss1<< ". allowed values: [100,10000] ";
		final_msg+= ss1.str() + " ;";
	}

	if (dcfg.check_coinc_interval>10000 || dcfg.check_coinc_interval<50)
	{
		std::stringstream ss1;
		ss1<< "check_coinc_interval value given: " << dcfg.check_coinc_interval;
		ss1<< ". allowed values: [50,10000] ";
		ss1<< " 150 ms so that master and slave can both send their packets.";
		final_msg+= ss1.str() + " ;";
	}

	if (dcfg.max_iteration_wait_interval>(60000)|| dcfg.max_iteration_wait_interval<50)
	{
		std::stringstream ss1;
		ss1<< "max_iteration_wait_interval value given: " << dcfg.max_iteration_wait_interval;
		ss1<< ". allowed values: [50,60000] between 50 ms and 60 seconds.";
		ss1<< " ";
		final_msg+= ss1.str() + " ;";
	}


	auto temp = dcfg.wvfm_process_wpre
						+ dcfg.wvfm_process_wpost
						+ dcfg.wvfm_process_offtwlen;

	if (temp>4000 || temp<20)
	{
		std::stringstream ss1;
		ss1<< "wvfm_process_wpre: " << dcfg.wvfm_process_wpre;
		ss1<< " wvfm_process_wpost: " << dcfg.wvfm_process_wpost;
		ss1<< " wvfm_process_offtwlen: " << dcfg.wvfm_process_offtwlen;
		ss1<< ". sum of all 3 can be between 20(adhoc) and 4000(len of trace) .";
		final_msg+= ss1.str() + " ;";
	}

	if (dcfg.diagnostics_interval>30*60*1000 || dcfg.diagnostics_interval<1000)
	{
		std::stringstream ss1;
		ss1<< "diagnostics_interval value given: " << dcfg.diagnostics_interval;
		ss1<< ". allowed values: [1*1000,30*60*1000] ";
		ss1<< " in ms ";
		final_msg+= ss1.str() + " ;";
	}

	return final_msg;
}

std::string verify_network_config(const std::vector<STATION_INFO>& nc)
{
	//added only a few sanity checks.
	std::string final_msg;
	std::vector<int> sta_nos;

	for (int i=0;i<nc.size();i++)
	{
		if (nc[i].type=="clientv1" || nc[i].type=="clientv2")
		{
			sta_nos.push_back(nc[i].no);
		}
		if (nc[i].type=="clientv1" && nc[i].name.find("lasa")!=0)
		{
			std::stringstream ss1;
			ss1 << "type: "<<nc[i].type << " and name: ";
			ss1 << nc[i].name <<", mismatch. ";
			final_msg+= ss1.str() + " ;";
		}
		else if (nc[i].type=="clientv2" && nc[i].name.find("lasa")!=0)
		{
			std::stringstream ss1;
			ss1 << "type: "<<nc[i].type << " and name: ";
			ss1 << nc[i].name <<", mismatch. ";
			final_msg+= ss1.str() + " ;";
		}
		else if (nc[i].type=="host" && nc[i].name.find("lora")!=0)
		{
			std::stringstream ss1;
			ss1 << "type: "<<nc[i].type << " and name: ";
			ss1 << nc[i].name <<", mismatch. ";
			final_msg+= ss1.str() + " ;";
		}
		else if (nc[i].type=="superhost" && nc[i].name.find("lofar")!=0)
		{
			std::stringstream ss1;
			ss1 << "type: "<<nc[i].type << " and name: ";
			ss1 << nc[i].name <<", mismatch. ";
			final_msg+= ss1.str() + " ;";
		}
	}

	int n_lasa_stations = nc.size() - 2 ; //superhost, host removed.
	if (sta_nos.size()!=n_lasa_stations)
	{
		std::stringstream ss1;
		ss1 << "found " << sta_nos.size() << " stations in network config";
		ss1 << " != " << n_lasa_stations;
		final_msg+= ss1.str() + " ;";
	}

	std::sort(sta_nos.begin(),sta_nos.end());

	int prev= sta_nos[0];
	for (int i=1;i<sta_nos.size();++i)
	{
		int delta = sta_nos[i]-prev;
		if (delta!=1)
		{
			std::stringstream ss1;
			ss1 << "sta no X in lasaX not in sequence for all stations listed.";
			final_msg+= ss1.str() + " ;";
		}
		prev = sta_nos[i];
	}
	return final_msg;
}

std::string verify_init_control_params(const std::vector<STATION_INFO>& nc)
{
	std::string err="";
	for (int i=0; i<nc.size(); i++)
	{
		if (nc[i].type=="clientv1")
		{
			std::stringstream ss1,ss2,ss3,ss4;
			ss1 << nc[i].name << " " << nc[i].type  << " " ;
			ss2 << nc[i].name << " " << nc[i].type  << " " ;
			ss3 << nc[i].name << " " << nc[i].type  << " " ;
			ss4 << nc[i].name << " " << nc[i].type  << " " ;
			ss1 << "Master Ch1";
			Check_HV(nc[i].init_control_params_m[14],ss1.str());
			ss2 << "Master Ch2";
			Check_HV(nc[i].init_control_params_m[15],ss2.str());
			ss3 << "Slave Ch1";
			Check_HV(nc[i].init_control_params_s[14],ss3.str());
			ss4 << "Slave Ch2";
			Check_HV(nc[i].init_control_params_s[15],ss4.str());
		}
	}
return err;
}

bool compare_by_time_stamp(const EVENT_SPOOL_SUMMARY &a,
													 const EVENT_SPOOL_SUMMARY &b)
{
 	return a.time_stamp < b.time_stamp;
}
