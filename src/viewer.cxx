#include <iostream>
#include <string>
#include <vector>
#include <deque>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <float.h>
#include <stdio.h>
#include <algorithm>
#include <ios>
#include "Riostream.h"
#include "TString.h"
#include "TStyle.h"
#include "TStyle.h"
#include "TCanvas.h"
#include "sstream"
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "TFile.h"
#include "TTree.h"
#include <math.h>
#include <TROOT.h>
#include <TChain.h>
#include "TH2.h"
#include "TH1.h"
#include "TPaveLabel.h"
#include "TPaveText.h"
#include "TSystem.h"
#include "TInterpreter.h"
#include <TObjArray.h>
#include "TPaveLabel.h"
#include "TPaveText.h"
#include <time.h>

using namespace std;

float read_parameter_from_file(string filename, string paramname);
string get_file_number(size_t i);
string int_to_string(int i);
string double_to_string(double val);
void wait ( double seconds );
void read_detector(string filename, vector<size_t> * expected_channels, vector<size_t> * expected_channels_x1, vector<size_t> * expected_channels_y1,
		   vector<size_t> * expected_channels_x2, vector<size_t> * expected_channels_y2,
		   vector<size_t> * expected_channels_x3, vector<size_t> * expected_channels_y3,
		   vector<size_t> * expected_channels_x4, vector<size_t> * expected_channels_y4,
		   vector<size_t> * expected_channels_x5, vector<size_t> * expected_channels_y5,
		   vector<size_t> * expected_channels_x6, vector<size_t> * expected_channels_y6,
		   vector<size_t> * expected_channels_x7, vector<size_t> * expected_channels_y7,
		   vector<size_t> * expected_channels_x8, vector<size_t> * expected_channels_y8);
bool is_lockfile_running(string asker, string commandname);
bool is_program_running(string asker, string commandname);
size_t get_n_layers(vector<size_t> expected_channels_1, vector<size_t> expected_channels_2, vector<size_t> expected_channels_3, vector<size_t> expected_channels_4, vector<size_t> expected_channels_5, vector<size_t> expected_channels_6, vector<size_t> expected_channels_7, vector<size_t> expected_channels_8);



int main() {

  // define bunch of parameters
  // (n channels) * (n boards) = 32 * 4 = 128
  //  const int LARGE_NUMBER=128;
  int _nsignals;
  int _nchannels_per_event[1000];
  int _event_number[1000];
  double _time[1000];
  int _channels[1000][128];
  bool print = (bool)(read_parameter_from_file("params.inp","print")); // print verbose
  size_t filenumber=0;
  TFile *input;
  int modulo=100000;
  string sfilenumber, fname;

  // load detector shape
  vector<size_t> expected_channels;
  vector<size_t> expected_channels_x1;
  vector<size_t> expected_channels_y1;
  vector<size_t> expected_channels_x2;
  vector<size_t> expected_channels_y2;
  vector<size_t> expected_channels_x3;
  vector<size_t> expected_channels_y3;
  vector<size_t> expected_channels_x4;
  vector<size_t> expected_channels_y4;
  vector<size_t> expected_channels_x5;
  vector<size_t> expected_channels_y5;
  vector<size_t> expected_channels_x6;
  vector<size_t> expected_channels_y6;
  vector<size_t> expected_channels_x7;
  vector<size_t> expected_channels_y7;
  vector<size_t> expected_channels_x8;
  vector<size_t> expected_channels_y8;
  read_detector("detector.inp",&expected_channels, &expected_channels_x1, &expected_channels_y1, 
		&expected_channels_x2, &expected_channels_y2, 
		&expected_channels_x3, &expected_channels_y3, 
		&expected_channels_x4, &expected_channels_y4, 
		&expected_channels_x5, &expected_channels_y5, 
		&expected_channels_x6, &expected_channels_y6, 
		&expected_channels_x7, &expected_channels_y7, 


		&expected_channels_x8, &expected_channels_y8);

  size_t detxmin=*min_element(expected_channels_x1.begin(), expected_channels_x1.end());
  size_t detxmin2=*min_element(expected_channels_x2.begin(), expected_channels_x2.end());
  detxmin=std::min(detxmin,detxmin2);
  detxmin2=*min_element(expected_channels_x3.begin(), expected_channels_x3.end());
  detxmin=std::min(detxmin,detxmin2);
  detxmin2=*min_element(expected_channels_x4.begin(), expected_channels_x4.end());
  detxmin=std::min(detxmin,detxmin2);
  detxmin2=*min_element(expected_channels_x5.begin(), expected_channels_x5.end());
  detxmin=std::min(detxmin,detxmin2);
  detxmin2=*min_element(expected_channels_x6.begin(), expected_channels_x6.end());
  detxmin=std::min(detxmin,detxmin2);
  detxmin2=*min_element(expected_channels_x7.begin(), expected_channels_x7.end());
  detxmin=std::min(detxmin,detxmin2);
  detxmin2=*min_element(expected_channels_x8.begin(), expected_channels_x8.end());

  size_t detxmax=*max_element(expected_channels_x1.begin(), expected_channels_x1.end());
  size_t detxmax2=*max_element(expected_channels_x2.begin(), expected_channels_x2.end());
  detxmax=std::max(detxmax,detxmax2);
  detxmax2=*max_element(expected_channels_x3.begin(), expected_channels_x3.end());
  detxmax=std::max(detxmax,detxmax2);
  detxmax2=*max_element(expected_channels_x4.begin(), expected_channels_x4.end());
  detxmax=std::max(detxmax,detxmax2);
  detxmax2=*max_element(expected_channels_x5.begin(), expected_channels_x5.end());
  detxmax=std::max(detxmax,detxmax2);
  detxmax2=*max_element(expected_channels_x6.begin(), expected_channels_x6.end());
  detxmax=std::max(detxmax,detxmax2);
  detxmax2=*max_element(expected_channels_x7.begin(), expected_channels_x7.end());
  detxmax=std::max(detxmax,detxmax2);
  detxmax2=*max_element(expected_channels_x8.begin(), expected_channels_x8.end());
  
  size_t detymin=*min_element(expected_channels_y1.begin(), expected_channels_y1.end());
  size_t detymin2=*min_element(expected_channels_y2.begin(), expected_channels_y2.end());
  detymin=std::min(detymin,detymin2);
  detymin2=*min_element(expected_channels_y3.begin(), expected_channels_y3.end());
  detymin=std::min(detymin,detymin2);
  detymin2=*min_element(expected_channels_y4.begin(), expected_channels_y4.end());
  detymin=std::min(detymin,detymin2);
  detymin2=*min_element(expected_channels_y5.begin(), expected_channels_y5.end());
  detymin=std::min(detymin,detymin2);
  detymin2=*min_element(expected_channels_y6.begin(), expected_channels_y6.end());
  detymin=std::min(detymin,detymin2);
  detymin2=*min_element(expected_channels_y7.begin(), expected_channels_y7.end());
  detymin=std::min(detymin,detymin2);
  detymin2=*min_element(expected_channels_y8.begin(), expected_channels_y8.end());
    
  size_t detymax=*max_element(expected_channels_y1.begin(), expected_channels_y1.end());
  size_t detymax2=*max_element(expected_channels_y2.begin(), expected_channels_y2.end());
  detymax=std::max(detymax,detymax2);
  detymax2=*max_element(expected_channels_y3.begin(), expected_channels_y3.end());
  detymax=std::max(detymax,detymax2);
  detymax2=*max_element(expected_channels_y4.begin(), expected_channels_y4.end());
  detymax=std::max(detymax,detymax2);
  detymax2=*max_element(expected_channels_y5.begin(), expected_channels_y5.end());
  detymax=std::max(detymax,detymax2);
  detymax2=*max_element(expected_channels_y6.begin(), expected_channels_y6.end());
  detymax=std::max(detymax,detymax2);
  detymax2=*max_element(expected_channels_y7.begin(), expected_channels_y7.end());
  detymax=std::max(detymax,detymax2);
  detxmax2=*max_element(expected_channels_x8.begin(), expected_channels_x8.end());

  size_t nlayers_x = get_n_layers(expected_channels_x1, expected_channels_x2, expected_channels_x3, expected_channels_x4, expected_channels_x5, expected_channels_x6, expected_channels_x7, expected_channels_x8);
   size_t nlayers_y = get_n_layers(expected_channels_y1, expected_channels_y2, expected_channels_y3, expected_channels_y4, expected_channels_y5, expected_channels_y6, expected_channels_y7, expected_channels_y8);

  // define root structures
  //gROOT->Reset(); this command does not seem to work on this system...
  gROOT->SetStyle("Plain");
  gStyle->SetPalette(1,0);
  gStyle->SetOptStat(0);
  gStyle->SetOptTitle(0);

  TCanvas c1("c1","canvas",0,0,1200,1000);
  c1.Divide(2);
  int ndigits = 20;
  double pave_xmin = 0.2;
  double pave_xmax = 0.8; 
  double pave_ymin = 0.8;
  double pave_ymax = 0.9;

  TPaveText paveTop(pave_xmin, pave_ymin, pave_xmax, pave_ymax, "NDC");
  paveTop.SetFillColor(4100); // text is black on white
  paveTop.SetTextSize(0.05);
  paveTop.SetTextAlign(12);

  TFile *output = new TFile("data.root","RECREATE");

  TH2F *detector = new TH2F("detector","detector",100,-0.5,(double)nlayers_x+1.5,100,-0.5,(double)nlayers_y+1.5);

  detector->SetMarkerStyle(2);
  detector->SetMarkerSize(3);
  detector->SetMarkerColor(kRed);

  TH2F *detector_accumulate = new TH2F("detector_accumulate","detector_accumulate",nlayers_x+2,-0.5,(double)nlayers_x+1.5,(double)nlayers_y+2,-0.5,(double)nlayers_y+1.5);
  detector_accumulate->SetMarkerStyle(2);
  detector_accumulate->SetMarkerSize(3);
  detector_accumulate->SetMarkerColor(kBlack);

  TObjArray *horlines = new TObjArray(nlayers_y);
  TObjArray *verlines = new TObjArray(nlayers_x);

  for(size_t i=0; i<nlayers_y; i++){
    TLine *l = new TLine(detxmin-0.5,-0.5+i,detxmax+0.5,-0.5+i);
    l->SetLineColor(kBlue);
    l->SetLineWidth(2);
    horlines->Add(l);
  }
  for(size_t i=0; i<nlayers_x; i++){
    TLine *l = new TLine(0.5+i,detymin-0.5,0.5+i,detymax+0.5);
    l->SetLineColor(kBlue);
    l->SetLineWidth(2);
    verlines->Add(l);
  }


  TChain chain("data_tree");


  // loop on input root files
  while(1){

    // if input file is not there, stay in eternal loop
    // must quit with ctrl+c
    sfilenumber=get_file_number(filenumber);
    fname = "coincidence_data_"+sfilenumber+".root";
    if( gSystem->AccessPathName( fname.c_str() ) ) {
    
      string asker="viewer";
      string commandname="./select";
      clog << asker << " needs file " << fname << " but it is not there yet, check if " << commandname << " is running " << endl;

      bool running=true;
      bool good;
      while(running){
	running=is_lockfile_running(asker.c_str(), "converter");
	  //	running=is_program_running(asker.c_str(), commandname.c_str());
	good = !gSystem->AccessPathName( fname.c_str() ) ;
	if( good ) break;
      }
      
      if( !running ){
	clog << asker << " will stop " << endl;
	break;
      }

      clog << asker << " will continue " << endl;
    }
  
  
    // read new input root file
    sfilenumber=get_file_number(filenumber);
    fname = "coincidence_data_"+sfilenumber+".root";
    input = new TFile(fname.c_str(),"READ");
    if( input->IsZombie() ){
      clog << " viewer needs file " << fname << " but it's a zombie, quitting " << endl;
      break;
    }
    clog << " viewer adding file " << fname << endl;


    // add file to chain
    chain.Add(fname.c_str());

    // read tree from file
    TTree *data_tree = (TTree*)(input->Get("data_tree"));
    data_tree->SetBranchAddress("nsignals",&_nsignals);
    data_tree->SetBranchAddress("nchannels_per_event",_nchannels_per_event);
    data_tree->SetBranchAddress("event_number",_event_number);
    data_tree->SetBranchAddress("time",_time);
    data_tree->SetBranchAddress("channels",_channels);

    TBranch *b_nsignals = data_tree->GetBranch("nsignals");
    TBranch *b_nchannels_per_event = data_tree->GetBranch("nchannels_per_event");
    TBranch *b_event_number = data_tree->GetBranch("event_number");
    TBranch *b_time = data_tree->GetBranch("time");
    TBranch *b_channels = data_tree->GetBranch("channels");

    // int the_event_number = -1;
    // loop on tree events
    for ( int ient = 0; ient < data_tree-> GetEntries(); ient++ ) {

      // read event
      b_nsignals->GetEvent(ient);
      b_nchannels_per_event->GetEvent(ient);
      b_event_number->GetEvent(ient);
      b_time->GetEvent(ient);
      b_channels->GetEvent(ient);
      
      if( print ){
	clog << " " << endl;
	clog << " reading to tree: nsignals " << _nsignals;
	for(size_t jj=0; jj<_nsignals; jj++){
	  //for(size_t jj=0; jj<100; jj++){
	  clog << " signal " << jj << " nchannels " << _nchannels_per_event[jj]  << " _event_number " << _event_number[jj]  << "  _time " << _time[jj]  << std::endl;
	  for(size_t kk=0; kk<_nchannels_per_event[jj]; kk++){
	    //for(size_t kk=0; kk<128; kk++){
	    std::clog << " channel " << kk  << " _channels " << _channels[jj][kk] << std::endl;
	  }
	}
      }
      
      // display event
      if( _nsignals != 0 ){
      	double tzero = _time[0];
	//the_event_number ++;
	// draw hits

	for(size_t is=0; is<_nsignals; is++){
	  if( _time[is] < tzero ) tzero = _time[is];
	  for(size_t ich=0; ich<_nchannels_per_event[is]; ich++){
	    vector<size_t>::iterator ic = std::find(expected_channels.begin(), expected_channels.end(), _channels[is][ich]);
	    if( ic != expected_channels.end() ){
	      size_t ind = ic - expected_channels.begin();

	      detector->Fill( (double)expected_channels_x1[ind],  (double)expected_channels_y1[ind]);
	      detector->Fill( (double)expected_channels_x2[ind],  (double)expected_channels_y2[ind]);
	      detector->Fill( (double)expected_channels_x3[ind],  (double)expected_channels_y3[ind]);
	      detector->Fill( (double)expected_channels_x4[ind],  (double)expected_channels_y4[ind]);
	      detector->Fill( (double)expected_channels_x5[ind],  (double)expected_channels_y5[ind]);
	      detector->Fill( (double)expected_channels_x6[ind],  (double)expected_channels_y6[ind]);
	      detector->Fill( (double)expected_channels_x7[ind],  (double)expected_channels_y7[ind]);
	      detector->Fill( (double)expected_channels_x8[ind],  (double)expected_channels_y8[ind]);
	      detector_accumulate->Fill( (double)expected_channels_x1[ind],  (double)expected_channels_y1[ind]);
	      detector_accumulate->Fill( (double)expected_channels_x2[ind],  (double)expected_channels_y2[ind]);
	      detector_accumulate->Fill( (double)expected_channels_x3[ind],  (double)expected_channels_y3[ind]);
	      detector_accumulate->Fill( (double)expected_channels_x4[ind],  (double)expected_channels_y4[ind]);
	      detector_accumulate->Fill( (double)expected_channels_x5[ind],  (double)expected_channels_y5[ind]);
	      detector_accumulate->Fill( (double)expected_channels_x6[ind],  (double)expected_channels_y6[ind]);
	      detector_accumulate->Fill( (double)expected_channels_x7[ind],  (double)expected_channels_y7[ind]);
	      detector_accumulate->Fill( (double)expected_channels_x8[ind],  (double)expected_channels_y8[ind]);
	    }
	  }
	}

	c1.cd(1);
	detector->Draw();
	// draw detector
	horlines->Draw("same");
	verlines->Draw("same");
      
      	// draw text
	string textev="event "+int_to_string(_event_number[0]);
	string textevname="event__"+int_to_string(_event_number[0]);
	//string textev="event "+int_to_string(the_event_number);
	string texttzero="t = "+double_to_string(tzero)+" s";
	paveTop.AddText(textev.c_str());
	paveTop.AddText(texttzero.c_str());
	paveTop.Draw("same");
	c1.cd(2);
	//detector_accumulate->Draw("colz,text");
	//detector_accumulate->Draw("colz"); this and the two lines below were commented out to disable event image production
	//horlines->Draw("same");
	//verlines->Draw("same");
	
	c1.Update();
      
	//	wait(0.1);

	//to disable production of event images; uncomment to enable c1.Print(("events/"+textevname+".jpg").c_str());
	detector->Clear();
	detector->Reset();
	paveTop.Clear();
      } // drawn event

    } // finish looping on tree

    delete input;
    filenumber ++;
    
    // write chain into output file
    chain.Merge(output->GetName());
    
  } // finish waiting for files
  
  c1.Divide(1);
  c1.cd();
  //detector_accumulate->Draw("colz,text"); don't uncomment, already commented out
  //detector_accumulate->Draw("colz"); this and the Draw() lines below were commented to disable image productin
  //horlines->Draw("same");
  //verlines->Draw("same");
  c1.Update();
  //to disable production of event images; uncomment to enable c1.Print("detector.eps");
  clog << " viewer finished " << endl; fflush(stdout);

  //  clog << " viewer free " << endl; fflush(stdout);
  exit(0);


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

  clog << " warning: could not find parameter " << paramname << " in file " << filename << endl;
  fclose(pFile);

  exit(0);

  return 0.;

}


string get_file_number(size_t i){
  
  size_t limit = 6;
  string numb=int_to_string((int)i);
  if( numb.size() > limit ){
    clog << " problem: file number " << i << " exceeds limit " << endl;
    exit(0);
  }
  size_t n_extra_zeros=6-numb.size();
  string output="";
  for(size_t i=0; i<n_extra_zeros; i++)
    output += '0';
  return output+numb;
  
}

  string int_to_string(int i){
    char c[20];
    sprintf(c,"%d",i);
    string sval(c);
    return sval;
  }


string double_to_string(double val){
  char c[20];
  sprintf(c,"%.9g",val);
  string sval(c);
  return sval;
}

  void wait ( double seconds )
  {
    clock_t endwait;
    endwait = clock () + seconds*1e6;
    while (clock() < endwait) {}
  }

void read_detector(string filename, vector<size_t> * expected_channels, vector<size_t> * expected_channels_x1, vector<size_t> * expected_channels_y1,
		   vector<size_t> * expected_channels_x2, vector<size_t> * expected_channels_y2,
		   vector<size_t> * expected_channels_x3, vector<size_t> * expected_channels_y3,
		   vector<size_t> * expected_channels_x4, vector<size_t> * expected_channels_y4,
		   vector<size_t> * expected_channels_x5, vector<size_t> * expected_channels_y5,
		   vector<size_t> * expected_channels_x6, vector<size_t> * expected_channels_y6,
		   vector<size_t> * expected_channels_x7, vector<size_t> * expected_channels_y7,
		   vector<size_t> * expected_channels_x8, vector<size_t> * expected_channels_y8){

  FILE * pFile = fopen (filename.c_str(),"r");

  int c;
  int x1, x2, x3, x4, x5, x6, x7, x8;
  int y1, y2, y3, y4, y5, y6, y7, y8;

  while( EOF != fscanf(pFile, "%d %d %d %d %d %d %d %d %d", &c, &x1, &y1, &x2, &y2, &x3, &y3, &x4, &y4) && EOF != fscanf(pFile, "%d %d %d %d %d %d %d %d", &x5, &y5, &x6, &y6, &x7, &y7, &x8, &y8))
 {

    expected_channels->push_back(c);
    expected_channels_x1->push_back((size_t)x1);
    expected_channels_y1->push_back((size_t)y1);
    expected_channels_x2->push_back((size_t)x2);
    expected_channels_y2->push_back((size_t)y2);
    expected_channels_x3->push_back((size_t)x3);   
    expected_channels_y3->push_back((size_t)y3);   
    expected_channels_x4->push_back((size_t)x4);   
    expected_channels_y4->push_back((size_t)y4);   
    expected_channels_x5->push_back((size_t)x5);
    expected_channels_y5->push_back((size_t)y5);
    expected_channels_x6->push_back((size_t)x6);
    expected_channels_y6->push_back((size_t)y6);
    expected_channels_x7->push_back((size_t)x7);
    expected_channels_y7->push_back((size_t)y7);
    expected_channels_x8->push_back((size_t)x8);
    expected_channels_y8->push_back((size_t)y8);

    std::clog << " x1 " << x1 << " x2 " << x2 << " x3 " << x3 << " y1 " << y1 << " y2 " << y2 << " y3 " << y3 << std::endl;

  }

  fclose(pFile);

  if( expected_channels->size() != expected_channels_x1->size() ||
      expected_channels->size() != expected_channels_y1->size() ||
      expected_channels->size() != expected_channels_x2->size() ||
      expected_channels->size() != expected_channels_y2->size() ||
      expected_channels->size() != expected_channels_x3->size() ||
      expected_channels->size() != expected_channels_y3->size() ||
      expected_channels->size() != expected_channels_x4->size() ||
      expected_channels->size() != expected_channels_y4->size() ||
      expected_channels->size() != expected_channels_x5->size() ||
      expected_channels->size() != expected_channels_y5->size() ||
      expected_channels->size() != expected_channels_x6->size() ||
      expected_channels->size() != expected_channels_x7->size() ||
      expected_channels->size() != expected_channels_y7->size() ||
      expected_channels->size() != expected_channels_x8->size() ||
      expected_channels->size() != expected_channels_y8->size()
 ){
    cout << " problem: detector sizes " << expected_channels->size() << " " << expected_channels_x1->size() << " " << expected_channels_y2->size() << 
      " " << expected_channels_x3->size() << " " << expected_channels_y3->size() << 
      " " << expected_channels_x4->size() << " " << expected_channels_y4->size() <<
      " " << expected_channels_x5->size() << " " << expected_channels_y5->size() <<
      " " << expected_channels_x6->size() << " " << expected_channels_y6->size() <<
      " " << expected_channels_x7->size() << " " << expected_channels_y7->size() <<
      " " << expected_channels_x8->size() << " " << expected_channels_y8->size() <<
      endl;
    exit(0);
  }


  return;
}


bool is_lockfile_running(string asker, string commandname){
  // check lock file

  //  clog << asker << " is checking if " << commandname << " has lockfile " << endl;

  ifstream lockfile_;
  string lockfile_name="__"+commandname+"_lock_file";
  lockfile_.open(lockfile_name.c_str());
  if( lockfile_.good() ){
    //    clog << commandname << " is running, so " << asker << " will hold on " << endl; fflush(stdout);
    return true;
  }
  return false;
}



bool is_program_running(string asker, string commandname){
  // Check if process is running via command-line
  
  //  clog << asker << " is checking if " << commandname << " is running " << endl;
  
  char child[100]; 
  char die[100];	

  string name1="pidof -x ";
  string name2=" > /dev/null ";
  // Check if process is running via command-line

  strcpy (child, name1.c_str());
  strcat (child, commandname.c_str());
  strcat (child, name2.c_str());

  // loop to execute kill
  if(0 == system(child)) {
    //    clog << commandname << " is running, so " << asker << " will hold on " << endl; fflush(stdout);
    return true;
  }

  return false;
}
  
size_t get_n_layers(vector<size_t> expected_channels_1, vector<size_t> expected_channels_2, vector<size_t> expected_channels_3, vector<size_t> expected_channels_4, vector<size_t> expected_channels_5, vector<size_t> expected_channels_6, vector<size_t> expected_channels_7, vector<size_t> expected_channels_8){
  // read a vector as input
  // this vector can have the same entry several times: (0 1 2 1 3 5 2 1 ...)
  // the function returns the number of different elements in the vector
  size_t counter = 0;
  vector<size_t> unique;
  for(size_t i=0; i<expected_channels_1.size(); i++){
    vector<size_t>::iterator ic = std::find(unique.begin(), unique.end(), expected_channels_1[i]);
    if( ic == unique.end() )
      unique.push_back(expected_channels_1[i]);
  }
  for(size_t i=0; i<expected_channels_2.size(); i++){
    vector<size_t>::iterator ic = std::find(unique.begin(), unique.end(), expected_channels_2[i]);
    if( ic == unique.end() )
      unique.push_back(expected_channels_2[i]);
  }
  for(size_t i=0; i<expected_channels_3.size(); i++){
    vector<size_t>::iterator ic = std::find(unique.begin(), unique.end(), expected_channels_3[i]);
    if( ic == unique.end() )
      unique.push_back(expected_channels_3[i]);
  }
  for(size_t i=0; i<expected_channels_4.size(); i++){
    vector<size_t>::iterator ic = std::find(unique.begin(), unique.end(), expected_channels_4[i]);
    if( ic == unique.end() )
      unique.push_back(expected_channels_4[i]);
  }
  for(size_t i=0; i<expected_channels_5.size(); i++){
    vector<size_t>::iterator ic = std::find(unique.begin(), unique.end(), expected_channels_5[i]);
    if( ic == unique.end() )
      unique.push_back(expected_channels_5[i]);
  }
  for(size_t i=0; i<expected_channels_6.size(); i++){
    vector<size_t>::iterator ic = std::find(unique.begin(), unique.end(), expected_channels_6[i]);
    if( ic == unique.end() )
      unique.push_back(expected_channels_6[i]);
  }
  for(size_t i=0; i<expected_channels_7.size(); i++){
    vector<size_t>::iterator ic = std::find(unique.begin(), unique.end(), expected_channels_7[i]);
    if( ic == unique.end() )
      unique.push_back(expected_channels_7[i]);
  }
  for(size_t i=0; i<expected_channels_8.size(); i++){
    vector<size_t>::iterator ic = std::find(unique.begin(), unique.end(), expected_channels_8[i]);
    if( ic == unique.end() )
      unique.push_back(expected_channels_8[i]);
  }

  return unique.size();

}
