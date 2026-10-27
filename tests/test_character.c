#include "chip.h"
#include "draw.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);exit(1);}++checks;}while(0)
static int checks;
static void synth_tests(void){
    int16_t full[16000],chunked[16000];uint32_t voices[3]={0};
    for(int sound=PG_SOUND_TAP;sound<PG_SOUND_COUNT;++sound)for(int species=0;species<3;++species)for(int v=0;v<3;++v){
        pg_chip a,b;pg_chip_start(&a,(pg_sound)sound,(pg_species)species,(uint32_t)v);b=a;
        size_t n=pg_chip_read(&a,full,16000),offset=0;CHECK(n>0&&n<16000&&!a.active);
        while(b.active){size_t got=pg_chip_read(&b,chunked+offset,37);CHECK(got>0);offset+=got;CHECK(offset<=16000);}
        CHECK(offset==n&&!memcmp(full,chunked,n*2));CHECK(full[0]==0&&full[n-1]==0);
        int peak=0;long sum=0;for(size_t i=0;i<n;++i){int q=abs(full[i]);if(q>peak)peak=q;sum+=full[i];CHECK(full[i]%256==0);}
        CHECK(peak>0&&peak<=6144);CHECK(fabs((double)sum/n)<200);
        if(sound==PG_SOUND_HELLO&&v==0)voices[species]=pg_crc32(full,n*2);
    }
    CHECK(voices[0]!=voices[1]&&voices[0]!=voices[2]&&voices[1]!=voices[2]);
    pg_chip a,b;pg_chip_start(&a,PG_SOUND_HATCH,PG_MOSSLET,0);pg_chip_start(&b,PG_SOUND_HATCH,PG_LUNAPIP,2);
    size_t n=pg_chip_read(&a,full,16000);CHECK(pg_chip_read(&b,chunked,16000)==n&&!memcmp(full,chunked,n*2));
}
static void performance_tests(void){
    pg_game g,m;pg_init(&g,42);g.pet.phase=PG_ALIVE;g.muted=false;g.goal=PG_ATTENTION;m=g;m.muted=true;
    uint32_t rng=g.pet.rng;pg_pet_gently(&g);pg_pet_gently(&m);
    CHECK(g.emote==PG_EMOTE_CUDDLE&&m.emote==g.emote&&m.emote_time==g.emote_time);
    CHECK(g.sound==PG_SOUND_CUDDLE&&m.sound==PG_SOUND_NONE&&g.pet.rng==rng);
    uint16_t pixels[72*72],quiet[72*72];pg_canvas c={pixels,72,72},d={quiet,72,72};
    for(int s=0;s<3;++s)for(int e=1;e<PG_EMOTE_COUNT;++e){
        g.pet.species=(uint32_t)s;pg_react(&g,(pg_emote)e,PG_SOUND_NONE);pg_game before=g;
        memset(pixels,0,sizeof pixels);pg_creature(&c,&g,36,66,1);uint32_t hash=pg_crc32(pixels,sizeof pixels);
        CHECK(!memcmp(&g,&before,sizeof g));m=g;m.muted=true;memset(quiet,0,sizeof quiet);pg_creature(&d,&m,36,66,1);
        CHECK(!memcmp(pixels,quiet,sizeof pixels));
        g.anim+=0.18f;g.emote_time-=0.18f;memset(pixels,0,sizeof pixels);pg_creature(&c,&g,36,66,1);
        CHECK(hash!=pg_crc32(pixels,sizeof pixels));
        /* Exercise the active particle field, fading tail and expired reaction.
           Rendering must remain deterministic and completely silent-state agnostic. */
        for(int frame=1;frame<12;++frame){
            g.emote_time=g.emote_duration*(1-frame/12.0f);m=g;m.muted=!g.muted;
            pg_game before=g;memset(pixels,0,sizeof pixels);memset(quiet,0,sizeof quiet);
            pg_creature(&c,&g,36,66,1);pg_creature(&d,&m,36,66,1);
            CHECK(!memcmp(&g,&before,sizeof g)&&!memcmp(pixels,quiet,sizeof pixels));
        }
        g.emote_time=0;m=g;m.emote=PG_EMOTE_NONE;
        memset(pixels,0,sizeof pixels);memset(quiet,0,sizeof quiet);
        pg_creature(&c,&g,36,66,1);pg_creature(&d,&m,36,66,1);
        CHECK(!memcmp(pixels,quiet,sizeof pixels));
    }
    uint32_t variant=g.performance_variant; rng=g.pet.rng;
    pg_react(&g,PG_EMOTE_GREET,PG_SOUND_HELLO);CHECK(g.performance_variant!=variant&&g.pet.rng==rng);
    pg_react(&g,PG_EMOTE_CHEER,PG_SOUND_WIN);for(int i=0;i<40;++i)pg_tick(&g,0.1);CHECK(g.emote==PG_EMOTE_NONE);
    g.sound=PG_SOUND_NONE;for(int i=0;i<100;++i)pg_tick(&g,0.1);CHECK(g.sound==PG_SOUND_NONE); /* Idle motion is quiet. */
    pg_sleep(&g,true);CHECK(g.emote==PG_EMOTE_YAWN);
    for(int i=0;i<80;++i)pg_tick(&g,0.1); /* Travel, then a fresh arrival yawn. */
    CHECK(g.emote==PG_EMOTE_NONE&&g.sleep_settle>=2&&g.x==195&&g.y==282);
    pg_sleep(&g,false);CHECK(g.emote==PG_EMOTE_GREET);
    pg_init(&g,42);pg_react(&g,PG_EMOTE_CHEER,PG_SOUND_HELLO);CHECK(g.emote==PG_EMOTE_NONE&&g.sound==PG_SOUND_NONE);
}
static void care_feedback_tests(void){
    pg_game g,m;pg_init(&g,42);uint32_t rng=g.pet.rng;
    pg_egg_care(&g,12,4);CHECK(g.warmth_time==2.8f&&g.pet.rng==rng);
    static uint16_t a[466*466],b[466*466];pg_canvas ca={a,466,466},cb={b,466,466};
    for(int frame=0;frame<56;++frame){
        pg_tick(&g,.05);m=g;m.pet.species=(g.pet.species+1)%3;m.muted=!g.muted;
        pg_game before=g;pg_render(&ca,&g);pg_render(&cb,&m);
        CHECK(!memcmp(a,b,sizeof a));CHECK(!memcmp(&before,&g,sizeof g));
    }
    CHECK(g.warmth_time<.001f);pg_egg_care(&g,12,4);pg_sleep(&g,true);CHECK(g.warmth_time==0);
    pg_sleep(&g,false);pg_egg_care(&g,12,4);pg_standby(&g,true);CHECK(g.warmth_time==0);
    pg_init(&g,42);g.pet.phase=PG_ALIVE;
    for(int sp=0;sp<3;++sp)for(int food=0;food<3;++food){
        g.pet.species=(uint32_t)sp;g.since_food=100;g.pet.satiety=30;g.muted=true;g.sound=PG_SOUND_NONE;
        rng=g.pet.rng;pg_feed(&g,food);CHECK(g.emote==PG_EMOTE_NIBBLE&&g.food_flavour==food&&g.pet.satiety==100&&g.pet.rng==rng);
        unsigned meals=g.pet.meals;pg_feed(&g,food);CHECK(g.pet.meals==meals); /* Repeated input cannot reward twice. */
        uint32_t last=0;
        for(int phase=0;phase<3;++phase){
            g.emote_time=g.emote_duration-(phase==0?.2f:phase==1?1.2f:2.6f);
            pg_game before=g;pg_render(&ca,&g);m=g;m.muted=false;pg_render(&cb,&m);
            CHECK(!memcmp(&before,&g,sizeof g)&&!memcmp(a,b,sizeof a));
            uint32_t hash=pg_crc32(a,sizeof a);CHECK(phase==0||hash!=last);last=hash;
        }
        CHECK(g.sound==PG_SOUND_NONE);
    }
    pg_standby(&g,true);CHECK(g.bowl_time==0&&g.emote_time==0);
}
int main(void){synth_tests();performance_tests();care_feedback_tests();printf("PASS %d character/synth checks; bounded cues, PCM fragmentation, mute, shell privacy, render purity, reactions and care phases\n",checks);return 0;}
