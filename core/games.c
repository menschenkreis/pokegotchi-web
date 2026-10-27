#include "game.h"
#include <math.h>
#include <string.h>

int pg_ttt_winner(const int8_t b[9]) {
    static const int lines[8][3] = {{0,1,2},{3,4,5},{6,7,8},{0,3,6},{1,4,7},{2,5,8},{0,4,8},{2,4,6}};
    for (int i = 0; i < 8; ++i) {
        int a = b[lines[i][0]];
        if (a && a == b[lines[i][1]] && a == b[lines[i][2]]) return a;
    }
    for (int i = 0; i < 9; ++i) if (!b[i]) return 0;
    return 2; /* draw */
}
static int minimax(int8_t b[9], int turn, int depth) {
    int w = pg_ttt_winner(b);
    if (w == -1) return 10 - depth;
    if (w == 1) return depth - 10;
    if (w == 2) return 0;
    int best = turn == -1 ? -100 : 100;
    for (int i = 0; i < 9; ++i) if (!b[i]) {
        b[i] = (int8_t)turn;
        int v = minimax(b, -turn, depth + 1);
        b[i] = 0;
        if (turn == -1 ? v > best : v < best) best = v;
    }
    return best;
}
int pg_ttt_best(const int8_t board[9], int player) {
    if ((player != -1 && player != 1) || pg_ttt_winner(board)) return -1;
    int8_t b[9]; memcpy(b, board, sizeof b);
    int best = player == -1 ? -100 : 100, move = -1;
    /* Center/corners first is deterministic and easy to understand. */
    static const int order[9] = {4,0,2,6,8,1,3,5,7};
    for (int j = 0; j < 9; ++j) {
        int i = order[j];
        if (b[i]) continue;
        b[i] = (int8_t)player;
        int v = minimax(b, -player, 0);
        b[i] = 0;
        if (move < 0 || (player == -1 ? v > best : v < best)) { best = v; move = i; }
    }
    return move;
}
void pg_game_result(pg_game *g, int which, int result) {
    if (which < 0 || which >= PG_GAME_COUNT || g->pet.phase != PG_ALIVE) return;
    ++g->pet.games_played[which]; ++g->session_games;
    if (result > 0) ++g->pet.games_won[which];
    g->pet.confidence[which] = pg_clamp(g->pet.confidence[which] + (result > 0 ? 7 : result < 0 ? -3 : 2), 0, 100);
    if(which<3)g->pet.toy_preference[which] = pg_clamp(g->pet.toy_preference[which] + (result >= 0 ? 3 : 1), 0, 100);
    g->pet.affection = pg_clamp(g->pet.affection + 5, 0, 100);
    g->pet.trust = pg_clamp(g->pet.trust + 0.75f, 0, 100);
    g->pet.playful = pg_clamp(g->pet.playful + 0.15f, 0, 100);
    if (g->pet.games_played[which] == 1) {
        ++g->pet.discoveries;
        if(which<3)g->pet.toy_style = (uint8_t)which;
        pg_event_add(g, PG_EV_DISCOVERY, 128 + which);
    }
    pg_event_add(g, result > 0 ? PG_EV_WON : result < 0 ? PG_EV_LOST : PG_EV_DRAW, which);
    pg_say(g, result > 0 ? "YOU DID IT! LITTLE VICTORY DANCE." : result < 0 ?
        "ONE MORE? WE'RE LEARNING TOGETHER." : "A PERFECTLY MATCHED PAIR.",
        result >= 0 ? PG_SOUND_WIN : PG_SOUND_LOSE);
    g->particles = result >= 0 ? 2 : 0;
}
void pg_ttt_start(pg_game *g) {
    bool starter = (g->ttt.round % 2) != 0;
    uint32_t round = g->ttt.round + 1;
    memset(&g->ttt, 0, sizeof g->ttt);
    g->ttt.round = round; g->ttt.pet_starts = starter;
    g->ttt.turn = starter ? -1 : 1; g->ttt.think = 0.7f;
    g->screen = PG_TTT;
}
static void ttt_check(pg_game *g) {
    int w = pg_ttt_winner(g->ttt.cells);
    if (!w) return;
    g->ttt.finished = true; g->ttt.winner = (int8_t)w;
    pg_game_result(g, 0, w == 1 ? 1 : w == -1 ? -1 : 0);
}
bool pg_ttt_move(pg_game *g, int cell) {
    if (g->pet.phase != PG_ALIVE || g->screen != PG_TTT || g->ttt.finished ||
        g->ttt.turn != 1 || cell < 0 || cell > 8 || g->ttt.cells[cell]) return false;
    g->ttt.cells[cell] = 1; g->ttt.turn = -1; g->ttt.think = 0.6f;
    ttt_check(g); return true;
}
static void tilt_target(pg_game *g) {
    do {
        float angle = pg_randf(&g->pet) * 6.2831853f;
        float r = sqrtf(pg_randf(&g->pet)) * 83;
        g->tilt.target_x = 233 + cosf(angle) * r;
        g->tilt.target_y = 244 + sinf(angle) * r;
    } while(hypotf(g->tilt.x-g->tilt.target_x,g->tilt.y-g->tilt.target_y)<40);
}
void pg_tilt_recenter(pg_game *g){
    if(g->screen!=PG_TILT||g->tilt.finished)return;
    g->tilt.ready=1.2f;g->tilt.vx=g->tilt.vy=0;
    g->tilt_x=g->tilt_y=0;++g->motion_serial;
}
void pg_tilt_start(pg_game *g) {
    memset(&g->tilt, 0, sizeof g->tilt);
    g->tilt_x=g->tilt_y=0;
    g->tilt.x = 233; g->tilt.y = 244; g->tilt.seconds = 30;
    tilt_target(g); g->screen = PG_TILT;pg_tilt_recenter(g);
}
void pg_rhythm_start(pg_game *g) {
    memset(&g->rhythm, 0, sizeof g->rhythm);
    g->rhythm.length = 2; g->rhythm.lit = -1;
    g->rhythm.showing = true; g->rhythm.timer = 0.7f;
    for (int i = 0; i < 16; ++i) g->rhythm.notes[i] = (uint8_t)(pg_random(&g->pet) % 3);
    g->screen = PG_RHYTHM;
}
void pg_rhythm_tap(pg_game *g, int pad) {
    pg_rhythm *r = &g->rhythm;
    if (g->screen != PG_RHYTHM || r->showing || r->finished || pad < 0 || pad > 2 || r->timer > 0) return;
    r->lit = pad; r->timer = 0.18f;
    if (!g->muted) g->sound = (pg_sound)(PG_SOUND_NOTE0 + pad);
    if (r->notes[r->cursor] != pad) {
        r->finished = true;
        pg_game_result(g, 2, r->score >= 3 ? 1 : -1);
        return;
    }
    if (++r->cursor == r->length) {
        ++r->score;
        if (r->length >= 10) {
            r->finished = true; pg_game_result(g, 2, 1);
        } else {
            ++r->length; r->showing = true; r->cursor = 0; r->lit = -1; r->timer = 1;
        }
    }
}
void pg_open_game(pg_game *g, int which) {
    if (g->standby || g->pet.phase != PG_ALIVE || g->sleeping || which<0 || which>=PG_GAME_COUNT) return;
    pg_echo_cancel(g);
    g->invitation = false; ++g->revision;
    if (which == 0) pg_ttt_start(g);
    else if (which == 1) pg_tilt_start(g);
    else if (which == 2) pg_rhythm_start(g);
    else if (which == 3) pg_echo_start(g);
    else if (which == 4) pg_balance_start(g);
    else if (which == 5) pg_wash_start(g);
}
void pg_games_tick(pg_game *g, float dt) {
    if (g->standby || g->pet.phase != PG_ALIVE || g->sleeping) return;
    pg_echo_tick(g,dt);
    pg_nurture_tick(g,dt);
    if (g->screen == PG_TTT && !g->ttt.finished && g->ttt.turn == -1) {
        g->ttt.think -= dt;
        if (g->ttt.think <= 0) {
            int move = pg_ttt_best(g->ttt.cells, -1);
            float skill = g->pet.confidence[0] / 100;
            if (pg_randf(&g->pet) > 0.5f + 0.5f * skill) {
                int free_cells[9], n = 0;
                for (int i = 0; i < 9; ++i) if (!g->ttt.cells[i]) free_cells[n++] = i;
                if (n) move = free_cells[pg_random(&g->pet) % (uint32_t)n];
            }
            if (move >= 0) g->ttt.cells[move] = -1;
            g->ttt.turn = 1; ttt_check(g);
        }
    } else if (g->screen == PG_TILT && !g->tilt.finished) {
        pg_tilt *t = &g->tilt;
        if(t->ready>0){t->ready=fmaxf(0,t->ready-dt);return;}
        t->seconds = fmaxf(0, t->seconds - dt);
        t->vx = (t->vx + g->tilt_x * 220 * dt) * expf(-2 * dt);
        t->vy = (t->vy + g->tilt_y * 220 * dt) * expf(-2 * dt);
        t->x += t->vx * dt; t->y += t->vy * dt;
        float dx = t->x - 233, dy = t->y - 244, d = sqrtf(dx * dx + dy * dy);
        if (d > 103) {
            t->x = 233 + dx / d * 103; t->y = 244 + dy / d * 103;
            float dot = (t->vx * dx + t->vy * dy) / d;
            if (dot > 0) { t->vx -= 1.5f * dot * dx / d; t->vy -= 1.5f * dot * dy / d; }
        }
        dx = t->x - t->target_x; dy = t->y - t->target_y;
        if (dx * dx + dy * dy < 22 * 22) { ++t->score; tilt_target(g); if (!g->muted) g->sound = PG_SOUND_HAPPY; }
        if (t->seconds <= 0) { t->finished = true; pg_game_result(g, 1, t->score >= 5 ? 1 : -1); }
    } else if (g->screen == PG_RHYTHM && !g->rhythm.finished) {
        pg_rhythm *r = &g->rhythm;
        r->timer = fmaxf(0, r->timer - dt);
        if (r->showing && r->timer <= 0) {
            if (r->lit >= 0) {
                r->lit = -1; ++r->cursor; r->timer = 0.24f;
                if (r->cursor == r->length) { r->showing = false; r->cursor = 0; r->timer = 0.3f; }
            } else {
                r->lit = r->notes[r->cursor]; r->timer = 0.55f;
                if (!g->muted) g->sound = (pg_sound)(PG_SOUND_NOTE0 + r->lit);
            }
        } else if (!r->showing && r->timer <= 0) r->lit = -1;
    }
}
