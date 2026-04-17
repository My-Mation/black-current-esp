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
const SERVER_AUDIO_ENDPOINT = '/api/upload_audio'; // Placeholder for server-side audio processing

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
let localStartTime = 0;   // browser-side safety timer
const $ = id => document.getElementById(id);
function show(id){ $(id).classList.remove('hidden'); }
function hide(id){ $(id).classList.add('hidden'); }
function fadeIn(id){ const el=$(id); el.classList.remove('hidden'); void el.offsetWidth; el.classList.add('fade'); }

let espSec      = 0;      // timer seconds from ESP

// ============================================================
//  INIT & EVENTS
// ============================================================
document.addEventListener('DOMContentLoaded', () => {
  const btnLoad = $('btnLoad'), btnCancel = $('btnCancelLoad'), btnAuto = $('btnAuto');
  const btnParse = $('btnParse'), btnSubmit = $('btnSubmit'), btnStart = $('btnStart');
  
  if(btnLoad) btnLoad.addEventListener('click', () => show('loadCard'));
  if(btnCancel) btnCancel.addEventListener('click', () => hide('loadCard'));
  if(btnAuto) btnAuto.addEventListener('click', runAutoGenerate);
  if(btnParse) btnParse.addEventListener('click', parseJsonInput);
  if(btnSubmit) btnSubmit.addEventListener('click', finalizeSubmission);
  
  // New review screen buttons
  const btnHWS = $('btnHWSubmit'), btnGBR = $('btnGoBackReview');
  if(btnHWS) btnHWS.addEventListener('click', finalizeSubmission);
  if(btnGBR) btnGBR.addEventListener('click', prevQuestion);

  if(btnStart) btnStart.addEventListener('click', async () => {
    console.log("btnStart clicked. Active:", testActive, "Q-len:", questions.length);
    if(!testActive && questions.length > 0) {
      console.log("Starting test sequence...");
      const micOk = await requestMic();
      if (micOk) {
        startActualTest();
      }
    } else {
      console.warn("Cannot start test: active or no questions");
    }
  });

  // Audio preview event setup for WhatsApp-style player
  const vPre = $('voicePreview');
  if(vPre){
    vPre.ontimeupdate = () => {
      if(!vPre.duration) return;
      const per = (vPre.currentTime / vPre.duration) * 100;
      $('waFill').style.width = per + '%';
      const m = Math.floor(vPre.currentTime / 60);
      const s = Math.floor(vPre.currentTime % 60);
      $('waTime').textContent = m + ':' + String(s).padStart(2,'0');
    };
    vPre.onended = () => {
      $('waPlayBtn').textContent = '▶';
      $('waFill').style.width = '0%';
    }
  }
  
  // Check for secure context early
  if (!window.isSecureContext) {
    console.warn("Warning: Browser requires HTTPS or Localhost for Microphone access.");
    setTimeout(() => {
        const d = document.createElement('div');
        d.style = "position:fixed;bottom:10px;left:10px;right:10px;background:#ff4757;color:white;padding:10px;border-radius:8px;z-index:9999;font-size:12px;text-align:center;box-shadow:0 4px 15px rgba(0,0,0,0.3)";
        d.innerHTML = "⚠️ <strong>Microphone Warning:</strong> Browser blocks mic on non-secure (HTTP) IPs. <br>Use <strong>localhost</strong> or enable <em>'Insecure origins treated as secure'</em> in browser flags.";
        document.body.appendChild(d);
        setTimeout(() => d.remove(), 10000);
    }, 2000);
  }

  startPoll();
});


// ============================================================
//  MIC PERMISSIONS
// ============================================================
let micStream = null;
async function requestMic() {
  if (micStream) return true;
  try {
    micStream = await navigator.mediaDevices.getUserMedia({ audio: true });
    console.log("Mic access granted");
    return true;
  } catch (e) {
    console.error("Mic access denied:", e);
    let msg = "Microphone access is required for voice questions.";
    if (!window.isSecureContext) msg += "\n\nNote: Browsers block microphone on HTTP connections. Please use HTTPS or access via localhost.";
    alert(msg);
    return false;
  }
}

// ============================================================
//  POLLING — reads ESP state every POLL_MS ms
// ============================================================
let isPolling = false;
async function doPoll(){
  if(isPolling) return;
  isPolling = true;
  try{
    const r = await fetch('/api/state');
    if(!r.ok) { isPolling = false; return; }
    const d = await r.json();
    onESPState(d);
    if(d.mode === 'DONE' && testActive) {
       // stop polling after short delay to allow UI to settle
       isPolling = false;
       return; 
    }
  } catch(e){}
  isPolling = false;
  setTimeout(doPoll, POLL_MS);
}

function startPoll(){
  doPoll();
}

let lastMode = '';
function onESPState(d){
  // HW heartbeat
  $('cHw').className = 'dot on';
  $('lblHw').textContent = 'HW Active';

  // Mode label
  $('lblMode').textContent = d.mode || 'IDLE';

  // Timer - track previous value before updating
  let prevEspSec = espSec;
  if(d.timer !== undefined){
    espSec = d.timer;
    renderTimer(espSec);
  }

  // ---- 1. Check for Test Completion (Global) ----
  if(d.mode === 'DONE' && testActive){
    // Save last question timer before finishing
    if(curQ >= 0 && curQ < questions.length && !timers[curQ]) {
        const locE = Math.floor((Date.now() - localStartTime)/1000);
        timers[curQ] = Math.max(prevEspSec, locE);
    }
    finalizeSubmission();
    return;
  }

  // ---- 2. Handle Events (Key/Touch) while still on current local index ----
  if(d.key && d.key !== ''){
    handleKey(d.key);
  }
  if(d.touch && d.touch !== 'NONE'){
    handleTouch(d.touch);
  }

  // ---- 3. Sync Index/Mode from ESP (or handle transition) ----
  if(testActive && d.index !== undefined && d.mode !== undefined){
    if(d.index !== curQ){
       console.log("Index change detected via ESP Poll: ", curQ, "->", d.index);
       // Save timer for the question we are LEAVING if not already saved
       if(curQ >= 0 && curQ < questions.length){
          const locE = Math.floor((Date.now() - localStartTime)/1000);
          // If the new espSec is 0, it means it was just reset, so we should 
          // probably use the last known espSec (prevEspSec) or the local timer.
          timers[curQ] = Math.max(timers[curQ] || 0, prevEspSec, locE);
          console.log(`Saved Q${curQ+1} timer: ${timers[curQ]}s`);
       }

       curQ = d.index;
       lastMode = d.mode;
       renderQuestion(d.mode);
    } 
    else if(d.mode !== lastMode){
       console.log("Mode change detected via ESP Poll:", lastMode, "->", d.mode);
       lastMode = d.mode;
       renderQuestion(d.mode);
    }
  }

  // ---- 4. Sync Numeric Input from ESP ----
  if(testActive && curQ >= 0){
    if(d.mode === 'ROLL') {
       // Roll input doesn't go into answers array
       // just update the display directly from poll data
       if(d.input !== undefined) renderNum(d.mode, d.input);
    } 
    else if(d.mode === 'NUM' || d.mode === 'NUMERIC'){
      if(!answers[curQ]) answers[curQ] = {value:''};
      if(d.input !== undefined && d.input !== answers[curQ].value){
        answers[curQ].value = d.input;
        renderNum(d.mode);
      }
    }
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

  if(!testActive || curQ < 0 || curQ >= questions.length) return;
  const q = questions[curQ];

  // Global skip/confirm logic for all question types via '#'
  if(key === '#'){
    console.log("Key #: Skip/Confirm triggered");
    
    // If we are in ROLL mode, don't store timers/answers for Q1
    if(lastMode === 'ROLL'){
       advanceQuestion();
       return;
    }

    if(!answers[curQ]) answers[curQ] = {value:''};
    
    // If empty, mark as "no answer"
    if(!answers[curQ].value || answers[curQ].value === '') {
      answers[curQ].value = 'no answer';
    } 
    // SPECIAL CASE: For voice, if they haven't stopped recording, 
    // we should probably stop it or ignore if they are mid-rec.
    // But per user request, just take it as "no answer" if not ready.
    if(q.type === 'voice' && voiceStage === 1) stopRecording();

    answers[curQ].submitted = true;
    const locE = Math.floor((Date.now() - localStartTime)/1000);
    timers[curQ] = Math.max(espSec, locE);
    advanceQuestion();
    return;
  }

  // VOICE specific keys: 1=Start, 2=Stop, 0=Play, D=Delete
  if(q.type === 'voice'){
    if(key === '1'){
      if(voiceStage === 1) return;
      console.log("Key 1: Start recording");
      voiceStage = 1;
      startRecording();
      updateVoiceUI();
      return;
    }
    if(key === '2'){
      if(voiceStage !== 1) return;
      console.log("Key 2: Stop recording");
      voiceStage = 2;
      stopRecording();
      updateVoiceUI();
      return;
    }
    if(key.toUpperCase() === 'D' && voiceStage === 2){
      console.log("Key D: Deleting recording");
      voiceBlobs[curQ] = null;
      voiceStage = 0;
      hide('voicePlayback');
      updateVoiceUI();
      $('vRing').className = 'v-ring idle';
      $('vStatus').textContent = 'Recording deleted. Press 1 to record again.';
      $('waFill').style.width = '0%';
      $('waTime').textContent = '0:00';
      fetch('/api/reset_voice').catch(()=>{}); 
      return;
    }
    if(key === '0' && voiceStage === 2){
      toggleLocalPlayback();
      return;
    }
  }

  // MCQ option selection
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
  if(!testActive || curQ < 0 || curQ >= questions.length) {
     if(testActive && curQ === questions.length && stage === 'CONFIRMED'){
        finalizeSubmission();
     }
     return;
  }
  const q = questions[curQ];
  $('cTouch').className = 'dot on';
  $('lblTouch').textContent = 'Touch: ' + stage;
  setTimeout(()=>{ $('lblTouch').textContent='Touch: –'; $('cTouch').className='dot'; }, 600);

  if(q.type === 'mcq' && stage === 'CONFIRMED'){
    if(!answers[curQ] || !answers[curQ].value) return; 
    answers[curQ].submitted = true;
    const locE_alt = Math.floor((Date.now() - localStartTime)/1000);
    timers[curQ] = Math.max(espSec, locE_alt);
    advanceQuestion();
  }
  else if(q.type === 'numeric' && stage === 'CONFIRMED'){
    if(!answers[curQ] || answers[curQ].value === '') return;
    answers[curQ].submitted = true;
    const locE_alt = Math.floor((Date.now() - localStartTime)/1000);
    timers[curQ] = Math.max(espSec, locE_alt);
    advanceQuestion();
  }
  else if(q.type === 'voice'){
    // In new logic, touch always means CONFIRM for voice if recording stopped
    if(stage === 'CONFIRMED' && voiceStage === 2){
      voiceStage = 3;
      if(!answers[curQ]) answers[curQ] = {};
      answers[curQ].value   = 'RECORDED';
      answers[curQ].submitted = true;
      const localElapsed = Math.floor((Date.now() - localStartTime)/1000);
      timers[curQ] = Math.max(espSec, localElapsed);
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
      const pre = $('voicePreview');
      if(pre) {
        pre.src = url;
        pre.load();
      }
      show('voicePlayback');
      $('vStatus').textContent = '✅ Recording ready. 0=Play, D=Delete, #=Confirm';
    };
    mediaRec.start();
    $('vRing').className = 'v-ring rec';
    $('vStatus').textContent = '🔴 Recording... Press 2 to stop';
    $('cTouch').className = 'dot rec';
  } catch(e){
    $('vStatus').textContent = '❌ Error: ' + e.message;
  }
}

function stopRecording(){
  if(mediaRec && mediaRec.state !== 'inactive') mediaRec.stop();
  $('vRing').className = 'v-ring done';
  $('cTouch').className = 'dot';
}

function toggleLocalPlayback() {
  const audio = $('voicePreview');
  if (!audio || !audio.src) return;
  const btn = $('waPlayBtn');
  if (audio.paused) {
    audio.play();
    btn.textContent = '⏸';
  } else {
    audio.pause();
    btn.textContent = '▶';
  }
}

function updateVoiceUI(){
  ['vs1', 'vs2', 'vs3'].forEach((id, i) => {
    const el = $(id);
    if (!el) return;
    el.className = 'vstep';
    if (i + 1 < voiceStage) el.classList.add('done2');
    else if (i + 1 === voiceStage) el.classList.add('active');
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
  if(curQ < questions.length){
    curQ++;
    const q = questions[curQ];
    renderQuestion(q ? q.type : '');
    fetch('/api/next_question').catch(()=>{});
  }
}

function prevQuestion(){
  if(curQ > 0){
    curQ--;
    const q = questions[curQ];
    renderQuestion(q ? q.type : '');
    fetch('/api/mode?m=PREV').catch(()=>{}); 
  }
}

function renderQuestion(mode){
  localStartTime = Date.now(); // Ensure timer starts fresh for every screen
  
  if(mode === 'ROLL'){
    hide('qCard'); hide('reviewCard'); hide('ctrlCard'); show('rollCard');
    renderNum(mode); return;
  }
  if(curQ >= questions.length){
    hide('qCard'); hide('rollCard'); show('reviewCard'); return;
  }
  const q = questions[curQ];
  localStartTime = Date.now();
  hide('reviewCard'); hide('rollCard'); show('qCard');
  $('qCard').classList.add('fade');
  $('qNo').textContent = `Question ${curQ+1} of ${questions.length}`;
  $('qTxt').textContent = q.question;
  const b = $('qBadge');
  b.textContent = q.type.toUpperCase();
  b.className = 'badge '+q.type;
  $('progFill').style.width = ((curQ+1)/questions.length*100)+'%';
  $('progLbl').textContent = `Question ${curQ+1} / ${questions.length}`;
  $('btnStart').textContent = curQ < questions.length-1 ? '▶ Next (use touch)' : '🏁 Finish (use touch)';
  $('btnStart').disabled = true;
  fetch('/api/mode?m='+q.type.toUpperCase()).catch(()=>{});
  hide('mcqArea'); hide('numArea'); hide('voiceArea'); hide('voicePlayback');
  if(q.type === 'mcq'){
    show('mcqArea'); buildMCQ(q);
  } else if(q.type === 'numeric'){
    show('numArea'); renderNum();
  } else if(q.type === 'voice'){
    show('voiceArea');
    voiceStage = 0; updateVoiceUI();
    $('vRing').className = 'v-ring idle';
    $('vStatus').textContent = 'Touch sensor to start recording';
    $('waFill').style.width = '0%';
    $('waTime').textContent = '0:00';
    if(voiceBlobs[curQ]){
      show('voicePlayback');
      const vPre = $('voicePreview');
      vPre.src = URL.createObjectURL(voiceBlobs[curQ]);
      vPre.load();
      voiceStage = 2; updateVoiceUI();
    }
  }
}

function buildMCQ(q){
  const grid = $('optsGrid'); grid.innerHTML = '';
  ['A','B','C','D'].forEach((lbl,i)=>{
    if(i >= q.options.length) return;
    const d = document.createElement('div');
    d.className = 'opt'; d.id = 'opt'+i;
    d.innerHTML = `<span class="opt-lbl">${lbl}</span><span>${q.options[i]}</span>`;
    grid.appendChild(d);
  });
  if(answers[curQ] && answers[curQ].value){
    const idx = answers[curQ].value.charCodeAt(0)-65;
    renderMCQSelect(idx);
  }
}

function renderMCQSelect(idx){
  document.querySelectorAll('.opt').forEach((el,i)=>{ el.classList.toggle('sel', i===idx); });
}

function renderNum(mode, valOverride){
  const isRoll = mode === 'ROLL';
  const targetId = isRoll ? 'rollDisp' : 'numDisp';
  const d = $(targetId);
  const val = valOverride !== undefined ? valOverride : (answers[curQ]?.value || '');
  if(!val){
    d.textContent = isRoll ? 'Enter Roll #' : 'Use keypad 0–9';
    d.className = 'num-display empty';
  } else {
    d.textContent = val; d.className = 'num-display has-val';
  }
}

function showResults(){
  testDone = true; showResult_inner(); show('resCard');
  $('resCard').classList.add('fade');
  hide('btnSubmit'); hide('qCard'); hide('reviewCard');
  $('ctrlCard').style.display='none';
}

function startActualTest() {
  testActive = true; curQ = 0; lastMode = '';
  answers = new Array(questions.length).fill(null).map(()=>({value:'',submitted:false}));
  timers  = new Array(questions.length).fill(0);
  voiceBlobs = new Array(questions.length).fill(null);
  show('btnSubmit'); fetch('/api/start_test').catch(()=>{});
  renderQuestion('ROLL'); startPoll();
}

async function finalizeSubmission(){
  if (testActive) {
      testActive = false; showResults();
      await uploadAllAudio();
      fetch('/api/mode?m=DONE').catch(()=>{});
  }
}

async function uploadAllAudio() {
  const statusEl = $('uploadStatus');
  const formData = new FormData();
  let hasAudio = false;
  voiceBlobs.forEach((blob, i) => { if (blob) { formData.append(`audio_q${i+1}`, blob, `voice_q${i+1}.webm`); hasAudio = true; } });
  if (!hasAudio) return;
  if (statusEl) { statusEl.textContent = "📡 Uploading audio files..."; statusEl.style.color = "var(--p1)"; }
  try {
    const response = await fetch(SERVER_AUDIO_ENDPOINT, { method: 'POST', body: formData });
    if (response.ok) { if (statusEl) statusEl.textContent = "✅ Audio files submitted successfully!"; }
    else { if (statusEl) statusEl.textContent = "❌ Audio upload failed (" + response.status + ")"; }
  } catch (e) { if (statusEl) statusEl.textContent = "❌ Network error during audio upload."; }
}

function parseJsonInput(){
  const raw = $('jsonIn').value.trim();
  const errEl = $('jsonErr'); errEl.style.display = 'none';
  try{ const arr = JSON.parse(raw); validateAndLoad(arr); }
  catch(e){ errEl.textContent = '⚠️ Invalid JSON: '+e.message; errEl.style.display = 'block'; }
}

function validateAndLoad(arr){
  if(!Array.isArray(arr) || arr.length===0) throw new Error('Must be a non-empty array');
  arr.forEach((q,i)=>{
    if(!q.type || !q.question) throw new Error(`Q${i+1}: missing "type" or "question"`);
    if(!['mcq','numeric','voice'].includes(q.type)) throw new Error(`Q${i+1}: type must be mcq/numeric/voice`);
    if(q.type==='mcq' && (!Array.isArray(q.options)||q.options.length<2)) throw new Error(`Q${i+1}: MCQ needs options[]`);
  });
  questions = arr; hide('loadCard'); $('btnStart').disabled = false;
  fetch('/api/load_questions',{
    method:'POST', headers:{'Content-Type':'application/json'},
    body: JSON.stringify({count:questions.length, types:questions.map(q=>q.type)})
  }).catch(()=>{});
}

function runAutoGenerate(){
  const sample = [
    {type:'mcq',question:'What is the capital of France?',options:['Berlin','Madrid','Paris','Rome'],answer:'C'},
    {type:'numeric',question:'What is 15 × 7?',answer:105},
    {type:'voice',question:'Explain the concept of photosynthesis in your own words.'},
    {type:'mcq',question:'ESP32 uses which framework in PlatformIO?',options:['FreeRTOS','Arduino','MicroPython','ESP-IDF'],answer:'B'},
    {type:'voice',question:'Describe how a microcontroller differs from a microprocessor.'}
  ];
  try{ validateAndLoad(sample); } catch(e){ alert(e.message); }
  $('jsonIn').value = JSON.stringify(sample,null,2);
}

function showResult_inner(){
  let correct=0, gradeable=0; const body = $('resBody'); if(!body) return; body.innerHTML = '';
  questions.forEach((q,i)=>{
    const ans = answers[i] || {value:'',submitted:false};
    let isCorrect = false, userAns = ans.value || 'No answer', correctAns = q.answer || 'N/A';
    
    if(q.type==='mcq'){
      gradeable++; const optMap = {A:0,B:1,C:2,D:3};
      const selIdx = optMap[ans.value], corIdx = optMap[q.answer];
      isCorrect = ans.value === q.answer; if(isCorrect) correct++;
      userAns = ans.value ? `${ans.value} — ${q.options[selIdx]||ans.value}` : 'No answer';
      correctAns = `${q.answer} — ${q.options[corIdx]||q.answer}`;
    } else if(q.type==='numeric'){
      gradeable++; isCorrect = String(ans.value).trim() === String(q.answer).trim();
      if(isCorrect) correct++;
    }

    // Capture user voice status clearly
    if(q.type === 'voice'){
       userAns = voiceBlobs[i] ? 'Given answer in audio' : 'No answer';
    }

    const mClass = q.type==='voice' ? 'v' : (isCorrect ? 'c' : 'w');
    const mTxt   = q.type==='voice' ? 'VOICE' : (isCorrect ? '✓ Correct' : '✗ Wrong');
    const card = document.createElement('div'); card.className = 'res-card fade';
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
      <p class="res-ans">⌛ Time taken: <strong>${timers[i]||0}s</strong></p>
    `;
    if(q.type==='voice' && voiceBlobs[i]){
      const url = URL.createObjectURL(voiceBlobs[i]);
      html += `<div class="wa-audio">
        <div class="wa-play" onclick="const a=this.parentElement.nextElementSibling; if(a.paused){a.play();this.textContent='⏸'}else{a.pause();this.textContent='▶'}">▶</div>
        <div class="wa-info"><div class="wa-title">Voice Submission</div></div>
      </div><audio class="hidden" src="${url}" onended="this.previousElementSibling.querySelector('.wa-play').textContent='▶'"></audio>`;
    }
    card.innerHTML = html; body.appendChild(card);
  });
  $('scoreBig').textContent = `${correct}/${gradeable}`;
}
)rawliteral";
