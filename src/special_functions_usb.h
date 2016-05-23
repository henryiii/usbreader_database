/* -*- mode: c++ -*- */

#ifndef SPECIALFUNCTIONS_H
#define SPECIALFUNCTIONS_H

#include <iostream>
#include <iomanip>
#include <cstring>
#include <fstream> 
#include <cmath>
#include <stdlib.h>
#include <stdio.h>
#include <string>       
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <stdint.h>
#include <libusb-1.0/libusb.h>
#include <usb.h>
#include "functions_usb.h"
#include "matrix/Matrix.h"
#include <bitset>
#include <sstream>
#include <vector>
#include <deque>
#include "Packet.h"
#include "Level.h"
#include "Signal.h"
#include <unistd.h>

using namespace std;

string convert_hex_to_bin(char *s){

  //  string input(s);
  char output[256];

  int i;
  string a,b;

  //decoding might be fast with array, map, something here
  for(i=0;i<2;i++)
    {
      if( i == 1 ) a=string(output);

      switch(s[i])
	{
	case '0':
	  sprintf(output,"0000");
	  break;
	case '1':
	  sprintf(output,"0001");
	  break;
	case '2':
	  sprintf(output,"0010");
	  break;
	case '3':
	  sprintf(output,"0011");
	  break;
	case '4':
	  sprintf(output,"0100");
	  break;
	case '5':
	  sprintf(output,"0101");
	  break;
	case '6':
	  sprintf(output,"0110");
	  break;
	case '7':
	  sprintf(output,"0111");
	  break;
	case '8':
	  sprintf(output,"1000");
	  break;
	case '9':
	  sprintf(output,"1001");
	  break;
	case 'a':
	case 'A':
	  sprintf(output,"1010");
	  break;
	case 'b':
	case 'B':
	  sprintf(output,"1011");
	  break;
	case 'c':
	case 'C':
	  sprintf(output,"1100");
	  break;
	case 'd':
	case 'D':
	  sprintf(output,"1101");
	  break;
	case 'e':
	case 'E':
	  sprintf(output,"1110");
	  break;
	case 'f':
	case 'F':
	  sprintf(output,"1111");
	  break;
	default:
	  printf("Number Is Not Hexadecimal");
	  exit(0);
	}
      
    }
  

  b=string(output);

  //  clog << " input " << input << " output " << a+b << endl;

  return a+b;


}


/* Returns 0 if device is desired Cypress module, < 0 otherwise*/
int isCypress(libusb_device *dev)
{

  /* Get descriptive data for device */
  struct libusb_device_descriptor desc;
  if (libusb_get_device_descriptor(dev,&desc) < 0)
    {
      printf("Error getting USB descriptor.\n");
      return -2;
    }

  /* Check to see if what we're checking is Cypress product */
  if ((desc.idVendor == 1204) && (desc.idProduct == 4100))
    return 0;
  
  return -1;
}


/* Does USB bulk transfer read.  Returns number of bytes transferred
 * on success, or a negative number on error or timeout.*/
int read_bulk(libusb_device_handle *handle,unsigned char bufR[],
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

bool read_hex_from_board(libusb_device_handle* handle, double timeout, std::string *packet){
  //std::stringstream ss;
  unsigned char *buf = NULL;
  buf=(unsigned char*)malloc(NUM_BYTES);
  
  // read buffer from usb device
  int t = read_bulk(handle,buf,(int)(timeout+0.5));

  if (t < 0){
    clog << " ========================= timeout has returned value: " << t << endl; fflush(stdout);
    free(buf);
    return false;
  }
  char bb[1024];
  for (size_t k = 0;k<NUM_BYTES;k++){
    char b[256]; 
    int n = sprintf(b,"%02x",buf[k]); //magic here, std::cout unable to read buf[k]
    bb[2*k  ] = b[0];
    bb[2*k+1] = b[1];
    //    ss << b;
  }
  
  *packet = (string)bb;
  //*packet = ss.str();
  free(buf);
  return true;
}




string db_hex_to_binary(int num_bytes, char buf[]){
  string output="";
  size_t ialternate=0;
  string first,second;
  for (size_t k = 0;k<num_bytes;k++)
    {
      char b[2] = {(char)buf[2*k],(char)buf[2*k+1]};

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
  

  return output;
}


/* Frees memory and closes out library upon error or exit.*/
void handleExit(libusb_device ** devs,libusb_device_handle *handle, 
			int *firstUse, unsigned char *buf)
{
	if (handle != NULL)
	{
		libusb_release_interface(handle,0);
		libusb_close(handle);
	}
	if (devs != NULL)
		libusb_free_device_list(devs, 1);
	*firstUse = 0;
	free(buf);
	libusb_exit(NULL);
}

static vector<libusb_device_handle*> retrieve_usb_devices(){

  unsigned char *buf = NULL;
  static libusb_device **devs = NULL;

  /* The handle is what we'll actually use to open usb for R/W*/
  static libusb_device_handle *handle = NULL;
  static vector<libusb_device_handle*> handles;
  
  /* Initialize USB device if first call to this mex file*/
  static int firstUse = 0;
  if (firstUse == 0)
    {
      ssize_t cnt;
      
      /* Initializes libusb library and gets list of usb devices */
      if (libusb_init(NULL) < 0)
	printf("Unable to initialize library");
      clog << " initialized library " << endl; fflush(stdout);
      
      cnt = libusb_get_device_list(NULL, &devs);
      if (cnt < 0)
	printf("Unable to get device list");
      
      //      clog << " there are " << cnt << " usb devices " << endl; fflush(stdout);
      

      /* Go through all USB devices and open the correct one*/
      libusb_device *dev;
      int i = 0;
      while ((dev = devs[i++]) != NULL) 
	{
	  //	  clog << " ... device " << i ; fflush(stdout);
	  /* If it's cypress device, open and break out of loop*/
	  if (isCypress(dev) == 0)
	    {
	      //	      clog << " is cypress " << endl; fflush(stdout);
	      /* Open USB for reading*/
	      
	      if (libusb_open(dev,&handle) < 0)
		{
		  handleExit(devs,handle,&firstUse,buf);
		  printf("Unable to open USB");
		}
	      //	      clog << " opened device" << endl; fflush(stdout);
	      //	      if( handles.size() < NUM_BOARDS )
		handles.push_back(handle);
	      
	      //	     reset(devs,handle,&firstUse,buf);
	      //	      break;
	    }
	}
      

      /* If it got through loop, it couldn't detect correct device.*/
      if (handles.size() == 0)
	{
	  handleExit(devs,handle,&firstUse,buf);
	  printf("Cannot find Cypress device. \n");
	  return handles;
	}
      
      /* Note that static stays in memory between mex calls */
      firstUse++;
    }

  return handles;

}

#endif
