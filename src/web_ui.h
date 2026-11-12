// Single-page control UI served by the ESP32.
#pragma once

#include <Arduino.h>

static const char kIndexHtml[] PROGMEM = R"HTML(<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<title>Antenna rotator</title>
<style>
:root{color-scheme:dark}
body{font-family:system-ui,sans-serif;background:#14181c;color:#e8ecef;margin:0;padding:16px;max-width:420px;margin:auto}
h1{font-size:18px;margin:0 0 12px}
.read{display:grid;grid-template-columns:1fr 1fr;gap:8px;margin-bottom:16px}
.read div{background:#1f262d;border-radius:10px;padding:10px}
.read small{display:block;color:#8a97a3}
.read b{font-size:26px;font-variant-numeric:tabular-nums}
.pad{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;margin-bottom:16px}
button{font-size:22px;padding:18px 0;border:0;border-radius:10px;background:#2b3640;color:inherit;touch-action:none;user-select:none;-webkit-user-select:none}
button:active{background:#3d7dd8}
button.stop{background:#a33;font-size:16px;font-weight:700}
form{display:flex;gap:8px;margin-bottom:10px;align-items:center}
input{flex:1;min-width:0;font-size:16px;padding:10px;border-radius:8px;border:1px solid #3a4651;background:#1f262d;color:inherit}
form button{font-size:15px;padding:10px 14px}
#msg{min-height:1.4em;color:#f0b35a;font-size:14px}
</style></head><body>
<h1>Antenna rotator</h1>
<div class="read">
<div><small>Azimuth</small><b id="az">–</b></div>
<div><small>Elevation</small><b id="el">–</b></div>
</div>
<div class="pad">
<span></span><button data-j="0,1" class="tilt">▲</button><span></span>
<button data-j="-1,0">◀</button><button class="stop" id="stop">STOP</button><button data-j="1,0">▶</button>
<span></span><button data-j="0,-1" class="tilt">▼</button><span></span>
</div>
<form id="goto"><input id="gaz" type="number" step="any" placeholder="azimuth °" required>
<input id="gel" type="number" step="any" placeholder="elevation °" value="0" required><button>Go</button></form>
<form id="north"><input id="nb" type="number" step="any" placeholder="antenna now points at bearing °" required><button>Set</button></form>
<form id="home"><button style="flex:1">Centre (pan 0, tilt 0)</button></form>
<div id="msg"></div>
<script>
const $=id=>document.getElementById(id),PAN=20,TILT=12;
let timer=null;
async function cmd(c){
  try{
    const r=await fetch('/api/cmd',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'c='+encodeURIComponent(c)});
    show(await r.text());
  }catch(e){$('msg').textContent='No connection to rotator';}
}
function show(t){
  if(!t.startsWith('OK')){$('msg').textContent=t.replace(/^ERR /,'');return;}
  $('msg').textContent='';
  const kv=Object.fromEntries(t.slice(3).split(' ').map(p=>p.split('=')));
  $('az').textContent=kv.az+'°';$('el').textContent=kv.tiltaxis==='1'?kv.el+'°':'manual';
  document.querySelectorAll('.tilt').forEach(b=>b.style.visibility=kv.tiltaxis==='1'?'visible':'hidden');
}
function release(){if(timer){clearInterval(timer);timer=null;cmd('STOP');}}
document.querySelectorAll('[data-j]').forEach(b=>{
  const [x,y]=b.dataset.j.split(',').map(Number),c='JOG '+x*PAN+' '+y*TILT;
  b.addEventListener('pointerdown',e=>{e.preventDefault();release();cmd(c);timer=setInterval(()=>cmd(c),200);});
  ['pointerup','pointercancel','pointerleave'].forEach(n=>b.addEventListener(n,release));
});
$('stop').onclick=()=>{release();cmd('STOP');};
$('goto').onsubmit=e=>{e.preventDefault();cmd('GOTO '+$('gaz').value+' '+$('gel').value);};
$('north').onsubmit=e=>{e.preventDefault();cmd('NORTH '+$('nb').value);};
$('home').onsubmit=e=>{e.preventDefault();cmd('HOME');};
setInterval(()=>{if(!timer)cmd('STATUS');},1000);cmd('STATUS');
</script></body></html>)HTML";
