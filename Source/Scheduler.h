#pragma once
#include <array>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace nmg {
struct Note { double start=0, duration=0; int pitch=60, velocity=80, voice=0; };
struct Event { double beat=0; int pitch=60, velocity=0, voice=0; bool on=false; };
struct Score {
 std::vector<Note> notes; std::vector<Event> events;
 std::array<int,8> channels{1,1,1,1,1,1,1,1};
 double bpm=120,end=0; uint64_t revision=0;
 void prepare(){events.clear();events.reserve(notes.size()*2);for(const auto& n:notes){events.push_back({n.start,n.pitch,n.velocity,n.voice,true});events.push_back({n.start+n.duration,n.pitch,0,n.voice,false});}std::stable_sort(events.begin(),events.end(),[](const auto&a,const auto&b){return a.beat<b.beat||(a.beat==b.beat&&a.on<b.on);});}
};
class Scheduler {
 std::array<std::array<unsigned,128>,16> held{};
 const Score* last=nullptr; double expected=0; bool running=false; size_t cursor=0;
 template<class Emit> void on(const Score& s,const Note& n,int offset,Emit& emit){const int ch=s.channels[n.voice]-1;auto& count=held[ch][n.pitch];if(count++==0)emit(ch+1,n.pitch,n.velocity,true,offset);}
 template<class Emit> void event(const Score& s,const Event& e,int offset,Emit& emit){const int ch=s.channels[e.voice]-1;auto& count=held[ch][e.pitch];if(e.on){if(count++==0)emit(ch+1,e.pitch,e.velocity,true,offset);}else if(count>0&&--count==0)emit(ch+1,e.pitch,0,false,offset);}
public:
 template<class Emit> void release(Emit emit){for(int ch=0;ch<16;ch++)for(int p=0;p<128;p++)if(held[ch][p]){emit(ch+1,p,0,false,0);held[ch][p]=0;}running=false;last=nullptr;}
 template<class Emit> void process(const Score* s,double beat,double beatsPerSample,int samples,bool play,bool force,Emit emit){
  if(!s||!play||samples<=0||beatsPerSample<=0||beat>=s->end||beat<0){if(running)release(emit);return;}
  const bool discontinuity=force||!running||s!=last||std::abs(beat-expected)>std::max(1e-7,beatsPerSample*2);
  if(discontinuity){release(emit);last=s;running=true;cursor=std::lower_bound(s->events.begin(),s->events.end(),beat,[](const Event&e,double b){return e.beat<b;})-s->events.begin();for(const auto&n:s->notes)if(n.start<beat&&n.start+n.duration>beat)on(*s,n,0,emit);}
  const double end=beat+samples*beatsPerSample;
  while(cursor<s->events.size()&&s->events[cursor].beat<end-1e-10){const auto&e=s->events[cursor++];if(e.beat+1e-9<beat)continue;const int offset=std::clamp((int)std::llround((e.beat-beat)/beatsPerSample),0,samples-1);event(*s,e,offset,emit);}
  expected=end;
 }
};
}
