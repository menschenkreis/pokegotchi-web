#include "game.h"
#include <math.h>
#include <string.h>

typedef struct { uint8_t *dst; const uint8_t *src; size_t at, cap; bool ok; } codec;
static void put32(codec *c, uint32_t v) {
    if (!c->ok || c->at + 4 > c->cap) { c->ok = false; return; }
    for (int i = 0; i < 4; ++i) c->dst[c->at++] = (uint8_t)(v >> (i * 8));
}
static uint32_t get32(codec *c) {
    if (!c->ok || c->at + 4 > c->cap) { c->ok = false; return 0; }
    uint32_t v = 0;
    for (int i = 0; i < 4; ++i) v |= (uint32_t)c->src[c->at++] << (i * 8);
    return v;
}
static void putf(codec *c, float f) { uint32_t u; memcpy(&u, &f, 4); put32(c, u); }
static float getf(codec *c) { uint32_t u = get32(c); float f; memcpy(&f, &u, 4); return f; }
static void putd(codec *c, double d) { uint64_t u; memcpy(&u, &d, 8); put32(c, (uint32_t)u); put32(c, (uint32_t)(u >> 32)); }
static double getd(codec *c) {
    uint64_t lo = get32(c), hi = get32(c), u = lo | (hi << 32);
    double d; memcpy(&d, &u, 8); return d;
}
uint32_t pg_crc32(const void *data, size_t len) {
    const uint8_t *b = data; uint32_t crc = UINT32_MAX;
    for (size_t i = 0; i < len; ++i) {
        crc ^= b[i];
        for (int j = 0; j < 8; ++j) crc = (crc >> 1) ^ (UINT32_C(0xedb88320) & (0u - (crc & 1u)));
    }
    return ~crc;
}

bool pg_save_valid_pet(const pg_pet *p) {
    if (!p || !p->rng || p->species >= PG_SPECIES_COUNT || p->phase < PG_EGG || p->phase > PG_DEAD ||
        !p->generation || p->brightness<20 || p->brightness>100 || !memchr(p->name, 0, sizeof p->name) || !p->name[0] ||
        !memchr(p->legacy_name,0,sizeof p->legacy_name)) return false;
    double times[] = {p->elapsed, p->age, p->incubation};
    for (size_t i = 0; i < sizeof times / sizeof *times; ++i)
        if (!isfinite(times[i]) || times[i] < 0 || times[i] > 36525.0 * PG_DAY) return false;
    if (!isfinite(p->last_egg_visit) || p->last_egg_visit < -60 || p->last_egg_visit > p->elapsed ||
        p->age > p->elapsed || p->incubation > PG_EGG_SECONDS ||
        p->plant_style > 2 || p->bed_style > 2 || p->toy_style > 2 || p->legacy_species >= PG_SPECIES_COUNT) return false;
    float meters[] = {p->satiety,p->energy,p->clean,p->affection,p->health,p->trust,
        p->warmth,p->comfort,p->bold,p->curious,p->playful};
    for (size_t i = 0; i < sizeof meters / sizeof *meters; ++i)
        if (!isfinite(meters[i]) || meters[i] < 0 || meters[i] > 100) return false;
    for (int i = 0; i < 3; ++i)
        if (!isfinite(p->food_preference[i]) || p->food_preference[i] < 0 || p->food_preference[i] > 100 ||
            !isfinite(p->toy_preference[i]) || p->toy_preference[i] < 0 || p->toy_preference[i] > 100 ||
            !isfinite(p->confidence[i]) || p->confidence[i] < 0 || p->confidence[i] > 100) return false;
    for(int i=3;i<PG_GAME_COUNT;++i)if(!isfinite(p->confidence[i])||p->confidence[i]<0||p->confidence[i]>100)return false;
    for(int i=0;i<PG_GAME_COUNT;++i)if(p->games_won[i]>p->games_played[i])return false;
    uint32_t n = p->event_count < PG_EVENT_CAP ? p->event_count : PG_EVENT_CAP;
    for (uint32_t i = 0; i < n; ++i)
        if (p->events[i].kind > PG_EV_SUCCESSOR || !isfinite(p->events[i].at) ||
            p->events[i].at < 0 || p->events[i].at > p->elapsed) return false;
    return true;
}
size_t pg_save_encode(const pg_pet *p, uint32_t sequence, uint8_t *out, size_t cap) {
    if (!out || cap < 32 || !pg_save_valid_pet(p)) return 0;
    codec c = {.dst=out,.cap=cap,.ok=true};
    put32(&c, UINT32_C(0x544f4750)); put32(&c, 4); put32(&c, sequence); put32(&c, 0);
    put32(&c,p->rng); put32(&c,p->identity); put32(&c,p->generation); put32(&c,p->species); put32(&c,(uint32_t)p->phase);
    putd(&c,p->elapsed); putd(&c,p->age); putd(&c,p->incubation); putd(&c,p->last_egg_visit);
    putf(&c,p->satiety); putf(&c,p->energy); putf(&c,p->clean); putf(&c,p->affection); putf(&c,p->health); putf(&c,p->trust);
    putf(&c,p->warmth); putf(&c,p->comfort); putf(&c,p->bold); putf(&c,p->curious); putf(&c,p->playful);
    for (int i=0;i<3;++i) { putf(&c,p->food_preference[i]); putf(&c,p->toy_preference[i]); putf(&c,p->confidence[i]); }
    put32(&c,p->meals); put32(&c,p->pets);
    for (int i=0;i<3;++i) { put32(&c,p->games_played[i]); put32(&c,p->games_won[i]); }
    put32(&c,p->discoveries); put32(&c,p->egg_visits); put32(&c,p->milestones); put32(&c,p->legacy_count);
    put32(&c,(uint32_t)p->plant_style | (uint32_t)p->bed_style << 8 | (uint32_t)p->toy_style << 16 | (uint32_t)p->legacy_species << 24);
    put32(&c,(p->sheltered?1u:0u) | (p->death_acknowledged?2u:0u) | (p->muted?4u:0u));
    for (int i=0;i<16;i+=4) {
        uint32_t word = 0; for (int j=0;j<4;++j) word |= (uint32_t)(uint8_t)p->name[i+j] << (8*j); put32(&c,word);
    }
    put32(&c,p->event_count);
    for (int i=0;i<PG_EVENT_CAP;++i) {
        put32(&c,(uint32_t)p->events[i].kind | (uint32_t)p->events[i].arg << 8); putd(&c,p->events[i].at);
    }
    for(int i=0;i<16;i+=4){
        uint32_t word=0;for(int j=0;j<4;++j)word|=(uint32_t)(uint8_t)p->legacy_name[i+j]<<(8*j);put32(&c,word);
    }
    putf(&c,p->confidence[3]);put32(&c,p->games_played[3]);put32(&c,p->games_won[3]);
    for(int i=4;i<PG_GAME_COUNT;++i){putf(&c,p->confidence[i]);put32(&c,p->games_played[i]);put32(&c,p->games_won[i]);}
    put32(&c,p->brightness);
    if (!c.ok || c.at + 4 > cap) return 0;
    size_t end = c.at;
    codec length = {.dst=out,.at=12,.cap=cap,.ok=true}; put32(&length,(uint32_t)(end - 16));
    put32(&c,pg_crc32(out,end));
    return c.ok ? c.at : 0;
}
bool pg_save_decode(pg_pet *p, uint32_t *sequence, const uint8_t *data, size_t len) {
    if (!p || !data || len < 32 || len > PG_SAVE_MAX) return false;
    codec c = {.src=data,.cap=len,.ok=true};
    if (get32(&c)!=UINT32_C(0x544f4750)) return false;
    uint32_t version=get32(&c);if(version<1||version>4)return false;
    uint32_t seq=get32(&c), size=get32(&c);
    if (size > PG_SAVE_MAX - 20 || len != (size_t)size + 20) return false;
    codec tail = {.src=data,.at=len-4,.cap=len,.ok=true};
    if (get32(&tail)!=pg_crc32(data,len-4)) return false;
    c.cap=len-4;
    pg_pet v; memset(&v,0,sizeof v);
    v.rng=get32(&c);v.identity=get32(&c);v.generation=get32(&c);v.species=get32(&c);v.phase=(pg_phase)get32(&c);
    v.elapsed=getd(&c);v.age=getd(&c);v.incubation=getd(&c);v.last_egg_visit=getd(&c);
    v.satiety=getf(&c);v.energy=getf(&c);v.clean=getf(&c);v.affection=getf(&c);v.health=getf(&c);v.trust=getf(&c);
    v.warmth=getf(&c);v.comfort=getf(&c);v.bold=getf(&c);v.curious=getf(&c);v.playful=getf(&c);
    for (int i=0;i<3;++i) {v.food_preference[i]=getf(&c);v.toy_preference[i]=getf(&c);v.confidence[i]=getf(&c);}
    v.meals=get32(&c);v.pets=get32(&c);
    for (int i=0;i<3;++i) {v.games_played[i]=get32(&c);v.games_won[i]=get32(&c);}
    v.discoveries=get32(&c);v.egg_visits=get32(&c);v.milestones=get32(&c);v.legacy_count=get32(&c);
    uint32_t styles=get32(&c), flags=get32(&c);
    if(flags & ~7u) return false;
    v.plant_style=(uint8_t)styles;v.bed_style=(uint8_t)(styles>>8);v.toy_style=(uint8_t)(styles>>16);v.legacy_species=(uint8_t)(styles>>24);
    v.sheltered=(flags&1)!=0;v.death_acknowledged=(flags&2)!=0;v.muted=(flags&4)!=0;
    for(int i=0;i<16;i+=4) {uint32_t word=get32(&c);for(int j=0;j<4;++j)v.name[i+j]=(char)(word>>(8*j));}
    v.event_count=get32(&c);
    for(int i=0;i<PG_EVENT_CAP;++i) {uint32_t e=get32(&c);v.events[i].kind=(uint8_t)e;v.events[i].arg=(uint8_t)(e>>8);v.events[i].at=getd(&c);}
    if(version>=2)for(int i=0;i<16;i+=4){uint32_t word=get32(&c);for(int j=0;j<4;++j)v.legacy_name[i+j]=(char)(word>>(8*j));}
    for(int i=3;i<PG_GAME_COUNT;++i)v.confidence[i]=40;
    v.brightness=65;
    if(version>=3){v.confidence[3]=getf(&c);v.games_played[3]=get32(&c);v.games_won[3]=get32(&c);}
    if(version>=4){for(int i=4;i<PG_GAME_COUNT;++i){v.confidence[i]=getf(&c);v.games_played[i]=get32(&c);v.games_won[i]=get32(&c);}
        uint32_t brightness=get32(&c);if(brightness>100)return false;v.brightness=(uint8_t)brightness;}
    if (!c.ok || c.at != c.cap || !pg_save_valid_pet(&v)) return false;
    *p=v;if(sequence)*sequence=seq;return true;
}
