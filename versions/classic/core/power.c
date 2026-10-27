#include "power.h"
void pg_power_button_init(pg_power_button *b,bool level,uint64_t now){
    *b=(pg_power_button){.down=level,.armed=!level,.changed_at=now};
}
bool pg_power_button_sample(pg_power_button *b,bool level,uint64_t now){
    if(now<b->changed_at){b->changed_at=now;return false;}
    uint64_t held=now-b->changed_at;
    if(level!=b->down){
        bool press=b->down&&!level&&b->armed&&held>=20000;
        if(!b->down&&held>=20000)b->armed=true;
        b->down=level;b->changed_at=now;
        if(press)b->armed=false;
        return press;
    }
    if(held<20000)return false;
    if(!level){b->armed=true;return false;}
    if(b->armed){b->armed=false;return true;}
    return false;
}
