#include "baudsearch.h"
#include "stdlib.h"

BaudSearch BaudSearch::nearest;
double BaudSearch::desiredbaud;
double BaudSearch::pclk;
Hook<const BaudSearch &> BaudSearch::snoop;


void BaudSearch::findBaudSettings(){
    nearest.baud=pclk; //+inf would also serve.
    double rawdivider= pclk/(16.0*desiredbaud);

    BaudSearch guess;
    for(guess.divider=rawdivider;//ignoring lower baud rates via ignoring truncation here
        guess.divider> rawdivider/2.0;
        --guess.divider){
        guess.seek();
    }
}

void BaudSearch::apply(){
    baud= pclk / (16.0*double(divider)*(1.0+double(div)/double(mul)));
}

double BaudSearch::error() const{
    return abs(baud-desiredbaud);
}

void BaudSearch::improve(){
    apply();

    snoop(*this);
    if(nearest.error()>error()){//to get lpc's value for 115200@12e6 we can add '=' here or reverse the search direction.
        nearest=*this;
    }
}

void BaudSearch::seek(){
    double bestclock=(16*desiredbaud*divider);
    if(pclk<bestclock){
        return; //can't achieve that
    }

    for(mul=15;mul>=1;--mul){//ordered for debug of most interesting case first ;)
        for(div=0;div<mul;++div){
            improve();
        }
    }
}
