#include "power.h"
#include <stdio.h>
#include <stdlib.h>
static unsigned checks;
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL power:%d %s\n",__LINE__,#x);exit(1);}++checks;}while(0)
int main(void){
    pg_power_button b;pg_power_button_init(&b,true,0);
    for(unsigned t=0;t<2000000;t+=10000)CHECK(!pg_power_button_sample(&b,true,t));
    CHECK(!pg_power_button_sample(&b,false,2000000));CHECK(!pg_power_button_sample(&b,false,2030000));
    CHECK(!pg_power_button_sample(&b,true,2050000));CHECK(pg_power_button_sample(&b,true,2080000));
    for(unsigned t=2090000;t<4000000;t+=10000)CHECK(!pg_power_button_sample(&b,true,t));
    CHECK(!pg_power_button_sample(&b,false,4000000));CHECK(!pg_power_button_sample(&b,false,4030000));
    CHECK(!pg_power_button_sample(&b,true,4040000));CHECK(pg_power_button_sample(&b,true,4070000));
    /* Contact bounce and a partial release cannot create another toggle. */
    CHECK(!pg_power_button_sample(&b,false,4080000));CHECK(!pg_power_button_sample(&b,true,4090000));
    CHECK(!pg_power_button_sample(&b,true,4120000));
    pg_power_button_init(&b,false,0);CHECK(!pg_power_button_sample(&b,true,1000));
    CHECK(!pg_power_button_sample(&b,false,3000));CHECK(!pg_power_button_sample(&b,true,7000));
    CHECK(pg_power_button_sample(&b,true,28000));CHECK(!pg_power_button_sample(&b,true,90000));
    CHECK(!pg_power_button_sample(&b,false,0));
    /* Both edges captured while rendering: an entire 35ms press between
       frames must still register, once, when the queued release is read. */
    pg_power_button_init(&b,false,0);
    CHECK(!pg_power_button_sample(&b,true,100000));
    CHECK(pg_power_button_sample(&b,false,135000));
    CHECK(!pg_power_button_sample(&b,false,180000));
    CHECK(!pg_power_button_sample(&b,true,200000));
    CHECK(!pg_power_button_sample(&b,false,205000));
    CHECK(!pg_power_button_sample(&b,false,240000));
    CHECK(!pg_power_button_sample(&b,true,260000));
    CHECK(pg_power_button_sample(&b,false,300000));
    printf("PASS %u power-button checks: held wake consumed, stable press/release, bounce, repeat and clock regression\n",checks);
}
