#include "checkpoint.h"
#include <math.h>
#include <string.h>

static uint32_t g32(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint64_t g64(const uint8_t *p){return (uint64_t)g32(p)|(uint64_t)g32(p+4)<<32;}
static void p32(uint8_t *p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(i*8));}
static void p64(uint8_t *p,uint64_t v){p32(p,(uint32_t)v);p32(p+4,(uint32_t)(v>>32));}
static void pf(uint8_t *p,float f){uint32_t n;memcpy(&n,&f,4);p32(p,n);}
static float gf(const uint8_t *p){uint32_t n=g32(p);float f;memcpy(&f,&n,4);return f;}
static bool pose_valid(const pg_sleep_pose *s){
    return s&&s->version==1&&s->night<=1&&isfinite(s->x)&&s->x>=0&&s->x<=PG_W&&
        isfinite(s->y)&&s->y>=0&&s->y<=PG_H&&isfinite(s->anim)&&s->anim>=0&&
        isfinite(s->settle)&&s->settle>=0&&s->settle<=3;
}
static void marker_bytes(uint8_t *b){memset(b,0,24);memcpy(b,"PGCM",4);p32(b+4,1);p32(b+8,24);p32(b+12,1);p32(b+20,pg_crc32(b,20));}
static bool durable(const pg_tutorial_reset_io *io,pg_reset_object obj,unsigned slot,const uint8_t *b,size_t n){
    uint8_t check[PG_CHECKPOINT_BYTES];size_t size=0;
    return io->write_durable(io->context,obj,slot,b,n)&&io->read(io->context,obj,slot,check,sizeof check,&size)==PG_RESET_READ_OK&&size==n&&!memcmp(check,b,n);
}
static bool ready(const pg_tutorial_reset_io *io){return io&&io->read&&io->write_durable;}
static bool token_valid(const pg_checkpoint *c){
    unsigned state=c->sleep_flags&3u;bool bound=(c->sleep_flags&PG_CK_BOUND)!=0;
    if(c->sleep_flags&~7u||state==3)return false;
    if(state==PG_CK_IDLE)return !c->sleep_flags&&!c->capsule_session&&!c->capsule_base_sequence&&!c->applied_us&&!c->total_us&&!c->capsule_start_us;
    if(!c->capsule_session||!c->capsule_base_sequence||c->capsule_base_sequence>c->life_sequence)return false;
    if(!bound)return state==PG_CK_OPEN&&!c->applied_us&&!c->total_us;
    if(c->applied_us>c->total_us)return false;
    return state!=PG_CK_COMPLETE||c->applied_us==c->total_us;
}

bool pg_checkpoint_encode(const pg_checkpoint *c,uint8_t *b,size_t cap){
    if(!c||!b||cap<PG_CHECKPOINT_BYTES||!c->sequence||!c->life_sequence||!pose_valid(&c->scene)||!token_valid(c))return false;
    memset(b,0,PG_CHECKPOINT_BYTES);memcpy(b,"PGCK",4);p32(b+4,1);p32(b+8,PG_CHECKPOINT_BYTES);
    p64(b+16,c->life.epoch);p64(b+24,c->sequence);p32(b+32,c->life.pet.identity);p32(b+36,c->life_sequence);
    p32(b+12,c->sleep_flags);p64(b+40,c->capsule_session);p32(b+48,c->capsule_base_sequence);
    p64(b+56,c->applied_us);p64(b+64,c->total_us);p64(b+72,c->capsule_start_us);
    if(pg_tutorial_life_encode_v7(&c->life,c->life_sequence,b+80,728)!=728)return false;
    uint8_t *s=b+808;memcpy(s,"PGSC",4);p32(s+4,2);p32(s+8,40);p64(s+12,c->life.epoch);
    p32(s+20,c->life.pet.identity);p32(s+24,c->life_sequence);p32(s+28,c->scene.night);
    pf(s+32,c->scene.x);pf(s+36,c->scene.y);pf(s+40,c->scene.anim);pf(s+44,c->scene.settle);p32(s+48,c->scene.variant);p32(s+52,pg_crc32(s,52));
    p32(b+864,pg_crc32(b,864));return true;
}
bool pg_checkpoint_decode(pg_checkpoint *out,const uint8_t *b,size_t n){
    if(!out||!b||n!=868||memcmp(b,"PGCK",4)||g32(b+4)!=1||g32(b+8)!=868||
       !g64(b+24)||!g32(b+36)||g32(b+52)||g32(b+864)!=pg_crc32(b,864))return false;
    pg_checkpoint c={0};uint32_t seq=0;
    if(!pg_tutorial_life_decode(&c.life,&seq,b+80,728)||c.life.epoch!=g64(b+16)||c.life.pet.identity!=g32(b+32)||seq!=g32(b+36))return false;
    const uint8_t *s=b+808;
    if(memcmp(s,"PGSC",4)||g32(s+4)!=2||g32(s+8)!=40||g64(s+12)!=c.life.epoch||g32(s+20)!=c.life.pet.identity||g32(s+24)!=seq||g32(s+52)!=pg_crc32(s,52))return false;
    c.scene.version=1;c.scene.night=g32(s+28);c.scene.x=gf(s+32);c.scene.y=gf(s+36);c.scene.anim=gf(s+40);c.scene.settle=gf(s+44);c.scene.variant=g32(s+48);
    if(!pose_valid(&c.scene))return false;
    c.scene.crc=pg_crc32(&c.scene,offsetof(pg_sleep_pose,crc));c.sequence=g64(b+24);c.life_sequence=seq;
    c.sleep_flags=g32(b+12);c.capsule_session=g64(b+40);c.capsule_base_sequence=g32(b+48);
    c.applied_us=g64(b+56);c.total_us=g64(b+64);c.capsule_start_us=g64(b+72);
    if(!token_valid(&c))return false;
    *out=c;return true;
}
pg_checkpoint_result pg_checkpoint_load(const pg_tutorial_reset_io *io,uint64_t epoch,pg_checkpoint *out){
    if(!ready(io)||!out)return PG_CHECKPOINT_RECOVERY;
    uint8_t expected[24];marker_bytes(expected);bool mode[2]={false,false};
    for(unsigned i=0;i<2;i++){uint8_t b[24];size_t n=0;pg_reset_read_result r=io->read(io->context,PG_RESET_CKMODE,i,b,sizeof b,&n);
        if(r==PG_RESET_READ_ABSENT)continue;
        if(r!=PG_RESET_READ_OK||n!=24||memcmp(b,expected,24))return PG_CHECKPOINT_RECOVERY;
        mode[i]=true;}
    if(!mode[0]&&!mode[1])return PG_CHECKPOINT_LEGACY;
    uint8_t b[2][868];bool present[2]={false,false};pg_checkpoint c[2];int best=-1;
    for(unsigned i=0;i<2;i++){size_t n=0;pg_reset_read_result r=io->read(io->context,PG_RESET_CHECKPOINT,i,b[i],868,&n);
        if(r==PG_RESET_READ_ABSENT)continue;
        if(r!=PG_RESET_READ_OK||!pg_checkpoint_decode(&c[i],b[i],n)||c[i].life.epoch!=epoch)return PG_CHECKPOINT_RECOVERY;
        present[i]=true;if(best<0||c[i].sequence>c[best].sequence)best=(int)i;
    }
    if(best<0)return PG_CHECKPOINT_RECOVERY;
    if(present[0]&&present[1]){
        if(c[0].sequence==c[1].sequence&&memcmp(b[0],b[1],868))return PG_CHECKPOINT_RECOVERY;
        if(c[0].sequence!=c[1].sequence){unsigned newer=c[0].sequence>c[1].sequence?0:1,older=1-newer;
            if(c[newer].life.pet.identity!=c[older].life.pet.identity||c[newer].life_sequence<=c[older].life_sequence)return PG_CHECKPOINT_RECOVERY;
        }
    }
    for(unsigned i=0;i<2;i++)if(!present[i]||memcmp(b[i],b[best],868))if(!durable(io,PG_RESET_CHECKPOINT,i,b[best],868))return PG_CHECKPOINT_RECOVERY;
    for(unsigned i=0;i<2;i++)if(!mode[i]&&!durable(io,PG_RESET_CKMODE,i,expected,24))return PG_CHECKPOINT_RECOVERY;
    *out=c[best];return PG_CHECKPOINT_VALID;
}
bool pg_checkpoint_activate(const pg_tutorial_reset_io *io,const pg_checkpoint *c){
    if(!ready(io))return false;
    uint8_t b[868],m[24];if(!pg_checkpoint_encode(c,b,sizeof b))return false;marker_bytes(m);
    for(unsigned i=0;i<2;i++)if(!durable(io,PG_RESET_CHECKPOINT,i,b,868))return false;
    for(unsigned i=0;i<2;i++)if(!durable(io,PG_RESET_CKMODE,i,m,24))return false;
    return true;
}
pg_checkpoint_result pg_checkpoint_resume_fresh(const pg_tutorial_reset_io *io,pg_checkpoint *out){
    if(!ready(io)||!out)return PG_CHECKPOINT_RECOVERY;
    uint8_t frames[2][868];pg_checkpoint candidate[2];bool present[2]={false,false};int selected=-1;
    for(unsigned i=0;i<2;i++){
        size_t length=0;pg_reset_read_result r=io->read(io->context,PG_RESET_CHECKPOINT,i,frames[i],868,&length);
        if(r==PG_RESET_READ_ABSENT)continue;
        if(r!=PG_RESET_READ_OK||!pg_checkpoint_decode(&candidate[i],frames[i],length)||
           candidate[i].life.epoch||candidate[i].sequence!=1||candidate[i].life_sequence!=1||
           candidate[i].life.stage!=PG_TUTORIAL_SWIPE||candidate[i].scene.night||candidate[i].sleep_flags)return PG_CHECKPOINT_RECOVERY;
        present[i]=true;selected=(int)i;
    }
    if(selected<0)return PG_CHECKPOINT_LEGACY;
    if(present[0]&&present[1]&&memcmp(frames[0],frames[1],868))return PG_CHECKPOINT_RECOVERY;
    if(!pg_checkpoint_activate(io,&candidate[selected]))return PG_CHECKPOINT_RECOVERY;
    *out=candidate[selected];return PG_CHECKPOINT_VALID;
}
bool pg_checkpoint_commit_full(const pg_tutorial_reset_io *io,pg_checkpoint *current,const pg_checkpoint *desired){
    if(!ready(io)||!current||!desired||current->sequence==UINT64_MAX||current->life_sequence==UINT32_MAX||desired->life.epoch!=current->life.epoch||desired->life.pet.identity!=current->life.pet.identity)return false;
    pg_checkpoint confirmed;
    uint8_t expected[868],actual[868];
    if(pg_checkpoint_load(io,desired->life.epoch,&confirmed)!=PG_CHECKPOINT_VALID||
       !pg_checkpoint_encode(current,expected,sizeof expected)||!pg_checkpoint_encode(&confirmed,actual,sizeof actual)||memcmp(expected,actual,sizeof expected))return false;
    pg_checkpoint next=*desired;next.sequence=current->sequence+1;next.life_sequence=current->life_sequence+1;uint8_t b[868];
    if(!pg_checkpoint_encode(&next,b,sizeof b))return false;
    unsigned first=(unsigned)(next.sequence&1);
    if(!durable(io,PG_RESET_CHECKPOINT,first,b,868)||!durable(io,PG_RESET_CHECKPOINT,1-first,b,868))return false;
    next.scene.crc=pg_crc32(&next.scene,offsetof(pg_sleep_pose,crc));
    *current=next;return true;
}
bool pg_checkpoint_commit(const pg_tutorial_reset_io *io,pg_checkpoint *current,const pg_tutorial_life *life,const pg_sleep_pose *scene){
    if(!current||!life||!scene||current->sleep_flags)return false;
    pg_checkpoint next=*current;next.life=*life;next.scene=*scene;
    return pg_checkpoint_commit_full(io,current,&next);
}
bool pg_checkpoint_install_reset(const pg_tutorial_reset_io *io,const uint8_t *bytes,size_t n){
    pg_checkpoint c={0};if(!ready(io)||!pg_tutorial_life_decode(&c.life,&c.life_sequence,bytes,n))return false;
    /* A reset can replace old-epoch checkpoints, but must not erase damaged mode metadata. */
    uint8_t marker[24];marker_bytes(marker);
    for(unsigned i=0;i<2;i++){
        uint8_t b[24];size_t length=0;pg_reset_read_result r=io->read(io->context,PG_RESET_CKMODE,i,b,sizeof b,&length);
        if(r!=PG_RESET_READ_ABSENT&&(r!=PG_RESET_READ_OK||length!=24||memcmp(b,marker,24)))return false;
    }
    c.sequence=1;c.scene.version=1;c.scene.x=233;c.scene.y=276;c.scene.night=c.life.stage==PG_TUTORIAL_LAMP_DAY;
    return pg_checkpoint_activate(io,&c);
}
