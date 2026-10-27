import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {createHash} from 'node:crypto';
import {versions,selectVersion} from '../site/versions.mjs';
assert.equal(selectVersion(''), 'forest');
assert.equal(selectVersion('?version=classic'), 'classic');
assert.equal(selectVersion('?version=untrusted'), 'forest');
assert.equal(versions.classic.storageKey,'pokegotchi-browser-v1','existing saves stay in Classic');
assert.notEqual(versions.classic.storageKey,versions.forest.storageKey,'independent progress');
const captures=[];
for(const version of ['forest','classic']){
 const {default:createGame}=await import(`../dist/versions/${version}/game.mjs`);
 const m=await createGame({wasmBinary:await readFile(new URL(`../dist/versions/${version}/game.wasm`,import.meta.url))});
 m._web_init(42);
 const pixels=m.HEAPU8.slice(m._web_frame(),m._web_frame()+466*466*2);
 captures.push(createHash('sha256').update(pixels).digest('hex'));
}
assert.notEqual(...captures,'switch loads two genuinely different renderers');
console.log('Both editions: distinct renderer hashes, safe version selection and isolated save keys pass');
