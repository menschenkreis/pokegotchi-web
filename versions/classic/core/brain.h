#ifndef PG_BRAIN_H
#define PG_BRAIN_H
#include "game.h"
typedef struct pg_brain pg_brain;
typedef struct {
    uint32_t identity, revision, rng;
    char state[384];
    uint8_t legal_mask;
} pg_brain_request;
typedef struct {
    uint32_t identity, revision;
    pg_goal goal;
    float confidence, probabilities[PG_GOAL_COUNT];
    int urgency;
    bool valid;
} pg_brain_result;

void pg_brain_encode(const pg_game *g,char *out,size_t cap);
void pg_brain_snapshot(const pg_game *g,pg_brain_request *request);
/* One loaded brain per process. Model bytes must outlive the brain. */
pg_brain *pg_brain_open(const uint8_t *model,size_t model_len,const uint8_t *tokenizer,size_t tokenizer_len);
void pg_brain_close(pg_brain *brain);
pg_brain_result pg_brain_infer(pg_brain *brain,const pg_brain_request *request,float temperature);
size_t pg_brain_memory(const pg_brain *brain);
#endif
