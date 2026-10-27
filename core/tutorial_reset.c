#include "tutorial_reset.h"
#include "tutorial_life.h"
#include "checkpoint.h"
#include "game.h"
#include <string.h>

static uint32_t get32(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint64_t get64(const uint8_t *p){return (uint64_t)get32(p)|(uint64_t)get32(p+4)<<32;}
static void put32(uint8_t *p,uint32_t v){for(unsigned i=0;i<4;++i)p[i]=(uint8_t)(v>>(8*i));}
static void put64(uint8_t *p,uint64_t v){put32(p,(uint32_t)v);put32(p+4,(uint32_t)(v>>32));}

static bool ready(const pg_tutorial_reset_io *io){return io&&io->read&&io->write_durable&&io->validate_payload;}
static bool semantic(const pg_tutorial_reset_io *io,const uint8_t *r){
    return pg_tutorial_reset_v2_frame_valid(r,PG_TUTORIAL_RESET_V2_BYTES)&&
           io->validate_payload(io->context,r+56,PG_TUTORIAL_LIFE_V7_BYTES,r+784,PG_TUTORIAL_DEVICE_V2_BYTES);
}
static bool same_transaction(const uint8_t *a,const uint8_t *b){
    return get64(a+16)==get64(b+16)&&get64(a+32)==get64(b+32)&&get32(a+48)==get32(b+48)&&
           !memcmp(a+56,b+56,PG_TUTORIAL_LIFE_V7_BYTES+PG_TUTORIAL_DEVICE_V2_BYTES);
}
static bool durable(const pg_tutorial_reset_io *io,pg_reset_object object,unsigned slot,const uint8_t *data,size_t n){
    uint8_t check[PG_TUTORIAL_RESET_V2_BYTES];size_t got=0;
    if(!io->write_durable(io->context,object,slot,data,n))return false;
    return io->read(io->context,object,slot,check,sizeof check,&got)==PG_RESET_READ_OK&&got==n&&!memcmp(check,data,n);
}
static void marker(uint8_t *r,uint32_t state,uint64_t operation){
    put32(r+12,state);put64(r+24,operation);put32(r+860,pg_crc32(r,860));
}
typedef struct {uint8_t bytes[2][PG_TUTORIAL_RESET_V2_BYTES];bool present[2];int best;} scan_result;
static bool scan(const pg_tutorial_reset_io *io,scan_result *s){
    memset(s,0,sizeof *s);s->best=-1;
    for(unsigned i=0;i<2;++i){
        size_t n=0;pg_reset_read_result result=io->read(io->context,PG_RESET_RECORD,i,s->bytes[i],sizeof s->bytes[i],&n);
        if(result==PG_RESET_READ_ABSENT)continue;
        if(result!=PG_RESET_READ_OK)return false;
        if(n==860){
            uint8_t normalized[PG_TUTORIAL_RESET_V2_BYTES];
            if(!pg_tutorial_reset_v1_normalize(s->bytes[i],n,normalized,sizeof normalized))return false;
            memcpy(s->bytes[i],normalized,sizeof normalized);n=sizeof normalized;
        }
        if(n!=PG_TUTORIAL_RESET_V2_BYTES||!semantic(io,s->bytes[i]))return false;
        s->present[i]=true;
        if(s->best<0||get64(s->bytes[i]+16)>get64(s->bytes[s->best]+16)||
           (get64(s->bytes[i]+16)==get64(s->bytes[s->best]+16)&&get64(s->bytes[i]+24)>get64(s->bytes[s->best]+24)))s->best=(int)i;
    }
    if(s->present[0]&&s->present[1]&&get64(s->bytes[0]+16)==get64(s->bytes[1]+16)){
        if(!same_transaction(s->bytes[0],s->bytes[1]))return false;
        if(get64(s->bytes[0]+24)==get64(s->bytes[1]+24)&&memcmp(s->bytes[0],s->bytes[1],PG_TUTORIAL_RESET_V2_BYTES))return false;
        /* Once APPLIED exists, a later COMMITTED marker is an invalid regression. */
        for(unsigned i=0;i<2;++i)if(get32(s->bytes[i]+12)==2&&get32(s->bytes[1-i]+12)==1&&
           get64(s->bytes[i]+24)<=get64(s->bytes[1-i]+24))return false;
    }
    return true;
}
static pg_tutorial_reset_result complete(const pg_tutorial_reset_io *io,scan_result *s){
    if(s->best<0)return PG_RESET_NONE;
    uint8_t r[PG_TUTORIAL_RESET_V2_BYTES];memcpy(r,s->bytes[s->best],sizeof r);
    uint64_t operation=get64(r+24);uint32_t state=get32(r+12);
    /* APPLIED must never replay the original payload over later life progress. */
    if(state==2){
        for(unsigned i=0;i<2;++i){
            if(s->present[i]&&get32(s->bytes[i]+12)==2&&same_transaction(r,s->bytes[i]))continue;
            if(operation==UINT64_MAX)return PG_RESET_RECOVERY_REQUIRED;
            marker(r,2,++operation);
            if(!durable(io,PG_RESET_RECORD,i,r,sizeof r))return PG_RESET_RECOVERY_REQUIRED;
        }
        return PG_RESET_APPLIED;
    }
    for(unsigned i=0;i<2;++i){
        if(s->present[i]&&same_transaction(r,s->bytes[i]))continue;
        if(operation==UINT64_MAX)return PG_RESET_RECOVERY_REQUIRED;
        marker(r,1,++operation);
        if(!durable(io,PG_RESET_RECORD,i,r,sizeof r))return PG_RESET_RECOVERY_REQUIRED;
    }
    if(io->checkpoints){
        if(!pg_checkpoint_install_reset(io,r+56,PG_TUTORIAL_LIFE_V7_BYTES))return PG_RESET_RECOVERY_REQUIRED;
    }else for(unsigned i=0;i<2;++i)if(!durable(io,PG_RESET_LIFE,i,r+56,PG_TUTORIAL_LIFE_V7_BYTES))return PG_RESET_RECOVERY_REQUIRED;
    for(unsigned i=0;i<2;++i)if(!durable(io,PG_RESET_DEVICE,i,r+784,PG_TUTORIAL_DEVICE_V2_BYTES))return PG_RESET_RECOVERY_REQUIRED;
    for(unsigned i=0;i<2;++i){
        if(operation==UINT64_MAX)return PG_RESET_RECOVERY_REQUIRED;
        marker(r,2,++operation);
        if(!durable(io,PG_RESET_RECORD,i,r,sizeof r))return PG_RESET_RECOVERY_REQUIRED;
    }
    return PG_RESET_APPLIED;
}
pg_tutorial_reset_result pg_tutorial_reset_resume(const pg_tutorial_reset_io *io){
    if(!ready(io))return PG_RESET_RECOVERY_REQUIRED;
    scan_result s;if(!scan(io,&s))return PG_RESET_RECOVERY_REQUIRED;
    return complete(io,&s);
}
bool pg_tutorial_reset_boundary(const pg_tutorial_reset_io *io,uint64_t *epoch,uint64_t *operation){
    if(!ready(io)||!epoch||!operation)return false;
    scan_result s;if(!scan(io,&s))return false;
    if(s.best<0){*epoch=0;*operation=0;return true;}
    if(get32(s.bytes[s.best]+12)!=2)return false;
    *epoch=get64(s.bytes[s.best]+16);*operation=get64(s.bytes[s.best]+24);return true;
}
pg_tutorial_reset_result pg_tutorial_reset_begin(const pg_tutorial_reset_io *io,const uint8_t *candidate,size_t length){
    if(!ready(io)||length!=PG_TUTORIAL_RESET_V2_BYTES||!candidate||!semantic(io,candidate)||get32(candidate+12)!=1)return PG_RESET_RECOVERY_REQUIRED;
    scan_result s;if(!scan(io,&s))return PG_RESET_RECOVERY_REQUIRED;
    if(s.best>=0&&(get64(candidate+16)<=get64(s.bytes[s.best]+16)||get64(candidate+24)<=get64(s.bytes[s.best]+24)))return PG_RESET_RECOVERY_REQUIRED;
    unsigned slot=s.best<0?0u:(unsigned)(1-s.best);
    if(!durable(io,PG_RESET_RECORD,slot,candidate,length))return PG_RESET_RECOVERY_REQUIRED;
    return pg_tutorial_reset_resume(io);
}
