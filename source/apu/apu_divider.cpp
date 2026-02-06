#include "apu_divider.h"
#include <iostream>

Divider::Divider(unsigned int p, unsigned int db)
{
    period = p;
    counter = 0;
    divideBy = db;
    dbCount = db;
}

bool Divider::clock()
{
    counter--;
    if(counter<=0){
        counter = period;
        if(dbCount>=divideBy){
            dbCount = 1;
            return true;
        }
        else{
            dbCount++;
            return false;
        }
    }
    else return false;
}

void Divider::reset()
{
    counter = period;
}

void Divider::changePeriod(unsigned int p)
{
    period = p;
}
