#pragma once

#include <string>
#include <vector>
#include <ctime>

#include "Structs.h"
#include "Buffer.h"
#include "Socket_Calls.h"

class LORA_STATION //LORA STATION CLASS.
//this is an abstract class
// all methods are pure virtual
// https://stackoverflow.com/questions/2652198/difference-between-a-virtual-function-and-a-pure-virtual-function
//https://stackoverflow.com/questions/270917/why-should-i-declare-a-virtual-destructor-for-an-abstract-class-in-c
{
public:
  // Pure virtual destructor for this class
  virtual ~LORA_STATION()=0;

  //Initializes all private members.
  virtual void Init(const STATION_INFO&, const std::string&)=0;

  //Opens 2 sockets + 2 spare sockets for this station
  virtual void Open()=0;

  //Sends initial control params to digitizer via spare socket.
  virtual void Send_Control_Params()=0;

  //Sends a small msg that tells lasa-client / digitzer to perform Electronics calibration
  virtual void Send_Electronics_Calib_Msg()=0;

  //Returns no of detectors for which electronics msg was received
  virtual void Receive_Electronics_Calib_Msg(int&, const std::string&,
                                            const STATION_INFO&)=0;

  //each station should add using FD_SET to active_stations_fds
  //and replace value in max_fd_val if one of its fds has a higher value.
  virtual void Add_readfds_To_List(fd_set&, int&)=0;

  //Listens to incoming packets from the client
  virtual void Listen(fd_set&)=0;

  //Accepts main and spare connections to the client
  virtual int Accept(fd_set&)=0;

  //When the readin buffer is full, interprets the type of
  //msg and moves packet to appropriate spool.
  virtual void Interpret_And_Store_Incoming_Msgs(tm&)=0;

  //sends summary of events in the event spool of this station.
  //so that ops class can construct an event out of it.
  virtual void Send_Event_Spool_Info(tvec_EVENT_SPOOL_SUMMARY&)=0;

  // gets a vector of <time, str> and deletes the hit at
  // event_spool[i][j] if time of the event matches
  virtual void Discard_Events_From_Spool(const tvec_EVENT_SPOOL_SUMMARY&)=0;

  // Moves data from buffers to appropriate containers
  // outside the scope of this class. in Ops class.
  // create a new vec of EVENT_DATA_STRUCTURE size 4
  // this vector is typedefed as tEvent_Data_Station
  // fill it and push to tEvent_Data_Station reference
  // given by the Ops class.
  // called by Ops Pull_Data_From_Stations()
  // should delete the pushed data from buffer.
  virtual void Send_Event_Data(tEvent_Data_Station&, const tvec_EVENT_SPOOL_SUMMARY&)=0;
  // same as above for cp- we don't save complete cp msgs since all info is same except threshold.
  //virtual void Send_CP_Delete_From_Spool(tControl_Params_Station&)=0;
  //same as above for osm
  virtual void Send_OSM_Delete_From_Spool(ONE_SEC_STRUCTURE&, ONE_SEC_STRUCTURE&)=0;
  //calculate and push noise. will need to add private
  //members to hold running noise mean and sigma.
  virtual void Send_Log(tLog_Station&)=0;

  // Closes connections. Call Stop and close socket.
  virtual void Close()=0;

  virtual std::string Send_Name()=0;

  virtual void Process_Event_Spool_Before_Coinc_Check(DETECTOR_CONFIG&,int&)=0;

  virtual void Run_Detectors_Diagnostics(const std::string&,
                                 const DETECTOR_CONFIG&, const tm&, bool&)=0;

  virtual void Calculate_New_Threshold(DETECTOR_CONFIG&,const tm&,int&)=0;

  virtual void Set_New_Threshold()=0;

  virtual void Status_Monitoring(bool&, std::string&)=0;

  virtual int Get_Sum_Size_of_Spools()=0;
};

// https://isocpp.org/wiki/faq/virtual-functions#virtual-dtors
// When someone will delete a derived-class object via a base-class pointer.
//
// In particular, here’s when you need to make your destructor virtual:
//
//     if someone will derive from your class,
//     and if someone will say new Derived, where Derived is derived from your class,
//     and if someone will say delete p, where the actual object’s type is Derived but the pointer p’s type is your class.
//
// Confused? Here’s a simplified rule of thumb that usually protects you and usually doesn’t cost you anything: make your destructor virtual if your class has any virtual functions. Rationale:
//
//     that usually protects you because most base classes have at least one virtual function.
//     that usually doesn’t cost you anything because there is no added per-object space-cost for the second or subsequent virtual in your class. In other words, you’ve already paid all the per-object space-cost that you’ll ever pay once you add the first virtual function, so the virtual destructor doesn’t add any additional per-object space cost. (Everything in this bullet is theoretically compiler-specific, but in practice it will be valid on almost all compilers.)
//
// Note: in a derived class, if your base class has a virtual destructor, your own destructor is automatically virtual. You might need an explicitly defined destructor for other reasons, but there’s no need to redeclare a destructor simply to make sure it is virtual. No matter whether you declare it with the virtual keyword, declare it without the virtual keyword, or don’t declare it at all, it’s still virtual.
//
// By the way, if you’re interested, here are the mechanical details of why you need a virtual destructor when someone says delete using a Base pointer that’s pointing at a Derived object. When you say delete p, and the class of p has a virtual destructor, the destructor that gets invoked is the one associated with the type of the object *p, not necessarily the one associated with the type of the pointer. This is A Good Thing. In fact, violating that rule makes your program undefined. The technical term for that is, “Yuck.”
