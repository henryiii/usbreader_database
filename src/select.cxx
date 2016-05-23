
#include "usbfunctions.h"
#include "Word.h"
#include "Event.h"
#include "Packet.h"
#include "Level.h"
#include "Signal.h"
#include <algorithm>
#include <stdio.h>
#include <ios>
#include <pqxx/pqxx>

using namespace std;

static const int MAX_CHANNELS_NUMBER=32;

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

void read_detector(string filename);
string int_to_string(size_t i);
string double_to_string(double i);
size_t count_layers_of_events(vector<Event> selected_events, bool print);

pqxx::connection * c;

pqxx::result execute_nontransaction(string sql, bool print);  
pqxx::result execute_transaction(string sql, bool print);
bool check_if_table_exists( std::string table_name );

void fill_channel_names();
string channel_names[MAX_CHANNELS_NUMBER];
string list_of_channel_names(size_t n);
string list_of_channel_names_and_types;

void trigger();

int main(int argc, char *argv[]){

  int modulo = 1;
  
  // read parameters
  double window = read_parameter_from_file("params.inp","window");
  size_t n_coincidences = (size_t)(read_parameter_from_file("params.inp","n_coincidences")+0.5);
  size_t n_layers_per_event = (size_t)(read_parameter_from_file("params.inp","n_layers_per_event")+0.5);
  bool print = (bool)(read_parameter_from_file("params.inp","print")); // print verbose
  bool emit_trigger = (bool)(read_parameter_from_file("params.inp","emit_trigger")); // emit_trigger verbose
  int NUM_BOARDS = (int)(read_parameter_from_file("data_params.txt", "NUM_BOARDS")+0.5);

  std::clog << " selection parameters " << std::endl;
  std::clog << " window " << window << std::endl;
  std::clog << " n_coincidences " << n_coincidences << std::endl;
  std::clog << " n_layers_per_event " << n_layers_per_event << std::endl;
  std::clog << " emit_trigger " << emit_trigger << std::endl;
  std::clog << " STORE_RAW " << STORE_RAW << std::endl;fflush(stdout);

  string string_dbname = read_string_from_file("params.inp","dbname");
  string string_host = read_string_from_file("params.inp","host");
  string string_user = read_string_from_file("params.inp","user");
  string string_password = read_string_from_file("params.inp","password");

  string string_connection = "user=" + string_user + " host=" + string_host + " password=" + string_password + " dbname=" + string_dbname;
  c = new pqxx::connection(string_connection.c_str());

  ofstream data_params_file;
  data_params_file.open("data_params.txt", ios_base::app);
  data_params_file << "window " << window << endl;
  data_params_file << "n_coincidences " << n_coincidences << endl;
  data_params_file << "n_layers_per_event " << n_layers_per_event << endl;
  data_params_file.close();

  // load detector shape
  read_detector("detector.inp");


  bool single_board = false;
  if( NUM_BOARDS < 2 ){
    single_board = true;
    clog << " allowing only " << NUM_BOARDS << " board, single = " << single_board << endl;
  }


  // get possible board addresses
  vector<string> possible_board_address = get_possible_board_addresses(NUM_BOARDS);


  // initialize asticella
  clog << "initialize asticella" << std::endl;
  Level asticella(NUM_BOARDS, window, string_connection);
  for(int iii=0;iii<possible_board_address.size();iii++){
    clog << " possible_board_address(es): " << possible_board_address.at(iii) << std::endl;
  }

  asticella.open_db_tables(possible_board_address);
  asticella.recalculate_minima();
  asticella.set_time_to_minimum();
  if( print ) {
    clog << " first asticella: "; asticella.dump();
  }

  double initial_time = asticella.minimum_time();


  std::string command;
  std::string table_name="coincidences";
  pqxx::result the_db_entry;

  // check if board table exists
  if( check_if_table_exists( table_name ) ){

    // count rows in table
    command = "SELECT COUNT(*) FROM " + table_name ; the_db_entry = execute_nontransaction(command, false);
    std::clog << " goind to delete table " << table_name << " with " << the_db_entry[0][0].as<int>() << " entries " << std::endl; fflush(stdout);
    
    // delete coincidence table from database
    command = "DROP TABLE " + table_name; execute_nontransaction(command, false);
  }

  fill_channel_names();

  // create coincidence table
  command = "CREATE TABLE " + table_name + " ("			\
    "db_entry    int NOT NULL PRIMARY KEY,"						\
    "event_number     int,"					\
    "board_address    int,"						\
    "time             decimal,"						\
    "n_layers         int,"						\
    "nchs             int,"
    + list_of_channel_names_and_types
    + ");"; // create table

  execute_nontransaction(command, false);


  // loop on data and write coincidences
  size_t n_input_signals = 0;
  size_t nevents = 0;
  size_t i_file = 0;
  vector<size_t> indexes;
  vector<Event> selected_events;

  // Event = one event from one board
  // N-event = the set of N events in the level right now (one per board)
  // C-event = the set of events in coincidence

  bool selected = false; // initialize as false
  size_t local_number_of_coincidences;
  size_t n_layers = 0;

  size_t ip = 0;
  string string_db_entry;
  string string_packet_number;
  string string_event_number;
  string string_board_address;
  string string_time;
  string string_n_layers;
  string string_nchs;
  string string_chs;

  std::clog << " selector now looping over database " << std::endl; fflush(stdout);

  while( true ){

    n_input_signals ++;

    if( selected ){ // if the previous N-event was written
      // read a new event from each board and restart acquisition
      if( !asticella.renew() ) break;
    }else{ // if 1st run or the previous N-event was not written
      // increase the level time by reading a new event in the earliest of boards
      if( !asticella.increase_the_time()) break;
    }

    // reset the indexes of boards in coincidence
    indexes.clear();
    
    //////////////////////////////////////
    /// see if the N-event is selected ///
    //////////////////////////////////////
    // the N-event is selected if it has the required number of boards in coincidence
    
    selected=false;
    if( single_board ){
      selected = true;
      indexes.push_back(0);
    }else{

      // " indexes " is filled with the indexes of boards in coincidence
      local_number_of_coincidences = asticella.n_coincidences(&indexes, false); // boolean is to print
      selected = ( local_number_of_coincidences >= n_coincidences );
	
    }
    
    if( selected ){ // if the N-event is selected, we have a C-event
      
      // print the event
      if( print ){
	std::clog << " asticella : "; asticella.dump();
      }

      // reset the list of selected events that make a single C-event
      selected_events.clear();
      // create the C-event
      for(vector<size_t>::iterator ii=indexes.begin(); ii!=indexes.end(); ++ii){
	asticella.set_event_written(*ii, true);
	asticella.set_event_number(*ii, nevents);
	selected_events.push_back(asticella.get_actual_event(*ii));
      }

      // count number of layers in the C-event
      n_layers = count_layers_of_events(selected_events, print);

      // the C-event is selected if it has the correct number of layers
      selected = ( n_layers >=  n_layers_per_event );
      
    
      if( selected ){// if the C-event is selected, we write it out
	
	if( emit_trigger ){
	  // send trigger signal
	  trigger();
	}

	string_n_layers = int_to_string(n_layers);
	n_layers = 0;

	// write all events in the C-event
	for(vector<Event>::iterator iev=selected_events.begin(); iev!=selected_events.end(); ++iev){
	  if( print ){
	    clog << " writing signal: ";  iev->dump();
	  }
	  
	  // insert row in board table
	  string_event_number = int_to_string(iev->event_number());
	  string_time = double_to_string(iev->time("s"));
	  string_board_address = iev->board_address();
	  
	  vector<size_t> channels = iev->channels();
	  
	  string_chs = "";
	  if( channels.size() ){
	    for(size_t u=0; u<channels.size() - 1; u++)
	      string_chs += pqxx::to_string(channels[u]) + ", ";
	    string_chs += pqxx::to_string(channels[channels.size() - 1]);
	  }

	  string_nchs = int_to_string(channels.size());

	  string_db_entry = int_to_string(ip);
	  
	  ip ++;
	  
	  command = "INSERT INTO " + table_name + " (db_entry, event_number, board_address, time, n_layers, nchs, " + list_of_channel_names(channels.size()) + ") " \
	    "VALUES (" + string_db_entry + ", " + string_event_number + ", " + string_board_address + ", " + string_time + ", " + string_n_layers + ", " + string_nchs + ", " + string_chs + "); ";
	  execute_transaction(command, false); // insert table

	  if( ip % modulo == 0 ){
	    std::clog << " board " << string_board_address << " ip " << ip << " evn " << string_event_number << " time " << string_time << " n_layers " << string_n_layers << " nchs " << string_nchs << " chs " << string_chs <<  std::endl; fflush(stdout);
	  }

	}
	
	// count written C-events
	nevents ++;
	i_file ++;
	
	
      } // C-event has been written
      
      
    } // N-event has been selected
    
      
    //std::clog << " ninputs " << n_input_signals << " ifile " << i_file << " ok2 " << ok << std::endl;

  } // infinite loop


  asticella.close();

  clog << " n of read signals: " << n_input_signals << endl;
  clog << " n of selected macro-events: " << nevents << endl; fflush(stdout);
  // count rows in table 
  if( check_if_table_exists( table_name ) ){
    command = "SELECT COUNT(*) FROM " + table_name ; the_db_entry = execute_nontransaction(command, false);
    clog << " n of selected signals: " << the_db_entry[0][0].as<int>() << endl; fflush(stdout);
  }


  exit(0);

  
  return 1;
}

void read_detector(string filename){

  FILE * pFile = fopen (filename.c_str(),"r");

  int cc;
  int x1, x2, x3, x4, x5, x6, x7, x8;
  int y1, y2, y3, y4, y5, y6, y7, y8;

  while( EOF != fscanf(pFile, "%d %d %d %d %d %d %d %d %d", &cc, &x1, &y1, &x2, &y2, &x3, &y3, &x4, &y4) && EOF != fscanf(pFile, "%d %d %d %d %d %d %d %d", &x5, &y5, &x6, &y6, &x7, &y7, &x8, &y8))
 {

    expected_channels.push_back(cc);
    expected_channels_x1.push_back((size_t)x1);
    expected_channels_y1.push_back((size_t)y1);
    expected_channels_x2.push_back((size_t)x2);
    expected_channels_y2.push_back((size_t)y2);
    expected_channels_x3.push_back((size_t)x3);   
    expected_channels_y3.push_back((size_t)y3);   
    expected_channels_x4.push_back((size_t)x4);   
    expected_channels_y4.push_back((size_t)y4);   
    expected_channels_x5.push_back((size_t)x5);
    expected_channels_y5.push_back((size_t)y5);
    expected_channels_x6.push_back((size_t)x6);
    expected_channels_y6.push_back((size_t)y6);
    expected_channels_x7.push_back((size_t)x7);
    expected_channels_y7.push_back((size_t)y7);
    expected_channels_x8.push_back((size_t)x8);
    expected_channels_y8.push_back((size_t)y8);

  }

  fclose(pFile);

#if 0
  std::clog << " channel, (x1, y1), (x2, y2), ... " << std::endl;
  for( vector<size_t>::iterator j = expected_channels.begin(); j != expected_channels.end(); ++j ){
    size_t index = j - expected_channels.begin();
    std::clog << * j 
	      << ", (" << expected_channels_x1.at(index) << ", " << expected_channels_y1.at(index) 
	      << "), (" << expected_channels_x2.at(index) << ", " << expected_channels_y2.at(index) 
	      << "), (" << expected_channels_x3.at(index) << ", " << expected_channels_y3.at(index) 
	      << "), (" << expected_channels_x4.at(index) << ", " << expected_channels_y4.at(index) 
	      << "), (" << expected_channels_x5.at(index) << ", " << expected_channels_y5.at(index) 
	      << "), (" << expected_channels_x6.at(index) << ", " << expected_channels_y6.at(index) 
	      << "), (" << expected_channels_x7.at(index) << ", " << expected_channels_y7.at(index) 
	      << "), (" << expected_channels_x8.at(index) << ", " << expected_channels_y8.at(index) 
	      << ")" << std::endl;
  }
#endif

  if( expected_channels.size() != expected_channels_x1.size() ||
      expected_channels.size() != expected_channels_y1.size() ||
      expected_channels.size() != expected_channels_x2.size() ||
      expected_channels.size() != expected_channels_y2.size() ||
      expected_channels.size() != expected_channels_x3.size() ||
      expected_channels.size() != expected_channels_y3.size() ||
      expected_channels.size() != expected_channels_x4.size() ||
      expected_channels.size() != expected_channels_y4.size() ||
      expected_channels.size() != expected_channels_x5.size() ||
      expected_channels.size() != expected_channels_y5.size() ||
      expected_channels.size() != expected_channels_x6.size() ||
      expected_channels.size() != expected_channels_x7.size() ||
      expected_channels.size() != expected_channels_y7.size() ||
      expected_channels.size() != expected_channels_x8.size() ||
      expected_channels.size() != expected_channels_y8.size()
 ){
    clog << " problem: detector sizes " << expected_channels.size() << " " << expected_channels_x1.size() << " " << expected_channels_y2.size() << 
      " " << expected_channels_x3.size() << " " << expected_channels_y3.size() << 
      " " << expected_channels_x4.size() << " " << expected_channels_y4.size() <<
      " " << expected_channels_x5.size() << " " << expected_channels_y5.size() <<
      " " << expected_channels_x6.size() << " " << expected_channels_y6.size() <<
      " " << expected_channels_x7.size() << " " << expected_channels_y7.size() <<
      " " << expected_channels_x8.size() << " " << expected_channels_y8.size() <<
      endl;
    exit(0);
  }


  return;
}

size_t count_layers_of_events(vector<Event> selected_events, bool print){


  
  vector<size_t> found_ys;
  vector<size_t> found_channels;
  vector<size_t> found_channels_without_offset;

  size_t offset_; // offset channel number from 0-31 to (32*N - 32*N+31), where N is the board index

  // loop over selected_events
  for(vector<Event>::iterator i = selected_events.begin(); i != selected_events.end(); ++i){

    Event e = *i;

    if( !strcmp(e.board_address().c_str(),"000000") ) offset_ = 0;
    else if( !strcmp(e.board_address().c_str(),"000001") ) offset_ = 1;
    else if( !strcmp(e.board_address().c_str(),"000010") ) offset_ = 2;
    else if( !strcmp(e.board_address().c_str(),"000011") ) offset_ = 3;
    else{
      std::clog << " problem: select: cannot assign offset to board address " << e.board_address() << std::endl;
      exit(0);
    }

    std::vector<size_t> ch = e.channels();

    // loop over channels in that event
    for(vector<size_t>::iterator j = ch.begin(); j != ch.end(); ++j ){

      size_t channel = *j + offset_*NUM_CHANNELS;

      // get index of channel
      size_t index = std::find(expected_channels.begin(), expected_channels.end(),channel) - expected_channels.begin();


      // check y coordinate of the channel

      size_t local_y = expected_channels_y1[index];

      if( std::find(found_ys.begin(), found_ys.end(), local_y) == found_ys.end() ){
	found_channels.push_back(channel);
	found_channels_without_offset.push_back(channel - offset_*NUM_CHANNELS);
	found_ys.push_back(local_y);
      }

    }

  }

  if( print ){
    std::clog << " found " << found_channels.size() << " channels, original channel and ys: " ;
    for(vector<size_t>::iterator j = found_channels.begin(); j != found_channels.end(); ++j ){
      std::clog << "(" << *j << ", " << found_channels_without_offset[j - found_channels.begin()]  << ", " << found_ys[j - found_channels.begin()] << ") ";
    }
    std::clog << " " << std::endl;
  }


  return found_ys.size();

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

string int_to_string(size_t i){
  char cc[20];
  sprintf(cc,"%d",(int)i);
  string sval(cc);
  return sval;
}

string double_to_string(double i){

  std::ostringstream out;
  out << std::setprecision(20) << i;
  return out.str();
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

  list_of_channel_names_and_types = "";
  for(size_t i=0; i<MAX_CHANNELS_NUMBER-1; i++){
    channel_names[i] = "ch" + int_to_string(i);
    list_of_channel_names_and_types += channel_names[i] + " int,";
  }

  channel_names[MAX_CHANNELS_NUMBER - 1] = "ch" + int_to_string(MAX_CHANNELS_NUMBER - 1);
  list_of_channel_names_and_types += channel_names[MAX_CHANNELS_NUMBER - 1] + " int";

}


string list_of_channel_names(size_t n){

  string   list = "";
  for(size_t i=0; i<n-1; i++){
    list += channel_names[i] + ", ";
  }

  list += channel_names[n - 1];
  return list;

}

void trigger(){

  std::clog << " execute trigger " << std::endl; fflush(stdout);
  system("./trigger.sh");
  std::clog << " executed trigger " << std::endl; fflush(stdout);

}

