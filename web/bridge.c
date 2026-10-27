#include "game.h"
#include "draw.h"
#include "chip.h"
#include <string.h>

static pg_game game;
static uint16_t pixels[PG_W*PG_H];
static uint8_t save[PG_SAVE_MAX];
static int16_t pcm[16000];
static pg_chip chip;
static pg_canvas canvas={pixels,PG_W,PG_H};
void web_init(unsigned seed){pg_init(&game,seed);game.echo_audio=true;}
void web_tick(double dt){pg_tick(&game,dt);}
uint16_t *web_frame(void){pg_render(&canvas,&game);return pixels;}
void web_pointer(int kind,float x,float y){pg_pointer(&game,kind,x,y);}
void web_tilt(float x,float y){if(!game.pointer_down){game.tilt_x=x;game.tilt_y=y;}}
void web_key(int key){
    if(key==0)pg_back(&game);
    if(key==1)pg_standby(&game,!game.standby);
    if(key==2){game.muted=!game.muted;game.pet.muted=game.muted;game.dirty=true;}
    if(key==3)pg_echo_shake_key(&game);
    if(key==4)pg_clap(&game);
    if(key>=5&&key<=7&&game.screen==PG_RHYTHM)pg_rhythm_tap(&game,key-5);
}
void web_hatch(void){pg_debug_hatch(&game);}
void web_open(int which){if(which>=0&&which<PG_GAME_COUNT&&game.pet.phase==PG_ALIVE){pg_sleep(&game,false);pg_standby(&game,false);pg_open_game(&game,which);}}
uint8_t *web_save_buffer(void){return save;}
int web_save(void){return (int)pg_save_encode(&game.pet,1,save,sizeof save);}
int web_load(int n){pg_pet pet;uint32_t seq;if(n<=0||n>PG_SAVE_MAX||!pg_save_decode(&pet,&seq,save,(size_t)n))return 0;pg_restore(&game,&pet);game.echo_audio=true;return 1;}
int web_state(int field){switch(field){case 0:return game.muted;case 1:return game.screen;case 2:return game.pet.phase;case 3:return game.echo.session;case 4:return game.echo.round;case 5:return game.echo.stage;case 6:return game.echo.prompt;case 7:return game.sleeping||game.standby;case 8:return game.echo.audio_wait;default:return 0;}}
int web_sound(void){pg_sound sound=game.sound;game.sound=PG_SOUND_NONE;if(sound==PG_SOUND_NONE||game.muted||game.standby||game.screen==PG_ECHO)return 0;pg_chip_start(&chip,sound,game.pet.phase==PG_ALIVE?(pg_species)game.pet.species:PG_MOSSLET,game.performance_variant);return (int)pg_chip_read(&chip,pcm,16000);}
int16_t *web_pcm(void){return pcm;}
void web_audio_done(unsigned session,unsigned round,int stage){pg_echo_audio_done(&game,session,round,(pg_echo_stage)stage);}
void web_audio_ready(int ready){game.echo_audio=ready!=0;}
