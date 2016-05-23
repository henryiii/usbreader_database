/* -*- mode: c++ -*- */
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
//#include "functions_usb.h"
#include "usbfunctions.h"
//#include "matrix/Matrix.h"
#include <bitset>
#include <sstream>
#include <vector>
#include <list>
#include <deque>
#include <unistd.h>
#include "Word.h"
#include "Event.h"
#include "Packet.h"
#include "Level.h"
#include "pthread.h"
#include <boost/shared_ptr.hpp>


#define NUM_FEBS 1

#define PACKET_NUM_BYTES 512 // n of bytes in a packet
#define PACKET_NUM_BITS 4096 // 8*PACKET_NUM_BYTES; // n of bits in a packe
#define PACKET_OFFSET_TO_PNUM 10 // ???

static libusb_context *ctx = 0;
static struct libusb_transfer *det_transfer;
static struct libusb_device_handle *devh[NUM_FEBS];
static unsigned char detbuf[NUM_FEBS][PACKET_NUM_BITS];

static pthread_t poll_thread;
static pthread_t process_thread;
static pthread_attr_t poll_thread_attr;
static pthread_attr_t process_thread_attr;
static pthread_cond_t exit_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t exit_cond_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t data_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t pcount_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t data_cond = PTHREAD_COND_INITIALIZER;

static uint64_t cutoffTS;
static uint64_t lastTimestamp[NUM_FEBS];
static uint64_t lastPacketNum[NUM_FEBS];
static uint64_t packetsReceived[NUM_FEBS];//shared b/t poll & process threads 
static uint64_t packetsDropped[NUM_FEBS]; //shared b/t poll & process threads 
static const uint64_t MAX_PACKET_NUM = 0xFFFFFFFFFFFFull;

int completed = 0; 
const int CYPRESS_VENDOR_ID = 1204;
const int CYPRESS_PRODUCT_ID = 4100;
const uint32_t  TIMEOUT = 1000;
const uint32_t NBYTES = 512;


struct RawHit
{
  unsigned char febNum;
  uint32_t hitVector;
  uint64_t ts;
};

typedef std::list<RawHit> RawHitList;
typedef boost::shared_ptr<RawHitList> RawHitListPtr;

static RawHitList hits;
static RawHitList febHits;
static std::list<RawHitListPtr> hitsQueue; //shared b/t poll & process threads
static std::list<uint64_t> hitsCutoffQueue;//shared b/t poll & process threads 
static std::list<RawHitListPtr> internalHitsQueue;
static std::list<uint64_t> internalHitsCutoffQueue;


// void cback(struct libusb_transfer *transfer){
//   std::clog << "In cback, found" << std::endl;
//   std::clog << "actual_length: " << transfer->actual_length << std::endl;
//   int *completed = transfer->user_data;
//   completed = 1;
//   int *p; 
//   p=(int*)transfer->user_data; 
//   *p=1; //  set completed 
//   std::clog << "set p" << std::endl;
//   //this part of the code should actually do something important, maybe IO? thread locking and unlocking like UT_MUON
// }


static void cb_det(struct libusb_transfer *transfer)
{
  if (transfer->status != LIBUSB_TRANSFER_COMPLETED)
    {
      std::clog << "cb_det: incomplete packet..." << std::endl;
      if(libusb_submit_transfer(transfer) < 0)
	{
	  std::clog << "inside callback... couldn't sumbit_transfer (incomplete)" << std::endl;
	  exit(0);
	}

      return;
    }

  const int ppos = PACKET_OFFSET_TO_PNUM;

  uint64_t packetNum = 0;
  *((unsigned char *)&packetNum) =  *(transfer->buffer+ppos+4);
  *(((unsigned char *)&packetNum)+1) = *(transfer->buffer+ppos+5);
  *(((unsigned char *)&packetNum)+2) = *(transfer->buffer+ppos+2);
  *(((unsigned char *)&packetNum)+3) = *(transfer->buffer+ppos+3);
  *(((unsigned char *)&packetNum)+4) = *(transfer->buffer+ppos);
  *(((unsigned char *)&packetNum)+5) = *(transfer->buffer+ppos+1);

  uint16_t feb = 0;
  memcpy(&feb, transfer->buffer+ppos+6, 2);

  pthread_mutex_lock(&pcount_lock);

  packetsReceived[feb] += 1;
  //std::clog << feb << ":" << lastPacketNum[feb] << ":" << packetNum;                                                                        
  if(packetsReceived[feb] > 10) //skip the first few packets, which may be stale from previous run                                            
    {
      uint64_t numDropped = 0;
      if(packetNum >= lastPacketNum[feb])
	{
	  numDropped = (packetNum - lastPacketNum[feb]) - 1;
	}
      else
	{
	  //don't forget the packetNum = 0                                                                                                        
	  numDropped = ((MAX_PACKET_NUM - lastPacketNum[feb]) + packetNum);
	}

      packetsDropped[feb] += numDropped;

      //std::clog << ":" << numDropped;                                                                                                         
    }
  else
    {
      //reset hit list until all febs have dropped the first few packets                                                                        
      hits.erase(hits.begin(), hits.end());
      lastPacketNum[feb] = packetNum;
      pthread_mutex_unlock(&pcount_lock);
      if(libusb_submit_transfer(transfer) < 0)
	{
	  std::clog << "inside callback... couldn't submit_transfer (stale)\n";
	  exit(0);
	}

      return;
    }

  pthread_mutex_unlock(&pcount_lock);

  //std::clog << std::endl;                                                                                                                   

  /*                                                                                                                                         
  if((packetsReceived[feb] % 100) == 0)                                                                                                       
  {                                                                                                                                           
  std::clog << feb << ":" << packetsReceived[feb]                                                                                           
  << "/" << packetsDropped[feb] << std::endl;                                                                                     
  }                                                                                                                                           
  */

  lastPacketNum[feb] = packetNum;

  uint16_t *ptr=0, *hitPtr=0;
  uint32_t pos=0;
  uint64_t maxTS = 0;

  for(RawHitList::iterator it = febHits.begin(); it != febHits.end(); it++)
    {
      ptr = (uint16_t *)((transfer->buffer)+pos);

      if((!(ptr[0] & 0x8000)) || (ptr[0] & 0x4000) ||
	 (ptr[1] & 0x8000) || (ptr[2] & 0x8000) || (ptr[3] & 0x8000)
	 || (ptr[4] & 0x8000) || (ptr[5] & 0x8000))
	{
	  std::cerr << "Processor: Error: Bad alignment on device "
		    << feb << " " << "Position: " << std::dec << pos%512 << std::endl;
	}

      hitPtr = (uint16_t *)(&(it->hitVector));
      hitPtr[1] = (ptr[0] << 2) | ((ptr[1] & 0x6000) >> 13);
      hitPtr[0] = (ptr[1] << 3) | ((ptr[2] & 0x7000) >> 12);
      hitPtr = (uint16_t *)(&(it->ts));
      hitPtr[0] = ptr[5]  | (ptr[4] << 15);
      hitPtr[1] = (ptr[4] >> 1) | (ptr[3] << 14);
      hitPtr[2] = (ptr[3] >> 2) | (ptr[2] << 13);
      hitPtr[3] = (ptr[2] & 0x0fff) >> 3;

      it->ts += tCorr.GetTSCorrection(32*feb + 1);


      if(it->ts > maxTS)
	maxTS = it->ts;

      it->febNum = feb;
      pos += EVENT_SIZE;
    }

  lastTimestamp[feb] = maxTS;
  hits.insert(hits.end(), febHits.begin(), febHits.end());

  if(!collectionIntervalFinished)
    {
      if(hits.size() >= 2000)
	{
	  collectionIntervalFinished = true;
	  cutoffTS = lastTimestamp[0];
	  for(int ii=0; ii < NUM_FEBS; ++ii)
	    {
	      if(lastTimestamp[ii] < cutoffTS)
		cutoffTS = lastTimestamp[ii];
	    }

	  //starting with the next callback we will begin to check whether
	  //every feb has reported a timestamp beyond the cutoff
	}
    }
  else
    {
      //now we have to make sure all febs have reported a ts > cutoff before 
      //we queue the hits 

      bool readyToProcess = true;
      if(((maxTS - cutoffTS) / clkrt) < 5)
	{
	  for(int ii=0; ii < NUM_FEBS; ++ii)
	    {
	      if(lastTimestamp[ii] < (cutoffTS+1))
		{
		  readyToProcess = false;
		  break;
		}
	    }
	}
      else
	std::clog << "forcing cutoff... " << std::endl;

      if(readyToProcess)
	{
	  RawHitListPtr plist(new RawHitList);
	  plist->splice(plist->end(), hits, hits.begin(), hits.end());

	  //try to get data_lock.  if unsuccessful, push into internalQueue,
	  //and move all elements of internalQueue to hitsQueue next time we
	  //get the lock (polling thread shouldn't wait for process thread)...
	  if(pthread_mutex_trylock(&data_lock) == 0)
	    {
	      if(hitsQueue.size() == 0)
		pthread_cond_signal(&data_cond);

	      if(internalHitsQueue.size() > 0)
		{
		  hitsQueue.splice(hitsQueue.end(),
				   internalHitsQueue,
				   internalHitsQueue.begin(),
				   internalHitsQueue.end());

		  hitsCutoffQueue.splice(hitsCutoffQueue.end(),
					 internalHitsCutoffQueue,
					 internalHitsCutoffQueue.begin(),
					 internalHitsCutoffQueue.end());
		}

	      hitsQueue.push_back(plist);
	      hitsCutoffQueue.push_back(cutoffTS);

	      pthread_mutex_unlock(&data_lock);
	    }
	  else
	    {
	      internalHitsQueue.push_back(plist);
	      internalHitsCutoffQueue.push_back(cutoffTS);
	    }

	  collectionIntervalFinished = false;
	}
    }

  if(libusb_submit_transfer(transfer) < 0)
    {
      std::clog << "inside callback... couldn't submit_transfer" << std::endl;
      exit(0);
    }
}



static int alloc_transfers(void)
{
  for(int ii=0; ii < NUM_FEBS; ++ii)
    {
      det_transfer[ii] = libusb_alloc_transfer(0);
      if (!det_transfer[ii])
	return -ENOMEM;
      
      det_transfer[ii]->flags = 0;
      det_transfer[ii]->type = LIBUSB_TRANSFER_TYPE_BULK;
      det_transfer[ii]->buffer = detbuf[ii];
      //det_transfer[ii]->length = PACKET_NUM_BITS
      det_transfer[ii]->length = PACKET_NUM_BYTES;
      det_transfer[ii]->user_data = NULL;
      det_transfer[ii]->timeout = 0;
      det_transfer[ii]->num_iso_packets = 0;
      det_transfer[ii]->endpoint = 0x86;
      det_transfer[ii]->callback = &cb_det; //careful
      det_transfer[ii]->dev_handle = devh[ii];
    }

  return 0;
}




static int find_detector_devices(libusb_context *ctx)
{
  struct libusb_device **devs;
  struct libusb_device *found = NULL;
  struct libusb_device *dev;
  struct libusb_device_handle *handle = NULL;

  size_t i = 0;
  int r;

  if(libusb_get_device_list(ctx, &devs) < 0)
    {
      std::clog << "couldn't get device list..." << std::endl;
      return(-1);
    }

  int detDevCount = 0;
  while((dev = devs[i++]) != NULL)
    {
      struct libusb_device_descriptor desc;
      r = libusb_get_device_descriptor(dev, &desc);
      if(r < 0)
	continue;
      //0x04B4 is HEX 1204
      //0x1004 is HEX 4100
      if((desc.idVendor == 0x04B4) && (desc.idProduct == 0x1004))
	{
	  //	  r = libusb_open(dev, &devh[detDevCount]); //if you want multiple handles
	  r = libusb_open(dev, &devh);
	  if(r < 0)
	    {
	      std::clog << "libusb_open failed..." << std::endl;
	      goto out;
	    }
	  
	  //	  r = libusb_claim_interface(devh[detDevCount], 0); //if you have multiple handles
	  r = libusb_claim_interface(devh, 0);
	  if(r < 0)
	    {
	      std::clog << "libusb_claim_interface failed..." << std::endl;
	      goto out;
	    }

	  std::clog << "claimed detector usb interface" << std::endl;
	  detDevCount++;
	}
    }
  std::clog << "found detDevCount: " << detDevCount << "\n";

 out:
  libusb_free_device_list(devs, 1);
  
  return((detDevCount == NUM_FEBS) ? 0 : -1);
}



int main(){

  
  //setup pthread vars.
  
  struct sigaction sigact;  
  
  sigact.sa_handler = sighandler;
  sigemptyset(&sigact.sa_mask);
  sigact.sa_flags = 0;
  sigaction(SIGINT, &sigact, NULL);
  sigaction(SIGTERM, &sigact, NULL);
  sigaction(SIGQUIT, &sigact, NULL);

  
  pthread_attr_init(&poll_thread_attr);
  pthread_attr_init(&process_thread_attr);

  //async libusb
  
  r = libusb_init(&ctx);
  r = find_detector_devices(ctx);



  return 0;
}
