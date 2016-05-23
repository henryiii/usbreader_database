/* -*- mode: c++ -*- */
#ifndef ILEVEL
#define ILEVEL

#include <iostream>
#include <string>
#include <vector>
#include <deque>
#include "Event.h"
#include "Packet.h"
#include "Signal.h"
#include <time.h>
#include <unistd.h>

#include <string.h>
#include <float.h>

#include <stdio.h>
#include <algorithm>
#include <ios>
#include <pqxx/pqxx>

using namespace std;

#define STORE_RAW false

#if STORE_RAW
#define N_BOARD_TABLE_COLUMNS 3
#else
#define N_BOARD_TABLE_COLUMNS 6 // min value, up to 134
#endif

class Level {

  double t_;
  size_t N_;
  double minimum_t_;
  size_t minimum_index_;
  double next_to_minimum_t_;
  size_t next_to_minimum_index_;
  double window_;
  ifstream in_[LARGE_NUMBER];
  size_t in_number_[LARGE_NUMBER]; // array that contains indexes of input data files (or input db packets)
  vector<Event>::iterator actual_event_[LARGE_NUMBER];
  vector<Event>::iterator next_event_[LARGE_NUMBER];
  pqxx::connection * c;

public:

  Packet packets_[100];
  Event es_[LARGE_NUMBER];

  // default constructor
  Level()
  {
    t_=0.;
    N_=0;
    window_=0.;

    //connect to the database
    // dbname = "boards"
    // host = 127.0.0.1
    // user = "postgres"
    // password = f
    c = new pqxx::connection("user=postgres host=127.0.0.1 password=f dbname=boards");
  }


  // constructor
  Level(size_t N, double window, string string_connection)
  {
    if( N < 1 ){
      clog << " error: building level with " << N << " events " << endl;
      exit(0);
    }
    N_ = N;
    window_=window;
    for(size_t i=0; i<N_; i++){
      // skip first file for each board
      // containing leftovers from previous run
      in_number_[i]=1;
    }
    c = new pqxx::connection(string_connection.c_str());
  }


  // constructor
  Level(size_t N, double window)
  {
    if( N < 1 ){
      clog << " error: building level with " << N << " events " << endl;
      exit(0);
    }
    N_ = N;
    window_=window;
    for(size_t i=0; i<N_; i++){
      // skip first file for each board
      // containing leftovers from previous run
      in_number_[i]=1;
    }
    c = new pqxx::connection("user=postgres host=127.0.0.1 password=f dbname=boards");
  }


  //! destructor
  virtual ~Level(){

    
  };


  void init(Packet* stored_packets,size_t n);
  void get_earliest_board();
  void assign_packet(size_t i, Packet p);
  std::string read_from_db(std::string board,int event);

  Event get_actual_event(size_t iboard){
#if STORE_RAW
    return *actual_event_[iboard];
#else
    return es_[iboard];
#endif
  }

  Event get_next_event(size_t iboard){
    return *next_event_[iboard];
  }


  virtual void dump (ostream & a_out         = clog,
		     const string & a_title  = "",
		     const string & a_indent = ""){
    string indent;
    if (! a_indent.empty ()) indent = a_indent;
    if (! a_title.empty ())
      {
	a_out << indent << a_title << endl;
      }
    
    for(size_t i=0; i<N_; i++){
      clog << "[";
      if( get_actual_event(i).written() ) clog << "*";
      clog << get_actual_event(i).board_address() << " "  << setprecision(20) << get_actual_event(i).time("s");
      for(size_t j=0; j<get_actual_event(i).channels().size(); j++)
	clog << " " << get_actual_event(i).channels()[j];
      clog << "]";
    }
    clog << " minimum index: " << minimum_index() << " time " << minimum_time() << endl;
    //      a_out << " t = " << setprecision(ndigits/2.) << t_ << " +- " << setprecision(ndigits/2) << window_ << endl;
    return;
  }

  /*
    void set_from_packets( deque< deque<Packet> > packets, size_t N ) {
    if( N != N_ ){
    clog << " error: setting level from packets, but N " << N << " != N_ " << N_ << endl;
    exit(0);
	
    }

    //    clog << " setting " << N << " packets " << endl;
    for(size_t i=0; i<N_; i++){
    //      clog << " packet " << i ; fflush(stdout);
    //      packets[i].dump();
    if( packets[i].size() == 0 ){
    clog << " error: setting level from packets, but board " << i << " of " << N_ << " has no packets " << endl;
    exit(0);
    }
    if( packets[i][0].events().size() == 0 ){
    clog << " error: setting level from packets, but packet 0 of board " << i << " of " << N_ << " has no events " << endl;
    exit(0);
    }
    events_[i] = packets[i][0].events()[0];
    }

    recalculate_minima();

    return;
    }
  
    void set_event( Event e, size_t index){
    if( index >= N_ ){
    clog << " error: setting event " << index << " maximum: " << N_ << endl;
    exit(0);
    }
    
    events_[index] = e;

    recalculate_minima();
    }
  
  */

  double time(void){
    return t_;
  }

  double window(void){
    return window_;
  }

  Event get_event(size_t index){
    if( index >= N_ ){
      clog << " error: getting event " << index << " maximum: " << N_ << endl;
      exit(0);
    }

    return get_actual_event(index);
     
  }

  double minimum_time(void){
    return minimum_t_;
  }

  size_t minimum_index(void){
    return minimum_index_;
  }

  double next_to_minimum_time(void){
    return next_to_minimum_t_;
  }

  double next_to_minimum_index(void){
    return next_to_minimum_index_;
  }

  void recalculate_minima(void){
    double min=9999999999999999.;
    size_t ind=0;
    double next_to_min=9999999999999999.;
    size_t next_to_ind=0;
    for(size_t i=0; i<N_; i++){
      if( get_actual_event(i).time("s") < min ){
	next_to_min = min;
	next_to_ind=ind;
	min = get_actual_event(i).time("s");
	ind=i;
      }
      else if( get_actual_event(i).time("s") < next_to_min ){
	next_to_min = get_actual_event(i).time("s");
	next_to_ind=i;
      }
    }

    minimum_t_=min;
    minimum_index_=ind;
    next_to_minimum_t_=next_to_min;
    next_to_minimum_index_=next_to_ind;

    return;

  }


  double maximum_time(){
    double max=-1;
    for(size_t i=0; i<N_; i++){
      if( get_actual_event(i).time("s") > max ){
	max = get_actual_event(i).time("s");
      }
    }

    return max;
  }


  void set_time_to_minimum(void){
    t_ = minimum_t_;
    return ;
  }


  bool select(vector<size_t> * indexes, vector< Signal > signals, bool print=false){

    if( signals.size() == 0 ){
      clog << " problem: no pattern to look for provided " << endl;
      exit(0);
    }

    for(vector<Signal>::iterator is=signals.begin(); is!=signals.end(); ++is){
      if( print )
	clog << " look for signal: "; is->dump();

      size_t iboard = get_board_with_address(is->board_address());
      
      if( board_has_channel(iboard, is->channel(), print) != is->active() ){
	if( print )
	  clog << " ... board " << is->board_address() << " has channel " << is->channel() << " on status different from requirement: " << is->active() << endl;
	return false;
      }
      
      if( !board_has_coincidence(iboard, minimum_time(), print)){
	if( print )
	  clog << " ... board " << is->board_address() << " has no coincidence " << endl;
	return false;
      } 
      if( print )
	clog << "  ... this signal was found " << endl;
      indexes->push_back(iboard);
    }

    if( print )
      clog << "  select !!! " << endl;

    return true;
    
  }


  size_t n_coincidences(vector<size_t> * indexes, bool print=false){
    // n of boards with actual event in coincidence with minimum time

    
    for(size_t iboard=0; iboard<N_; iboard++)
      if( board_has_coincidence(iboard, minimum_time(), print))
	indexes->push_back(iboard);

    
    return indexes->size();
    
  }
  


  string int_to_string(int i){
    char c[20];
    sprintf(c,"%d",i);
    string sval(c);
    return sval;
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


  void open_data_files(vector<string> names){
    for(size_t i=0; i<N_; i++){
      string snumb = get_file_number(in_number_[i]);
      string fname = "raw_data_"+names[i]+"_"+snumb+".txt";
      string snumbnext = get_file_number(in_number_[i]+1);
      string fnamenext = "raw_data_"+names[i]+"_"+snumbnext+".txt";
      in_[i].open(fname.c_str());
      ifstream nextfile;
      nextfile.open(fnamenext.c_str());
      if (!in_[i].good() || !nextfile.good() ){
	clog << " selector needs file " << fname << " but this file or the next are not there yet " << endl;
	string asker="./select";
	string commandname="./receive_one";
	bool running=true;
	bool good;
	while(running){
	  running=is_program_running(asker.c_str(), commandname.c_str());
	  in_[i].open(fname.c_str());
	  nextfile.open(fnamenext.c_str());
	  good = in_[i].good() && nextfile.good();
	  if( good ) break;
	}

	if( !running ){
	  clog << asker << " will stop (opening data files) " << endl;fflush(stdout);
	  exit(0);
	}
      }

      std::string lineData;
      getline(in_[i], lineData);

      //      clog << " board " << i << " first event: " << lineData << endl;
      Packet p(lineData);
      p.packet_to_physical_parameters();

      packets_[i] = p;
      next_event_[i]=packets_[i].events_.begin();
      next_event_[i]++;
      p.events_[0].set_analyzed(true);
      actual_event_[i]=packets_[i].events_.begin();

    }

    dump();

    return;

  }

  void open_db_tables(vector<string> names){

    string table_name;
    size_t db_entry;
    std::string command;
    pqxx::result the_db_entry;
    bool nextgood;
    string asker="./select";
    string commandname="./receive_one";
    bool running=true;
    bool good;

    for(size_t i=0; i<N_; i++){

      table_name = "board_" + names[i];
      db_entry = in_number_[i];

      // check if board table exists
      if( !check_if_table_exists( table_name ) ){
	clog << " db " << table_name << " does not exist, Level quitting " << endl;
	break;
      }

      // check if this entry exists
      command = " SELECT * FROM " + table_name + " WHERE db_entry = " + int_to_string(db_entry);
      the_db_entry = execute_nontransaction(command, false);

      nextgood = (the_db_entry.size() && the_db_entry.begin()->size() >= N_BOARD_TABLE_COLUMNS);

      if ( !nextgood ){
	clog << " selector needs db_entry " << db_entry << " but this entry or the next are not there yet " << endl;
	running=true;
	while(running){
	  running=is_program_running(asker.c_str(), commandname.c_str());

	  the_db_entry = execute_nontransaction(command, false);
	  good = (the_db_entry.size() && the_db_entry.begin()->size() >= N_BOARD_TABLE_COLUMNS);
	  if( good ) break;
	}

	if( !running ){
	  clog << asker << " will stop (opening db tables) " << endl;fflush(stdout);
	  if( check_if_table_exists( "coincidences" ) ){
	    command = "SELECT COUNT(*) FROM coincidences "; the_db_entry = execute_nontransaction(command, false);
	    std::clog << " filled table coincidences with " << the_db_entry[0][0].as<int>() << " entries " << std::endl; fflush(stdout);
	  }
	  exit(0);
	}
      }

#if STORE_RAW
      Packet p(the_db_entry[0]["word"].as<std::string>());
      p.packet_to_physical_parameters();

      packets_[i] = p;
      next_event_[i]=packets_[i].events_.begin();
      next_event_[i]++;
      p.events_[0].set_analyzed(true);
      actual_event_[i]=packets_[i].events_.begin();

#else

      std::vector<size_t> channels;
      string channel_name;

      size_t local_nchs = the_db_entry[0]["nchs"].as<size_t>();
      for(size_t u=0; u<local_nchs; u++){
	channel_name = "ch" + int_to_string(u);
	channels.push_back(std::stoi(the_db_entry[0][channel_name.c_str()].as<std::string>()));
      }

      double time = the_db_entry[0]["time"].as<double>();
      double packet_number = the_db_entry[0]["packet_number"].as<double>();
      string board_address = get_board_address_from_binary(the_db_entry[0]["board_address"].as<string>());
      size_t n_layers = the_db_entry[0]["n_layers"].as<size_t>();
      Event e(channels, time, packet_number, board_address, n_layers);

      e.set_analyzed(true);

      es_[i] = e;

#endif
    

    }

    dump();

    return;

  }

  void close(void){
    for(size_t i=0; i<N_; i++){
      in_[i].close();
    }

  }


  size_t get_board_with_address(string address){
    for(size_t i=0; i<N_; i++)
      if( get_actual_event(i).board_address() == address )
	return i;

    clog << " problem: level is requested board address " << address << " which is not found, addresses are: ";
    for(size_t i=0; i<N_; i++)
      clog << " " <<  get_actual_event(i).board_address();
    clog << " quitting " << endl;
    exit(0);


  }


  bool increase_the_time(void){

    return increase_the_time_for_board(minimum_index());
    
  }

  

  void delete_file(string filename){
    string strpath=get_current_dir()+"/";
    char *spath=(char*)strpath.c_str();
    strcat(spath, filename.c_str());
    unlink(spath);
    return;
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


  bool fexists(const char *filename)
  {
    ifstream ifile(filename);
    return ifile.good();
  }

  bool increase_the_time_for_board(size_t index){

    if( index >= N_ ){
      clog << " Index: " << index << " Input files size: " << N_ << endl;
      exit(0);
    }


    if( !read_next_event(index) ){
      std::clog << " level: quitting at increase " << std::endl; fflush(stdout);
      return false;
    }
  
    //    std::clog << " Recalculating " << endl;fflush(stdout);

    recalculate_minima();
    set_time_to_minimum();
    
    return true;
    
  }

  bool renew(){

    for( size_t index =0; index < N_; index++ ){
      if( !read_next_event(index) ){
	std::clog << " level: quitting at renew " << std::endl; fflush(stdout);
	return false;
      }
    }
  
    recalculate_minima();
    set_time_to_minimum();
    
    return true;

  }

  bool read_next_event(size_t index){
 
#if STORE_RAW
    // read next event from new packet, if:
    // 1) actual event is the last of the packet, or
    // 2) last event of the packet is still below reference
    if( (next_event_[index] == packets_[index].events_.end()) ||
	(packets_[index].events_.back().time("s") <= minimum_time()) ){
      return read_next_event_from_new_packet(index);
    }

    // read next event from same packet (hence the same file)
    //std::clog << " reading next " << endl;fflush(stdout);
    return  read_next_event_from_same_packet(index);
#else

    return read_next_event_not_raw(index);

#endif

  }

  string double_to_string(double i){
    
    std::ostringstream out;
    out << std::setprecision(20) << i;
    return out.str();
  }

  bool read_next_event_from_new_packet(size_t index){


    string table_name;
    size_t db_entry;
    std::string command;
    pqxx::result the_db_entry;
    bool nextgood;
    string asker="./select";
    string commandname="./receive_one";
    bool running=true;
    bool good;
    int modulo = 100;

    table_name = "board_" + get_board_address(index);

    // increase event number
#if 1

    command = " DELETE FROM " + table_name + " WHERE db_entry <= " + int_to_string(in_number_[index]);
    execute_transaction(command, false); 

    in_number_[index] ++;
#else

    command = " DELETE FROM " + table_name + " WHERE db_entry <= " + int_to_string(in_number_[index]);
    execute_transaction(command, false); 

    int naive_next_number = in_number_[index] + 1;
    double max_time = maximum_time();
    command = " SELECT db_entry, time, ABS( time - " + double_to_string(max_time)  + " ) AS distance FROM " + table_name + " ORDER BY distance LIMIT 1";
    the_db_entry = execute_nontransaction(command, false); // returns (entry, time, distance)
    int smart_next_number = naive_next_number;
    if( the_db_entry.size() && the_db_entry.begin()->size() >= N_BOARD_TABLE_COLUMNS )
      smart_next_number = the_db_entry[0][0].as<int>();


    in_number_[index] = std::max(naive_next_number, smart_next_number);

#endif


    db_entry = in_number_[index];

    // check if this entry exists
    command = " SELECT * FROM " + table_name + " WHERE db_entry = " + int_to_string(db_entry);
    the_db_entry = execute_nontransaction(command, false);

    nextgood = (the_db_entry.size() && the_db_entry.begin()->size() >= N_BOARD_TABLE_COLUMNS);
    if ( !nextgood ){
      clog << " selector needs db_entry " << db_entry << " but this entry or the next are not there yet " << endl;
      running=true;
      while(running){
	running=is_program_running(asker.c_str(), commandname.c_str());
	
	the_db_entry = execute_nontransaction(command, false);
	good = (the_db_entry.size() && the_db_entry.begin()->size() >= N_BOARD_TABLE_COLUMNS);
	if( good ) break;
      }
      
      if( !running ){
        clog << asker << " will stop (reading next event from new packet) " << endl; fflush(stdout);
	// count rows in table 
	if( check_if_table_exists( "coincidences" ) ){
           command = "SELECT COUNT(*) FROM coincidences "; the_db_entry = execute_nontransaction(command, false);
	   std::clog << " filled table coincidences with " << the_db_entry[0][0].as<int>() << " entries " << std::endl; fflush(stdout);
        }
	return false;
      }
  }

  Packet p(the_db_entry[0]["word"].as<std::string>());
  p.packet_to_physical_parameters();
  
  packets_[index] = p;
  next_event_[index]=packets_[index].events_.begin();
  next_event_[index]++;
  p.events_[0].set_analyzed(true);
  actual_event_[index]=packets_[index].events_.begin();

  return true;

}


  bool read_next_event_from_same_packet(size_t index){
    next_event_[index]++;
    actual_event_[index]++;
    actual_event_[index]->set_analyzed(true);
    return true;
  }


  bool read_next_event_not_raw(size_t index){

    string table_name;
    size_t db_entry;
    std::string command;
    pqxx::result the_db_entry;
    bool nextgood;
    string asker="./select";
    string commandname="./receive_one";
    bool running=true;
    bool good;
    int modulo = 100;

    table_name = "board_" + get_board_address(index);

    command = " DELETE FROM " + table_name + " WHERE db_entry <= " + int_to_string(in_number_[index]);
    execute_transaction(command, false); 

    in_number_[index] ++;

    db_entry = in_number_[index];

    // check if this entry exists
    command = " SELECT * FROM " + table_name + " WHERE db_entry = " + int_to_string(db_entry);
    the_db_entry = execute_nontransaction(command, false);

    nextgood = (the_db_entry.size() && the_db_entry.begin()->size() >= N_BOARD_TABLE_COLUMNS);
    if ( !nextgood ){
      clog << " selector needs db_entry " << db_entry << " but this entry or the next are not there yet " << endl;
      running=true;
      while(running){
	running=is_program_running(asker.c_str(), commandname.c_str());
	
	the_db_entry = execute_nontransaction(command, false);
	good = (the_db_entry.size() && the_db_entry.begin()->size() >= N_BOARD_TABLE_COLUMNS);
	if( good ) break;
      }
      
      if( !running ){
        clog << asker << " will stop (reading next event from new packet) " << endl; fflush(stdout);
	// count rows in table 
	if( check_if_table_exists( "coincidences" ) ){
           command = "SELECT COUNT(*) FROM coincidences "; the_db_entry = execute_nontransaction(command, false);
	   std::clog << " filled table coincidences with " << the_db_entry[0][0].as<int>() << " entries " << std::endl; fflush(stdout);
        }
	return false;
      }
  }

    std::vector<size_t> channels;
    string channel_name;
    
    size_t local_nchs = the_db_entry[0]["nchs"].as<size_t>();
    for(size_t i=0; i<local_nchs; i++){
	channel_name = "ch" + int_to_string(i);
	channels.push_back(std::stoi(the_db_entry[0][channel_name.c_str()].as<std::string>()));
    }
    
    double time = the_db_entry[0]["time"].as<double>();
    double packet_number = the_db_entry[0]["packet_number"].as<double>();
    string board_address = get_board_address_from_binary(the_db_entry[0]["board_address"].as<string>());
    Event e(channels, time, packet_number, board_address);
    
    e.set_analyzed(true);
    es_[index]=e;

    return true;

  }


  void set_event_number(size_t ievent, size_t eventnumber){
    if( ievent >= N_ ){
      clog << " problem: cannot set event number for event " << ievent << " only " << N_ << " events are expected " << endl;
      exit(0);
    }

#if STORE_RAW
    actual_event_[ievent]->set_event_number(eventnumber);
#else
    es_[ievent].set_event_number(eventnumber);
#endif

    return;
  }

  void set_event_written(size_t ievent, bool written){
    if( ievent >= N_ ){
      clog << " problem: cannot set event written event " << ievent << " only " << N_ << " events are expected " << endl;
      exit(0);
    }

#if STORE_RAW
    actual_event_[ievent]->set_written(written);
#else
    es_[ievent].set_written(written);
#endif

    return;
  }


  void set_event_type(size_t ievent, string type){
    if( ievent >= N_ ){
      clog << " problem: cannot set type for event " << ievent << " only " << N_ << " events are expected " << endl;
      exit(0);
    }

#if STORE_RAW
    actual_event_[ievent]->set_type(type);
#else
    es_[ievent].set_type(type);
#endif

    return;
  }


  bool board_has_channel(size_t iboard, size_t ichannel, bool print=false){

    if( !get_actual_event(iboard).is_channel_on(ichannel ) ){
      if( print ){
	//	clog << " channel " << ichannel << " of board " << iboard << " is not on " << endl;
      }
      
      return false;
    }

    if( print ){
      //      clog << " channel " << ichannel << " of board " << iboard << " is on ";
    }
    return true;

  }


  void wait ( double seconds )
  {
    clock_t endwait;
    endwait = clock () + (clock_t)(seconds * 1e6) ;
    while (clock() < endwait) {}
  }

  bool board_has_coincidence(size_t iboard, double tzero, bool print=false){

    if( fabs(get_actual_event(iboard).time("s") - tzero) >= window_ ){
      if( print ){
	clog << " time " << get_actual_event(iboard).time("s") << " incompatible with reference time " << tzero << endl;
      }
      return false;
    }
    
    if( print ){
      clog << "; time " << get_actual_event(iboard).time("s") << " compatible with reference time " << tzero << endl;
    }
    return true;
  }



  bool is_program_running(string asker, string commandname){
    // Check if process is running via command-line
    
    //    clog << asker << " is checking if " << commandname << " is running " << endl;
    
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
      //      clog << commandname << " is running, so " << asker << " will hold on " << endl; fflush(stdout);
      return true;
    }
    
    return false;
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

  
  string integer_to_binary(unsigned n){
    
    const size_t size = sizeof(n) * 8;
    char res[size];
    unsigned ind = size;
    do{
      res[--ind] = '0' + (n & 1);
    } while (n >>= 1);
    
  return std::string(res + ind, res + size);
  
  }
  
  string get_board_address(size_t i){
    
    string extra = integer_to_binary(i);
    return get_board_address_from_binary(extra);

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

  string get_board_address_from_binary(string bin){
    
    bin.erase(bin.find_last_not_of(' ')+1);         //surfixing spaces

    string address="";
    for(size_t j=0; j<(NUM_BITS_BOARD_ADDRESS-bin.size()); j++)
      address += "0";

    return address+bin;
  }
  
};

#endif

