#include <iostream>
#include <fstream>
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
#include <pqxx/pqxx>
#include "Riostream.h"
#include "TString.h"
#include "iostream"
#include "sstream"
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "usbfunctions.h"
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


std::string lockfile_name;
bool print;
TTree *data_tree;
size_t n_variables_before_channels;
double default_value;
int default_value_int;
double mintime;
double maxtime;
double minpn;
double maxpn;
int nchannels_per_event;
ifstream in;
size_t old_evn;
size_t n_coincidence_events_per_root_file;
size_t filenumber;
string sfilenumber;
string fname;
TFile *f;
pqxx::result the_db_entry;
std::string command;
std::string table_name="coincidences";
int db_entry = -1;
bool first = true;
string asker="converter";
string commandname="./select";
bool running=true;

// initialize a bunch of parameters
static const int MAX_CHANNELS_NUMBER=128;
static const int MAX_COINCIDENCES_NUMBER=1000;
static const int MAX_N_FILES=5000;
static const int MAX_CHANNELS_NUMBER_PER_BOARD=32;

#define N_COINCIDENCE_TABLE_COLUMNS 6 // min value, up to 134


// initialize root structures
int _nchannels_per_event[MAX_COINCIDENCES_NUMBER];  // n of channels per hit
int _nsignals;  // n of hits per coincidence event
int _packet_number[MAX_COINCIDENCES_NUMBER];
bool _type[MAX_COINCIDENCES_NUMBER];
int _event_number[MAX_COINCIDENCES_NUMBER];
int _board_address[MAX_COINCIDENCES_NUMBER];
double _time[MAX_COINCIDENCES_NUMBER];
int _channels[MAX_COINCIDENCES_NUMBER][MAX_CHANNELS_NUMBER];
double _n_layers;

void write_accumulated_signals_as_tree_entry();

void load_signal_into_root_variables();

void prepare_tree();

void read_next_event_from_db();

void reinitialize();

float read_parameter_from_file(string filename, string paramname, string lockfile_name);

string get_file_number(size_t i, string lockfilename);

bool is_program_running(string asker, string commandname);

string int_to_string(int i);

pqxx::connection * c;

pqxx::result execute_nontransaction(string sql, bool print);  
pqxx::result execute_transaction(string sql, bool print);
bool check_if_table_exists( std::string table_name );

void fill_channel_names();
string channel_names[MAX_CHANNELS_NUMBER_PER_BOARD];
string list_of_channel_names(size_t n);

using namespace std;

int main(int argc, char *argv[]){

//  Read data from an ascii file and create a root file with an histogram and an ntuple.


  // create lock file (for viewer to check)
  ofstream lockfile_;
  lockfile_name="__converter_lock_file";
  lockfile_.open(lockfile_name.c_str());
  lockfile_ << " converter active " << endl;
  lockfile_.close();


  TString dir = gSystem->UnixPathName(gInterpreter->GetCurrentMacroName());
  dir.ReplaceAll("read_data.C","");
  dir.ReplaceAll("/./","/");


  default_value=-666.;
  default_value_int=-666;
  print = (bool)(read_parameter_from_file("params.inp","print",lockfile_name)); // print verbose
  double NUM_BOARDS = read_parameter_from_file("data_params.txt", "NUM_BOARDS",lockfile_name);
  int NUM_ALL_CHANNELS=(int)(NUM_BOARDS+0.5)*NUM_CHANNELS;
  n_coincidence_events_per_root_file = (size_t)(read_parameter_from_file("params.inp","n_coincidence_events_per_root_file",lockfile_name)+0.5);
  mintime=999999999999999999.;
  maxtime=0.;
  minpn=99999999999999999.;
  maxpn=0.;
  old_evn=0;
  nchannels_per_event=0;
  int in_number = 0;
  bool good_in=true;
  string fnum, fname;

  // important: expected data structure

  // evn, boardaddress, packetnumber, type, time, n_layers
  n_variables_before_channels=6; 

  filenumber=0;
  sfilenumber=get_file_number(filenumber,lockfile_name);
  fname = "coincidence_data_"+sfilenumber+".root";
  f = new TFile(fname.c_str(),"RECREATE");
  
  prepare_tree();

  string string_dbname = read_string_from_file("params.inp","dbname");
  string string_host = read_string_from_file("params.inp","host");
  string string_user = read_string_from_file("params.inp","user");
  string string_password = read_string_from_file("params.inp","password");

  string string_connection = "user=" + string_user + " host=" + string_host + " password=" + string_password + " dbname=" + string_dbname;
  c = new pqxx::connection(string_connection.c_str());

  fill_channel_names();

  bool good, nextgood;

  if( !check_if_table_exists( table_name ) ){
    clog << " db " << table_name << " does not exist, converter quitting " << endl;

    // store min time, max time, min pknumber, max pknumber for future plotting
    ofstream data_params_file;
    data_params_file.open("data_limits.txt",ios_base::out );
    data_params_file << " mintime " << mintime << endl;
    data_params_file << " maxtime " << maxtime << endl;
    data_params_file << " minpn " << minpn << endl;
    data_params_file << " maxpn " << maxpn << endl;
    data_params_file.close();

    // save root file
    f->Write();
    f->Close();
    
    
    remove(lockfile_name.c_str());
    exit(0);

    return 1;
  }

  // count rows in board table
  command = "SELECT COUNT(*) FROM " + table_name ; the_db_entry = execute_nontransaction(command, false);
  std::clog << " convert data to root: looping over db with " << the_db_entry[0][0].as<int>() << " entries " << std::endl; fflush(stdout);

  while(true){

    db_entry ++;

    // check if this entry exists
    command = " SELECT * FROM " + table_name + " WHERE db_entry = " + int_to_string(db_entry);
    the_db_entry = execute_nontransaction(command, false);

    nextgood = (the_db_entry.size() && the_db_entry.begin()->size() >= N_COINCIDENCE_TABLE_COLUMNS);

    if ( !nextgood ){

      clog << asker << " needs event " << db_entry << " but it's not there yet, check if " << commandname << " is running " << endl; fflush(stdout);

      running=true;
      while(running){
	running=is_program_running(asker.c_str(), commandname.c_str());
	the_db_entry = execute_nontransaction(command, false);
	good = (the_db_entry.size() && the_db_entry.begin()->size() >= N_COINCIDENCE_TABLE_COLUMNS);
	if( good ) break;
      }
      
      if( !running ){
	clog << asker << " will stop in coincidence loop " << endl; fflush(stdout);
	break;
      }

    }
  
    _nsignals=0;

    read_next_event_from_db();


  } // end of looping on txt files
  

  // store min time, max time, min pknumber, max pknumber for future plotting
  ofstream data_params_file;
  data_params_file.open("data_limits.txt",ios_base::out );
  data_params_file << " mintime " << mintime << endl;
  data_params_file << " maxtime " << maxtime << endl;
  data_params_file << " minpn " << minpn << endl;
  data_params_file << " maxpn " << maxpn << endl;
  data_params_file.close();

  // save root file
  f->Write();
  f->Close();
  
   
  remove(lockfile_name.c_str());
  exit(0);

  return 1;

}


int binaryToBase10(int n)
{
  int output = 0;

  int m = 1;

  for(int i=0; n > 0; i++) {

    if(n % 10 == 1) {
      output += m;
    }
    n /= 10;
    m <<= 1;
  }

  return output;
}


float read_parameter_from_file(string filename, string paramname, string lockfile_name){

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

  remove(lockfile_name.c_str());
  exit(0);

  return 0.;


}



string get_file_number(size_t i, string lockfilename){
  
  size_t limit = 6;
  string numb=int_to_string((int)i);
  if( numb.size() > limit ){
    clog << " problem: file number " << i << " exceeds limit " << endl;
    remove(lockfile_name.c_str());
    exit(0);
  }
  size_t n_extra_zeros=6-numb.size();
  string output="";
  for(size_t i=0; i<n_extra_zeros; i++)
    output += '0';
  return output+numb;
  
}

  string int_to_string(int i){
    char cc[20];
    sprintf(cc,"%d",i);
    string sval(cc);
    return sval;
  }


  void wait ( double seconds )
  {
    clock_t endwait;
    endwait = clock () + seconds * 1e6 ;
    while (clock() < endwait) {}
  }


string get_current_dir(){

  char cCurrentPath[100];
  
  if (!getcwd(cCurrentPath, sizeof(cCurrentPath) / sizeof(char)))
    {
      return 0;
    }
  
  cCurrentPath[sizeof(cCurrentPath) - 1] = '\0'; /* not really required */
  
  string answer=cCurrentPath;

  return answer;
  
}



bool is_program_running(string asker, string commandname){
  // Check if process is running via command-line
  
  //  clog << asker << " is checking if " << commandname << " is running " << endl;
  
  char child[100]; 
  char die[100];	

  string name1="/bin/pidof -x ";
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
  
  

void write_accumulated_signals_as_tree_entry(){

  if( _nsignals <= 0 ) return;
  for(size_t jj=0; jj<_nsignals; jj++)
    if( _nchannels_per_event[jj] <= 0 ) return;


  if( print ){
    clog << " " << endl;
    clog << " writing to tree: nsignals " << _nsignals;
    for(size_t jj=0; jj<_nsignals; jj++){
      //for(size_t jj=0; jj<100; jj++){
      clog << " signal " << jj << " nchannels " << _nchannels_per_event[jj]  << " _event_number " << _event_number[jj] << " _board_address " << _board_address[jj] << " _packet_number " << _packet_number[jj] << "  _time " << _time[jj]  << "  _n_layers " << _n_layers  << std::endl;
      for(size_t kk=0; kk<_nchannels_per_event[jj]; kk++){
	//for(size_t kk=0; kk<128; kk++){
	std::clog << " channel " << kk  << " _channels " << _channels[jj][kk] << std::endl;
      }
    }
  }
  
      
  data_tree->Fill();
  
  return;

}

void load_signal_into_root_variables(){

  _event_number[_nsignals] = the_db_entry[0]["event_number"].as<size_t>();
  _board_address[_nsignals] = (size_t)binaryToBase10((int) the_db_entry[0]["board_address"].as<int>());
  _packet_number[_nsignals] = 0;
  _type[_nsignals] = true;
  _time[_nsignals] = the_db_entry[0]["time"].as<double>();
  _n_layers = the_db_entry[0]["n_layers"].as<size_t>();

  if( _time[_nsignals] < mintime ) mintime=_time[_nsignals];
  if( _time[_nsignals] > maxtime ) maxtime=_time[_nsignals];

  size_t local_nchs = the_db_entry[0]["nchs"].as<size_t>();
  for(size_t i=0; i<local_nchs; i++){
    _channels[_nsignals][i]=NUM_CHANNELS*_board_address[_nsignals]+std::stoi(the_db_entry[0][channel_names[i].c_str()].as<std::string>());
  }

  if( print ){
    std::clog << "" << std::endl;
    clog << " reading evn " << _event_number[_nsignals] <<  " b " << _board_address[_nsignals] << " p " << _packet_number[_nsignals] << " t " << setprecision(12) << _time[_nsignals] << " n " << _n_layers << " old " << old_evn << endl;
  }
  
  nchannels_per_event += local_nchs;
  old_evn=_event_number[_nsignals] ;

}


void prepare_tree(){

  data_tree = new TTree("data_tree","ntuple of data");
  data_tree->Branch("nsignals",&_nsignals,"nsignals/I");
  data_tree->Branch("nchannels_per_event",(_nchannels_per_event),"nchannels_per_event[nsignals]/I");
  data_tree->Branch("event_number",(_event_number),"event_number[nsignals]/I");
  data_tree->Branch("packet_number",(_packet_number),"packet_number[nsignals]/I");
  data_tree->Branch("type",(_type),"type[nsignals]/O");
  data_tree->Branch("board_address",(_board_address),"board_address[nsignals]/I");
  data_tree->Branch("time",(_time),"time[nsignals]/D");
  data_tree->Branch("n_layers",&(_n_layers),"n_layers/D");
  data_tree->Branch("channels",(_channels),"channels[nsignals][128]/I");

}


void read_next_event_from_db(){

  nchannels_per_event=0;
  bool nextgood;
  while( true ){

    // check if this entry exists
    command = " SELECT * FROM " + table_name + " WHERE db_entry = " + int_to_string(db_entry);
    the_db_entry = execute_nontransaction(command, false);

    nextgood = (the_db_entry.size() && the_db_entry.begin()->size() >= N_COINCIDENCE_TABLE_COLUMNS);

    if( !nextgood ){

      clog << asker << " needs event " << db_entry << " but it's not there yet, check if " << commandname << " is running " << endl; fflush(stdout);

      running=true;
      while(running){
	running=is_program_running(asker.c_str(), commandname.c_str());
	the_db_entry = execute_nontransaction(command, false);
	nextgood = (the_db_entry.size() && the_db_entry.begin()->size() >= N_COINCIDENCE_TABLE_COLUMNS);
	if( nextgood ) break;
      }
      
      if( !running ){
	clog << asker << " will stop in signal loop " << endl; fflush(stdout);
	break;
      }

    }

    // if the new line belongs to a new event
    // write all accumulated signals to one event
    if( _nsignals >= MAX_COINCIDENCES_NUMBER - 1 || 
	(!first && the_db_entry[0]["event_number"].as<int>() != old_evn) ){
      
      write_accumulated_signals_as_tree_entry();

      // reinitialize
      reinitialize();

    }
    
    
    // load signal from row into root variables
    load_signal_into_root_variables();
    
    if( nchannels_per_event >= MAX_CHANNELS_NUMBER ){
      clog << " problem: nchannels_per_event " << nchannels_per_event << " MAX CHANNELS NUMBER " << MAX_CHANNELS_NUMBER << endl;
      nchannels_per_event=MAX_CHANNELS_NUMBER-1;
      break;
    }

    _nchannels_per_event[_nsignals]=nchannels_per_event;

    if( first ){
      old_evn = _event_number[_nsignals];
      first = false;
    }
    
    // open new root file
    if( data_tree->GetEntries() >= n_coincidence_events_per_root_file ){
      f->Write();
      f->Close();
      delete f;
      filenumber ++;
      sfilenumber=get_file_number(filenumber,lockfile_name);
      fname = "coincidence_data_"+sfilenumber+".root";
      f = new TFile(fname.c_str(),"RECREATE");
      prepare_tree();
    }
    
    _nsignals++;
    db_entry++;
    
  } // end of reading txt file


  write_accumulated_signals_as_tree_entry();

  

}


void reinitialize(){

  // reinitialize
  nchannels_per_event=0;
  _nsignals=0;
  _n_layers = default_value;
  for(size_t i=0; i<MAX_COINCIDENCES_NUMBER; i++){
    _nchannels_per_event[i]=0;
    _event_number[i] = default_value_int;
    _board_address[i] = default_value_int;
    _packet_number[i] = default_value_int;
    _type[i] = false;
    _time[i] = default_value;
    for(size_t j=0; j<MAX_CHANNELS_NUMBER; j++){
      _channels[i][j]=default_value_int;
    }
  }


}

  pqxx::result execute_nontransaction(string sql, bool print){
  
    /* Create a non-transactional object. */
    pqxx::nontransaction N(*c);
  
    /* Execute SQL query */
    pqxx::result R( N.exec( sql ));
  
    if( print ){
      std::clog << " executing nontransaction: " << sql << std::endl; fflush(stdout);
    
      std::clog << " rows: " << R.size() << std::endl;
      for (pqxx::result::const_iterator row = R.begin(); row != R.end();++row){
	std::clog << " ... row: " << row - R.begin() << " columns: " << row->size() << ":";
	for (pqxx::result::tuple::const_iterator field = row->begin(); field != row->end();++field){
	  std::clog << " " << field->c_str();
	}
	std::clog << std::endl;
      }
      //std::clog << " nontransaction done successfully" << endl; fflush(stdout);
    }
  
    return R;
  
  }
  
  pqxx::result execute_transaction(string sql, bool print){
 
    /* Create a transactional object. */
    pqxx::work N(*c);
  
    /* Execute SQL query */
    pqxx::result R( N.exec( sql ));
  
    N.commit();
  
    if( print ){
      std::clog << " executing transaction: " << sql << std::endl; fflush(stdout);

      std::clog << " rows: " << R.size() << std::endl;
      for (pqxx::result::const_iterator row = R.begin(); row != R.end();++row){
	std::clog << " ... row: " << row - R.begin() << " columns: " << row->size() << ":";
	for (pqxx::result::tuple::const_iterator field = row->begin(); field != row->end();++field){
	  std::clog << " " << field->c_str();
	}
	std::clog << std::endl;
      }
    
      //std::clog << " transaction done successfully" << endl; fflush(stdout);
    }
  
    return R;
  
  }

bool check_if_table_exists( std::string table_name ){

  std::string command;
  pqxx::result R;

  command = "SELECT Distinct TABLE_NAME FROM information_schema.TABLES where TABLE_TYPE = 'BASE TABLE'";  R = execute_nontransaction(command, false);

  std::string local_name;

  for (pqxx::result::const_iterator row = R.begin(); row != R.end();++row){
    if( row->size() ){
      local_name = row->at(0).as<std::string>();
      if( local_name.compare(table_name) == 0 ){
	return true;
      }
    }
  }

  return false;
}


void fill_channel_names(){

  for(size_t i=0; i<MAX_CHANNELS_NUMBER_PER_BOARD-1; i++){
    channel_names[i] = "ch" + int_to_string(i);
  }

  channel_names[MAX_CHANNELS_NUMBER_PER_BOARD - 1] = "ch" + int_to_string(MAX_CHANNELS_NUMBER_PER_BOARD - 1);

}


string list_of_channel_names(size_t n){

  string   list = "";
  for(size_t i=0; i<n-1; i++){
    list += channel_names[i] + ", ";
  }

  list += channel_names[n - 1];
  return list;

}

