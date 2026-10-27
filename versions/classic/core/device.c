#include "device.h"
#include <string.h>
extern uint32_t pg_crc32(const void *,size_t);
static void put(uint8_t *p,uint64_t v,int n){for(int i=0;i<n;++i)p[i]=(uint8_t)(v>>(8*i));
    }
static uint64_t get(const uint8_t *p,int n){uint64_t v=0;
    for(int i=0;i<n;++i)v|=(uint64_t)p[i]<<(8*i);
    return v;
    }
void pg_device_defaults(pg_device_data *d){if(d){memset(d,0,sizeof *d);
        d->volume=50;
        }}
static bool valid(const pg_device_data *d){return d&&d->volume<=100&&d->volume%25==0&&(!d->clock_valid||(d->clock_seconds>=0&&d->clock_seconds<PG_CALENDAR_END&&d->source_session));
    }
size_t pg_device_encode(const pg_device_data *d,uint32_t seq,uint8_t *out,size_t cap){
    if(!valid(d)||!out||cap<PG_DEVICE_BYTES)return 0;
    memset(out,0,PG_DEVICE_BYTES);
    memcpy(out,"PGDV",4);
    put(out+4,1,4);
    put(out+8,seq,4);
    put(out+12,48,4);
    put(out+16,d->echo_best,4);
    put(out+20,d->volume,4);
    put(out+24,d->auto_sleep,4);
    if(d->clock_valid){put(out+32,(uint64_t)d->clock_seconds,8);
        put(out+40,d->anchor_us,8);
        put(out+48,d->source_session,8);
        put(out+56,1,4);
        }
    put(out+64,pg_crc32(out,64),4);
    return PG_DEVICE_BYTES;
}
bool pg_device_decode(pg_device_data *d,uint32_t *seq,const uint8_t *in,size_t n){
    if(!d||!in||n!=PG_DEVICE_BYTES||memcmp(in,"PGDV",4)||get(in+4,4)!=1||get(in+12,4)!=48||get(in+28,4)||get(in+60,4)||get(in+24,4)>1||get(in+56,4)>1||get(in+64,4)!=pg_crc32(in,64))return false;
    pg_device_data v={0};
    v.echo_best=(uint32_t)get(in+16,4);
    v.volume=(unsigned)get(in+20,4);
    v.auto_sleep=get(in+24,4)!=0;
    v.clock_valid=get(in+56,4)!=0;
    v.clock_seconds=(int64_t)get(in+32,8);
    v.anchor_us=get(in+40,8);
    v.source_session=get(in+48,8);
    if(!valid(&v)||(!v.clock_valid&&(v.clock_seconds||v.anchor_us||v.source_session)))return false;
    *d=v;
    if(seq)*seq=(uint32_t)get(in+8,4);
    return true;
}
size_t pg_utility_capsule_encode(const pg_utility_capsule *c,uint8_t *out,size_t cap){
    if(!c||!out||cap<PG_UTILITY_CAPSULE_BYTES||!c->session||!pg_device_encode(&c->device,c->device_sequence,out+12,cap-12))return 0;
    memcpy(out,"PGUT",4);
    put(out+4,1,4);
    put(out+8,96,4);
    put(out+80,c->session,8);
    put(out+88,c->now_us,8);
    put(out+96,(uint64_t)c->wall_us,8);
    put(out+104,c->life_sequence,4);
    put(out+108,pg_crc32(out,108),4);
    return PG_UTILITY_CAPSULE_BYTES;
}
bool pg_utility_capsule_decode(pg_utility_capsule *c,const uint8_t *in,size_t n){
    if(!c||!in||n!=PG_UTILITY_CAPSULE_BYTES||memcmp(in,"PGUT",4)||get(in+4,4)!=1||get(in+8,4)!=96||get(in+108,4)!=pg_crc32(in,108))return false;
    pg_utility_capsule v={0};
    if(!pg_device_decode(&v.device,&v.device_sequence,in+12,68))return false;
    v.session=get(in+80,8);
    v.now_us=get(in+88,8);
    v.wall_us=(int64_t)get(in+96,8);
    v.life_sequence=(uint32_t)get(in+104,4);
    if(!v.session)return false;
    *c=v;
    return true;
}
bool pg_battery_decode(pg_battery_snapshot *out,uint8_t s1,uint8_t s2,uint8_t gauge,uint8_t pct,uint64_t at){
    if(!out)return false;
    *out=(pg_battery_snapshot){.sampled_us=at};
    if(((s2>>5)&3)==3)return false;
    out->present=(s1&8)!=0;
    if(out->present&&(!(gauge&8)||pct>100))return false;
    out->valid=true;
    out->percent=out->present?pct:0;
    out->charging=out->present&&((s2>>5)&3)==1;
    return true;
}
