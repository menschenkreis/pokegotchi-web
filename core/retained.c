#include "retained.h"
#include "tutorial_format.h"
#include <math.h>
#include <string.h>
static uint32_t g32(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint64_t g64(const uint8_t *p){return (uint64_t)g32(p)|(uint64_t)g32(p+4)<<32;}
static void p32(uint8_t *p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(8*i));}
static void p64(uint8_t *p,uint64_t v){p32(p,(uint32_t)v);p32(p+4,(uint32_t)(v>>32));}
static void pd(uint8_t *p,double d){uint64_t v;memcpy(&v,&d,8);p64(p,v);}
static double gd(const uint8_t *p){uint64_t v=g64(p);double d;memcpy(&d,&v,8);return d;}
typedef struct {uint64_t epoch,session,start;uint32_t base,identity;int64_t wall;double elapsed;} capsule_data;
static bool decode(capsule_data *out,const uint8_t *b,size_t n){
    if(!out||!b||n!=132||memcmp(b,"PGUT",4)||g32(b+4)!=2||g32(b+8)!=116||g32(b+128)!=pg_crc32(b,128))return false;
    pg_device_data device;uint64_t epoch;uint32_t seq;
    if(!pg_tutorial_device_v2_decode(&device,&seq,&epoch,b+12,76))return false;
    capsule_data c={.epoch=epoch,.session=g64(b+88),.start=g64(b+96),.wall=(int64_t)g64(b+104),.base=g32(b+112),.elapsed=gd(b+116),.identity=g32(b+124)};
    if(!c.session||!c.base||!c.identity||!isfinite(c.elapsed)||c.elapsed<0)return false;
    *out=c;return true;
}
static bool matching(const pg_checkpoint *c,const capsule_data *d){
    return d->epoch==c->life.epoch&&d->identity==c->life.pet.identity&&d->session==c->capsule_session&&
        d->base==c->capsule_base_sequence&&d->start==c->capsule_start_us;
}
bool pg_retained_capsule_epoch(const uint8_t *capsule,size_t length,uint64_t *epoch){
    capsule_data data;if(!epoch||!decode(&data,capsule,length))return false;
    *epoch=data.epoch;return true;
}
bool pg_retained_rebuild(const pg_checkpoint *current,const pg_device_data *device,uint32_t device_sequence,
                         int64_t wall_us,double base_elapsed,uint8_t *b,size_t cap){
    if(!current||!device||!b||cap<132||!current->sleep_flags||!isfinite(base_elapsed)||base_elapsed<0)return false;
    memset(b,0,132);memcpy(b,"PGUT",4);p32(b+4,2);p32(b+8,116);
    if(pg_tutorial_device_v2_encode(device,device_sequence,current->life.epoch,b+12,76)!=76)return false;
    p64(b+88,current->capsule_session);p64(b+96,current->capsule_start_us);p64(b+104,(uint64_t)wall_us);
    p32(b+112,current->capsule_base_sequence);pd(b+116,base_elapsed);p32(b+124,current->life.pet.identity);
    p32(b+128,pg_crc32(b,128));return true;
}
bool pg_retained_begin(const pg_tutorial_reset_io *io,pg_checkpoint *current,const pg_device_data *device,
                       uint32_t device_sequence,uint64_t session,uint64_t start_us,int64_t wall_us,uint8_t *b,size_t cap){
    if(!current||!device||!b||cap<132||!session||current->sleep_flags||current->life_sequence==UINT32_MAX)return false;
    memset(b,0,132);memcpy(b,"PGUT",4);p32(b+4,2);p32(b+8,116);
    if(pg_tutorial_device_v2_encode(device,device_sequence,current->life.epoch,b+12,76)!=76)return false;
    p64(b+88,session);p64(b+96,start_us);p64(b+104,(uint64_t)wall_us);p32(b+112,current->life_sequence+1);
    pd(b+116,current->life.pet.elapsed);p32(b+124,current->life.pet.identity);p32(b+128,pg_crc32(b,128));
    pg_checkpoint next=*current;next.sleep_flags=PG_CK_OPEN;next.capsule_session=session;
    next.capsule_base_sequence=current->life_sequence+1;next.capsule_start_us=start_us;
    return pg_checkpoint_commit_full(io,current,&next);
}
static void clear_token(pg_checkpoint *c){c->sleep_flags=0;c->capsule_session=0;c->capsule_base_sequence=0;c->applied_us=c->total_us=c->capsule_start_us=0;}
bool pg_retained_finish(const pg_tutorial_reset_io *io,pg_checkpoint *current){
    if(!current||current->sleep_flags!=(PG_CK_COMPLETE|PG_CK_BOUND))return false;
    pg_checkpoint next=*current;clear_token(&next);return pg_checkpoint_commit_full(io,current,&next);
}
pg_retained_result pg_retained_recover(const pg_tutorial_reset_io *io,pg_checkpoint *current,
                                        const uint8_t *capsule,size_t length,bool present,int64_t now_wall_us){
    if(!current)return PG_RETAINED_RECOVERY;
    if(!current->sleep_flags)return present?PG_RETAINED_RECOVERY:PG_RETAINED_READY;
    capsule_data data={0};
    if(present&&(!decode(&data,capsule,length)||!matching(current,&data)))return PG_RETAINED_RECOVERY;
    if((current->sleep_flags&3u)==PG_CK_COMPLETE)return present?PG_RETAINED_CLEAR_CAPSULE:
        (pg_retained_finish(io,current)?PG_RETAINED_READY:PG_RETAINED_RECOVERY);
    if((current->sleep_flags&3u)!=PG_CK_OPEN)return PG_RETAINED_RECOVERY;
    if(!(current->sleep_flags&PG_CK_BOUND)){
        if(!present){pg_checkpoint next=*current;clear_token(&next);return pg_checkpoint_commit_full(io,current,&next)?PG_RETAINED_READY:PG_RETAINED_RECOVERY;}
        if(data.wall<0||now_wall_us<data.wall||now_wall_us-data.wall>INT64_C(315360000000000)||data.elapsed!=current->life.pet.elapsed)return PG_RETAINED_RECOVERY;
        pg_checkpoint next=*current;next.sleep_flags|=PG_CK_BOUND;next.total_us=(uint64_t)(now_wall_us-data.wall);
        if(!pg_checkpoint_commit_full(io,current,&next))return PG_RETAINED_RECOVERY;
    }
    if(current->total_us>INT64_C(315360000000000))return PG_RETAINED_RECOVERY;
    pg_game game;pg_restore(&game,&current->life.pet);game.pet.muted=current->life.pet.muted;game.muted=game.pet.muted;
    if(!pg_sleep_pose_restore(&game,&current->scene))return PG_RETAINED_RECOVERY;
    if(current->life.stage==PG_TUTORIAL_LANDED)pg_advance_care(&game,(double)(current->total_us-current->applied_us)/1000000.0,current->scene.night!=0);
    pg_checkpoint next=*current;if(!pg_tutorial_life_update_pet(&next.life,&game.pet))return PG_RETAINED_RECOVERY;
    next.sleep_flags=PG_CK_COMPLETE|PG_CK_BOUND;next.applied_us=next.total_us;
    if(game.pet.phase==PG_DEAD){next.scene.night=0;next.scene.settle=0;}
    if(!pg_checkpoint_commit_full(io,current,&next))return PG_RETAINED_RECOVERY;
    return present?PG_RETAINED_CLEAR_CAPSULE:(pg_retained_finish(io,current)?PG_RETAINED_READY:PG_RETAINED_RECOVERY);
}
