#include "Timer.h"

void Timer::restart(Timestamp now)
{
   
        expiration_ = addTime(now, interval_);
    
    
    
}
Timer::~Timer(){
        
}