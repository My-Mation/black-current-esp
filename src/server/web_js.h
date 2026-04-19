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
const SERVER_AUDIO_ENDPOINT = '/api/upload_audio';

// ============================================================
//  STATE
// ============================================================
let curQDoc     = null;   // current question object from ESP
let curIndex    = -1;     // sequence number from ESP
let curMode     = 'IDLE';
let curQuizId   = "";
let curQuizTitle = "";
let curTotalCount = 0;
let curRootIndex = -1;
let curIsFollowUp = false;
let answers      = {};     // Keyed by rootIndex: { q, type, answer, followUpAnswer, audioFile, followUpAudioFile, time }
let voiceBlobs  = {};     // Map by index
let pendingEncodings = 0; // Tracks async MP3 conversion
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

let espSec      = 0;      // timer seconds from ESP
let lastInput   = "";

// ============================================================
//  INIT & EVENTS
// ============================================================
document.addEventListener('DOMContentLoaded', () => {
  const btnLoad = $('btnLoad'), btnCancel = $('btnCancelLoad'), btnAuto = $('btnAuto');
  const btnParse = $('btnParse'), btnSubmit = $('btnSubmit'), btnStart = $('btnStart'), btnEnd = $('btnEndQuiz');
  
  if(btnLoad) btnLoad.addEventListener('click', () => show('loadCard'));
  if(btnCancel) btnCancel.addEventListener('click', () => hide('loadCard'));
  if(btnAuto) btnAuto.addEventListener('click', runAutoGenerate);
  if(btnParse) btnParse.addEventListener('click', parseJsonInput);
  if(btnSubmit) btnSubmit.addEventListener('click', finalizeSubmission);
  
  const btnHWS = $('btnHWSubmit'), btnGBR = $('btnGoBackReview'), btnHome = $('btnHome');
  if(btnHWS) btnHWS.addEventListener('click', finalizeSubmission);
  if(btnGBR) btnGBR.addEventListener('click', () => fetch('/api/mode?m=NEXT').catch(()=>{}));
  if(btnHome) btnHome.addEventListener('click', resetToHome);

  if(btnStart) btnStart.addEventListener('click', async () => {
    if(!testActive) {
      const micOk = await requestMic();
      if (micOk) startActualTest();
    }
  });

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
    vPre.onended = () => { $('waPlayBtn').textContent = '▶'; $('waFill').style.width = '0%'; }
  }

  if(btnEnd) btnEnd.addEventListener('click', () => {
      if(confirm("End test and view results now?")) {
          finalizeSubmission();
      }
  });

  const btnSaveCfg = $('btnSaveConfig');
  if(btnSaveCfg) {
    btnSaveCfg.addEventListener('click', () => {
      const ip = $('serverIpIn').value.trim();
      localStorage.setItem('backendIp', ip);
      $('saveStatus').textContent = "✅ Saved!";
      setTimeout(() => { $('saveStatus').textContent = ""; }, 2000);
    });
  }

  // Load saved IP (Default to hardcoded if none in storage)
  const savedIp = localStorage.getItem('backendIp');
  if(savedIp) $('serverIpIn').value = savedIp;
  else $('serverIpIn').value = "10.30.233.172:3000";

  startPoll();
});

// ============================================================
//  MIC & POLL
// ============================================================
let micStream = null;
async function requestMic() {
  if (micStream) return true;
  try {
    micStream = await navigator.mediaDevices.getUserMedia({ audio: true });
    return true;
  } catch (e) {
    alert("Microphone access required.");
    return false;
  }
}

let isPolling = false;
async function doPoll(){
  if(isPolling) return;
  isPolling = true;
  try{
    const r = await fetch('/api/state');
    if(r.ok) onESPState(await r.json());
  } catch(e){}
  isPolling = false;
  setTimeout(doPoll, POLL_MS);
}
function startPoll(){ doPoll(); }

// ============================================================
//  CORE ENGINE SYNC
// ============================================================
function onESPState(d){
  $('cHw').className = 'dot on';
  $('lblHw').textContent = 'HW Active';
  $('lblMode').textContent = d.mode || 'IDLE';

  if(d.fetchStatus) $('fetchStatus').textContent = "Status: " + d.fetchStatus;
  
  curQuizId = d.quizId || "";
  curQuizTitle = d.quizTitle || "";
  curTotalCount = d.total || 0;
  curIsFollowUp = d.isFollowUp || false;
  curRootIndex = d.rootIndex !== undefined ? d.rootIndex : curRootIndex;
  
  if (curQuizId) {
      const btnStart = $('btnStart');
      if (btnStart) btnStart.disabled = false;
  }

  let prevEspSec = espSec;
  if(d.timer !== undefined){ espSec = d.timer; renderTimer(espSec); }

  // Transition to DONE / Submit
  if(d.mode === 'DONE' && testActive){ 
      if (curQDoc && curIndex !== -1) {
          const finalVal = lastInput || d.prevSel || d.prevNum || "";
          saveCurrentAnswer(espSec, finalVal, curRootIndex);
      }
      finalizeSubmission(); 
      return; 
  }
  
  if(d.mode === 'ROLL' && !testActive) {
      console.log("[AUTO-START] Transitioning to ROLL phase. d.mode:", d.mode, "testActive:", testActive);
      startActualTest(); 
  }

  if(d.key) handleKey(d.key);
  if(d.touch && d.touch !== 'NONE') handleTouch(d.touch);

  if(testActive && d.index !== undefined){
    const newText = d.q?.text || d.q?.question || "";
    const oldText = curQDoc?.text || curQDoc?.question || "";
    
    // Capture input live
    if(d.input) lastInput = d.input;
    if(d.sel) lastInput = d.sel;
    if(d.num) lastInput = d.num;

    if(d.index !== curIndex || d.mode !== curMode || (d.q && newText !== oldText)){
       if (curQDoc && curIndex !== -1) {
           // Use prev values from ESP if available for perfect sync
           let finalVal = d.prevSel || d.prevNum || lastInput;
           saveCurrentAnswer(prevEspSec, finalVal, curRootIndex);
       }
       curIndex = d.index;
       curMode = d.mode;
       curRootIndex = d.rootIndex !== undefined ? d.rootIndex : curRootIndex;
       curQDoc = d.q;
       lastInput = ""; // Reset for new question
       renderQuestion(d.mode, d.q);
    }
  }

  if(testActive){
    if(d.mode === 'ROLL' || d.mode === 'NUM' || d.mode === 'NUMERIC'){
       renderNum(d.mode, d.input);
    }
    // Update current answer live if possible
    if(curIndex >= 0 && d.sel) {
        if(curQDoc?.type === 'mcq') renderMCQSelect(d.sel.charCodeAt(0)-65);
    }
  }

  // Handle End Quiz button visibility
  if(testActive && !testDone && (d.mode !== 'IDLE' && d.mode !== 'DONE')){
    show('endQuizContainer');
  } else {
    hide('endQuizContainer');
  }
}

function saveCurrentAnswer(sec, val, rootIdx) {
    if (!curQDoc || rootIdx === -1) return;
    const dur = Math.max(sec, Math.floor((Date.now() - localStartTime)/1000));
    const isVoice = (curQDoc.type === 'voice');
    const answerVal = isVoice ? "VOICE" : (val || 'no answer');
    
    if (!answers[rootIdx]) {
        answers[rootIdx] = { 
            question: curQDoc.text || curQDoc.question,
            type: curQDoc.type,
            answer: answerVal,
            followUpAnswer: "",
            audioFile: isVoice ? `audio_q${rootIdx}.mp3` : "",
            followUpAudioFile: "",
            time: dur,
            options: curQDoc.options
        };
    } else {
        // It's a follow-up
        if (isVoice) {
            answers[rootIdx].followUpAnswer = "VOICE_FOLLOWUP";
            answers[rootIdx].followUpAudioFile = `audio_q${rootIdx}_f.mp3`;
        } else {
            // Append if multiple follow-ups, or just set if first
            if (answers[rootIdx].followUpAnswer) answers[rootIdx].followUpAnswer += " | " + val;
            else answers[rootIdx].followUpAnswer = val;
        }
        answers[rootIdx].time += dur;
    }
}

function renderQuestion(mode, q){
  console.log("[RENDER] Mode:", mode, "Question:", q ? (q.text || q.question) : "null");
  localStartTime = Date.now();
  if(mode === 'ROLL'){
    hide('qCard'); hide('reviewCard'); hide('ctrlCard'); hide('configCard'); show('rollCard');
    renderNum(mode, ""); return;
  }
  if(!q || mode === 'DONE') {
    hide('qCard'); hide('rollCard'); show('reviewCard'); return;
  }

  hide('reviewCard'); hide('rollCard'); show('qCard');
  $('qNo').textContent = `Step ${curIndex + 1} (Adaptive)`;
  $('qTxt').textContent = q.text || q.question;
  const b = $('qBadge');
  b.textContent = q.type.toUpperCase();
  b.className = 'badge '+q.type;
  
  hide('mcqArea'); hide('numArea'); hide('voiceArea'); hide('voicePlayback');
  if(q.type === 'mcq'){ show('mcqArea'); buildMCQ(q); } 
  else if(q.type === 'numeric'){ show('numArea'); renderNum('NUMERIC', ""); } 
  else if(q.type === 'voice'){
    show('voiceArea'); voiceStage = 0; updateVoiceUI();
    $('vRing').className = 'v-ring idle';
    $('vStatus').textContent = 'Use keypad 1/2 or touch to record';
  }
}

function handleKey(key){
  if(!testActive || !curQDoc) return;
  if(curQDoc.type === 'mcq'){
    const k = key.toUpperCase();
    if(['A','B','C','D'].includes(k)){
      const idx = k.charCodeAt(0)-65;
      if(curQDoc.options && idx < curQDoc.options.length) renderMCQSelect(idx);
    }
  }
  if(curQDoc.type === 'voice'){
    if(key === '1') { voiceStage = 1; startRecording(); updateVoiceUI(); }
    if(key === '2') { voiceStage = 2; stopRecording(); updateVoiceUI(); }
    if(key === '0') toggleLocalPlayback();
    if(key.toUpperCase() === 'D' && voiceStage === 2) {
      voiceStage = 0; hide('voicePlayback'); updateVoiceUI();
      const voiceKey = curIsFollowUp ? `${curRootIndex}_f` : `${curRootIndex}_m`;
      voiceBlobs[voiceKey] = null; fetch('/api/reset_voice');
    }
  }
}

async function handleTouch(stage){
  if(!testActive || !curQDoc || curQDoc.type !== 'voice') return;
  if(stage === 'REC_START') { voiceStage = 1; startRecording(); updateVoiceUI(); }
  else if(stage === 'REC_STOP') { voiceStage = 2; stopRecording(); updateVoiceUI(); }
  else if(stage === 'CONFIRMED' && voiceStage === 2){
    voiceStage = 3; updateVoiceUI();
  }
}

function buildMCQ(q){
  const grid = $('optsGrid'); grid.innerHTML = '';
  const labels = ['A','B','C','D'];
  if (q.options) {
      q.options.forEach((opt, i) => {
          let text = typeof opt === 'object' ? opt.text : opt;
          const cleanText = text.replace(/^[A-D][\)\.\s]\s*/i, '');
          const d = document.createElement('div');
          d.className = 'opt'; d.id = 'opt'+i;
          d.innerHTML = `<span class="opt-lbl">${labels[i]}</span><span>${cleanText}</span>`;
          d.onclick = () => { 
              renderMCQSelect(i); 
              // Predictive State: if this option has a followup, realize it before the poll return
              if (typeof opt === 'object' && opt.followUp) {
                  curIsFollowUp = true;
                  console.log("[UI] Predictive: Moving to follow-up");
              }
              fetch('/api/sync_ans?val=' + labels[i]); 
          };
          grid.appendChild(d);
      });
  }
}

function renderMCQSelect(idx){
  document.querySelectorAll('.opt').forEach((el,i)=> el.classList.toggle('sel', i===idx));
}

function renderNum(mode, val) {
    const d = mode === 'ROLL' ? $('rollDisp') : $('numDisp');
    if(!val) { d.textContent = 'Use keypad'; d.className = 'num-display empty'; }
    else { d.textContent = val; d.className = 'num-display has-val'; }
}

function renderTimer(sec){
  const m = Math.floor(sec/60), s = sec%60;
  $('timerDisp').textContent = String(m).padStart(2,'0')+':'+String(s).padStart(2,'0');
}

// ============================================================
//  VOICE LOGIC
// ============================================================
async function startRecording(){
    if (!micStream) {
        console.log("[VOICE] Mic stream missing, requesting now...");
        const ok = await requestMic();
        if (!ok) {
            $('vStatus').textContent = 'Microphone access denied';
            return;
        }
    }
    
    // Snapshot the current question indexing to prevent race conditions during async encoding
    const snapRootIdx = curRootIndex;
    const snapIsFollowUp = curIsFollowUp;
    
    try {
        audioChunks = []; mediaRec = new MediaRecorder(micStream);
        mediaRec.ondataavailable = e => audioChunks.push(e.data);
        mediaRec.onstop = async () => {
          pendingEncodings++;
          const webmBlob = new Blob(audioChunks, {type:'audio/webm'});
          $('vStatus').textContent = 'Converting to MP3...';
          try {
               const mp3Blob = await encodeToMp3(webmBlob);
               // Determine field based on snapshot
               const voiceKey = snapIsFollowUp ? `${snapRootIdx}_f` : `${snapRootIdx}_m`;
               voiceBlobs[voiceKey] = mp3Blob;
               
               $('voicePreview').src = URL.createObjectURL(mp3Blob);
               show('voicePlayback');
               $('vStatus').textContent = 'MP3 Ready. #=Confirm';
           } catch(e) {
               console.error("[VOICE] MP3 Conversion failed:", e);
               const voiceKey = snapIsFollowUp ? `${snapRootIdx}_f` : `${snapRootIdx}_m`;
               voiceBlobs[voiceKey] = webmBlob; // Fallback
               $('vStatus').textContent = 'WebM fallback. #=Confirm';
           } finally {
               pendingEncodings--;
           }
        };
        mediaRec.start();
        $('vRing').className = 'v-ring rec';
        $('vStatus').textContent = 'Recording...';
    } catch(e) {
        console.error("[VOICE] MediaRecorder Error:", e);
        $('vStatus').textContent = 'Recording failed to start';
    }
}

async function encodeToMp3(blob) {
    const arrayBuffer = await blob.arrayBuffer();
    const audioCtx = new (window.AudioContext || window.webkitAudioContext)({ sampleRate: 44100 });
    const audioBuffer = await audioCtx.decodeAudioData(arrayBuffer);
    
    if (typeof lamejs === 'undefined') {
        throw new Error("lamejs not loaded");
    }
    const mp3encoder = new lamejs.Mp3Encoder(1, audioBuffer.sampleRate, 128);
    const samples = audioBuffer.getChannelData(0); 
    const sampleBlockSize = 1152;
    const mp3Data = [];
    
    // Scale float32 to int16
    const int16Samples = new Int16Array(samples.length);
    for (let i = 0; i < samples.length; i++) {
        const s = Math.max(-1, Math.min(1, samples[i]));
        int16Samples[i] = s < 0 ? s * 0x8000 : s * 0x7FFF;
    }
    
    for (let i = 0; i < int16Samples.length; i += sampleBlockSize) {
        const sampleChunk = int16Samples.subarray(i, i + sampleBlockSize);
        const mp3buf = mp3encoder.encodeBuffer(sampleChunk);
        if (mp3buf.length > 0) mp3Data.push(mp3buf);
    }
    const end = mp3encoder.flush();
    if (end.length > 0) mp3Data.push(end);
    
    return new Blob(mp3Data, { type: 'audio/mp3' });
}

function stopRecording(){ if(mediaRec) mediaRec.stop(); $('vRing').className = 'v-ring done'; }
function toggleLocalPlayback(btn, audioId) {
  const a = audioId ? $(audioId) : $('voicePreview');
  const b = btn || $('waPlayBtn');
  if(a.paused){ 
    // Pause all other audios in results if any
    document.querySelectorAll('audio').forEach(el => { if(el !== a) el.pause(); });
    document.querySelectorAll('.wa-play').forEach(el => { if(el !== b) el.textContent = '▶'; });
    
    a.play(); b.textContent = '⏸'; 
  } else { 
    a.pause(); b.textContent = '▶'; 
  }
}
function updateVoiceUI(){
  ['vs1', 'vs2', 'vs3'].forEach((id, i) => {
    const el = $(id); if(!el) return;
    el.className = 'vstep';
    if (i + 1 < voiceStage) el.classList.add('done2');
    else if (i + 1 === voiceStage) el.classList.add('active');
  });
}

// ============================================================
//  SUBMISSION & RESULTS
// ============================================================
function startActualTest() {
  testActive = true; curIndex = -1; curQDoc = null; answers = {}; voiceBlobs = {}; lastInput = "";
  show('btnSubmit'); fetch('/api/start_test');
  renderQuestion('ROLL', null);
}

async function finalizeSubmission() {
  if (!testActive && testDone) return; // Prevent double trigger
  console.log("[SUBMIT] Finalizing quiz...");
  
  testActive = false;
  testDone = true;
  
  // Inform ESP that we are done
  try {
    await fetch('/api/mode?m=DONE');
  } catch(e) {}
  
  // 0. Wait for audio encoding to finish
  if (pendingEncodings > 0) {
      const banner = document.createElement('div');
      banner.id = 'processingBanner';
      banner.className = 'submit-status-banner info';
      banner.textContent = "⏳ Processing Audio Recordings... Please Wait";
      document.body.appendChild(banner);
      
      while (pendingEncodings > 0) {
          console.log("[SUBMIT] Waiting for " + pendingEncodings + " encodings...");
          await new Promise(r => setTimeout(r, 500));
      }
      if ($('processingBanner')) $('processingBanner').remove();
  }

  // 1. Show results locally (preliminary)
  showResults();

  // 2. Background sync to backend
  const serverIp = $('serverIpIn').value.trim();
  if(!serverIp) return;

  const backendUrl = `http://${serverIp}/api/submit`;
  const formData = new FormData();
  formData.append('rollNumber', $('rollDisp').textContent || 'unknown');
  formData.append('quizId', curQuizId || 'demo_quiz');
  
  // Format answers array EXACTLY as requested: {answer, followUpAnswer}
  // We use curTotalCount to ensure the array is complete and in order
  const answersList = [];
  for (let i = 0; i < curTotalCount; i++) {
      const ansObj = answers[i] || { answer: null, followUpAnswer: null };
      
      // Map "VOICE" placeholder to null string per contract "Voice -> answer = null"
      let ans = ansObj.answer === "VOICE" ? null : ansObj.answer;
      if (!ans && ans !== 0) ans = null;

      let fAns = ansObj.followUpAnswer;
      if (fAns === "VOICE_FOLLOWUP") fAns = null;
      else if (!fAns && fAns !== 0) fAns = null;

      answersList.push({
          answer: ans,
          followUpAnswer: fAns
      });
  }
  
  formData.append('answers', JSON.stringify(answersList));

  // Attach audio files with required keys: audio_q{i} and audio_q{i}_f
  Object.keys(voiceBlobs).forEach(key => {
      const blob = voiceBlobs[key];
      if (!blob) return;
      const [rootIdx, type] = key.split('_');
      const fieldName = type === 'm' ? `audio_q${rootIdx}` : `audio_q${rootIdx}_f`;
      formData.append(fieldName, blob, `${fieldName}.mp3`);
  });

  // 3. Send with up to 2 retries
  const MAX_RETRIES = 2;
  for (let attempt = 0; attempt <= MAX_RETRIES; attempt++) {
    try {
      console.log(`[SUBMIT] Attempt ${attempt + 1} to ${backendUrl}...`);
      const response = await fetch(backendUrl, { 
        method: 'POST', 
        body: formData,
        // Ensure no cache and let browser set boundary automatically
      });

      if (response.status === 201) {
        const result = await response.json();
        console.log("[SUBMIT] Success 201:", result);
        
        // Extract score/totalQuestions as per Task 7
        const score = result.score !== undefined ? result.score : '?';
        const total = result.totalQuestions !== undefined ? result.totalQuestions : '?';
        
        // Display on screen
        $('scoreBig').textContent = score;
        const msg = `Score: ${score} / ${total}`;
        const statusEl = document.createElement('div');
        statusEl.className = 'submit-status-banner success';
        statusEl.textContent = `✅ Successfully Submitted! ${msg}`;
        document.body.appendChild(statusEl);
        
        alert(`Test Submitted Successfully!\nScore: ${score} / ${total}`);
        return; // Success
      } else {
        const errText = await response.text();
        console.warn(`[SUBMIT] Server returned ${response.status}: ${errText}`);
        if (attempt === MAX_RETRIES) throw new Error(`Server error: ${response.status}`);
      }
    } catch (e) {
      console.error(`[SUBMIT] Attempt ${attempt + 1} failed:`, e);
      if (attempt === MAX_RETRIES) {
        alert("Fatal Error: Could not submit test after multiple attempts. Please check network.");
        const statusEl = document.createElement('div');
        statusEl.className = 'submit-status-banner error';
        statusEl.textContent = `❌ Submission Failed after ${MAX_RETRIES + 1} attempts.`;
        document.body.appendChild(statusEl);
      }
      // Wait a bit before retry
      await new Promise(r => setTimeout(r, 2000));
    }
  }
}


function showResults() {
    const resBody = $('resBody'); resBody.innerHTML = '';
    let total = 0;
    
    if (curQDoc && curIndex !== -1 && !answers[curRootIndex]?.answer) {
        saveCurrentAnswer(espSec, null, curRootIndex); 
    }

    const keys = Object.keys(answers).sort((a,b)=>a-b);
    keys.forEach((k, idx) => {
        const ans = answers[k];
        total++;
        
        const card = document.createElement('div');
        card.className = 'res-card fade';
        
        // Helper to build audio player UI
        const buildAudioPlayer = (blob, label, id) => {
            if (!blob) return '';
            const url = URL.createObjectURL(blob);
            return `
                <div class="wa-audio" style="margin: 10px 0 0 0; max-width: 100%;">
                    <div class="wa-play" onclick="toggleLocalPlayback(this, '${id}')">▶</div>
                    <div class="wa-info">
                        <div class="wa-title"><span>${label}</span></div>
                        <div class="wa-prog"><div class="wa-fill" id="${id}_fill"></div></div>
                        <div class="wa-dur">Click to listen</div>
                    </div>
                </div>
                <audio id="${id}" src="${url}" class="hidden"></audio>
            `;
        };

        const mAudio = buildAudioPlayer(voiceBlobs[`${k}_m`], 'Main Recording', `aud_${k}_m`);
        const fAudio = buildAudioPlayer(voiceBlobs[`${k}_f`], 'Follow-up Recording', `aud_${k}_f`);

        card.innerHTML = `
            <div class="res-head">
                <div class="q-header">
                    <span class="q-idx">Q${parseInt(k)+1}</span>
                    <span class="badge ${ans.type}">${ans.type}</span>
                </div>
                <div class="time-taken">${ans.time}s</div>
            </div>
            <div class="res-qtxt">${ans.question}</div>
            
            <div class="res-ans-row">
                <span class="res-ans-lbl">Main Answer:</span>
                <span class="res-ans-val">${ans.answer || (voiceBlobs[`${k}_m`] ? 'Voice Response' : 'N/A')}</span>
            </div>
            
            ${(ans.followUpAnswer || voiceBlobs[`${k}_f`]) ? `
            <div class="res-ans-row">
                <span class="res-ans-lbl">Adaptive Follow-up:</span>
                <span class="res-ans-val">${ans.followUpAnswer === "VOICE_FOLLOWUP" || voiceBlobs[`${k}_f`] ? 'Voice Response' : (ans.followUpAnswer || 'N/A')}</span>
            </div>` : ''}

            ${mAudio}
            ${fAudio}
        `;

        resBody.appendChild(card);
        
        // Add progress tracking for the newly created audio elements
        [`aud_${k}_m`, `aud_${k}_f`].forEach(id => {
            const a = document.getElementById(id);
            if (a) {
                a.ontimeupdate = () => {
                    const fill = $(id + '_fill');
                    if (fill && a.duration) fill.style.width = (a.currentTime / a.duration * 100) + '%';
                };
                a.onended = () => {
                    const btn = a.previousElementSibling.querySelector('.wa-play');
                    if (btn) btn.textContent = '▶';
                    const fill = $(id + '_fill');
                    if (fill) fill.style.width = '0%';
                };
            }
        });
    });

    $('scoreBig').textContent = total || '0';
    show('resCard'); hide('qCard'); hide('ctrlCard'); hide('reviewCard'); hide('rollCard');
}

function resetToHome() {
    testActive = false;
    testDone = false;
    hide('resCard');
    show('ctrlCard');
    fetch('/api/mode?m=IDLE').catch(()=>{});
}

async function uploadAllAudio() {
  const formData = new FormData(); let hasAudio = false;
  Object.keys(voiceBlobs).forEach(key => { if(voiceBlobs[key]){ formData.append('audio_'+key, voiceBlobs[key], 'voice_'+key+'.webm'); hasAudio = true; } });
  if (!hasAudio) return;
  try { await fetch(SERVER_AUDIO_ENDPOINT, { method: 'POST', body: formData }); } catch (e) {}
}

// ============================================================
//  JSON & REMOTE
// ============================================================
function parseJsonInput(){
  const raw = $('jsonIn').value.trim();
  try { const obj = JSON.parse(raw); validateAndLoad(obj); } catch(e){ alert('Invalid JSON: '+e.message); }
}
function validateAndLoad(obj){
    hide('loadCard');
    fetch('/api/load_questions',{ method:'POST', body: JSON.stringify(obj) });
}
function runAutoGenerate(){
  const sample = {
    type:'mcq', text:'What is the capital of France?',
    options:[
      {text:'Berlin', isCorrect:false, followUp:null},
      {text:'Paris', isCorrect:true, followUp:{
          type:'numeric', text:'How many districts are in Paris?', followUp:null
      }}
    ]
  };
  $('jsonIn').value = JSON.stringify(sample, null, 2);
  validateAndLoad(sample);
}
)rawliteral";
