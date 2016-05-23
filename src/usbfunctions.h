#include <iostream>
#include <iomanip>
#include <cstring>
#include <fstream> 
#include <cmath>
#include <stdlib.h>
#include <stdio.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <locale>         // std::locale, std::isspace
#include <sys/types.h>
#include <algorithm>
#include <string>
#include <cctype>
#include <stdint.h>
#include <libusb-1.0/libusb.h>
#include <libusb-1.0/libusb.h>
#include <usb.h>
//#include "functions_usb.h"
#include "matrix/Matrix.h"
#include <bitset>
#include <sstream>
#include <vector>
#include <deque>
#include "Packet.h"
#include "Level.h"
#include "Signal.h"
#include <unistd.h>
#include "special_functions_usb.h"

using namespace std;



/* Does USB bulk transfer read.  Returns number of bytes transferred
 * on success, or a negative number on error or timeout.*/
int read(libusb_device_handle *handle,unsigned char bufR[],
			unsigned int to)
{

  //  clog << " reading " << endl; fflush(stdout);

	int ok;
	int transferred;
	int end_point = 134;
	
	//	ok = libusb_bulk_transfer(handle,0x86,bufR,512,&transferred,to);
	ok = libusb_bulk_transfer(handle,end_point,bufR,512,&transferred,to);
	if (ok < 0)
		return ok;
	
	return transferred;
}

/* USB bulk transfer write.  Returns 0 on success or (negative) error 
 * code on error.*/
int write(libusb_device_handle *handle,unsigned char bufW[],int buflen,
			unsigned int to)
{
	int ok;
	int transferred;
	ok = libusb_bulk_transfer(handle,0x02,bufW,buflen,&transferred,to);
	if (ok < 0)
	{
		printf("Bulk write error %d\n", ok);
		return ok;
	}

	return transferred;
}

/* Attempts to reset all library configurations for USB device.*/
void reset(libusb_device ** devs, libusb_device_handle *handle, 
			int * firstUse, unsigned char *buf)
{
	if (libusb_reset_device(handle) < 0)
	{
		handleExit(devs,handle,firstUse,buf);
		printf("Error resetting device.\n");
	}
	if (libusb_set_configuration(handle, 1) < 0)
	{
		handleExit(devs,handle,firstUse,buf);
		printf("libusb_set_configuration error\n");
	}
	if (libusb_kernel_driver_active(handle,0) != 0)
	{
		if (libusb_detach_kernel_driver(handle,0) < 0)
		{
			handleExit(devs,handle,firstUse,buf);
			printf("Error detaching kernel\n");
		}
	}
	if (libusb_claim_interface(handle, 0) < 0)
	{
		handleExit(devs,handle,firstUse,buf);
		printf("libusb_claim_interface error\n");
	}
	if (libusb_set_interface_alt_setting(handle,0,0)<0)
	{
		handleExit(devs,handle,firstUse,buf);
		printf("libusb_set_interface_alt_setting \n");
	}
}


/* Takes in an input mxArray of numbers and returns array with
 * its valus stored in hex (unsigned char) array */
#if 0
unsigned char *getNumToHexArray(const mxArray *prs)
{
	unsigned char *mem = NULL;
	double *act = mxGetData(prs);
	if (prs != NULL)
	  //	  mem = (unsigned char*)malloc(2*prs.size());
		mem = malloc(mxGetM(prs)*2,sizeof(char));

	if (mem != NULL)	
	{
		int i = 0;
		for (i;i<mxGetM(prs);i++)
		{
			int here = act[i];
			mem[2*i] = (unsigned char) (here >> 8);
			mem[2*i+1] = (unsigned char) (here & 255);
		}
	}
	return mem;
}
#endif
/* Display possible commands on request or incorrect usage.*/
void usage()
{
	printf("Usage:  \n");
	printf("usbLib('regWrite',addr,data) where addr and data are\n");
	printf("		  input integers - ie hex2dec('ffff')\n");
	printf("usbLib('regRead',addr,NumberOfRegsToRead) moves data\n");
	printf("		  from addr to USB to be available for reading\n");
	printf("usbLib('usbRead',NumberOfTimesToRead) does a bulk \n");
	printf("		  transfer from USB to computer an int number\n");
	printf("	      of times.\n");
	printf("usbLib('reset') resets the USB device.\n");
	printf("usbLib('exit') frees memory and closes everything; \n");
	printf("		  call this function after you've finished.\n");
}





bool get_channel_number(size_t iword, size_t ichar, size_t * ichannel, char a){

  if( a=='0' ) // even if it's a channel, it's off
    return false;

  // if it's a channel, it's on
  switch(iword){
  case 0:{
    if( ichar==0 ) return false;
    if( ichar==1) return false;
    *ichannel = 33-ichar;  // 31 30 ... 18
    return true;
  }
  case 1:{
    if( ichar==0 ) return false ;
    *ichannel = 18-ichar; // 17 16 ... 3
    return true;
  }
  case 2:{
    if( ichar==0 ) return false ;
    if( ichar>3 ) return false ;
    *ichannel = 3-ichar; // 2 1 0
    return true;
  }
  default:
    return false ;
  }

  return false ;

}


bool time_stamp(size_t iword, size_t ichar){

  switch(iword){
  case 0:{
    return false;
  }
  case 1:{
    return false;
  }
  case 2:{
    if( ichar <= 3 ) return false ;
    return true;
  }
  case 3:{
    if( ichar == 0 ) return false ;
    return true;
  }
  case 4:{
    if( ichar == 0 ) return false ;
    return true;
  }
  case 5:{
    if( ichar == 0 ) return false ;
    return true;
  }
  default:
    return false ;
  }

  return false ;

}

string hexagonal_to_binary(int num_bytes, unsigned char *buf){

  string output="";
  //  clog << " hexagonal: ";
  size_t ialternate=0;
  string first,second;
  for (size_t k = 0;k<num_bytes;k++)
    {
      //      printf("%02x",buf[k]);
      
      char b[256];
      int n = sprintf(b,"%02x",buf[k]);

      if( ialternate==0){
	second=convert_hex_to_bin(b);
	ialternate=1;
      }
      else{
	first=convert_hex_to_bin(b);
	ialternate=0;
	output += first+second;
      }
      
    }

  //  clog << endl; clog << " binary: " << output << endl;

  return output;
}


bool read_packet_from_board(libusb_device_handle* handle, double timeout, Packet * packet){

  unsigned char *buf = NULL;

  buf=(unsigned char*)malloc(NUM_BYTES);
  // read buffer from usb device
  int t = read(handle,buf,(int)(timeout+0.5));
  if (t < 0){
    clog << " ========================= timeout has returned value: " << t << endl; fflush(stdout);
    free(buf);
    return false;
  }
	
  // convert hexagonal output to binary
  string packet_string=hexagonal_to_binary(NUM_BYTES, buf);
  
  // decipher packet
  *packet = Packet(packet_string);
  (*packet).packet_to_physical_parameters();
  //  (*packet).dump();
  free(buf);

  return true;

}

bool read_raw_packet_from_board(libusb_device_handle* handle, double timeout, Packet * packet){

  unsigned char *buf = NULL;

  buf=(unsigned char*)malloc(NUM_BYTES);
	
  // read buffer from usb device
  int t = read(handle,buf,(int)(timeout+0.5));
  if (t < 0){
    clog << " timeout has returned value: " << t << endl; fflush(stdout);
    free(buf);
    return false;
  }
	
  // convert hexagonal output to binary
  string packet_string=hexagonal_to_binary(NUM_BYTES, buf);
  *packet = Packet(packet_string);

  free(buf);

  return true;

}


float read_parameter_from_file(string filename, string paramname){

  FILE * pFile = fopen (filename.c_str(),"r");

  char name[256];
  float value;

  while( EOF != fscanf(pFile, "%s %e", name, &value) ){
    if( paramname.compare(name) == 0 ){
      fclose(pFile);
      return value;
    }
  }

  cout << " warning: could not find parameter " << paramname << " in file " << filename << endl;

  exit(0);

  return 0.;


}


string read_string_from_file(string filename, string paramname){

  FILE * pFile = fopen (filename.c_str(),"r");

  char name[256];
  char value[256];

  while( EOF != fscanf(pFile, "%s %s", name, value) ){
    if( paramname.compare(name) == 0 ){
      fclose(pFile);
      return value;
    }
  }

  cout << " warning: could not find parameter " << paramname << " in file " << filename << endl;

  exit(0);

  return "0";


}

string read_address_from_file(string filename, string paramname){

  FILE * pFile = fopen (filename.c_str(),"r");

  char name[256];
  float value;

  while( EOF != fscanf(pFile, "%s %e", name, &value) ){

    if( paramname.compare(name) == 0 ){
      string address;
      char c[20];
      sprintf(c,"%d",(int)value);
      string extra(c);
      if( value < 10 ){
	address="00000";
      }
      else{
	address="0000";
      }
      return address+extra;
    }
  }

  clog << " warning: could not find address " << paramname << " in file " << filename << endl;

  exit(0);

  return "";


}


string integer_to_binary(unsigned n){

  const size_t size = sizeof(n) * 8;
  char res[size];
  unsigned ind = size;
  do{
    res[--ind] = '0' + (n & 1);
  } while (n >>= 1);

  return std::string(res + ind, res + size);

}

string get_possible_board_address_from_binary(string bin){

  string address="";
  for(size_t j=0; j<(NUM_BITS_BOARD_ADDRESS-bin.size()); j++)
    address += "0";

  return address+bin;
}


string get_possible_board_address_from_decimal(size_t i){

  string extra = integer_to_binary(i);
  return get_possible_board_address_from_binary(extra);
}


vector<string> get_possible_board_addresses(int NUM_BOARDS){

  vector<string> possible_board_address;
  for(int i=0; i<NUM_BOARDS; i++){
    possible_board_address.push_back(get_possible_board_address_from_decimal(i));
  }

  return possible_board_address;
}

size_t index_of_board_address(string address){

  return (size_t)bitset<6>(address).to_ulong();

}

/*
void increase_time(Level *asticella, deque< deque<Packet> > * packets, vector<libusb_device_handle*> handles, double timeout){

  /// 1) find min and 2nd min in Level,
  ///    skip all packets of min_index that are < 2nd min time
  ///    and put the new 1st event of min_index in Level

  size_t index = asticella->minimum_index();
  //  clog << " minimum is " << setprecision(10) << asticella->minimum_time() << " will replace event " << index;
  if( index >= handles.size() ){
    clog << " error: index " << index << " handles size " << handles.size() << endl;
    exit(0);
  }
  bool read_next_packet = false;
  while( (*packets)[index][0].events().back().time("s") < asticella->next_to_minimum_time() ){
    //    clog << " last time " << (*packets)[index][0].events().back().time("s") << " of 1st packet is < 2nd min " << asticella->next_to_minimum_time() << " index " << asticella->next_to_minimum_index() << " will jump to next packet ";
    read_next_packet = true;
    if( (*packets)[index].size() == 0 ) break;
    (*packets)[index].pop_front();
    if( (*packets)[index].size() == 0 ) break;

    asticella->set_event((*packets)[index][0].events()[0],index);
    asticella->recalculate_minima();
    asticella->set_time_to_minimum();
    index = asticella->minimum_index();
  }


  // read one more packet from each board
  for(size_t i=0; i<handles.size(); i++){
    deque<Packet> old=(*packets)[index];
    Packet packet;
    read_packet_from_board(handles[i], timeout, &packet);
    old.push_back(packet);
    (*packets)[index]=old;
  }



  Event next_event;
  if( (*packets)[index][0].goto_next_event(&next_event) ){
    //    clog << " from old packet " << endl;
    //   clog << " old event: " << asticella->get_event(index).time("s") << endl;
    //   clog << " new event: " << next_event.time("s") << endl;
    asticella->set_event(next_event,index);
    asticella->recalculate_minima();
    asticella->set_time_to_minimum();
    //    clog << " done: "; asticella->dump();
    return;
  }

  Packet packet;
  read_packet_from_board(handles[index], timeout, &packet);
  asticella->set_event(packet.events()[0],index);
  asticella->recalculate_minima();
  asticella->set_time_to_minimum();

  //  clog << " done: "; asticella->dump();
  return;



}
*/

void read_pattern_file(string filename, vector <Signal> * signals){

  signals->clear();

  FILE * pFile = fopen (filename.c_str(),"r");

  int active;
  char boardWord[256];
  char board_address[256];
  char channelWord[256];
  int channelnumber;

  string board="board";
  string channel="channel";

  while( EOF != fscanf(pFile, "%d %s %s %s %d", &active, boardWord, board_address, channelWord, &channelnumber) ){

    if( board.compare(boardWord) || channel.compare(channelWord)  ){
      clog << " unrecognized pattern signal: " << boardWord << " " << channelWord << endl;
      if( board.compare(boardWord) ) clog << " board " << board << " is " << boardWord << endl;
      if( channel.compare(channelWord) ) clog << " channel " << channel << " is " << channelWord << endl;
      exit(0);
    }


    Signal s(board_address,channelnumber, (bool)active);
    signals->push_back(s);

  }



  return;
}

