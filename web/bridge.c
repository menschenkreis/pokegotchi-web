#include "game.h"
#include "draw.h"
#include "chip.h"
#include "checkpoint.h"
#include "tutorial_input.h"
#include <math.h>
#include <string.h>

static pg_game game;
static uint16_t pixels[PG_W*PG_H];
static uint8_t save[PG_SAVE_MAX];
static int16_t pcm[16000];
static pg_chip chip;
static pg_canvas canvas={pixels,PG_W,PG_H};
static pg_tutorial tutorial;
static uint64_t epoch;
static double utility_seconds;
static void begin(unsigned seed,pg_tutorial_stage stage){
    pg_init(&game,seed);game.echo_audio=true;game.transactional_resets=true;
    memset(game.pet.events,0,sizeof game.pet.events);game.pet.event_count=0;
    game.caption_time=game.hud_seconds=game.top_hud_seconds=0;
    pg_tutorial_restore(&tutorial,stage);utility_seconds=0;
}
void web_init(unsigned seed){epoch=0;begin(seed,PG_TUTORIAL_SWIPE);}
void web_reset(unsigned seed){
    pg_device_data device=game.device;device.echo_best=0;
    bool muted=game.muted;uint8_t brightness=game.pet.brightness;
    ++epoch;begin(seed,PG_TUTORIAL_SWIPE);pg_device_attach(&game,&device,true);
    game.muted=game.pet.muted=muted;game.pet.brightness=brightness;
}
void web_tick(double dt){
    if(!isfinite(dt)||dt<=0||dt>3600)return;
    utility_seconds+=dt;
    pg_utilities_step(&game,(pg_time_source){1,(uint64_t)(utility_seconds*1000000),true});
    if(tutorial.committed!=PG_TUTORIAL_LANDED){
        if(!game.standby){game.anim+=(float)dt;game.top_hud_seconds=fmaxf(0,game.top_hud_seconds-(float)dt);}
        pg_tutorial_tick(&tutorial,(float)dt,game.standby);
        pg_tutorial_stage next;
        if(pg_tutorial_commit_requested(&tutorial,&next)){
            pg_tutorial_resolve_commit(&tutorial,PG_TUTORIAL_COMMIT_OK);
            if(next==PG_TUTORIAL_LANDED){pg_event_add(&game,PG_EV_FOUND,0);game.sleeping=false;}
        }
    }else pg_tick(&game,dt);
    if(game.reset_request!=PG_REQUEST_NONE){
        bool successor=game.reset_request==PG_REQUEST_SUCCESSOR;
        web_reset(pg_random(&game.pet));
        if(successor)pg_tutorial_restore(&tutorial,PG_TUTORIAL_DELIVERY);
    }
}
uint16_t *web_frame(void){
    if(tutorial.committed!=PG_TUTORIAL_LANDED)pg_render_tutorial_preview(&canvas,&game,&tutorial);
    else pg_render(&canvas,&game);return pixels;
}
void web_pointer(int kind,float x,float y){
    if(tutorial.committed!=PG_TUTORIAL_LANDED)pg_tutorial_pointer(&game,&tutorial,kind,x,y,true);
    else pg_pointer(&game,kind,x,y);
}
void web_tilt(float x,float y){if(!game.pointer_down){game.tilt_x=x;game.tilt_y=y;}}
void web_key(int key){
    if(key==0&&tutorial.committed==PG_TUTORIAL_LANDED)pg_back(&game);
    if(key==1)pg_standby(&game,!game.standby);
    if(key==2){game.muted=!game.muted;game.pet.muted=game.muted;game.dirty=true;}
    if(key==3)pg_echo_shake_key(&game);
    if(key==4)pg_clap(&game);
    if(key>=5&&key<=7&&game.screen==PG_RHYTHM)pg_rhythm_tap(&game,key-5);
}
void web_hatch(void){if(tutorial.committed==PG_TUTORIAL_LANDED)pg_debug_hatch(&game);}
void web_open(int which){if(which>=0&&which<PG_GAME_COUNT&&game.pet.phase==PG_ALIVE&&!game.pet.naming_pending){pg_sleep(&game,false);pg_standby(&game,false);pg_open_game(&game,which);}}
uint8_t *web_save_buffer(void){return save;}
/* localStorage atomically replaces this single life/scene/settings value.
   Hardware replicated-write and retained-power protocols are separate. */
int web_save(void){
    pg_checkpoint checkpoint={.sequence=1,.life_sequence=1};
    if(!pg_tutorial_life_from_pet(&checkpoint.life,&game.pet,epoch,tutorial.committed))return 0;
    pg_sleep_pose_capture(&game,&checkpoint.scene);
    memcpy(save,"PGWB\1\0\0\0",8);
    if(!pg_checkpoint_encode(&checkpoint,save+8,PG_CHECKPOINT_BYTES))return 0;
    if(pg_tutorial_device_v2_encode(&game.device,1,epoch,save+8+PG_CHECKPOINT_BYTES,PG_TUTORIAL_DEVICE_V2_BYTES)!=PG_TUTORIAL_DEVICE_V2_BYTES)return 0;
    return 8+PG_CHECKPOINT_BYTES+PG_TUTORIAL_DEVICE_V2_BYTES;
}
int web_load(int n){
    if(n<=0||n>PG_SAVE_MAX)return 0;
    pg_tutorial_life life;pg_checkpoint checkpoint;pg_device_data device;
    bool modern=n==8+PG_CHECKPOINT_BYTES+PG_TUTORIAL_DEVICE_V2_BYTES&&!memcmp(save,"PGWB\1\0\0\0",8);
    if(modern){
        uint64_t device_epoch;
        if(!pg_checkpoint_decode(&checkpoint,save+8,PG_CHECKPOINT_BYTES)||checkpoint.sleep_flags||
           !pg_tutorial_device_v2_decode(&device,NULL,&device_epoch,save+8+PG_CHECKPOINT_BYTES,PG_TUTORIAL_DEVICE_V2_BYTES)||device_epoch!=checkpoint.life.epoch)return 0;
        life=checkpoint.life;
    }else if(!pg_tutorial_life_decode(&life,NULL,save,(size_t)n))return 0;
    pg_restore(&game,&life.pet);epoch=life.epoch;
    if(modern){pg_device_attach(&game,&device,true);pg_sleep_pose_restore(&game,&checkpoint.scene);}
    game.echo_audio=true;game.transactional_resets=true;utility_seconds=0;
    pg_tutorial_restore(&tutorial,life.stage);
    if(life.stage!=PG_TUTORIAL_LANDED){game.caption_time=game.hud_seconds=game.top_hud_seconds=0;game.sleeping=life.stage==PG_TUTORIAL_LAMP_DAY;}
    return 1;
}
int web_state(int field){switch(field){case 0:return game.muted;case 1:return game.screen;case 2:return game.pet.phase;case 3:return game.echo.session;case 4:return game.echo.round;case 5:return game.echo.stage;case 6:return game.echo.prompt;case 7:return game.sleeping||game.standby;case 8:return game.echo.audio_wait;case 9:return tutorial.committed;case 10:return game.pet.naming_pending;case 11:return (int)game.pet.incubation;case 12:return PG_EGG_SECONDS;default:return 0;}}
int web_sound(void){pg_sound sound=game.sound;game.sound=PG_SOUND_NONE;if(sound==PG_SOUND_NONE||game.muted||game.standby||game.screen==PG_ECHO)return 0;pg_chip_start(&chip,sound,game.pet.phase==PG_ALIVE?(pg_species)game.pet.species:PG_MOSSLET,game.performance_variant);return (int)pg_chip_read(&chip,pcm,16000);}
int16_t *web_pcm(void){return pcm;}
void web_audio_done(unsigned session,unsigned round,int stage){pg_echo_audio_done(&game,session,round,(pg_echo_stage)stage);}
void web_audio_ready(int ready){game.echo_audio=ready!=0;}
