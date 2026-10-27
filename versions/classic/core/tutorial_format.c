#include "tutorial_format.h"
#include "tutorial.h"
#include "game.h"
#include <string.h>

static uint32_t get32(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24;
}
static uint64_t get64(const uint8_t *p) {
    return (uint64_t)get32(p) | (uint64_t)get32(p+4)<<32;
}
static void put32(uint8_t *p, uint32_t v) {
    for (unsigned i=0;i<4;++i) p[i]=(uint8_t)(v>>(i*8));
}
static void put64(uint8_t *p, uint64_t v) {
    put32(p,(uint32_t)v); put32(p+4,(uint32_t)(v>>32));
}
static bool checksum(const uint8_t *p,size_t n) {
    return n>=4 && get32(p+n-4)==pg_crc32(p,n-4);
}
static bool life_base(const uint8_t *p,size_t n,uint32_t version) {
    return p && n==(version==6?PG_TUTORIAL_LIFE_V6_BYTES:PG_TUTORIAL_LIFE_V7_BYTES) &&
           !memcmp(p,"PGOT",4) && get32(p+4)==version && get32(p+12)==n-20 && checksum(p,n);
}
static bool stage_flags_valid(const uint8_t *p,uint32_t stage) {
    uint32_t flags=get32(p+676), arrival=(flags>>1)&3u;
    if (!pg_tutorial_stage_valid(stage) || (flags & ~7u)) return false;
    if (stage==PG_TUTORIAL_LANDED) return arrival==0;
    if (get32(p+32)!=PG_EGG || get32(p+20)==0) return false;
    return arrival==(stage==PG_TUTORIAL_DELIVERY?1u:2u);
}
bool pg_tutorial_life_v7_frame_valid(const uint8_t *life,size_t length) {
    return life_base(life,length,7) && stage_flags_valid(life,get32(life+720));
}
bool pg_tutorial_life_v7_from_v6(const uint8_t *v6,size_t length,uint32_t stage,uint8_t *v7,size_t capacity) {
    if (!v7 || capacity<PG_TUTORIAL_LIFE_V7_BYTES || !life_base(v6,length,6) || !stage_flags_valid(v6,stage)) return false;
    /* memmove permits converting a v6 frame in an adequately sized buffer in place. */
    memmove(v7,v6,720); put32(v7+4,7); put32(v7+12,708); put32(v7+720,stage);
    put32(v7+724,pg_crc32(v7,724)); return true;
}
static bool device_frame(const uint8_t *p,size_t n) {
    return p && n==PG_TUTORIAL_DEVICE_V2_BYTES && !memcmp(p,"PGDV",4) &&
           get32(p+4)==2 && get32(p+12)==56 && checksum(p,n);
}
bool pg_tutorial_reset_v2_frame_valid(const uint8_t *r,size_t n) {
    if (!r || n!=PG_TUTORIAL_RESET_V2_BYTES || memcmp(r,"PGRT",4) || get32(r+4)!=2 ||
        get32(r+8)!=n || (get32(r+12)!=1 && get32(r+12)!=2) || !get64(r+16) ||
        !get64(r+24) || !get64(r+32) || get32(r+40)!=PG_TUTORIAL_LIFE_V7_BYTES ||
        get32(r+44)!=PG_TUTORIAL_DEVICE_V2_BYTES || get32(r+48)>1 || get32(r+52) || !checksum(r,n)) return false;
    const uint8_t *life=r+56,*device=r+784;
    if (!pg_tutorial_life_v7_frame_valid(life,PG_TUTORIAL_LIFE_V7_BYTES) ||
        !device_frame(device,PG_TUTORIAL_DEVICE_V2_BYTES) ||
        get64(life+664)!=get64(r+16) || get64(device+64)!=get64(r+16)) return false;
    uint32_t stage=get32(life+720);
    return get32(r+48)==0?stage==PG_TUTORIAL_DELIVERY:stage==PG_TUTORIAL_SWIPE;
}
bool pg_tutorial_reset_v2_frame_make(const uint8_t *life,size_t life_length,const uint8_t *device,size_t device_length,
                                     uint32_t state,uint64_t epoch,uint64_t operation,uint64_t transaction,uint32_t entry,
                                     uint8_t *r,size_t capacity) {
    if (!r || capacity<PG_TUTORIAL_RESET_V2_BYTES || !pg_tutorial_life_v7_frame_valid(life,life_length) ||
        !device_frame(device,device_length) || (state!=1 && state!=2) || !epoch || !operation || !transaction || entry>1 ||
        get64(life+664)!=epoch || get64(device+64)!=epoch ||
        get32(life+720)!=(entry==0?PG_TUTORIAL_DELIVERY:PG_TUTORIAL_SWIPE)) return false;
    memset(r,0,PG_TUTORIAL_RESET_V2_BYTES);memcpy(r,"PGRT",4);put32(r+4,2);put32(r+8,PG_TUTORIAL_RESET_V2_BYTES);
    put32(r+12,state);put64(r+16,epoch);put64(r+24,operation);put64(r+32,transaction);
    put32(r+40,PG_TUTORIAL_LIFE_V7_BYTES);put32(r+44,PG_TUTORIAL_DEVICE_V2_BYTES);put32(r+48,entry);
    memcpy(r+56,life,life_length);memcpy(r+784,device,device_length);put32(r+860,pg_crc32(r,860));
    return true;
}
