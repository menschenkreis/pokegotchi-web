import createGame from './game.mjs';
const $=s=>document.querySelector(s),canvas=$('#screen'),ctx=canvas.getContext('2d',{alpha:false});
const status=$('#status'),saveNote=$('#save-note'),storageKey='pokegotchi-browser-v1';
let m,activePointer=null,last=0,accum=0,saveElapsed=0,keys=new Set(),pad=[0,0],audio,playing=[],echoToken='',ready=false,storageBlocked=false;
function stopAudio(){for(const s of playing){s.onended=null;try{s.stop();}catch{}}playing=[];}
async function unlockAudio(){try{audio??=new AudioContext({sampleRate:16000});if(audio.state==='suspended')await audio.resume();await loadClips();}catch{}}
function play(buffer,done){if(!audio||audio.state!=='running')return false;const source=audio.createBufferSource();source.buffer=buffer;const gain=audio.createGain();gain.gain.value=.55;source.connect(gain).connect(audio.destination);source.onended=()=>{playing=playing.filter(x=>x!==source);done?.();};playing.push(source);source.start();return true;}
const clips={};
async function loadClips(){if(!audio||Object.keys(clips).length===6)return;await Promise.all(['shake','swipe','tap','go','pass','retry'].map(async name=>{try{const response=await fetch(`audio/${name}.wav`);if(!response.ok)throw Error();clips[name]=await audio.decodeAudioData(await response.arrayBuffer());}catch{}}));}
function state(n){return m._web_state(n);}
function updateSound(){const muted=state(0);$('#sound').textContent=`Sound: ${muted?'off':'on'}`;$('#sound').setAttribute('aria-pressed',String(!muted));if(muted||state(7))stopAudio();}
function key(n){if(!ready)return;m._web_key(n);updateSound();persist();}
function persist(){if(!ready||storageBlocked)return;try{const n=m._web_save(),p=m._web_save_buffer();if(!n)throw Error("Save validation failed");const bytes=m.HEAPU8.subarray(p,p+n);localStorage.setItem(storageKey,btoa(String.fromCharCode(...bytes)));saveNote.textContent='Saved on this browser. Time pauses while this tab is hidden. Sound starts off.';}catch{saveNote.textContent='Browser storage is unavailable. This session will not be saved.';}}
function release(){if(activePointer!==null&&ready)m._web_pointer(3,-100,-100);activePointer=null;pad=[0,0];keys.clear();if(ready)m._web_tilt(0,0);}
function position(e){const r=canvas.getBoundingClientRect();return [(e.clientX-r.left)*466/r.width,(e.clientY-r.top)*466/r.height];}
canvas.addEventListener('pointerdown',e=>{if(!ready||activePointer!==null||e.button!==0)return;e.preventDefault();canvas.focus({preventScroll:true});unlockAudio();activePointer=e.pointerId;canvas.setPointerCapture(e.pointerId);m._web_pointer(0,...position(e));});
canvas.addEventListener('pointermove',e=>{if(ready&&activePointer===e.pointerId)m._web_pointer(1,...position(e));});
canvas.addEventListener('pointerup',e=>{if(ready&&activePointer===e.pointerId){m._web_pointer(2,...position(e));activePointer=null;updateSound();persist();}});
canvas.addEventListener('pointercancel',release);
canvas.addEventListener('lostpointercapture',()=>{if(activePointer!==null)release();});
$('#back').onclick=()=>key(0);$('#power').onclick=()=>{stopAudio();key(1);};
$('#sound').onclick=async()=>{await unlockAudio();await loadClips();key(2);};
$('#shake').onclick=()=>key(3);
$('#hatch').onclick=()=>{if(!ready)return;m._web_hatch();persist();canvas.scrollIntoView({behavior:'smooth',block:'center'});};
document.querySelectorAll('[data-game]').forEach(b=>b.onclick=()=>{if(!ready)return;if(state(10)){alert("Save your companion’s name on the screen first.");return;}if(state(2)!==2){alert(state(2)===1?'Your companion is hatching. Give it a few seconds.':'Hatch your egg first using the workshop shortcut.');return;}stopAudio();m._web_open(Number(b.dataset.game));canvas.scrollIntoView({behavior:'smooth',block:'center'});});
$('#reset').onclick=()=>{if(!ready||!confirm('Restart from first steps? Your current browser companion will be replaced; device settings stay.'))return;stopAudio();release();storageBlocked=false;m._web_reset(crypto.getRandomValues(new Uint32Array(1))[0]);echoToken='';persist();updateSound();};
document.querySelectorAll('[data-tilt]').forEach(b=>{b.onpointerdown=e=>{e.preventDefault();b.setPointerCapture(e.pointerId);pad=b.dataset.tilt.split(',').map(Number);};b.onpointerup=b.onpointercancel=b.onlostpointercapture=()=>{pad=[0,0];};});
const shortcuts={Escape:0,' ':1,m:2,M:2,s:3,S:3,c:4,C:4,'1':5,'2':6,'3':7};
document.addEventListener('keydown',e=>{if(!ready||!$('#play').contains(document.activeElement)||/INPUT|TEXTAREA|SELECT/.test(e.target.tagName))return;if(e.key.startsWith('Arrow')){e.preventDefault();keys.add(e.key);}else if(e.key in shortcuts&&!(e.target.tagName==='BUTTON'&&e.key===' ')){e.preventDefault();if(!e.repeat){unlockAudio();key(shortcuts[e.key]);}}});
document.addEventListener('keyup',e=>keys.delete(e.key));window.addEventListener('blur',release);
document.addEventListener('visibilitychange',()=>{release();stopAudio();persist();last=0;accum=0;});window.addEventListener('pagehide',persist);
function soundTick(){
 const muted=state(0),sleeping=state(7),screen=state(1);
 if(muted||sleeping){stopAudio();m._web_sound();return;}
 if(screen===11){
  const session=state(3),round=state(4),stage=state(5),token=`${session}:${round}:${stage}`;
  if(token!==echoToken){stopAudio();echoToken=token;const name=stage===1?['shake','swipe','tap'][state(6)]:stage===2?'go':stage===4||stage===6?'pass':stage===5?'retry':null;
   if(name&&clips[name])play(clips[name],()=>m._web_audio_done(session,round,stage));
  }
 }else{if(echoToken){stopAudio();echoToken='';}const n=m._web_sound();if(n&&audio){stopAudio();const buffer=audio.createBuffer(1,n,16000),target=buffer.getChannelData(0),start=m._web_pcm()/2;for(let i=0;i<n;i++)target[i]=m.HEAP16[start+i]/32768;play(buffer);}}
}
function render(){const p=m._web_frame()/2,pixels=m.HEAPU16.subarray(p,p+466*466),image=ctx.createImageData(466,466),out=image.data;for(let i=0;i<pixels.length;i++){const v=pixels[i],j=i*4;out[j]=(v>>11)*255/31;out[j+1]=((v>>5)&63)*255/63;out[j+2]=(v&31)*255/31;out[j+3]=255;}ctx.putImageData(image,0,0);}
function frame(now){requestAnimationFrame(frame);if(document.hidden||!ready)return;if(!last)last=now;accum+=Math.min((now-last)/1000,.1);last=now;if(accum<.04)return;const dt=accum;accum=0;m._web_tilt(pad[0]+Number(keys.has('ArrowRight'))-Number(keys.has('ArrowLeft')),pad[1]+Number(keys.has('ArrowDown'))-Number(keys.has('ArrowUp')));m._web_audio_ready(audio?.state==='running'&&Object.keys(clips).length===6?1:0);m._web_tick(dt);soundTick();render();updateSound();saveElapsed+=dt;if(saveElapsed>=5){persist();saveElapsed=0;}status.textContent=state(9)!==0?['','First steps · swipe down from the top','Tap the lantern to try night','Tap the lantern to return to day','A special delivery · your egg is arriving'][state(9)]:state(10)?'A name for your companion · swipe letters, then save':state(7)?'Resting · tap PWR to wake':state(2)===0?'Mystery egg · hold gently to warm it':state(2)===1?'A little hello is on its way…':'Your little world · tap to reveal the controls';}
try{m=await createGame();m._web_init(crypto.getRandomValues(new Uint32Array(1))[0]);try{const saved=localStorage.getItem(storageKey);if(saved){const bytes=Uint8Array.from(atob(saved),c=>c.charCodeAt(0));if(bytes.length>8192)throw Error();m.HEAPU8.set(bytes,m._web_save_buffer());if(!m._web_load(bytes.length))throw Error();}}catch{storageBlocked=true;saveNote.textContent='The saved companion could not be restored. Its original record is kept in browser storage until you choose to save or reset.';}ready=true;document.querySelectorAll('button').forEach(b=>b.disabled=false);updateSound();render();requestAnimationFrame(frame);}catch(error){status.textContent='The emulator could not load. Please reload in a browser with WebAssembly support.';console.error(error);}
