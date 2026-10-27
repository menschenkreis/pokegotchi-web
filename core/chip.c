#include "chip.h"
#include <math.h>
#include <string.h>

static void note(pg_chip *s,int midi,int ms){
    if(s->count<PG_CHIP_NOTES)s->notes[s->count++]=(pg_chip_note){(int8_t)midi,(uint16_t)ms};
}
static void begin_note(pg_chip *s){
    pg_chip_note n=s->notes[s->cursor];
    s->offset=0;s->length=(uint32_t)n.ms*(PG_CHIP_RATE/1000);s->phase=0;
    s->step=n.midi<0?0:440.0f*powf(2.0f,(n.midi-69)/12.0f)/PG_CHIP_RATE;
}
int pg_sound_priority(pg_sound sound){
    if(sound==PG_SOUND_HATCH||sound==PG_SOUND_WIN||sound==PG_SOUND_LOSE)return 3;
    if(sound>=PG_SOUND_NOTE0&&sound<=PG_SOUND_NOTE2)return 4;
    return sound==PG_SOUND_NONE?0:sound==PG_SOUND_TAP?1:2;
}
void pg_chip_start(pg_chip *s,pg_sound sound,pg_species species,uint32_t variation){
    memset(s,0,sizeof *s);
    if(sound<=PG_SOUND_NONE||sound>=PG_SOUND_COUNT)return;
    /* Neutral shell/menu/game notes never reveal a hidden species. */
    bool voice=!(sound==PG_SOUND_TAP||sound==PG_SOUND_WARM||sound==PG_SOUND_HATCH||
                 (sound>=PG_SOUND_NOTE0&&sound<=PG_SOUND_NOTE2));
    s->voice=voice&&species>=PG_MOSSLET&&species<PG_SPECIES_COUNT?(uint8_t)species:0;
    int root=voice?(s->voice==PG_MOSSLET?72:s->voice==PG_EMBERKIN?76:79):72;
    int beat=voice?(s->voice==PG_MOSSLET?75:s->voice==PG_EMBERKIN?55:95):65;
    int v=voice?(int)(variation%3):0;
    switch(sound){
    case PG_SOUND_TAP:note(s,84,28);break;
    case PG_SOUND_WARM:note(s,72,75);note(s,-1,25);note(s,76,100);break;
    case PG_SOUND_HATCH:
        note(s,60,80);note(s,-1,35);note(s,67,80);note(s,-1,35);
        note(s,72,90);note(s,76,90);note(s,79,90);note(s,84,180);break;
    case PG_SOUND_WIN:
        note(s,root,beat);note(s,root+4,beat);note(s,root+7,beat);
        note(s,-1,beat/2);note(s,root+12,beat*2);note(s,root+7,beat);note(s,root+12,beat*2);break;
    case PG_SOUND_LOSE: /* Encouraging little shrug; no punishment buzzer. */
        note(s,root+7,beat);note(s,root+4,beat);note(s,-1,beat/2);note(s,root+2,beat*2);break;
    case PG_SOUND_FOOD:
        note(s,root+7,35);note(s,-1,45);note(s,root+7,35);note(s,-1,45);
        note(s,root+v*2,95);break;
    case PG_SOUND_CUDDLE:
        note(s,root+4,beat);note(s,root+7,beat);note(s,-1,30);note(s,root+4,beat*2);break;
    case PG_SOUND_CURIOUS:
        note(s,root,beat);note(s,-1,beat/2);note(s,root+7+(v==2?5:0),beat*2);break;
    case PG_SOUND_SLEEP:
        note(s,root+4,beat);note(s,root+2,beat);note(s,root,beat*2);break;
    case PG_SOUND_CLEAN:
        note(s,root,35);note(s,root+7,35);note(s,root+12,90);break;
    case PG_SOUND_HELLO:
        note(s,root,beat);note(s,-1,beat/2);note(s,root+7,beat);note(s,root+4,beat*2);break;
    case PG_SOUND_HAPPY:
        note(s,root+(v==1?4:0),beat);note(s,root+7,beat);note(s,root+12,beat*2);break;
    case PG_SOUND_NOTE0:note(s,72,125);break;
    case PG_SOUND_NOTE1:note(s,76,125);break;
    case PG_SOUND_NOTE2:note(s,79,125);break;
    default:break;
    }
    s->active=s->count>0;if(s->active)begin_note(s);
}
size_t pg_chip_read(pg_chip *s,int16_t *out,size_t samples){
    size_t n=0;
    while(n<samples&&s->active){
        float t=(float)s->offset/(float)s->length;
        float env=fminf(1,(float)s->offset/64)*fminf(1,(float)(s->length-1-s->offset)/160);
        env*=1-0.38f*t;float wave=0;
        if(s->step>0){
            /* Pulse has its DC mean removed; Lunapip uses a round triangle. */
            float duty=s->voice==PG_EMBERKIN?0.25f:0.5f;
            wave=s->voice==PG_LUNAPIP?1-4*fabsf(s->phase-0.5f):
                 (s->phase<duty?1.0f:-1.0f)-(2*duty-1);
            wave*=s->voice==PG_EMBERKIN?0.65f:0.85f;
            s->phase+=s->step;if(s->phase>=1)s->phase-=1;
        }
        /* Deliberate 8-bit amplitude steps inside a standard 16-bit PCM stream. */
        out[n++]=(int16_t)(roundf(wave*env*23)*256);
        if(++s->offset==s->length){
            if(++s->cursor==s->count)s->active=false;else begin_note(s);
        }
    }
    return n;
}
