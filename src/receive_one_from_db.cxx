/* -*- mode: c++ -*- */
#include <stdio.h>
#include "usbfunctions.h"
#include "Word.h"
#include "Event.h"
#include "Packet.h"
#include "Level.h"
#include <algorithm>
#include <ios>
#include <pqxx/pqxx>

using namespace std;

unsigned int MAX_NUM_PACKETS = 1000;
string board_address;

string get_file_number(size_t i);
string int_to_string(size_t i);
void example();
void prepare_raw_insert(pqxx::connection_base &c,std::string table) ;
pqxx::result execute_insert(pqxx::transaction_base &t, std::string binary, std::string table) ;

void execute_transaction(string sql);
void execute_nontransaction(char * sql);


//connect to the database
// dbname = "boards"
// host = 127.0.0.1
// user = "postgres"
// password = f

pqxx::connection c("user=postgres host=127.0.0.1 password=f dbname=boards");


int main(int argc, char *argv[]){
 
  if( !c.is_open() ){
    std::clog << " the connection to database is not open; quit " << std::endl;
    exit(0);
  }

  std::clog << " the connection to database " << c.dbname() << " is open " << std::endl;

  if( argc <= 2 ){
    std::clog << " please provide device number and n of events " << endl; fflush(stdout);
    exit(0);
  }
 
  string devicename=argv[1];
  MAX_NUM_PACKETS=atoi(argv[2]);
  
  // initialize parameters
  unsigned int timeout = (unsigned int)read_parameter_from_file("params.inp","timeout");
  bool print = (bool)(read_parameter_from_file("params.inp","print"));
  // print verbositys  size_t n_packets_to_read_together = (size_t)(read_parameter_from_file("params.inp","n_packets_to_read_together")+0.5);
  board_address = read_address_from_file("data_params.txt",devicename);
  size_t n_packets_per_file = (size_t)(read_parameter_from_file("params.inp","n_packets_per_file")+0.5);
  size_t n_packets_to_read_together = (size_t)(read_parameter_from_file("params.inp","n_packets_to_read_together")+0.5);
  size_t ip = 0;

  // read all usb devices and get cypress ones
  static vector<libusb_device_handle*> handles;
  handles = retrieve_usb_devices();
  if( handles.size() == 0 ){
    std::clog << " problem: no usb devices " << endl; fflush(stdout);
    exit(0);
  }

  std::clog << " receive: found " << handles.size() << " devices, look for board " << board_address << endl; fflush(stdout);

  // read one packet from each board and find board with chosen address
  bool found = false;
  size_t index_of_board_with_good_address;
  static libusb_device_handle* handle;
  for(size_t i=0; i<handles.size(); i++){
    Packet local_packet;
    bool this_ok = read_packet_from_board(handles[i], timeout, &local_packet);
    if( !this_ok ) break;
    if( local_packet.board_address() == board_address ){
      found = true;
      handle = handles[i];
      index_of_board_with_good_address = i;
      break;
    }
  }
  if( !found ){
    std::clog << " cannot find board with address " << board_address << " quitting " << endl; fflush(stdout);
    exit(0);
  }

  for(vector<libusb_device_handle*>::iterator ih = handles.begin(); ih!=handles.end(); ++ih){
    if( ih - handles.begin() != index_of_board_with_good_address ){
      libusb_release_interface(*ih,0);
      libusb_close(*ih);
    }
  }

  std::clog << " receive: freed devices for board " << board_address << endl; fflush(stdout);

  //execute_nontransaction("select * from master.dbo.sysdatabases"); // list all databases
  //execute_nontransaction("SELECT * FROM information_schema.tables where TABLE_TYPE = 'BASE TABLE'"); // list all tables

  //execute_nontransaction("SELECT Distinct TABLE_NAME FROM information_schema.TABLES where TABLE_TYPE = 'BASE TABLE'"); // list all tables
  //execute_nontransaction("select * from INFORMATION_SCHEMA.COLUMNS where TABLE_NAME='board_000000' "); // list all tables
  //execute_nontransaction("DROP TABLE board_000000"); // delete b0
  //execute_nontransaction("select * from INFORMATION_SCHEMA.COLUMNS where TABLE_NAME='board_000000' "); // list all tables


  std::string command = "CREATE TABLE board_" + board_address + "(key serial primary key, word text not null)";
  execute_transaction(command);

		      /*
  execute_transaction("CREATE TABLE COMPANY("	\
		      "ID INT PRIMARY KEY     NOT NULL,"	\
		      "NAME           TEXT    NOT NULL,"	\
		      "AGE            INT     NOT NULL,"	\
		      "ADDRESS        CHAR(50),"			\
		      "SALARY         REAL );"); // create table
  execute_transaction("INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY) "	\
		      "VALUES (1, 'Paul', 32, 'California', 20000.00 ); " \
		      "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY) " \
		      "VALUES (2, 'Allen', 25, 'Texas', 15000.00 ); "	\
		      "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY)" \
		      "VALUES (3, 'Teddy', 23, 'Norway', 20000.00 );"	\
		      "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY)" \
		      "VALUES (4, 'Mark', 25, 'Rich-Mond ', 65000.00 );"); // insert table
  execute_transaction("SELECT * from COMPANY"); // select table
		      */

  std::clog << " will collect " << MAX_NUM_PACKETS << " packets, timeout = " << timeout << ", n of boards: " << handles.size() << ", n of packets to read together = " << n_packets_to_read_together << " from board " << board_address << " write " << n_packets_per_file << " packets per file " << endl; fflush(stdout);
  
  
  // collect and write events
  size_t i_together=0;
  static unsigned int nrt = n_packets_to_read_together;
  std::string stored_packets[nrt];
  


  prepare_raw_insert(c,board_address);

  //this is the main loop
  std::string local_packet;
  while( ip < MAX_NUM_PACKETS ){
    local_packet.clear();
    bool this_ok = read_hex_from_board(handle, timeout, &local_packet);
    
    if( !this_ok ) break;
    //stored_packets[i_together] = local_packet; //STORING RAW HEX FAILED (in writing to DB) PLEASE FIX, conversion to binary may be slow!!!!!!!!
    //ERROR:  invalid byte sequence for encoding "UTF8"
    stored_packets[i_together] = db_hex_to_binary(512,(char*)local_packet.c_str());
    //Packet *p = new Packet(db_hex_to_binary(512,(char*)local_packet.c_str()));
    //p->packet_to_physical_parameters();
    //std::clog << "packnum: " << std::setprecision(12) << p->packet_number() << std::endl;
    ip++;
    i_together++;
    
    if( i_together == n_packets_to_read_together ){
      // database transaction object
      pqxx::work txn(c); 
      i_together=0;
      
      for(size_t i = 0 ;i<nrt;++i){
	// database result
	pqxx::result r = execute_insert(txn, stored_packets[i], board_address);
	stored_packets[i]="";
      } 
      // database make work final
      txn.commit();
    }
    
  }
  
 
  std::clog << "closing psql database" << std::endl; fflush(stdout);
  std::clog << " collected " << ip << " packets from board " << board_address  << std::endl; fflush(stdout);
  std::exit(0);
  
  return 1;
}


std::string int_to_string(size_t i){
  char c[20];
  sprintf(c,"%d",(int)i);
  string sval(c);
  return sval;
}

std::string get_file_number(size_t i){

  size_t limit = 6;
  string numb=int_to_string(i);
  if( numb.size() > limit ){
    std::clog << " problem: file number " << i << " exceeds limit " << endl; fflush(stdout);
    exit(0);
  }
  size_t n_extra_zeros=6-numb.size();
  string output="";
  for(size_t i=0; i<n_extra_zeros; i++)
    output += '0';
  return output+numb;

}

void prepare_raw_insert(pqxx::connection_base &c,std::string table) {
  std::clog << " qqq receive: prepare raw insert " << std::endl; fflush(stdout);
  const std::string sql = "INSERT into board_" + table + " (word) VALUES ($1)";
  const std::string nme = "binary_insert" + table;
  c.prepare(nme, sql)/*("text", pqxx::prepare::treat_string)*/;
  std::clog << " qqq receive: prepare raw insert: done " << std::endl; fflush(stdout);
}

pqxx::result execute_insert(pqxx::transaction_base &t, std::string binary, std::string table) {
  const std::string nme = "binary_insert" + table;
  // database execute string 
  return t.prepared(nme)(pqxx::to_string(binary)).exec();
}



void example(){

  pqxx::work xxx(c); 
  std::stringstream ctable;
  
  // create table 
  ctable << "CREATE TABLE board_" << board_address << "(key serial primary key, word text not null)";

  //ctable << "CREATE TABLE Persons(PersonID int,LastName varchar(255),FirstName varchar(255),Address varchar(255),City varchar(255))"; 

  std::clog << " receive: ctable " << (ctable.str()).c_str() << endl; fflush(stdout);

#if 1
  // database execute string 
  xxx.exec((ctable.str()).c_str());
#endif

  std::clog << " receive: exec " << endl; fflush(stdout);

  // database make work final
  xxx.commit();

}


void execute_nontransaction(char * sql){

  std::clog << " executing nontransaction: " << sql << std::endl; fflush(stdout);

  /* Create a non-transactional object. */
  pqxx::nontransaction N(c);
  
  /* Execute SQL query */
  pqxx::result R( N.exec( sql ));

  std::clog << " rows: " << R.size() << std::endl;
  for (pqxx::result::const_iterator row = R.begin(); row != R.end();++row){
    std::clog << " ... row: " << row - R.begin() << " columns: " << row->size() << ":";
    for (pqxx::result::tuple::const_iterator field = row->begin(); field != row->end();++field){
      std::clog << " " << field->c_str();
    }
    std::clog << std::endl;
  }
  
  std::clog << " nontransaction done successfully" << endl; fflush(stdout);
      
  return;

}

void execute_transaction(string sql){

  std::clog << " executing transaction: " << sql << std::endl; fflush(stdout);

  /* Create a transactional object. */
  pqxx::work N(c);
  
  /* Execute SQL query */
  pqxx::result R( N.exec( sql ));

  N.commit();

  std::clog << " rows: " << R.size() << std::endl;
  for (pqxx::result::const_iterator row = R.begin(); row != R.end();++row){
    std::clog << " ... row: " << row - R.begin() << " columns: " << row->size() << ":";
    for (pqxx::result::tuple::const_iterator field = row->begin(); field != row->end();++field){
      std::clog << " " << field->c_str();
    }
    std::clog << std::endl;
  }

  std::clog << " transaction done successfully" << endl; fflush(stdout);
      
  return;

}
