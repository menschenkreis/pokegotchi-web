#include "game.h"
#include "utilities.h"
#include <math.h>
#include <string.h>
void pg_trace_event(pg_game *g,int event,int detail){pg_diagnostic_entry *e=&g->utilities.trace[g->utilities.trace_count%64];*e=(pg_diagnostic_entry){.at_us=g->utilities.source.now_us,.session=g->echo.session,.round=g->echo.round,.score=g->echo.score,.screen=g->screen,.stage=g->echo.stage,.event=event,.detail=detail,.gesture=g->handling.last_gesture,.recipient=g->handling.last_recipient,.attempts=g->echo.attempts};++g->utilities.trace_count;}
static int days(int y,int m){static const int n[]={31,28,31,30,31,30,31,31,30,31,30,31};
    return m==2?n[1]+(y%4==0):n[m-1];
    }
bool pg_calendar_to_seconds(const pg_calendar *c,int64_t *out){
    if(!c||!out||c->year<2000||c->year>2099||c->month<1||c->month>12||c->day<1||c->day>days(c->year,c->month)||c->hour<0||c->hour>23||c->minute<0||c->minute>59||c->second<0||c->second>59)return false;
    int64_t d=0;
    for(int y=2000;y<c->year;++y)d+=365+(y%4==0);
    for(int m=1;m<c->month;++m)d+=days(c->year,m);
    *out=((d+c->day-1)*24+c->hour)*3600+c->minute*60+c->second;
    return true;
}
bool pg_calendar_from_seconds(int64_t s,pg_calendar *out){
    if(!out||s<0||s>=PG_CALENDAR_END)return false;
    pg_calendar c={.year=2000,.month=1};
    c.second=(int)(s%60);
    s/=60;
    c.minute=(int)(s%60);
    s/=60;
    c.hour=(int)(s%24);
    s/=24;
    while(s>=365+(c.year%4==0)){s-=365+(c.year%4==0);
        ++c.year;
        }
    while(s>=days(c.year,c.month)){s-=days(c.year,c.month);
        ++c.month;
        }c.day=(int)s+1;
    *out=c;
    return true;
}
void pg_device_attach(pg_game *g,const pg_device_data *d,bool writable){g->device=*d;
    memset(&g->utilities,0,sizeof g->utilities);
    g->utilities.writable=writable;
    g->utilities.storage_fault=!writable;
    }
void pg_device_changed(pg_game *g){g->utilities.prefs_dirty=true;
    g->utilities.preference_age=0;
    ++g->utilities.revision;
    }
void pg_user_activity(pg_game *g,pg_activity_kind kind){(void)kind;
    if(!g->standby)g->utilities.idle=0;
    }
bool pg_audio_enabled(const pg_game *g){return !g->muted&&!g->standby&&g->device.volume>0;
    }
void pg_pcm_gain(int16_t *p,size_t n,unsigned level){if(level>100)level=100;
    /* +6 dB output headroom; saturate arbitrary input rather than wrapping. */
    for(size_t i=0;i<n;++i){int32_t v=(int32_t)p[i]*(int32_t)level/50;
        p[i]=(int16_t)(v>32767?32767:v< -32768?-32768:v);
    }
    }
bool pg_calendar_now(const pg_game *g,pg_calendar *out){
    const pg_device_data *d=&g->device;
    const pg_time_source *s=&g->utilities.source;
    if(!d->clock_valid||!s->valid||s->session!=d->source_session||s->now_us<d->anchor_us)return false;
    uint64_t elapsed=(s->now_us-d->anchor_us)/1000000;
    if(elapsed>=(uint64_t)PG_CALENDAR_END)return false;
    return pg_calendar_from_seconds(d->clock_seconds+(int64_t)elapsed,out);
}
bool pg_calendar_save(pg_game *g,const pg_calendar *c){
    int64_t s;
    if(!g->utilities.source.valid||!pg_calendar_to_seconds(c,&s))return false;
    g->device.clock_valid=true;
    g->device.clock_seconds=s-c->second;
    g->device.anchor_us=g->utilities.source.now_us;
    g->device.source_session=g->utilities.source.session;
    pg_device_changed(g);
    g->utilities.editing=false;
    return true;
}
void pg_clock_begin(pg_game *g){if(!pg_calendar_now(g,&g->utilities.edit))g->utilities.edit=(pg_calendar){2026,1,1,12,0,0};
    g->utilities.editing=true;g->utilities.clock_error=false;
    g->utilities.clock_field=0;
    g->screen=PG_CLOCK_EDIT;
    }
void pg_clock_adjust(pg_game *g,int direction){
    pg_calendar *c=&g->utilities.edit;
    int *v=NULL,lo=0,hi=0;
    switch(g->utilities.clock_field){case 0:v=&c->year;
        lo=2000;
        hi=2099;
        break;
        case 1:v=&c->month;
        lo=1;
        hi=12;
        break;
        case 2:v=&c->day;
        lo=1;
        hi=days(c->year,c->month);
        break;
        case 3:v=&c->hour;
        hi=23;
        break;
        default:v=&c->minute;
        hi=59;
        break;
        }
    *v+=direction;
    if(g->utilities.clock_field==0){if(*v<lo)*v=lo;
        if(*v>hi)*v=hi;
        }else{if(*v<lo)*v=hi;
        if(*v>hi)*v=lo;
        }if(c->day>days(c->year,c->month))c->day=days(c->year,c->month);
}
void pg_utilities_step(pg_game *g,pg_time_source s){
    pg_device_runtime *u=&g->utilities;
    double dt=0;
    if(!s.session)s.valid=false;
    bool reset=!s.valid||(u->initialized&&(s.session!=u->source.session||s.now_us<u->source.now_us||s.now_us-u->source.now_us>UINT64_C(315360000000000)));
    if(reset){g->device.clock_valid=false;
        g->device.clock_seconds=0;
        g->device.anchor_us=g->device.source_session=0;
        u->idle=0;
        u->editing=false;if(g->screen==PG_CLOCK_EDIT)g->screen=PG_SETTINGS;
        u->contact_age=u->contact_beat=0;pg_handling_reset(g);g->handling.cooldown_us=g->handling.cosmetic_us=0;
        }
    else if(u->initialized)dt=(double)(s.now_us-u->source.now_us)/1000000;
    u->source=s;
    u->initialized=true;
    if(g->device.clock_valid&&g->device.source_session!=s.session){g->device.clock_valid=false;
        g->device.clock_seconds=0;
        g->device.anchor_us=g->device.source_session=0;
        }
    u->preference_age+=dt;
    if(g->standby){u->idle=0;
        return;
        }
    u->idle+=dt;
    if(g->pointer_down){u->contact_age+=dt;
        u->contact_beat+=dt;
        if(u->contact_age<=30&&u->contact_beat>=1){pg_user_activity(g,PG_ACTIVITY_HELD_CONTACT);
            u->contact_beat=0;
            }}
    else u->contact_age=u->contact_beat=0;
    if(hypotf(g->tilt_x,g->tilt_y)>=.15f&&hypotf(g->tilt_x-u->last_tilt_x,g->tilt_y-u->last_tilt_y)>=.1f){u->last_tilt_x=g->tilt_x;
        u->last_tilt_y=g->tilt_y;
        pg_user_activity(g,PG_ACTIVITY_TILT_CHANGE);
        }
    if(g->device.auto_sleep&&u->idle>=120){u->checkpoint=true;
        pg_standby(g,true);
        }
}
