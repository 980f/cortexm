#include <iostream>
#define TAB <<'\t'<<

#include "baudsearch.h"

void toString(const BaudSearch &bs) {
#ifdef __linux__
  std::cout<<bs.baud TAB bs.divider TAB int(bs.mul) TAB int(bs.div);
  std::cout<<std::endl;
#endif
}

int main(int argc, char *argv[]){

  BaudSearch::desiredbaud=(argc>1)?atof(argv[1]):115200;
  BaudSearch::pclk=(argc>2)?atof(argv[2]):12000000;

  BaudSearch::snoop=&toString;

  BaudSearch::findBaudSettings();
  std::cout<<"baud" TAB "divider" TAB "div" TAB "mul" <<std::endl;
  toString(BaudSearch::nearest);
#ifdef __linux__
  std::cout
      << "/*"<< BaudSearch::nearest.baud << "*/"
      << BaudSearch::nearest.divider << ","
      << int(BaudSearch::nearest.mul) << ","
      << int(BaudSearch::nearest.div) << ","
      << unsigned(BaudSearch::pclk) ;
  std::cout<<std::endl;
#endif

  return 0;
}
