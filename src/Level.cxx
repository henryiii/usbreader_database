#include "Level.h"

void Level::init(Packet* stored_packets,size_t n){

  std::clog << " qqq init " << n << std::endl; fflush(stdout);

  for(size_t i =0 ;i<n;++i){
    assign_packet(i,stored_packets[i]);
   
  }
  
  std::clog << " qqq assign " << n << std::endl; fflush(stdout);

  for(size_t i = 0 ; i<N_; ++i){
    actual_event_[i] = packets_[i].events_.begin();
   
  }

  std::clog << " qqq recalc " << n << std::endl; fflush(stdout);

  recalculate_minima();
  set_time_to_minimum();
  
  std::clog << " qqq done " << n << std::endl; fflush(stdout);

}

void Level::get_earliest_board(){
  //find board with the lowest time stamp
    
  double time_min = FLT_MAX;
  double time_local;
  size_t imin;
    
  for(size_t i = 0 ; i < N_; ++i){

    time_local = actual_event_[i]->time("s");
      
    if(time_local < time_min){
      time_min = time_local;
      imin = i;
    }
      
  }
  
  minimum_index_ = imin;
  minimum_t_     = time_min;
  
  
}

void Level::assign_packet(size_t i, Packet p){
  packets_[i] = p;
  packets_[i].packet_to_physical_parameters();
}

std::string Level::read_from_db(std::string board,int event){
  return "SELECT word FROM board_" + board + " WHERE key=" + std::to_string(event);
}

