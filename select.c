
#include "usbfunctions.h"
#include "Word.h"
#include "Event.h"
#include "Packet.h"
#include "Level.h"
#include "Signal.h"
#include <algorithm>

using namespace std;

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

size_t count_layers_of_events(vector<Event> selected_events, bool print);


int main(int argc, char *argv[]){

  // read parameters
  double window = read_parameter_from_file("params.inp","window");
  size_t n_coincidences = (size_t)(read_parameter_from_file("params.inp","n_coincidences")+0.5);
  size_t n_layers_per_event = (size_t)(read_parameter_from_file("params.inp","n_layers_per_event")+0.5);
  bool print = (bool)(read_parameter_from_file("params.inp","print")); // print verbose
  int NUM_BOARDS = (int)(read_parameter_from_file("data_params.txt", "NUM_BOARDS")+0.5);
  size_t n_coincidence_events_per_file = (size_t)(read_parameter_from_file("params.inp","n_coincidence_events_per_file")+0.5);

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
  Level asticella(NUM_BOARDS, window);
  for(int iii=0;iii<possible_board_address.size();iii++){
    clog << " possible_board_address(es): " << possible_board_address.at(iii) << std::endl;
  }

  asticella.open_data_files(possible_board_address);
  asticella.recalculate_minima();
  asticella.set_time_to_minimum();
  if( print ) {
    clog << " first asticella: "; asticella.dump();
  }

  double initial_time = asticella.minimum_time();

  // open output data file
  size_t filenumber=0;
  string sfilenumber=asticella.get_file_number(filenumber);
  //std::clog << "sfilenumber: " << sfilenumber << std::endl;
  ofstream data_file;
  string fname = "coincidence_data_"+sfilenumber+".txt";
  data_file.open(fname.c_str(), ios_base::out);


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

  while( true ){

    n_input_signals ++;

    if( selected ){ // if the previous N-event was written
      // read a new event from each board and restart acquisition
      asticella.renew();
    }else{ // if 1st run or the previous N-event was not written
      // increase the level time by reading a new event in the earliest of boards
      asticella.increase_the_time();
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
      size_t n_layers = count_layers_of_events(selected_events, print);

      // the C-event is selected if it has the correct number of layers
      selected = ( n_layers >=  n_layers_per_event );


      if( selected ){// if the C-event is selected, we write it out
	
	// write all events in the C-event
	for(vector<Event>::iterator iev=selected_events.begin(); iev!=selected_events.end(); ++iev){
	  if( print ){
	    clog << " writing signal: ";  iev->dump();
	  }
	  iev->Write(data_file, n_layers);
	}
	
	// count written C-events
	nevents ++;
	i_file ++;

	  
      } // C-event has been written
    

#if 0
      //////////////////////////////////////
      /// see if the C-event is new ///
      //////////////////////////////////////
      // - if one of the boards in coincidence has the actual event already written, the C-event is old
      // - if instead none of the actual events has been written, the C-event is new
      // also, all events that are not written are now set as written
      
      // print the event
      if( print ){
	std::clog << " event has boards : ";
	for(vector<size_t>::const_iterator ib=indexes.begin(); ib!=indexes.end(); ++ib)
	  std::clog << " " << *ib;
	std::clog << " " << std::endl;
      }
      
      
      bool newevent=true;
      for(vector<size_t>::iterator ii=indexes.begin(); ii!=indexes.end(); ++ii){ // loop on boards in coincidence
	
	if( print )
	  clog << " board " << *ii << " written " << asticella.get_actual_event(*ii).written() << " time " << asticella.get_actual_event(*ii).time() << " min " << asticella.minimum_time() << endl;
	
	if( asticella.get_actual_event(*ii).written() ){
	  newevent=false;
	}else{
	  asticella.set_event_written(*ii, true);
	}
	asticella.set_event_number(*ii, nevents);
	//asticella.get_actual_event(*ii).Write(data_file);
      }
      

      // if the C-event is new, see how many layers it has
      if( newevent ){
	  
	// count number of layers in the N-event
	size_t n_layers = count_layers_of_events(selected_events, print);
	  
	if( print ){
	  std::clog << " n_layers: " << n_layers << " newevent " << newevent << std::endl;
	}

	// if the C-event has the required number of layers,
	// we write all of its events
	if( n_layers >=  n_layers_per_event ){

	  for(vector<Event>::iterator iev=selected_events.begin(); iev!=selected_events.end(); ++iev){
	    if( print ){
	      clog << " writing signal: ";  iev->dump();
	    }
	    iev->Write(data_file, n_layers);
	  }
	}

	// reset the list of selected events that make a single C-event
	selected_events.clear();
	nevents ++;
	for(vector<size_t>::iterator ii=indexes.begin(); ii!=indexes.end(); ++ii){
	  asticella.set_event_number(*ii, nevents);
	}
	  
      }


      for(vector<size_t>::iterator ii=indexes.begin(); ii!=indexes.end(); ++ii){
	selected_events.push_back(asticella.get_actual_event(*ii));
	if( print )
	  std::clog << " adding event from board " << *ii << " n of events = " << selected_events.size() << std::endl;
      }

      if( newevent == 0 && delay == 0){
	// if we are going to look for delay, don't increase ev number yet
	//std::clog << "increment i_file" << std::endl;
	i_file ++;
      }
      
      
      if( delay != 0. ){
	if( print )
	  clog << " delay: " << delay << std::endl;
	// open window of time delay and store 1st signal in window
	double tzero=asticella.time();
	while( asticella.increase_the_time() && 
	       asticella.time() < tzero+delay ){
	  size_t minindex = asticella.minimum_index();
	  if( !asticella.get_actual_event(minindex).written() ){
	    asticella.set_event_written(minindex, true);
	    asticella.set_event_type(minindex, "SLOW");
	    asticella.set_event_number(minindex, nevents);
	    if( print ){
	      clog << " writing slow signal: dt " << asticella.get_actual_event(minindex).time("s") - tzero << " ";  asticella.get_actual_event(minindex).dump();
	    }
	    asticella.get_actual_event(minindex).Write(data_file, 0);
	    break;
	  }
	}
	nevents ++;
	i_file ++;
      }
#endif

      
    } // N-event has been selected
    
      
    if( i_file >= n_coincidence_events_per_file ){
      clog << " selector closing file " << fname << endl;
      data_file.close();
      filenumber ++;
      sfilenumber=asticella.get_file_number(filenumber);
      fname = "coincidence_data_"+sfilenumber+".txt";
      data_file.open(fname.c_str());
      i_file = 0;
    }

    //std::clog << " ninputs " << n_input_signals << " ifile " << i_file << " ok2 " << ok << std::endl;

  } // infinite loop


  asticella.close();

  data_file.close();
  
  clog << " n of read signals: " << n_input_signals << endl;
  clog << " n of selected macro-events: " << nevents << endl;
  
  exit(0);

  
  return 1;
}

void read_detector(string filename){

  FILE * pFile = fopen (filename.c_str(),"r");

  int c;
  int x1, x2, x3, x4, x5, x6, x7, x8;
  int y1, y2, y3, y4, y5, y6, y7, y8;

  while( EOF != fscanf(pFile, "%d %d %d %d %d %d %d %d %d", &c, &x1, &y1, &x2, &y2, &x3, &y3, &x4, &y4) && EOF != fscanf(pFile, "%d %d %d %d %d %d %d %d", &x5, &y5, &x6, &y6, &x7, &y7, &x8, &y8))
 {

    expected_channels.push_back(c);
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
    cout << " problem: detector sizes " << expected_channels.size() << " " << expected_channels_x1.size() << " " << expected_channels_y2.size() << 
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

