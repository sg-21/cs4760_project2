#include <iostream>
#include <fstream>
#include <unistd.h>

#include "bin_adder.h"
#include "same.h"

extern bool choosing[n]; /* Shared Boolean array */  
extern int number[n]; /* Shared integer array to hold turn number */  

void process_i ( const int i ) /* ith Process */ {     
  do       
  choosing[i] = true;        
  number[i] = 1 + max(number[0], ..., number[n-1]);        
  choosing[i] = false;        
  for ( int j = 0; j < n; j++ ) {           
    while ( choosing[j] ); // Wait if j happens to be choosing           
    while ( (number[j] != 0)  
    &&  ( number[j] < number[i] || (number[j] == number[i] && j < i) );        
  }        

  critical_section();        

  number[i] = 0;        

  remainder_section();     

  while ( 1 );  
} 
