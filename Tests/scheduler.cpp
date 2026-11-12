#include "Scheduler.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>
#include <tuple>
using M=std::tuple<int,int,int,bool,int>;
int main(){nmg::Score s;s.end=8;s.notes={{0,2,60,90,0},{1,2,60,70,1},{0,1,48,80,2}};s.prepare();nmg::Scheduler q;std::vector<M> out;auto emit=[&](int c,int p,int v,bool on,int o){out.emplace_back(c,p,v,on,o);};for(int b=0;b<4;b++)q.process(&s,b,.01,100,true,false,emit);assert(out.size()==4);assert(std::get<3>(out[0]));assert(std::get<0>(out[0])==1); // two logical C4s become one physical gate
out.clear();q.release(emit);s.channels[1]=16;s.prepare();for(int b=0;b<4;b++)q.process(&s,b,.01,100,true,false,emit);assert(out.size()==6);bool ch16=false;for(auto m:out)ch16|=std::get<0>(m)==16;assert(ch16);
out.clear();q.process(&s,1.5,.01,10,true,true,emit);assert(out.size()==2);q.process(&s,0,.01,100,false,false,emit);assert(out.size()==4);
out.clear();q.process(&s,0,.01,100,true,false,emit);q.process(&s,0,.01,100,true,false,emit); // loop must release/retrigger
assert(out.size()==6);q.release(emit);
// sample-accurate boundaries and all sixteen channels
for(int ch=1;ch<=16;ch++){nmg::Score x;x.end=2;x.channels.fill(ch);x.notes={{.25,.5,64,100,7}};x.prepare();nmg::Scheduler z;out.clear();z.process(&x,0,.01,100,true,false,emit);assert(out.size()==2&&std::get<4>(out[0])==25&&std::get<4>(out[1])==75&&std::get<0>(out[0])==ch);}
std::cout<<"PASS scheduler: shared pitches, channels 1–16, stop, seek, loop, sample offsets\n";
}
