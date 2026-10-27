#ifndef PG_TUTORIAL_INPUT_H
#define PG_TUTORIAL_INPUT_H
#include "game.h"
#include "tutorial.h"
/* Persistent callers acknowledge only after the requested transition is durable. */
void pg_tutorial_pointer(pg_game *game,pg_tutorial *tutorial,int kind,float x,float y,bool auto_commit);
#endif
