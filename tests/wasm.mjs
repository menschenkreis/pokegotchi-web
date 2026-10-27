import createGame from '../dist/game.mjs';
import {readFile} from 'node:fs/promises';
import assert from 'node:assert/strict';
const m=await createGame({wasmBinary:await readFile(new URL('../dist/game.wasm',import.meta.url))});
m._web_init(42);
assert.equal(m._web_state(2),0,'starts with mystery egg');
assert.equal(m._web_state(0),1,'startup muted');
const first=m.HEAPU16.slice(m._web_frame()/2,m._web_frame()/2+466*466);
assert.ok(new Set(first).size>15,'renderer produces artwork');
m._web_hatch();assert.equal(m._web_state(2),1,'hatch reveal starts');
for(let i=0;i<250;i++)m._web_tick(.04);
assert.equal(m._web_state(2),2,'reveal completes');
for(let i=0;i<6;i++){m._web_open(i);for(let t=0;t<100;t++)m._web_tick(0.04);m._web_frame();assert.equal(m._web_state(1),[5,6,7,11,13,14][i]);m._web_key(0);}
const n=m._web_save(),p=m._web_save_buffer();const bytes=m.HEAPU8.slice(p,p+n);
m._web_init(9);m.HEAPU8.set(bytes,p);assert.equal(m._web_load(n),1);assert.equal(m._web_state(2),2);
m.HEAPU8[p]^=255;assert.equal(m._web_load(n),0,'rejects corrupt saves');
m._web_key(1);assert.equal(m._web_state(7),1);m._web_key(1);assert.equal(m._web_state(7),0);
console.log('WASM: render, six games, silent startup, save roundtrip, corruption rejection and standby pass');
