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


pqxx::result the_db_entry;
std::string command;

pqxx::connection * c;

pqxx::result execute_nontransaction(string sql, bool print);  
pqxx::result execute_transaction(string sql, bool print);
bool check_if_table_exists( std::string table_name );
void show_table( std::string table_name );

using namespace std;

int main(int argc, char *argv[]){

  string string_dbname = read_string_from_file("params.inp","dbname");
  string string_host = read_string_from_file("params.inp","host");
  string string_user = read_string_from_file("params.inp","user");
  string string_password = read_string_from_file("params.inp","password");

  string string_connection = "user=" + string_user + " host=" + string_host + " password=" + string_password + " dbname=" + string_dbname;
  c = new pqxx::connection(string_connection.c_str());

  show_table( "board_000000" );
  show_table( "board_000001" );
  show_table( "board_000010" );
  show_table( "board_000011" );
  show_table( "coincidences" );

  exit(0);
  return 0;

}


pqxx::result execute_nontransaction(string sql, bool print){
  
  /* Create a non-transactional object. */
  pqxx::nontransaction N(*c);
  
  /* Execute SQL query */
  pqxx::result R( N.exec( sql ));
  
  if( print ){
    for (pqxx::result::const_iterator row = R.begin(); row != R.end();++row){
      std::clog << " ... row: " << row - R.begin() << " columns: " << row->size() << ":";
      for (pqxx::result::tuple::const_iterator field = row->begin(); field != row->end();++field){
	std::clog << " " << field->c_str();
      }
      std::clog << std::endl;
    }
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


void show_table( std::string table_name ){
  
  int n_rows_max = 5;
  
  if( ! check_if_table_exists(table_name) ){
    std::clog  << " table " << table_name << " does not exist " << std::endl;
    return;
  }

  command = "SELECT COUNT(*) FROM " + table_name ; the_db_entry = execute_nontransaction(command, false);
  int n_rows = the_db_entry[0][0].as<int>();
  int n_rows_print = std::min(n_rows_max, n_rows);

  std::clog << " table " << table_name << " has " << n_rows << " entries, print first " << n_rows_print << std::endl; fflush(stdout);

  command = "SELECT * FROM " + table_name ; 
  pqxx::nontransaction N(*c);
  
  /* Execute SQL query */
  pqxx::result R( N.exec( command ));

  for (pqxx::result::const_iterator row = R.begin(); row != R.end();++row){
    if( row - R.begin() >= n_rows_max ) break;
    std::clog << " ... row: " << row - R.begin() << " columns: " << row->size() << ":";
    for (pqxx::result::tuple::const_iterator field = row->begin(); field != row->end();++field){
      std::clog << " " << field->c_str();
    }
    std::clog << std::endl;
  }

  return;
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


