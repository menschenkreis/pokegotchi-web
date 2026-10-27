#include "tutorial_life.h"
#include "tutorial_format.h"
#include <math.h>
#include <string.h>

static uint32_t get32(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint64_t get64(const uint8_t *p){return (uint64_t)get32(p)|(uint64_t)get32(p+4)<<32;}
static void put32(uint8_t *p,uint32_t v){for(unsigned i=0;i<4;++i)p[i]=(uint8_t)(v>>(8*i));}
static void put64(uint8_t *p,uint64_t v){put32(p,(uint32_t)v);put32(p+4,(uint32_t)(v>>32));}
static double get_double(const uint8_t *p){uint64_t v=get64(p);double d;memcpy(&d,&v,8);return d;}
static void put_double(uint8_t *p,double d){uint64_t v;memcpy(&v,&d,8);put64(p,v);}
static bool fixed_matches(float value,uint64_t fixed){
    return fixed<=PG_TUTORIAL_NEED_MAX&&value==(float)((double)fixed/(double)PG_TUTORIAL_NEED_SCALE);
}
static bool valid(const pg_tutorial_life *l){
    if(!l||!pg_save_valid_pet(&l->pet)||l->policy!=1||!pg_tutorial_stage_valid(l->stage)||
       !isfinite(l->fractional_us)||l->fractional_us<0||l->fractional_us>=1||
       !fixed_matches(l->pet.satiety,l->food_fixed)||!fixed_matches(l->pet.energy,l->energy_fixed)||
       !fixed_matches(l->pet.clean,l->clean_fixed))return false;
    if(l->episode_active){
        if(l->pet.phase!=PG_ALIVE||l->episode_us>=PG_TUTORIAL_EPISODE_MAX_US)return false;
    }else if(l->episode_us!=0 && !(l->pet.phase==PG_DEAD&&l->episode_us==PG_TUTORIAL_EPISODE_MAX_US))return false;
    if(l->stage!=PG_TUTORIAL_LANDED &&
       (l->pet.phase!=PG_EGG||l->pet.elapsed!=0||l->pet.age!=0||l->pet.incubation!=0||
        l->pet.egg_visits||l->pet.event_count||l->episode_active))return false;
    return true;
}
static uint64_t from_legacy(float v){return (uint64_t)floor((double)v*(double)PG_TUTORIAL_NEED_SCALE+0.5);}
bool pg_tutorial_life_update_pet(pg_tutorial_life *l,const pg_pet *pet){
    if(!l||!pet||!pg_save_valid_pet(pet))return false;
    pg_tutorial_life candidate=*l;candidate.pet=*pet;
    candidate.food_fixed=pet->care_initialized&&fixed_matches(pet->satiety,pet->care_food)?pet->care_food:from_legacy(pet->satiety);
    candidate.energy_fixed=pet->care_initialized&&fixed_matches(pet->energy,pet->care_energy)?pet->care_energy:from_legacy(pet->energy);
    candidate.clean_fixed=pet->care_initialized&&fixed_matches(pet->clean,pet->care_clean)?pet->care_clean:from_legacy(pet->clean);
    candidate.episode_active=pet->care_initialized&&pet->neglect_active;
    candidate.episode_us=pet->care_initialized?pet->neglect_us:0;
    candidate.fractional_us=pet->care_initialized?pet->care_fraction_us:0;
    candidate.pet.satiety=(float)((double)candidate.food_fixed/(double)PG_TUTORIAL_NEED_SCALE);
    candidate.pet.energy=(float)((double)candidate.energy_fixed/(double)PG_TUTORIAL_NEED_SCALE);
    candidate.pet.clean=(float)((double)candidate.clean_fixed/(double)PG_TUTORIAL_NEED_SCALE);
    if(!valid(&candidate))return false;
    *l=candidate;return true;
}
bool pg_tutorial_life_from_pet(pg_tutorial_life *l,const pg_pet *pet,uint64_t epoch,pg_tutorial_stage stage){
    if(!l||!pet)return false;
    pg_tutorial_life candidate={0};candidate.policy=1;candidate.epoch=epoch;candidate.stage=stage;
    if(!pg_tutorial_life_update_pet(&candidate,pet))return false;
    *l=candidate;return true;
}
bool pg_tutorial_life_decode(pg_tutorial_life *out,uint32_t *sequence,const uint8_t *b,size_t n){
    if(!out||!b||n<20||memcmp(b,"PGOT",4))return false;
    uint32_t version=get32(b+4),seq=0;
    pg_tutorial_life l={0};l.policy=1;
    if(version>=1&&version<=5){
        if(!pg_save_decode(&l.pet,&seq,b,n))return false;
        l.food_fixed=from_legacy(l.pet.satiety);l.energy_fixed=from_legacy(l.pet.energy);l.clean_fixed=from_legacy(l.pet.clean);
    }else if(version==6||version==7){
        size_t expected=version==6?PG_TUTORIAL_LIFE_V6_BYTES:PG_TUTORIAL_LIFE_V7_BYTES;
        if(n!=expected||get32(b+12)!=n-20||get32(b+n-4)!=pg_crc32(b,n-4))return false;
        if(version==7&&!pg_tutorial_life_v7_frame_valid(b,n))return false;
        uint8_t old[668];memcpy(old,b,664);put32(old+4,5);put32(old+12,648);put32(old+664,pg_crc32(old,664));
        if(!pg_save_decode(&l.pet,&seq,old,sizeof old))return false;
        l.epoch=get64(b+664);l.policy=get32(b+672);uint32_t flags=get32(b+676);
        if((flags&~7u)||((flags>>1)&3u)==3)return false;
        l.episode_active=(flags&1u)!=0;l.episode_us=get64(b+680);
        l.food_fixed=get64(b+688);l.energy_fixed=get64(b+696);l.clean_fixed=get64(b+704);l.fractional_us=get_double(b+712);
        uint32_t arrival=(flags>>1)&3u;
        l.stage=version==7?(pg_tutorial_stage)get32(b+720):arrival==0?PG_TUTORIAL_LANDED:arrival==1?PG_TUTORIAL_DELIVERY:PG_TUTORIAL_SWIPE;
        if(!valid(&l))return false;
    }else return false;
    l.pet.care_food=l.food_fixed;l.pet.care_energy=l.energy_fixed;l.pet.care_clean=l.clean_fixed;
    l.pet.neglect_us=l.episode_us;l.pet.neglect_active=l.episode_active;
    l.pet.care_fraction_us=l.fractional_us;l.pet.care_initialized=true;
    *out=l;if(sequence)*sequence=seq;return true;
}
size_t pg_tutorial_life_encode_v7(const pg_tutorial_life *l,uint32_t sequence,uint8_t *out,size_t cap){
    if(!out||cap<PG_TUTORIAL_LIFE_V7_BYTES||!valid(l))return 0;
    uint8_t old[PG_SAVE_MAX];size_t n=pg_save_encode(&l->pet,sequence,old,sizeof old);
    if(n!=668||get32(old+4)!=5)return 0;
    memcpy(out,old,664);put32(out+4,7);put32(out+12,708);put64(out+664,l->epoch);put32(out+672,l->policy);
    uint32_t arrival=l->stage==PG_TUTORIAL_LANDED?0:l->stage==PG_TUTORIAL_DELIVERY?1:2;
    put32(out+676,(l->episode_active?1u:0u)|(arrival<<1));put64(out+680,l->episode_us);
    put64(out+688,l->food_fixed);put64(out+696,l->energy_fixed);put64(out+704,l->clean_fixed);
    put_double(out+712,l->fractional_us);put32(out+720,(uint32_t)l->stage);put32(out+724,pg_crc32(out,724));
    return PG_TUTORIAL_LIFE_V7_BYTES;
}
bool pg_tutorial_device_v2_decode(pg_device_data *out,uint32_t *sequence,uint64_t *epoch,const uint8_t *b,size_t n){
    if(!out||!b||n!=PG_TUTORIAL_DEVICE_V2_BYTES||memcmp(b,"PGDV",4)||get32(b+4)!=2||
       get32(b+12)!=56||get32(b+72)!=pg_crc32(b,72))return false;
    uint8_t old[68];memcpy(old,b,64);put32(old+4,1);put32(old+12,48);put32(old+64,pg_crc32(old,64));
    pg_device_data d;uint32_t seq;
    if(!pg_device_decode(&d,&seq,old,sizeof old))return false;
    *out=d;if(sequence)*sequence=seq;if(epoch)*epoch=get64(b+64);return true;
}
size_t pg_tutorial_device_v2_encode(const pg_device_data *d,uint32_t seq,uint64_t epoch,uint8_t *out,size_t cap){
    if(!out||cap<PG_TUTORIAL_DEVICE_V2_BYTES)return 0;
    uint8_t old[68];if(pg_device_encode(d,seq,old,sizeof old)!=sizeof old)return 0;
    memcpy(out,old,64);put32(out+4,2);put32(out+12,56);put64(out+64,epoch);put32(out+72,pg_crc32(out,72));
    return PG_TUTORIAL_DEVICE_V2_BYTES;
}
bool pg_tutorial_reset_v1_normalize(const uint8_t *b,size_t n,uint8_t *out,size_t cap){
    if(!b||!out||n!=860||cap<PG_TUTORIAL_RESET_V2_BYTES||memcmp(b,"PGRT",4)||get32(b+4)!=1||
       get32(b+8)!=860||get32(b+40)!=724||get32(b+44)!=76||get32(b+52)||get32(b+856)!=pg_crc32(b,856))return false;
    pg_tutorial_life life;pg_device_data device;uint32_t life_seq;uint64_t device_epoch;
    if(!pg_tutorial_life_decode(&life,&life_seq,b+56,724)||
       !pg_tutorial_device_v2_decode(&device,NULL,&device_epoch,b+780,76)||
       life.epoch!=get64(b+16)||device_epoch!=life.epoch)return false;
    uint8_t encoded[728];
    if(pg_tutorial_life_encode_v7(&life,life_seq,encoded,sizeof encoded)!=sizeof encoded)return false;
    return pg_tutorial_reset_v2_frame_make(encoded,sizeof encoded,b+780,76,get32(b+12),get64(b+16),
                                           get64(b+24),get64(b+32),get32(b+48),out,cap);
}
