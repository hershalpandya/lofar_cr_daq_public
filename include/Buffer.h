//Implementation of the boost circular_buffer for this Software
#pragma once
#include <boost/circular_buffer.hpp>
#include <vector>
#include "Structs.h"

class BUFFER
{
public:
  void Init(unsigned int);
  //int Event_Bytes = 12023;
  // i.e. abt 11 mb is maximum packet size
  // 20 mb circular buffer to read in i.e. 20480 .
  void char_pushback(unsigned char*, size_t);
  //extract method looks for a complete packet
  //with header and tail. and store into output vector.
  //deletes the extracted content from the buffer.
  std::vector<unsigned char> extract_first_msg_in_buf(unsigned char,
                                                      unsigned char,
                                                      vec_pair_unsgnchar_int);
                                                      
  std::vector<unsigned char> extract_electronics_calib_msg_in_buf();

  size_t Get_Buffer_Size();

  bool Is_Buffer_Full();

private:
  unsigned int buf_capacity;
  boost::circular_buffer<unsigned char> buf;
};
