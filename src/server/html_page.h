#pragma once
// =============================================================
//  html_page.h — Full web UI stored in PROGMEM
//  Separated from web_server.cpp to keep it manageable.
//  The JS poll loop drives all question flow based on ESP state.
// =============================================================

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1.0">
<title>ESP32 Test Station</title>
<style>
/* ===================== RESET & TOKENS ===================== */
*{margin:0;padding:0;box-sizing:border-box;}
:root{
  --bg:#07070f;
  --s1:#0f0f1c;--s2:#161628;--s3:#1e1e38;
  --bdr:#2a2a4a;--bdr2:#3a3a60;
  --txt:#eaeaf8;--txt2:#a0a0c8;--txt3:#5a5a80;
  --acc:#7c6ff0;--acc2:#a89dff;--acc3:#5a4fd0;
  --ok:#00b894;--err:#e74c3c;--warn:#fdcb6e;--info:#74b9ff;
  --rad:14px;--rad2:10px;--rad3:6px;
  --glow:0 0 28px rgba(124,111,240,.35);
  --glow2:0 0 14px rgba(124,111,240,.2);
}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;
  background:var(--bg);color:var(--txt);min-height:100vh;overflow-x:hidden;}

/* ===================== LAYOUT ===================== */
.wrap{max-width:740px;margin:0 auto;padding:12px 14px 40px;}

/* ===================== HEADER ===================== */
.hdr{text-align:center;padding:22px 0 14px;}
.hdr h1{font-size:1.75em;font-weight:800;letter-spacing:-0.5px;
  background:linear-gradient(135deg,var(--acc),var(--acc2),#fd79a8);
  -webkit-background-clip:text;-webkit-text-fill-color:transparent;}
.hdr p{color:var(--txt3);font-size:.82em;margin-top:3px;}
.chips{display:flex;gap:8px;justify-content:center;flex-wrap:wrap;margin-top:10px;}
.chip{background:var(--s2);border:1px solid var(--bdr);
  border-radius:20px;padding:4px 13px;font-size:.73em;color:var(--txt2);
  display:flex;align-items:center;gap:5px;}
.dot{width:6px;height:6px;border-radius:50%;background:var(--err);}
.dot.on{background:var(--ok);box-shadow:0 0 6px var(--ok);}
@keyframes blink{0%,100%{opacity:1}50%{opacity:.3}}
.dot.rec{background:var(--err);animation:blink .8s infinite;}

/* ===================== CARD ===================== */
.card{background:var(--s1);border:1px solid var(--bdr);border-radius:var(--rad);
  padding:18px;margin-bottom:14px;transition:border-color .3s,box-shadow .3s;}
.card:hover{border-color:var(--acc);box-shadow:var(--glow2);}
.card-ttl{font-size:.9em;font-weight:600;color:var(--acc2);margin-bottom:12px;
  display:flex;align-items:center;gap:7px;}

/* ===================== BUTTONS ===================== */
.btn{border:none;border-radius:var(--rad2);padding:11px 22px;
  font-family:inherit;font-weight:600;font-size:.88em;cursor:pointer;
  transition:all .25s;display:inline-flex;align-items:center;gap:7px;outline:none;}
.btn:active{transform:scale(.97);}
.btn-p{background:linear-gradient(135deg,var(--acc),var(--acc3));color:#fff;
  box-shadow:0 4px 16px rgba(124,111,240,.4);}
.btn-p:hover{box-shadow:0 6px 26px rgba(124,111,240,.6);transform:translateY(-1px);}
.btn-s{background:var(--s2);color:var(--txt2);border:1px solid var(--bdr);}
.btn-s:hover{background:var(--s3);color:var(--txt);}
.btn-ok{background:linear-gradient(135deg,var(--ok),#00cec9);color:#fff;
  box-shadow:0 4px 16px rgba(0,184,148,.35);}
.btn-err{background:linear-gradient(135deg,var(--err),#e17055);color:#fff;}
.btn-warn{background:linear-gradient(135deg,var(--warn),#e17055);color:#000;}
.btn-blk{width:100%;justify-content:center;}
.btn-lg{padding:15px 30px;font-size:1em;border-radius:var(--rad);}
.btn:disabled{opacity:.38;cursor:not-allowed;transform:none!important;box-shadow:none!important;}
.row{display:flex;gap:8px;margin-top:10px;flex-wrap:wrap;}

/* ===================== PROGRESS ===================== */
.prog-bar{height:5px;background:var(--s2);border-radius:3px;overflow:hidden;margin:8px 0;}
.prog-fill{height:100%;background:linear-gradient(90deg,var(--acc),var(--acc2));
  border-radius:3px;transition:width .5s ease;}
.prog-lbl{font-size:.77em;color:var(--txt3);margin-bottom:4px;}

/* ===================== TIMER ===================== */
.timer{font-family:'Courier New',monospace;font-size:1.3em;font-weight:700;
  color:var(--warn);text-align:center;padding:7px 10px;
  background:var(--s2);border:1px solid rgba(253,203,110,.2);
  border-radius:var(--rad2);margin:8px 0;letter-spacing:3px;}

/* ===================== QUESTION BOX ===================== */
.qbox{background:var(--s2);border:1px solid var(--bdr2);border-radius:var(--rad);
  padding:18px;margin:10px 0;}
.qno{font-size:.75em;color:var(--acc2);font-weight:600;text-transform:uppercase;
  letter-spacing:1px;margin-bottom:5px;}
.qrow{display:flex;align-items:flex-start;gap:8px;flex-wrap:wrap;}
.qtxt{font-size:1.08em;font-weight:500;line-height:1.6;flex:1;}
.badge{padding:3px 11px;border-radius:var(--rad3);font-size:.68em;font-weight:700;
  text-transform:uppercase;letter-spacing:.8px;white-space:nowrap;}
.badge.mcq{background:rgba(124,111,240,.2);color:var(--acc2);}
.badge.numeric{background:rgba(253,203,110,.2);color:var(--warn);}
.badge.voice{background:rgba(232,67,147,.2);color:#f672b5;}

/* ===================== MCQ OPTIONS ===================== */
.opts{display:grid;grid-template-columns:1fr 1fr;gap:9px;margin-top:14px;}
.opt{background:var(--s1);border:2px solid var(--bdr);border-radius:var(--rad2);
  padding:13px 14px;font-size:.88em;text-align:left;transition:all .3s;
  display:flex;align-items:flex-start;gap:10px;}
.opt-lbl{font-weight:700;color:var(--acc2);font-size:.8em;min-width:16px;
  margin-top:1px;}
.opt.sel{border-color:var(--acc);background:rgba(124,111,240,.14);
  box-shadow:0 0 14px rgba(124,111,240,.2);}
.opt.ok{border-color:var(--ok);background:rgba(0,184,148,.13);}
.opt.wrong{border-color:var(--err);background:rgba(231,76,60,.13);}
.hint{font-size:.73em;color:var(--txt3);text-align:center;margin-top:7px;}

/* ===================== NUMERIC ===================== */
.num-display{background:var(--s1);border:2px solid var(--bdr2);border-radius:var(--rad);
  padding:18px;text-align:center;font-size:2.2em;font-weight:700;
  font-family:'Courier New',monospace;color:var(--acc2);min-height:72px;
  margin-top:14px;letter-spacing:5px;transition:border-color .3s;}
.num-display.empty{font-size:1em;color:var(--txt3);font-weight:400;letter-spacing:0;}
.num-display.has-val{border-color:var(--acc2);}

/* ===================== VOICE ===================== */
.v-area{text-align:center;padding:22px 16px 14px;}
.v-ring{width:88px;height:88px;border-radius:50%;margin:0 auto 14px;
  display:flex;align-items:center;justify-content:center;
  font-size:2.2em;transition:all .4s;position:relative;}
.v-ring.idle{background:var(--s2);border:3px solid var(--bdr);}
.v-ring.ready{background:rgba(0,184,148,.12);border:3px solid var(--ok);}
.v-ring.rec{background:rgba(231,76,60,.18);border:3px solid var(--err);}
.v-ring.rec::after{content:'';position:absolute;inset:-8px;border-radius:50%;
  border:2px solid var(--err);animation:ripple 1.2s infinite;}
.v-ring.done{background:rgba(0,184,148,.18);border:3px solid var(--ok);}
@keyframes ripple{from{transform:scale(1);opacity:.7}to{transform:scale(1.35);opacity:0}}
.v-status{font-size:.92em;color:var(--txt2);line-height:1.5;}
.v-stage{display:flex;justify-content:center;gap:6px;margin-top:12px;flex-wrap:wrap;}
.vstep{padding:4px 12px;border-radius:20px;font-size:.72em;font-weight:600;
  background:var(--s2);border:1px solid var(--bdr);color:var(--txt3);}
.vstep.active{background:rgba(124,111,240,.2);border-color:var(--acc);color:var(--acc2);}
.vstep.done2{background:rgba(0,184,148,.2);border-color:var(--ok);color:var(--ok);}

/* ===================== LOAD JSON ===================== */
.json-area{width:100%;min-height:180px;background:var(--bg);color:var(--txt);
  border:1px solid var(--bdr);border-radius:var(--rad2);padding:12px;
  font-family:'Courier New',monospace;font-size:.78em;resize:vertical;
  outline:none;transition:border-color .3s;}
.json-area:focus{border-color:var(--acc);}
.err-msg{color:var(--err);font-size:.8em;margin-top:6px;display:none;}

/* ===================== RESULTS ===================== */
.score-hdr{text-align:center;padding:20px 10px;}
.score-big{font-size:3.2em;font-weight:800;
  background:linear-gradient(135deg,var(--acc),var(--ok));
  -webkit-background-clip:text;-webkit-text-fill-color:transparent;}
.score-lbl{color:var(--txt3);font-size:.82em;margin-top:3px;}
.res-card{border:1px solid var(--bdr);border-radius:var(--rad);padding:15px;
  margin-bottom:10px;background:var(--s1);}
.res-head{display:flex;justify-content:space-between;align-items:center;
  margin-bottom:8px;flex-wrap:wrap;gap:6px;}
.mbadge{padding:3px 12px;border-radius:var(--rad3);font-size:.73em;font-weight:700;}
.mbadge.c{background:rgba(0,184,148,.2);color:var(--ok);}
.mbadge.w{background:rgba(231,76,60,.2);color:var(--err);}
.mbadge.v{background:rgba(116,185,255,.2);color:var(--info);}
.res-ans{font-size:.83em;color:var(--txt3);margin:3px 0;}
.res-ans strong{color:var(--txt);}
audio{width:100%;margin-top:8px;border-radius:8px;}

/* ===================== ANIM ===================== */
.fade{animation:fa .35s ease;}
@keyframes fa{from{opacity:0;transform:translateY(8px)}to{opacity:1;transform:translateY(0)}}
.hidden{display:none!important;}

/* ===================== RESPONSIVE ===================== */
@media(max-width:480px){
  .opts{grid-template-columns:1fr;}
  .row{flex-direction:column;}
  .row .btn{width:100%;justify-content:center;}
  .hdr h1{font-size:1.4em;}
}
</style>
</head>
<body>
<div class="wrap">

<!-- =========== HEADER =========== -->
<div class="hdr">
  <h1>⚡ ESP32 Test Station</h1>
  <p>Hardware-Integrated Examination System</p>
  <div class="chips">
    <div class="chip"><div class="dot on" id="cWifi"></div><span id="lblWifi">WiFi OK</span></div>
    <div class="chip"><div class="dot" id="cHw"></div><span id="lblHw">Waiting HW</span></div>
    <div class="chip"><div class="dot" id="cTouch"></div><span id="lblTouch">Touch: –</span></div>
    <div class="chip">Mode: <strong id="lblMode">IDLE</strong></div>
  </div>
</div>

<!-- =========== CONTROL CARD =========== -->
<div class="card" id="ctrlCard">
  <div class="card-ttl">🎮 Control Panel</div>
  <p class="prog-lbl" id="progLbl">No questions loaded — use Load or Auto-Generate</p>
  <div class="prog-bar"><div class="prog-fill" id="progFill" style="width:0%"></div></div>
  <div class="timer" id="timerDisp">00:00</div>
  <div class="row" style="margin-top:12px;">
    <button class="btn btn-p btn-lg btn-blk" id="btnStart" disabled>🚀 Test Starter</button>
  </div>
  <div class="row">
    <button class="btn btn-s" id="btnLoad">📋 Load Questions</button>
    <button class="btn btn-s" id="btnAuto">⚡ Auto-Generate</button>
    <button class="btn btn-err hidden" id="btnSubmit">📤 Submit Test</button>
  </div>
</div>

<!-- =========== LOAD JSON PANEL =========== -->
<div class="card hidden fade" id="loadCard">
  <div class="card-ttl">📋 Paste Question JSON</div>
  <textarea class="json-area" id="jsonIn" placeholder='Paste JSON array here:&#10;[&#10;  {"type":"mcq","question":"...","options":["A","B","C","D"],"answer":"A"},&#10;  {"type":"numeric","question":"...","answer":42},&#10;  {"type":"voice","question":"..."}&#10;]'></textarea>
  <div class="err-msg" id="jsonErr">⚠️ JSON error</div>
  <div class="row">
    <button class="btn btn-ok" id="btnParse">✅ Load</button>
    <button class="btn btn-s" id="btnCancelLoad">Cancel</button>
  </div>
</div>

<!-- =========== QUESTION CARD =========== -->
<div class="card hidden fade" id="qCard">
  <div class="qbox">
    <div class="qno" id="qNo">Question 1 of 5</div>
    <div class="qrow">
      <span class="qtxt" id="qTxt">–</span>
      <span class="badge" id="qBadge">MCQ</span>
    </div>
  </div>

  <!-- MCQ -->
  <div id="mcqArea" class="hidden">
    <div class="opts" id="optsGrid"></div>
    <p class="hint">Keypad A–D to select · Touch sensor to confirm</p>
  </div>

  <!-- Numeric -->
  <div id="numArea" class="hidden">
    <div class="num-display empty" id="numDisp">Use keypad 0–9</div>
    <p class="hint">Digits accumulate · * clears · Touch confirms</p>
  </div>

  <!-- Voice -->
  <div id="voiceArea" class="hidden">
    <div class="v-area">
      <div class="v-ring idle" id="vRing">🎤</div>
      <p class="v-status" id="vStatus">Touch sensor to start recording</p>
      <div class="v-stage">
        <div class="vstep active" id="vs1">1 · Start rec</div>
        <div class="vstep" id="vs2">2 · Stop rec</div>
        <div class="vstep" id="vs3">3 · Confirm</div>
      </div>
    </div>
  </div>
</div>

<!-- =========== RESULTS CARD =========== -->
<div class="card hidden fade" id="resCard">
  <div class="card-ttl">📊 Results</div>
  <div class="score-hdr">
    <div class="score-big" id="scoreBig">0/0</div>
    <div class="score-lbl">Questions Answered Correctly</div>
  </div>
  <div id="resBody"></div>
</div>

</div><!-- /wrap -->

<script>
// ============================================================
//  CONSTANTS
// ============================================================
const POLL_MS = 300;

// ============================================================
//  STATE
// ============================================================
let questions   = [];     // loaded question objects
let curQ        = -1;     // current question index (-1 = not started)
let answers     = [];     // per-question: {value, submitted}
let timers      = [];     // seconds per question at submission
let voiceBlobs  = [];     // Blob per question for voice
let testActive  = false;
let testDone    = false;
let pollId      = null;
let mediaRec    = null;
let audioChunks = [];
let voiceStage  = 0;      // 0=none, 1=started, 2=stopped, 3=confirmed
let espSec      = 0;      // timer seconds from ESP

// ============================================================
//  DOM HELPERS
// ============================================================
const $ = id => document.getElementById(id);
function show(id){ $(id).classList.remove('hidden'); }
function hide(id){ $(id).classList.add('hidden'); }
function fadeIn(id){ const el=$(id); el.classList.remove('hidden'); void el.offsetWidth; el.classList.add('fade'); }

// ============================================================
//  POLLING — reads ESP state every POLL_MS ms
// ============================================================
function startPoll(){
  if(pollId) clearInterval(pollId);
  pollId = setInterval(doPoll, POLL_MS);
}

async function doPoll(){
  try{
    const r = await fetch('/api/state');
    if(!r.ok) return;
    const d = await r.json();
    onESPState(d);
  } catch(e){}
}

function onESPState(d){
  // HW heartbeat
  $('cHw').className = 'dot on';
  $('lblHw').textContent = 'HW Active';

  // Mode label
  $('lblMode').textContent = d.mode || 'IDLE';

  // Timer
  if(d.timer !== undefined){
    espSec = d.timer;
    renderTimer(espSec);
  }

  // ---- Key press event ----
  if(d.key && d.key !== '' && testActive && curQ >= 0){
    handleKey(d.key);
  }

  // ---- Touch stage event ----
  if(d.touch && d.touch !== 'NONE' && testActive && curQ >= 0){
    handleTouch(d.touch);
  }
}

// ============================================================
//  KEY HANDLER  (from ESP poll)
// ============================================================
function handleKey(key){
  if(!testActive || curQ < 0) return;
  const q = questions[curQ];

  if(q.type === 'mcq'){
    const k = key.toUpperCase();
    if(!['A','B','C','D'].includes(k)) return;
    const idx = k.charCodeAt(0)-65;
    if(idx >= q.options.length) return;
    if(!answers[curQ]) answers[curQ] = {};
    answers[curQ].value = k;
    renderMCQSelect(idx);
  }
  else if(q.type === 'numeric'){
    if(!answers[curQ]) answers[curQ] = {value:''};
    if(key === '*'){
      answers[curQ].value = '';
    } else if(key >= '0' && key <= '9'){
      answers[curQ].value += key;
    }
    renderNum();
  }
}

// ============================================================
//  TOUCH HANDLER  (from ESP poll — multi-stage)
// ============================================================
async function handleTouch(stage){
  const q = questions[curQ];
  $('cTouch').className = 'dot on';
  $('lblTouch').textContent = 'Touch: ' + stage;
  setTimeout(()=>{ $('lblTouch').textContent='Touch: –'; $('cTouch').className='dot'; }, 600);

  // ---- MCQ: CONFIRMED → advance ----
  if(q.type === 'mcq' && stage === 'CONFIRMED'){
    if(!answers[curQ] || !answers[curQ].value) return; // guard
    answers[curQ].submitted = true;
    timers[curQ] = espSec;
    advanceQuestion();
  }

  // ---- Numeric: CONFIRMED → advance ----
  else if(q.type === 'numeric' && stage === 'CONFIRMED'){
    if(!answers[curQ] || answers[curQ].value === '') return;
    answers[curQ].submitted = true;
    timers[curQ] = espSec;
    advanceQuestion();
  }

  // ---- Voice: three-stage ----
  else if(q.type === 'voice'){
    if(stage === 'REC_START'){
      voiceStage = 1;
      await startRecording();
      updateVoiceUI();
    }
    else if(stage === 'REC_STOP'){
      voiceStage = 2;
      stopRecording();
      updateVoiceUI();
    }
    else if(stage === 'CONFIRMED'){
      voiceStage = 3;
      if(!answers[curQ]) answers[curQ] = {};
      answers[curQ].value   = 'RECORDED';
      answers[curQ].submitted = true;
      timers[curQ] = espSec;
      updateVoiceUI();
      setTimeout(advanceQuestion, 500);
    }
  }
}

// ============================================================
//  VOICE RECORDING
// ============================================================
async function startRecording(){
  try{
    const stream = await navigator.mediaDevices.getUserMedia({audio:true});
    audioChunks = [];
    mediaRec = new MediaRecorder(stream);
    mediaRec.ondataavailable = e => audioChunks.push(e.data);
    mediaRec.onstop = () => {
      voiceBlobs[curQ] = new Blob(audioChunks, {type:'audio/webm'});
      stream.getTracks().forEach(t=>t.stop());
    };
    mediaRec.start();
    $('vRing').className = 'v-ring rec';
    $('vStatus').textContent = '🔴 Recording... Touch 2× to stop';
    $('cTouch').className = 'dot rec';
  } catch(e){
    $('vStatus').textContent = '❌ Mic access denied — allow in browser';
  }
}

function stopRecording(){
  if(mediaRec && mediaRec.state !== 'inactive'){
    mediaRec.stop();
  }
  $('vRing').className = 'v-ring done';
  $('vStatus').textContent = '✅ Saved. Touch once more to confirm & continue.';
  $('cTouch').className = 'dot';
}

function updateVoiceUI(){
  ['vs1','vs2','vs3'].forEach((id,i)=>{
    const el=$(id);
    el.className = 'vstep';
    if(i+1 < voiceStage) el.classList.add('done2');
    else if(i+1 === voiceStage) el.classList.add('active');
  });
}

// ============================================================
//  QUESTION FLOW
// ============================================================
function renderTimer(sec){
  const m = Math.floor(sec/60), s = sec%60;
  $('timerDisp').textContent = String(m).padStart(2,'0')+':'+String(s).padStart(2,'0');
}

function advanceQuestion(){
  curQ++;
  if(curQ >= questions.length){
    endTest();
  } else {
    renderQuestion();
    fetch('/api/next_question').catch(()=>{});
  }
}

function renderQuestion(){
  const q = questions[curQ];
  show('qCard');
  $('qCard').classList.add('fade');

  $('qNo').textContent = `Question ${curQ+1} of ${questions.length}`;
  $('qTxt').textContent = q.question;

  const b = $('qBadge');
  b.textContent = q.type.toUpperCase();
  b.className = 'badge '+q.type;

  // Progress
  $('progFill').style.width = ((curQ+1)/questions.length*100)+'%';
  $('progLbl').textContent = `Question ${curQ+1} / ${questions.length}`;

  // Button label
  $('btnStart').textContent = curQ < questions.length-1 ? '▶ Next (use touch)' : '🏁 Finish (use touch)';
  $('btnStart').disabled = true;  // hardware-driven; disable UI click

  // Tell ESP which mode
  fetch('/api/mode?m='+q.type.toUpperCase()).catch(()=>{});

  // Hide all input areas
  hide('mcqArea'); hide('numArea'); hide('voiceArea');

  if(q.type === 'mcq'){
    show('mcqArea');
    buildMCQ(q);
  } else if(q.type === 'numeric'){
    show('numArea');
    renderNum();
  } else if(q.type === 'voice'){
    show('voiceArea');
    voiceStage = 0;
    updateVoiceUI();
    $('vRing').className = 'v-ring idle';
    $('vStatus').textContent = 'Touch sensor to start recording';
  }
}

function buildMCQ(q){
  const grid = $('optsGrid');
  grid.innerHTML = '';
  ['A','B','C','D'].forEach((lbl,i)=>{
    if(i >= q.options.length) return;
    const d = document.createElement('div');
    d.className = 'opt';
    d.id = 'opt'+i;
    d.innerHTML = `<span class="opt-lbl">${lbl}</span><span>${q.options[i]}</span>`;
    grid.appendChild(d);
  });
  // Restore previous selection if any
  if(answers[curQ] && answers[curQ].value){
    const idx = answers[curQ].value.charCodeAt(0)-65;
    renderMCQSelect(idx);
  }
}

function renderMCQSelect(idx){
  document.querySelectorAll('.opt').forEach((el,i)=>{
    el.classList.toggle('sel', i===idx);
  });
}

function renderNum(){
  const d = $('numDisp');
  const val = answers[curQ]?.value || '';
  if(!val){
    d.textContent = 'Use keypad 0–9';
    d.className = 'num-display empty';
  } else {
    d.textContent = val;
    d.className = 'num-display has-val';
  }
}

// ============================================================
//  TEST FLOW — control panel buttons
// ============================================================
$('btnStart').addEventListener('click', ()=>{
  if(!testActive && questions.length > 0){
    // Initial start
    testActive = true;
    curQ = 0;
    answers = new Array(questions.length).fill(null).map(()=>({value:'',submitted:false}));
    timers  = new Array(questions.length).fill(0);
    voiceBlobs = new Array(questions.length).fill(null);
    show('btnSubmit');
    fetch('/api/start_test').catch(()=>{});
    renderQuestion();
    startPoll();
  }
  // Once started, touch sensor advances — no more JS click flow for questions
});

function endTest(){
  testActive = false;
  testDone   = true;
  hide('qCard');
  $('btnStart').textContent = '✅ Test Complete';
  $('btnStart').disabled = true;
  fetch('/api/mode?m=DONE').catch(()=>{});
  $('lblMode').textContent = 'DONE';
}

$('btnSubmit').addEventListener('click', ()=>{
  showResults();
});

// ============================================================
//  LOAD QUESTIONS
// ============================================================
$('btnLoad').addEventListener('click', ()=>{
  $('loadCard').classList.toggle('hidden');
});
$('btnCancelLoad').addEventListener('click', ()=>{ hide('loadCard'); });

$('btnParse').addEventListener('click', ()=>{
  const raw = $('jsonIn').value.trim();
  const errEl = $('jsonErr');
  errEl.style.display = 'none';
  try{
    const arr = JSON.parse(raw);
    validateAndLoad(arr);
  } catch(e){
    errEl.textContent = '⚠️ Invalid JSON: '+e.message;
    errEl.style.display = 'block';
  }
});

function validateAndLoad(arr){
  const errEl = $('jsonErr');
  if(!Array.isArray(arr) || arr.length===0) throw new Error('Must be a non-empty array');
  arr.forEach((q,i)=>{
    if(!q.type || !q.question) throw new Error(`Q${i+1}: missing "type" or "question"`);
    if(!['mcq','numeric','voice'].includes(q.type)) throw new Error(`Q${i+1}: type must be mcq/numeric/voice`);
    if(q.type==='mcq'){
      if(!Array.isArray(q.options)||q.options.length<2) throw new Error(`Q${i+1}: MCQ needs options[]`);
    }
  });
  questions = arr;
  hide('loadCard');
  $('btnStart').disabled = false;
  $('progLbl').textContent = `${questions.length} questions loaded — click Test Starter`;
  fetch('/api/load_questions',{
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body: JSON.stringify({count:questions.length, types:questions.map(q=>q.type)})
  }).catch(()=>{});
}

// ============================================================
//  AUTO-GENERATE  (static sample data, no AI)
// ============================================================
$('btnAuto').addEventListener('click', ()=>{
  const sample = [
    {type:'mcq',question:'What is the capital of France?',
     options:['Berlin','Madrid','Paris','Rome'],answer:'C'},
    {type:'mcq',question:'Which planet is called the Red Planet?',
     options:['Venus','Mars','Jupiter','Saturn'],answer:'B'},
    {type:'mcq',question:'Who invented the telephone?',
     options:['Edison','Bell','Tesla','Marconi'],answer:'B'},
    {type:'numeric',question:'What is 15 × 7?',answer:105},
    {type:'numeric',question:'Square root of 144?',answer:12},
    {type:'numeric',question:'How many bits in a byte?',answer:8},
    {type:'voice',question:'Explain the concept of photosynthesis in your own words.'},
    {type:'mcq',question:'ESP32 uses which framework in PlatformIO?',
     options:['FreeRTOS alone','Arduino','MicroPython','ESP-IDF only'],answer:'B'},
    {type:'numeric',question:'What is 2 to the power of 10?',answer:1024},
    {type:'voice',question:'Describe how a microcontroller differs from a microprocessor.'}
  ];
  try{ validateAndLoad(sample); }
  catch(e){ alert(e.message); }
  $('jsonIn').value = JSON.stringify(sample,null,2);
});

// ============================================================
//  RESULTS
// ============================================================
function showResults(){
  showResult_inner();
  show('resCard');
  $('resCard').classList.add('fade');
  hide('btnSubmit');
  hide('qCard');
  $('ctrlCard').style.display='none';
}

function showResult_inner(){
  let correct=0, gradeable=0;
  const body = $('resBody');
  body.innerHTML = '';

  questions.forEach((q,i)=>{
    const ans = answers[i] || {value:'',submitted:false};
    let isCorrect = false;
    let userAns   = ans.value || 'No answer';
    let correctAns = q.answer !== undefined ? String(q.answer) : 'N/A';

    if(q.type==='mcq'){
      gradeable++;
      // Map option letter to text for display
      const optMap = {A:0,B:1,C:2,D:3};
      const selIdx = optMap[ans.value];
      const selTxt = (selIdx !== undefined && q.options[selIdx]) ? q.options[selIdx] : ans.value;
      const corIdx = optMap[q.answer];
      const corTxt = (corIdx !== undefined && q.options[corIdx]) ? q.options[corIdx] : q.answer;
      isCorrect = ans.value === q.answer;
      if(isCorrect) correct++;
      userAns = ans.value ? `${ans.value} — ${selTxt}` : 'No answer';
      correctAns = `${q.answer} — ${corTxt}`;
    }
    else if(q.type==='numeric'){
      gradeable++;
      isCorrect = String(ans.value).trim() === String(q.answer).trim();
      if(isCorrect) correct++;
    }

    const mClass = q.type==='voice' ? 'v' : (isCorrect ? 'c' : 'w');
    const mTxt   = q.type==='voice' ? 'VOICE' : (isCorrect ? '✓ Correct' : '✗ Wrong');

    const card = document.createElement('div');
    card.className = 'res-card fade';
    let html = `
      <div class="res-head">
        <div><span style="font-size:.75em;color:var(--txt3)">Q${i+1}</span>
          <span class="badge ${q.type}" style="margin-left:6px">${q.type.toUpperCase()}</span>
        </div>
        <span class="mbadge ${mClass}">${mTxt}</span>
      </div>
      <p style="margin:6px 0;line-height:1.55">${q.question}</p>
      <p class="res-ans">Your answer: <strong>${userAns}</strong></p>
      ${q.type!=='voice'?`<p class="res-ans">Correct answer: <strong style="color:var(--ok)">${correctAns}</strong></p>`:''}
      <p class="res-ans">Time taken: ${timers[i]||0}s</p>
    `;
    if(q.type==='voice' && voiceBlobs[i]){
      const url = URL.createObjectURL(voiceBlobs[i]);
      html += `<audio controls src="${url}"></audio>`;
    }
    card.innerHTML = html;
    body.appendChild(card);
  });

  $('scoreBig').textContent = `${correct}/${gradeable}`;
}

// ============================================================
//  INIT
// ============================================================
startPoll();
</script>
</body>
</html>
)rawliteral";
