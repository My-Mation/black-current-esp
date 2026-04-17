#pragma once
#include <pgmspace.h>
// =============================================================
//  web_js.h — JavaScript logic for the web UI
// =============================================================

const char WEB_JS[] PROGMEM = R"rawliteral(
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
const $ = id => document.getElementById(id);
function show(id){ $(id).classList.remove('hidden'); }
function hide(id){ $(id).classList.add('hidden'); }
function fadeIn(id){ const el=$(id); el.classList.remove('hidden'); void el.offsetWidth; el.classList.add('fade'); }

let espSec      = 0;      // timer seconds from ESP

// ============================================================
//  INIT & EVENTS
// ============================================================
document.addEventListener('DOMContentLoaded', () => {
  $('btnLoad').addEventListener('click', () => { show('loadCard'); });
  $('btnCancelLoad').addEventListener('click', () => { hide('loadCard'); });
  $('btnAuto').addEventListener('click', () => { runAutoGenerate(); });
  $('btnParse').addEventListener('click', () => { parseJsonInput(); });
  $('btnGoToReview').addEventListener('click', () => { startReview(); });
  $('btnSubmit').addEventListener('click', () => { finalizeSubmission(); });
  $('btnStart').addEventListener('click', async () => {
    if(!testActive && questions.length > 0) {
      await requestMic();
      startActualTest();
    }
  });
  startPoll();
});


// ============================================================
//  MIC PERMISSIONS
// ============================================================
let micStream = null;
async function requestMic() {
  try {
    micStream = await navigator.mediaDevices.getUserMedia({ audio: true });
    console.log("Mic access granted");
    return true;
  } catch (e) {
    console.error("Mic access denied:", e);
    alert("Microphone access is required for voice questions. Please allow it in your browser.");
    return false;
  }
}

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

  // ---- Sync Index from ESP ----
  if(testActive && d.index !== undefined && d.index !== curQ && d.index >= 0){
    console.log("Syncing index from ESP:", d.index);
    curQ = d.index;
    renderQuestion();
  }

  // ---- Sync Numeric Input from ESP ----
  if(testActive && curQ >= 0 && (d.mode === 'NUM' || d.mode === 'ROLL' || d.mode === 'NUMERIC')){
    if(!answers[curQ]) answers[curQ] = {value:''};
    if(d.input !== undefined && d.input !== answers[curQ].value){
      answers[curQ].value = d.input;
      renderNum();
    }
  }

  // ---- Key press event (mostly for MCQ and Voice confirmation) ----
  if(d.key && d.key !== ''){
    handleKey(d.key);
  }

  // ---- Touch stage event ----
  if(d.touch && d.touch !== 'NONE'){
    handleTouch(d.touch);
  }
}

// ============================================================
//  KEY HANDLER  (from ESP poll)
// ============================================================
function handleKey(key){
  // Handle global review start if reached end
  if(key === '#' && !testActive && questions.length > 0 && !testDone){
    if(!$('reviewCard').classList.contains('hidden')){
      $('btnGoToReview').click();
      return;
    }
  }

  if(!testActive || curQ < 0) return;
  const q = questions[curQ];

  // Navigation and Numeric clearing are now ESP-driven and synced via poll.
  // We only handle MCQ option selection here (browser side state).
  if(q.type === 'mcq'){
    const k = key.toUpperCase();
    if(!['A','B','C','D'].includes(k)) return;
    const idx = k.charCodeAt(0)-65;
    if(idx >= q.options.length) return;
    if(!answers[curQ]) answers[curQ] = {};
    answers[curQ].value = k;
    renderMCQSelect(idx);
    
    // Also tell ESP about selection so it stays in sync
    fetch('/api/sync_ans?val='+k).catch(()=>{});
  }
}

// ============================================================
//  TOUCH HANDLER  (from ESP poll — multi-stage)
// ============================================================
async function handleTouch(stage){
  if(!testActive || curQ < 0) return;
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
    if(!micStream) {
      const granted = await requestMic();
      if(!granted) return;
    }
    audioChunks = [];
    mediaRec = new MediaRecorder(micStream);
    mediaRec.ondataavailable = e => audioChunks.push(e.data);
    mediaRec.onstop = () => {
      const blob = new Blob(audioChunks, {type:'audio/webm'});
      voiceBlobs[curQ] = blob;
      const url = URL.createObjectURL(blob);
      $('voicePreview').src = url;
      show('voicePlayback');
    };
    mediaRec.start();
    $('vRing').className = 'v-ring rec';
    $('vStatus').textContent = '🔴 Recording... Touch 2× to stop';
    $('cTouch').className = 'dot rec';
  } catch(e){
    $('vStatus').textContent = '❌ Error: ' + e.message;
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
    endFinished();
  } else {
    renderQuestion();
    fetch('/api/next_question').catch(()=>{});
  }
}

function prevQuestion(){
  if(curQ > 0){
    curQ--;
    renderQuestion();
    // No explicit API for prev_question yet, using index sync if needed
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
  hide('mcqArea'); hide('numArea'); hide('voiceArea'); hide('voicePlayback');

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
    if(voiceBlobs[curQ]){
      show('voicePlayback');
      $('voicePreview').src = URL.createObjectURL(voiceBlobs[curQ]);
    }
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


function endFinished(){
  testActive = false;
  hide('qCard');
  show('reviewCard');
  fetch('/api/mode?m=READY').catch(()=>{}); // Set to ready/idle
}

function showResults(){
  testDone = true;
  showResult_inner();
  show('resCard');
  $('resCard').classList.add('fade');
  hide('btnSubmit');
  hide('qCard');
  hide('reviewCard');
  $('ctrlCard').style.display='none';
}

function startActualTest() {
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


function startReview(){
  testActive = true;
  curQ = 0;
  hide('reviewCard');
  renderQuestion();
}

function finalizeSubmission(){
  showResults();
  fetch('/api/mode?m=DONE').catch(()=>{});
}


function parseJsonInput(){
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
}

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
function runAutoGenerate(){
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
)rawliteral";
