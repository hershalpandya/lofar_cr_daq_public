#include "functions_library.h"
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <string>
#include <ctime>
#include "Structs.h"
#include <chrono>
#include <algorithm> //std::max_element
#include <numeric> //accumulate
#include <cmath> //sqrt, pow
#include <boost/circular_buffer.hpp>

//independent function for getting local time in utc.
std::tm* Get_Current_Time()
{
  std::time_t now = std::time(nullptr);
  std::tm *utc = std::gmtime(&now) ;
  return utc;
}

unsigned long int Get_Current_Time_ms()
{
  unsigned long int t_now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
  return t_now_ms;
}


tchrono Get_Current_Time_chrono()
{
  return std::chrono::high_resolution_clock::now();
}

unsigned long int Get_s_Since_Epoch()
{
  tchrono t = Get_Current_Time_chrono();
  auto temp = std::chrono::duration_cast<std::chrono::seconds>(t.time_since_epoch());
  unsigned long int tE= temp.count();
  return tE;
}

int Get_ms_Elapsed_Since(tchrono t0)
{
  tchrono t1 = Get_Current_Time_chrono();
  int dt= std::chrono::duration_cast<std::chrono::milliseconds>(t1-t0).count();
  return dt;
}

// //independent function for getting detector_no from lasax and Master/slave
unsigned int Get_Detector_Number(std::string sta_name, std::string m_or_s, int channel)
{
  unsigned int x = 99 ;
  unsigned int y = 99 ;

  if (channel!=0 && channel!=1)
  {
    std::stringstream ss1;
    ss1 << "Get_Detector_Number() behaviour only defined for channel = 0 or 1";
    ss1 << "Received " <<  channel <<" as third arg" ;
    throw std::runtime_error(ss1.str());
  }

  std::stringstream ss22(sta_name.substr(4,2));
  //2 because lasa10 will have 2 digits
  ss22 >> x;

  if (sta_name.substr(0,4)!="lasa")
  {
    std::stringstream ss1;
    ss1 << "Get_Detector_Number() behaviour only defined for lasaX";
    ss1 << "Received " <<  sta_name <<" as first arg" ;
    throw std::runtime_error(ss1.str());
  }

  if (m_or_s == "Master")
  {
    y=1;
  }
  else if (m_or_s == "Slave")
  {
    y=2;
  }
  else
  {
    std::stringstream ss1;
    ss1 << "Get_Detector_Number() behaviour only defined for 'Master' / 'Slave'.";
    ss1 << "Received " << m_or_s <<" as second arg" ;
    throw std::runtime_error(ss1.str());
  }

  unsigned int detno = (x-1)*4 + (y-1)*2 + channel +1 ;

  return detno;
}

unsigned int Get_Detector_Number(int sta_no, int channel)
{
  unsigned int detno = 4*(sta_no-1) + channel + 1 ;
  return detno;
}

//This function returns the trigger condition for a Hisparc unit corresponding
//to the input trigger code "num". Ref: Lookup table provided in the HISPARC manual.
//
char Trigger_Condition(unsigned short num)
{
	unsigned char trigger ;
	if(num==1) trigger=0x01 ;
	if(num==2) trigger=0x02 ;
	if(num==3) trigger=0x03 ;
	if(num==4) trigger=0x04 ;
	if(num==5) trigger=0x08 ;
	if(num==6) trigger=0x09 ;
	if(num==7) trigger=0x0A ;
	if(num==8) trigger=0x0B ;
	if(num==9) trigger=0x10 ;
	if(num==10) trigger=0x11 ;
	if(num==11) trigger=0x12 ;
	if(num==12) trigger=0x18 ;
	if(num==13) trigger=0x19 ;
	if(num==14) trigger=0x20 ;
	if(num==15) trigger=0x0C ;
	if(num==16) trigger=0x0D ;
	if(num==17) trigger=0x0E ;
	if(num==18) trigger=0x0F ;
	if(num==19) trigger=0x14 ;
	if(num==20) trigger=0x15 ;
	if(num==21) trigger=0x16 ;
	if(num==22) trigger=0x17 ;
	if(num==23) trigger=0x1C ;
	if(num==24) trigger=0x1D ;
	if(num==25) trigger=0x1E ;
	if(num==26) trigger=0x1F ;
	if(num==27) trigger=0x24 ;
	if(num==28) trigger=0x25 ;
	if(num==29) trigger=0x26 ;
	if(num==30) trigger=0x27 ;
	if(num==31) trigger=0x40 ;	//External trigger only
	//if(num==32) trigger=0x48 ;	//External trigger with atleast one high signal
	if(num==32) trigger=0x41 ;	//External trigger with atleast one low signal
	if(num==33) trigger=0x80 ;	//Calibration mode

	if(num>33 || num<=0)
	{
		printf("Unknown Trigger Condition !!\n") ;
		exit(1) ;
	}
	return trigger ;
}

void Build_Hisparc_Messages(const unsigned short* cp, unsigned char* new_cp)
{
  /*
  Called before sending Messages to Hisparc Digitizers.
  */

	new_cp[0]=(cp[0]) & 0x00ff ;
	new_cp[1]=(cp[1]) & 0x00ff ;
	new_cp[2]=(cp[2]) & 0x00ff ;
	new_cp[3]=(cp[3]) & 0x00ff ;
	new_cp[4]=(cp[4]) & 0x00ff ;
	new_cp[5]=(cp[5]) & 0x00ff ;
	new_cp[6]=(cp[6]) & 0x00ff ;
	new_cp[7]=(cp[7]) & 0x00ff ;
	new_cp[8]=(cp[8]) & 0x00ff ;
	new_cp[9]=(cp[9]) & 0x00ff ;
	new_cp[10]=(cp[10]) & 0x00ff ;
	new_cp[11]=(cp[11]) & 0x00ff ;
	new_cp[12]=(cp[12]) & 0x00ff ;
	new_cp[13]=(cp[13]) & 0x00ff ;
	new_cp[14]=(unsigned short)((cp[14]+34.0714)/7.93155) & 0x00ff ;
	new_cp[15]=(unsigned short)((cp[15]+34.0714)/7.93155) & 0x00ff ;
	new_cp[16]=((unsigned short)(cp[16]/0.4883) & 0xff00)>>8 ;
	new_cp[17]=((unsigned short)(cp[16]/0.4883) & 0x00ff) ;
	new_cp[18]=((unsigned short)(cp[17]/0.4883) & 0xff00)>>8 ;
	new_cp[19]=((unsigned short)(cp[17]/0.4883) & 0x00ff) ;
	new_cp[20]=((unsigned short)(cp[18]/0.4883) & 0xff00)>>8 ;
	new_cp[21]=((unsigned short)(cp[18]/0.4883) & 0x00ff) ;
	new_cp[22]=((unsigned short)(cp[19]/0.4883) & 0xff00)>>8 ;
	new_cp[23]=((unsigned short)(cp[19]/0.4883) & 0x00ff) ;
	new_cp[24]=Trigger_Condition(cp[20]) ;

	unsigned short Pre_Coin_Time=(unsigned short)(cp[21]/5) ;	//no. of 5ns steps
	new_cp[25]=(Pre_Coin_Time & 0xff00)>>8 ;
	new_cp[26]=(Pre_Coin_Time & 0x00ff) ;

	unsigned short Coin_Time=(unsigned short)(cp[22]/5) ;		//no. of 5ns steps
	new_cp[27]=(Coin_Time & 0xff00)>>8 ;
	new_cp[28]=(Coin_Time & 0x00ff) ;

	unsigned short Post_Coin_Time=(unsigned short)(cp[23]/5) ;	//no. of 5ns steps
	new_cp[29]=(Post_Coin_Time & 0xff00)>>8 ;
	new_cp[30]=(Post_Coin_Time & 0x00ff) ;

	new_cp[31]=((unsigned short)(cp[24]) & 0xff00)>>8 ;
	new_cp[32]=((unsigned short)(cp[24]) & 0x00ff) ;

  return;
}

void Check_HV(const unsigned short tHV,std::string ch_name)
{
  if (tHV>=1799)
  {
    std::stringstream sserr;
    sserr << ch_name ;
    sserr << " HV is too high. Max allowed value is 1799.";
    sserr << " Assigned value is " << tHV;
    throw std::runtime_error(sserr.str());
  }
}

void Process_Waveform(const std::vector<int>& trace, //4000 bins long
                      const int& windw_len_pre_peak, // n bins before peak to include in window
                      const int& windw_len_post_peak, // n bins after peak to include in window
                      const int& offwtrace_length,//n bins off window to include for baseline.
                      int& raw_peak,
                      float& baseline,
                      float& corrected_peak,
                      float& integrated_counts,
                      float& offw_std)
{
  // trace is 4000 bins long. i.e. 10,000 ns.
  // on window should be about 400 ns.
  // i.e. about 160 bins.
  // lets say about 25 bins before peak and 135 bins after peak
  int peakIndex = std::max_element(trace.begin(),trace.end()) - trace.begin();
  raw_peak = *std::max_element(trace.begin(), trace.end());

  if (windw_len_pre_peak+windw_len_post_peak+offwtrace_length>trace.size())
  {
    std::cout << "ERROR: time window /off-tw length too large" ;
    std::cout << __LINE__ <<  ", " << __FILE__ << std::endl;
  }

  int tw_start = peakIndex - windw_len_pre_peak ;
  int tw_stop = peakIndex + windw_len_post_peak;

  if (tw_start<0)
  { //lets say peakIndex is at 20
    tw_stop += -1.0*tw_start; // extend end of window
    tw_start = 0;
  }
  else if (tw_stop>trace.size())
  {//lets say peakIndex is 3900 then 3900+135  > 4000
    tw_start -= (tw_stop - trace.size());
    //increase pre length from 25 to , 25 + 135 - 4000 + 3900
    tw_stop = trace.size();
    // reduce post length from 135 to , 4000-3900 i.e. 100.
  }

  float n_integrated_counts=0.0;
  std::vector<float> offwtrace;
  std::vector<float> onwtrace;
  // std::cout << "trace size: " << trace.size() << std::endl;
  // std::cout << "tw start, stop: " << tw_start << " " << tw_stop << std::endl;

  //just collect on time window trace.
  for (int i=0; i<trace.size(); ++i)
  {
    if (i>=tw_start && i<tw_stop)
     {
       onwtrace.push_back(static_cast<float> (trace[i]));
     }
    else if (i>=tw_stop)
    {
      break;
    }
  }

  //first attempt to collect off time window trace. starting from just before tw.
  for (int i=tw_start-1;i>=0;--i)
  {
    if (offwtrace.size()<offwtrace_length)
    {
      offwtrace.push_back(static_cast<float> (trace[i]));
    }
    else
    {
      break;
    }
  }

  //if enough waveform not available before ontimewindow, then go to after
  for (int i=tw_stop+1;i<trace.size();++i)
  {
    if (offwtrace.size()<offwtrace_length)
    {
      offwtrace.push_back(static_cast<float> (trace[i]));
    }
    else
    {
      break;
    }
  }

  baseline = 1.0* std::accumulate(offwtrace.begin(),offwtrace.end(),0);
  baseline /= offwtrace.size()*1.0;

  offw_std=0.0;
  for (int i=0; i<offwtrace.size(); ++i)
    offw_std += std::pow(offwtrace[i] - baseline, 2);
  offw_std /= offwtrace.size();
  offw_std = std::sqrt(offw_std);

  if (onwtrace.size()!=(windw_len_pre_peak+windw_len_post_peak))
    std::cout << "on/off trace size" << onwtrace.size() << " " << offwtrace.size() << std::endl;

  integrated_counts=0.0;
  for (int i=0;i<onwtrace.size();++i) integrated_counts+=(onwtrace[i]-baseline);

  corrected_peak = raw_peak - baseline;
}

void Time_Average_From_Circ_Buffer(const boost::circular_buffer<float>& main_buffer,
                                   const boost::circular_buffer<long long unsigned>& time_buffer,
                                   const long long unsigned& time_stamp_now_ns,
                                   const int& time_interval_ms,
                                   float& mean, int& n)
{
  mean=0;
  n=0;
  double delta_ms=0;

  if (main_buffer.size()==0) return;

  if (main_buffer.size()!=time_buffer.size())
  {
    std::cout << __LINE__ <<" " << __FILE__ << " " <<std::endl;
    std::cout << "main_buffer and time_buffer have to be of equal size";
    std::cout << std::endl;
    return;
  }

  int last = main_buffer.size()-1;
  delta_ms = (time_stamp_now_ns - time_buffer[last]) *1.0;
  delta_ms /= (1000*1000);

  if (delta_ms > time_interval_ms) return;

  mean = main_buffer[last];
  n = 1;
  for (int j=last-1; j>=0; --j)
  {
    delta_ms = (time_stamp_now_ns - time_buffer[j]) *1.0 ;
    delta_ms /= (1000*1000);

    if (delta_ms > time_interval_ms) break;

    mean +=  main_buffer[j];
    n += 1;
  }

  mean /= 1.0*n;

  return;
}

void Time_Average_From_Circ_Buffer(const boost::circular_buffer<long long unsigned>& time_buffer,
                                   const long long unsigned& time_stamp_now_ns,
                                   const int& time_interval_ms,
                                   int& n)
{
  n=0;
  double delta_ms=0;

  if (time_buffer.size()==0) return;

  int last = time_buffer.size()-1;
  delta_ms = (time_stamp_now_ns - time_buffer[last]) *1.0;
  delta_ms /= (1000*1000);

  if (delta_ms > time_interval_ms) return;

  n = 1;
  for (int j=last-1; j>=0; --j)
  {
    delta_ms = (time_stamp_now_ns - time_buffer[j]) *1.0 ;
    delta_ms /= (1000*1000);

    if (delta_ms > time_interval_ms) break;

    n += 1;
  }

  return;
}

void Build_V2_Stn_Messages(const unsigned short* cp, unsigned char* new_cp)//katie
{
    // (0) triger condition, (1) full scale enable, (2) pps enable, (3) daq enable, (4) ch1 en, (5) ch2 en, (6) ch3 en, (7) ch4 en, (8) cal en, (9) 10 sec en, (10) ext en, (11) ch1 readout en, (12) ch2 readout en, (13) ch3 readout en, (14) ch4 readout en, (15) triger rate divider, (16) coincidence readout time
    //CONTROL REGISTER -> 2 bits

    new_cp[0]=((unsigned short)0x99);  // start message
    new_cp[1]=((unsigned short)0x21);  //identifier

    //CONTROL REGISTER -> 2 bits
    unsigned short control=0;
    control= (cp[1]<<2) | (cp[2]<<1)| (cp[3]<<0);

    new_cp[2]=((unsigned short)(control) & 0xff00)>>8 ;
    new_cp[3]=((unsigned short)(control) & 0x00ff) ;


    //TRIGGER ENABLE MASK-> 2 bits
    unsigned short trigger=0;
    trigger= (cp[7]<<11) | (cp[6]<<10)| (cp[5]<<9) | (cp[4]<<8) | (0<<7) | (cp[8]<<6) | (cp[9]<<5) | (cp[10]<<4);
    //printf("%x\n",trigger);
    new_cp[4]=((unsigned short)(trigger) & 0xff00)>>8 ;
    new_cp[5]=((unsigned short)(trigger) & 0x00ff) ;


    //CHANNEL READOUT -> 1 bits, TRIGGER RATE DIVIDER
    unsigned short chan=0;
    unsigned short rate=0;

    chan= (cp[14]<<3) | (cp[13]<<2)| (cp[12]<<1)|(cp[11]<<0);
    //printf("%x\n",chan);
    rate=cp[15];


    new_cp[6]=rate;
    new_cp[7]=chan;


    //COMMON COIN READOUT TIME -> 2 bits
    unsigned short coin=0;
    coin=cp[16]/5;
    new_cp[8]=((unsigned short)(coin) & 0xff00)>>8 ;
    new_cp[9]=((unsigned short)(coin) &  0x00ff);
    new_cp[10]=(unsigned short)cp[0];
    new_cp[39]=((unsigned short)0x66);

    return;
}

void Build_V2_Det_Messages(const unsigned short* cp, unsigned char* new_cp, int ch)//katie
{
    //(0) HV, (1) pre_coin_time, (2) post_coin_time,(3) gain_correction,
    // (4) offset_correction,(5) integration_time, (6) base_max, (7) base_min,
    // (8) sig_T1, (9) sig_T2, (10) Tprev, (11) Tper, (12) TCmax, (13) NCmax,
    //(14)  NCmin, (15) Qmax, (16) Qmin
    new_cp[0]=((unsigned short)0x99);  // start message
    new_cp[1]=((unsigned short)0x20);  //identifier
    new_cp[2]=((unsigned short) (ch+1)) ; // ch

    new_cp[3]=((unsigned short) (cp[0])& 0xff00)>>8 ; // HV
    new_cp[4]=((unsigned short) (cp[0]) & 0x00ff)  ; // HV
    // [5] was trigger condition (ie 3/4)

    new_cp[5]=((unsigned short) ((cp[1]/5) & 0xff00));//>>8 ; // pre-coincidence time
    new_cp[6]=((unsigned short) ((cp[1]/5)& 0x00ff))  ; // pre-coincidence time
    // std::cout<<"pre-coincidence time: "<<cp[1]<<"  "<<cp[1]/5<<"\n";

    // std::cout<<"pre-coincidence time: "<<(unsigned short)new_cp[5]<<"  "<<(unsigned short)new_cp[6]<<"\n";

    new_cp[7]=((unsigned short) (cp[2]/5) & 0xff00)>>8 ; // post-coincidence time
    new_cp[8]=((unsigned short) (cp[2]/5) & 0x00ff)  ; // post-coincidence time

    new_cp[9]=((unsigned short) (cp[3]) & 0xff00)>>8 ; // gain correction
    new_cp[10]=((unsigned short) (cp[3]) & 0x00ff)  ; // gain correction

    new_cp[11]=((unsigned short) (cp[4])) ; // offset correction
    new_cp[12]=((unsigned short) (cp[5])) ; // integration time

    new_cp[13]=((unsigned short) (cp[6]) & 0xff00)>>8 ; // base max
    new_cp[14]=((unsigned short) (cp[6]) & 0x00ff)  ; // base max

    new_cp[15]=((unsigned short) (cp[7]) & 0xff00)>>8 ; // base min
    new_cp[16]=((unsigned short) (cp[7]) & 0x00ff)  ; // base min

    new_cp[17]=((unsigned short) (cp[8]) & 0xff00)>>8 ; // sig T1
    new_cp[18]=((unsigned short) (cp[8]) & 0x00ff)  ; // sig T1
    // std::cout<<"T1 send: "<<cp[8]<<"\n";

    new_cp[19]=((unsigned short) (cp[9]) & 0xff00)>>8 ; // sig T2
    new_cp[20]=((unsigned short) (cp[9]) & 0x00ff)  ; // sig T2

    new_cp[21]=((unsigned short) (cp[10]) )  ; // t prev
    new_cp[22]=((unsigned short) (cp[11]) )  ; // t per
    new_cp[23]=((unsigned short) (cp[12]))  ; // tc max
    new_cp[24]=((unsigned short) (cp[13]))  ; // nc max
    new_cp[25]=((unsigned short) (cp[14]))  ; // nc min
    new_cp[26]=((unsigned short) (cp[15]))  ; // qmax
    new_cp[27]=((unsigned short) (cp[16]))  ; // qmin

    new_cp[39]=((unsigned short)0x66); // end message

    return;
}
