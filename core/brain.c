#include "brain.h"
#include "q4_model.h"
#include "word_tok.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef ESP_PLATFORM
#include "esp_heap_caps.h"
#endif

/* Track all engine allocations, including aligned prefill scratch, so the
   permanent-allocation upstream engine can be closed cleanly on desktop. */
struct pg_brain {
    q4_model_t *model;
    word_tok_t tokenizer;
    int goals[PG_GOAL_COUNT];
    void *blocks[96];
    size_t block_count, bytes;
};
static pg_brain *allocating;
static void *tracked_alloc(size_t n) {
    if(!allocating||allocating->block_count>=96||n>4718592||allocating->bytes>4718592-n)return NULL;
    #ifdef ESP_PLATFORM
    void *ptr=heap_caps_malloc(n,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
#else
    void *ptr=malloc(n);
#endif
    if(ptr){allocating->blocks[allocating->block_count++]=ptr;allocating->bytes+=n;}
    return ptr;
}
static int drive(float v){return (int)pg_clamp(v*0.09f,0,9);}
void pg_brain_encode(const pg_game *g,char *out,size_t cap) {
    const pg_pet *p=&g->pet;
    static const char *const stages[]={"baby","young","adult"};
    const char *memory="none";
    if(p->event_count){
        const pg_event *e=&p->events[(p->event_count-1)%PG_EVENT_CAP];
        if(e->kind==PG_EV_FED)memory="fed";
        else if(e->kind==PG_EV_WON)memory="won";
        else if(e->kind==PG_EV_LOST)memory="lost";
        else if(e->kind==PG_EV_PETTED)memory="petted";
        else if(e->kind==PG_EV_DISCOVERY)memory="new";
    }
    const char *favorite="food";
    float food=p->food_preference[pg_favorite_food(g)],toy=p->toy_preference[pg_favorite_toy(g)];
    if(toy>food)favorite="toy";
    else if(p->curious>75)favorite="plant";
    else if(p->energy<25)favorite="bed";
    snprintf(out,cap,
      "hunger %d energy %d clean %d affection %d trust %d bold %d curious %d playful %d bored %d "
      "stage %s last %s food %s toy yes plant yes player %s favorite %s memory %s",
      drive(100-p->satiety),drive(p->energy),drive(p->clean),drive(p->affection),drive(p->trust),
      drive(p->bold),drive(p->curious),drive(p->playful),drive(g->boredom),
      stages[g->stage],pg_goal_names[g->goal],g->bowl_time>0?"yes":"no",
      g->player_idle<90?"yes":"no",favorite,memory);
}
void pg_brain_snapshot(const pg_game *g,pg_brain_request *r) {
    memset(r,0,sizeof *r);
    r->identity=g->pet.identity;r->revision=g->revision;r->rng=g->pet.rng ^ g->goal_serial;
    pg_brain_encode(g,r->state,sizeof r->state);
    for(int i=0;i<PG_GOAL_COUNT;++i)if(pg_goal_feasible(g,(pg_goal)i))r->legal_mask|=(uint8_t)(1u<<i);
}
pg_brain *pg_brain_open(const uint8_t *model,size_t len,const uint8_t *tok,size_t tok_len) {
    if(allocating||!model||len<256||len>8*1024*1024)return NULL;
    int config[7];memcpy(config,model+8,sizeof config);
    if(config[0]<=0||config[0]>384||config[2]<=0||config[2]>8||config[5]!=51||config[6]<40||config[6]>96)return NULL;
    pg_brain *b=calloc(1,sizeof *b);if(!b)return NULL;
    allocating=b;q4_fast_alloc=NULL;
    b->model=q4_model_open(model,len,tracked_alloc);
    if(!b->model){pg_brain_close(b);return NULL;}
    const q4_config_t *cfg=q4_model_config(b->model);
    if(cfg->vocab_size!=51||cfg->seq_len<40||cfg->seq_len>96||cfg->dim>384||cfg->n_layers>8||
       word_tok_init(&b->tokenizer,tok,tok_len,cfg->vocab_size)) {pg_brain_close(b);return NULL;}
    for(int i=0;i<PG_GOAL_COUNT;++i){
        b->goals[i]=-1;
        for(int j=3;j<b->tokenizer.vocab_size;++j){
            const char *piece=word_tok_piece(&b->tokenizer,j);
            if(piece[0]==' '&&!strcmp(piece+1,pg_goal_names[i]))b->goals[i]=j;
        }
        if(b->goals[i]<0){pg_brain_close(b);return NULL;}
    }
    return b;
}
void pg_brain_close(pg_brain *b) {
    if(!b)return;
    for(int i=0;i<b->tokenizer.vocab_size;++i)free(b->tokenizer.vocab[i]);
    free(b->tokenizer.vocab);
    for(size_t i=0;i<b->block_count;++i)free(b->blocks[i]);
    if(allocating==b)allocating=NULL;
    free(b);
}
size_t pg_brain_memory(const pg_brain *b){return b?b->bytes:0;}
pg_brain_result pg_brain_infer(pg_brain *b,const pg_brain_request *r,float temperature) {
    pg_brain_result result={.goal=PG_GOAL_COUNT};
    if(!b||!r||!r->legal_mask)return result;
    result.identity=r->identity;result.revision=r->revision;
    char prompt[400];snprintf(prompt,sizeof prompt,"%s ->",r->state);
    int tokens[96],ids[PG_GOAL_COUNT],map[PG_GOAL_COUNT],n=0;
    int count=word_tok_encode(&b->tokenizer,prompt,tokens,90);
    if(count+4>q4_model_config(b->model)->seq_len)return result;
    for(int i=0;i<count;++i)if(tokens[i]==0)return result;
    for(int i=0;i<PG_GOAL_COUNT;++i)if(r->legal_mask&(1u<<i)){ids[n]=b->goals[i];map[n++]=i;}
    int out[3];float prob[PG_GOAL_COUNT]={0};uint32_t rng=r->rng?r->rng:1;
    allocating=b;
    int generated=q4_model_decide(b->model,tokens,count,ids,n,temperature,&rng,prob,out,3);
    if(generated<1)return result;
    for(int i=0;i<n;++i){
        if(!isfinite(prob[i])||prob[i]<0||prob[i]>1)return result;
        result.probabilities[map[i]]=prob[i];
        if(ids[i]==out[0]){result.goal=(pg_goal)map[i];result.confidence=prob[i];}
    }
    bool format_valid=false;
    result.urgency=5;
    if(generated==3&&!strcmp(word_tok_piece(&b->tokenizer,out[1])," urgency")){
        const char *digit=word_tok_piece(&b->tokenizer,out[2]);
        if(strlen(digit)==2&&digit[0]==' '&&digit[1]>='0'&&digit[1]<='9'){
            result.urgency=digit[1]-'0';format_valid=true;
        }
    }
    result.valid=format_valid&&result.goal>=0&&result.goal<PG_GOAL_COUNT;
    return result;
}
