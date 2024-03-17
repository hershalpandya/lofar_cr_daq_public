#include "Buffer.h"
#include <vector>
#include <iostream>
#include <stdexcept>
#include <boost/circular_buffer.hpp>
#include "Structs.h"

//https://www.boost.org/doc/libs/1_55_0/doc/html/boost/circular_buffer.html

void BUFFER::Init(unsigned int buf_capacity)
{
  buf.set_capacity(buf_capacity);
}


size_t BUFFER::Get_Buffer_Size()
{
  return buf.size();
}

bool BUFFER::Is_Buffer_Full()
{
  return buf.capacity()==buf.size();
}


void BUFFER::char_pushback(unsigned char chars[], size_t len_of_chars)
{
  for (int i=0; i<len_of_chars; ++i)
  {
      buf.push_back(chars[i]);
  }
}


std::vector<unsigned char> BUFFER::extract_first_msg_in_buf(unsigned char header,
                                                            unsigned char tail,
                                                            vec_pair_unsgnchar_int identifiers)
{
  std::vector<unsigned char> msg;
  // fn copies msg char array between header e.g. 0x99 and tail e.g. 0x66
  // into msg vector
  bool msg_begin = false;
  bool msg_end = false;
  boost::circular_buffer<unsigned char>::iterator it, it_msg_begin, it_msg_end;
  int expected_msg_size=0;
  unsigned char msg_identifier;

  for (it=buf.begin();it!=buf.end()-1; ++it)
  //buf.end()-1 because later on we de-reference (it+1)
  {
    if (*it==header && !msg_begin)
    {//look for starting of a msg.
      for (int j_id=0; j_id<identifiers.size(); ++j_id)
      {
        if (*(it+1)==identifiers[j_id].first)
        {
          msg.clear();
          msg_begin=true;
          it_msg_begin= it;
          expected_msg_size=identifiers[j_id].second;
          msg_identifier=identifiers[j_id].first;
          break;
        }
      }
    }
    //FIXIT: what if a header+identifier is found before tail .
    //probably: gets kicked out of the buffer eventually.
    //but need to worry about it. may be print on screen and see if this happens?

    if (!msg_begin) continue; //keep looking for a header.

    if (msg_begin) msg.push_back(*it);

    if (*it==tail && msg.size()==expected_msg_size)
    {
      msg_end=true;
      it_msg_end=++it;//erase works on [first, last).
      break;
    }
  }


  if (!msg_begin || !msg_end) msg.clear();

  if (msg_begin && msg_end) buf.erase(it_msg_begin, it_msg_end); //erase works on [first, last).

  return msg;
}


std::vector<unsigned char> BUFFER::extract_electronics_calib_msg_in_buf()
{
  std::vector<unsigned char> msg;
  bool msg_begin = false;
  bool msg_end = false;
  boost::circular_buffer<unsigned char>::iterator it, it_msg_begin, it_msg_end;
  int expected_msg_size=9;

  for (it=buf.begin();it!=buf.end(); ++it)
  {
    if (!msg_begin)
    {
      msg.clear();
      msg_begin=true;
      it_msg_begin= it;
    }

    if (msg_begin) msg.push_back(*it);

    if (msg.size()==expected_msg_size)
    {
      msg_end=true;
      it_msg_end=++it;//erase works on [first, last).
      break;
    }
  }

  if (!msg_begin || !msg_end) msg.clear();

  if (msg_begin && msg_end) buf.erase(it_msg_begin, it_msg_end); //erase works on [first, last).

  return msg;
}
