#ifndef WEB_PAGE_H
#define WEB_PAGE_H

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1.0">
<title>ESP32 Test Station</title>
<style>
*{margin:0;padding:0;box-sizing:border-box;}
@import url('https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700;800&display=swap');
:root{
  --bg:#0a0a0f;--surface:#12121a;--surface2:#1a1a28;--surface3:#222236;
  --border:#2a2a44;--text:#e8e8f0;--text2:#a0a0b8;--text3:#6a6a82;
  --accent:#6c5ce7;--accent2:#a29bfe;--accent3:#7c6ff0;
  --success:#00b894;--danger:#e74c3c;--warn:#fdcb6e;
  --glow:0 0 20px rgba(108,92,231,0.3);
}
body{font-family:'Inter',sans-serif;background:var(--bg);color:var(--text);min-height:100vh;overflow-x:hidden;}
.container{max-width:720px;margin:0 auto;padding:16px;}

/* Header */
.header{text-align:center;padding:24px 0 16px;position:relative;}
.header h1{font-size:1.8em;font-weight:800;background:linear-gradient(135deg,var(--accent),var(--accent2),#fd79a8);-webkit-background-clip:text;-webkit-text-fill-color:transparent;letter-spacing:-0.5px;}
.header .subtitle{color:var(--text3);font-size:0.85em;margin-top:4px;}
.status-bar{display:flex;gap:8px;justify-content:center;margin-top:12px;flex-wrap:wrap;}
.status-chip{background:var(--surface2);border:1px solid var(--border);border-radius:20px;padding:4px 14px;font-size:0.75em;color:var(--text2);display:flex;align-items:center;gap:6px;}
.status-chip .dot{width:6px;height:6px;border-radius:50%;background:var(--danger);animation:pulse 2s infinite;}
.status-chip .dot.active{background:var(--success);}
@keyframes pulse{0%,100%{opacity:1;}50%{opacity:0.4;}}

/* Cards */
.card{background:var(--surface);border:1px solid var(--border);border-radius:16px;padding:20px;margin-bottom:16px;transition:all 0.3s ease;}
.card:hover{border-color:var(--accent);box-shadow:var(--glow);}
.card-title{font-size:1em;font-weight:600;color:var(--accent2);margin-bottom:12px;display:flex;align-items:center;gap:8px;}

/* Buttons */
.btn{border:none;border-radius:12px;padding:12px 24px;font-family:'Inter',sans-serif;font-weight:600;font-size:0.9em;cursor:pointer;transition:all 0.3s ease;display:inline-flex;align-items:center;gap:8px;position:relative;overflow:hidden;}
.btn::after{content:'';position:absolute;inset:0;background:linear-gradient(135deg,rgba(255,255,255,0.1),transparent);opacity:0;transition:opacity 0.3s;}
.btn:hover::after{opacity:1;}
.btn:active{transform:scale(0.97);}
.btn-primary{background:linear-gradient(135deg,var(--accent),var(--accent3));color:#fff;box-shadow:0 4px 15px rgba(108,92,231,0.4);}
.btn-primary:hover{box-shadow:0 6px 25px rgba(108,92,231,0.6);transform:translateY(-1px);}
.btn-secondary{background:var(--surface2);color:var(--text2);border:1px solid var(--border);}
.btn-secondary:hover{background:var(--surface3);color:var(--text);}
.btn-success{background:linear-gradient(135deg,var(--success),#00cec9);color:#fff;box-shadow:0 4px 15px rgba(0,184,148,0.4);}
.btn-danger{background:linear-gradient(135deg,var(--danger),#e17055);color:#fff;}
.btn-block{width:100%;justify-content:center;}
.btn-lg{padding:16px 32px;font-size:1.05em;border-radius:14px;}
.btn:disabled{opacity:0.4;cursor:not-allowed;transform:none !important;}

/* Question area */
.question-box{background:var(--surface2);border:1px solid var(--border);border-radius:14px;padding:20px;margin:12px 0;}
.q-number{font-size:0.8em;color:var(--accent2);font-weight:500;margin-bottom:6px;text-transform:uppercase;letter-spacing:1px;}
.q-text{font-size:1.15em;font-weight:500;line-height:1.6;}
.q-type-badge{display:inline-block;padding:3px 12px;border-radius:8px;font-size:0.7em;font-weight:700;text-transform:uppercase;letter-spacing:1px;margin-left:8px;}
.q-type-badge.mcq{background:rgba(108,92,231,0.2);color:var(--accent2);}
.q-type-badge.numeric{background:rgba(253,203,110,0.2);color:var(--warn);}
.q-type-badge.voice{background:rgba(232,67,147,0.2);color:#e84393;}

/* Options */
.options-grid{display:grid;grid-template-columns:1fr 1fr;gap:8px;margin-top:16px;}
.option-card{background:var(--surface);border:2px solid var(--border);border-radius:12px;padding:14px 16px;font-size:0.9em;text-align:center;transition:all 0.3s;position:relative;}
.option-card.selected{border-color:var(--accent);background:rgba(108,92,231,0.15);box-shadow:0 0 15px rgba(108,92,231,0.2);}
.option-card.correct{border-color:var(--success);background:rgba(0,184,148,0.15);}
.option-card.wrong{border-color:var(--danger);background:rgba(231,76,60,0.15);}
.option-label{font-weight:700;color:var(--accent2);font-size:0.8em;display:block;margin-bottom:4px;}

/* Numeric display */
.numeric-display{background:var(--surface);border:2px solid var(--border);border-radius:14px;padding:20px;text-align:center;font-size:2em;font-weight:700;font-family:'Courier New',monospace;color:var(--accent2);margin-top:16px;min-height:70px;letter-spacing:4px;}
.numeric-display.empty{color:var(--text3);font-size:1em;font-weight:400;}

/* Voice */
.voice-area{text-align:center;margin-top:16px;padding:20px;}
.voice-indicator{width:80px;height:80px;border-radius:50%;margin:0 auto 16px;display:flex;align-items:center;justify-content:center;font-size:2em;transition:all 0.3s;}
.voice-indicator.idle{background:var(--surface);border:3px solid var(--border);}
.voice-indicator.recording{background:rgba(231,76,60,0.2);border:3px solid var(--danger);animation:rec-pulse 1s infinite;}
@keyframes rec-pulse{0%,100%{box-shadow:0 0 0 0 rgba(231,76,60,0.4);}50%{box-shadow:0 0 0 20px rgba(231,76,60,0);}}
.voice-status{font-size:0.9em;color:var(--text2);}

/* Timer */
.timer-display{font-family:'Courier New',monospace;font-size:1.4em;font-weight:700;color:var(--warn);text-align:center;padding:8px;background:var(--surface);border-radius:10px;border:1px solid rgba(253,203,110,0.2);margin:8px 0;}

/* Progress */
.progress-bar{height:4px;background:var(--surface2);border-radius:2px;margin:12px 0;overflow:hidden;}
.progress-fill{height:100%;background:linear-gradient(90deg,var(--accent),var(--accent2));border-radius:2px;transition:width 0.5s ease;}

/* JSON area */
.json-area{width:100%;min-height:200px;background:var(--bg);color:var(--text);border:1px solid var(--border);border-radius:12px;padding:14px;font-family:'Courier New',monospace;font-size:0.8em;resize:vertical;outline:none;transition:border-color 0.3s;}
.json-area:focus{border-color:var(--accent);}
.btn-row{display:flex;gap:8px;margin-top:12px;flex-wrap:wrap;}

/* Results */
.results-card{background:var(--surface);border:1px solid var(--border);border-radius:14px;padding:16px;margin-bottom:12px;}
.results-card .q-header{display:flex;justify-content:space-between;align-items:center;margin-bottom:8px;}
.score-display{text-align:center;padding:24px;margin-bottom:16px;}
.score-big{font-size:3em;font-weight:800;background:linear-gradient(135deg,var(--accent),var(--success));-webkit-background-clip:text;-webkit-text-fill-color:transparent;}
.score-label{color:var(--text3);font-size:0.85em;margin-top:4px;}
.mark-badge{padding:4px 12px;border-radius:8px;font-size:0.75em;font-weight:700;}
.mark-badge.correct{background:rgba(0,184,148,0.2);color:var(--success);}
.mark-badge.wrong{background:rgba(231,76,60,0.2);color:var(--danger);}
.mark-badge.na{background:rgba(160,160,184,0.2);color:var(--text3);}
audio{width:100%;margin-top:8px;border-radius:8px;}

/* HW indicator */
.hw-status{display:flex;gap:12px;justify-content:center;flex-wrap:wrap;margin:8px 0;}
.hw-chip{font-size:0.75em;padding:4px 10px;background:var(--surface2);border:1px solid var(--border);border-radius:8px;color:var(--text3);}
.hw-chip.active{border-color:var(--success);color:var(--success);}

/* Hide sections */
.hidden{display:none !important;}

/* Animations */
.fade-in{animation:fadeIn 0.4s ease;}
@keyframes fadeIn{from{opacity:0;transform:translateY(10px);}to{opacity:1;transform:translateY(0);}}

/* Responsive */
@media(max-width:480px){
  .options-grid{grid-template-columns:1fr;}
  .btn-row{flex-direction:column;}
  .btn-row .btn{width:100%;justify-content:center;}
}
</style>
</head>
<body>
<div class="container">
  <!-- HEADER -->
  <div class="header">
    <h1>⚡ ESP32 Test Station</h1>
    <p class="subtitle">Hardware-Integrated Examination System</p>
    <div class="status-bar">
      <div class="status-chip"><span class="dot active" id="wifiDot"></span><span id="wifiStatus">Connected</span></div>
      <div class="status-chip"><span class="dot" id="hwDot"></span><span id="hwStatus">Hardware Ready</span></div>
      <div class="status-chip" id="modeChip"><span>Mode: IDLE</span></div>
    </div>
  </div>

  <!-- CONTROL PANEL -->
  <div class="card" id="controlPanel">
    <div class="card-title">🎮 Control Panel</div>
    <div class="progress-bar"><div class="progress-fill" id="progressFill" style="width:0%"></div></div>
    <p style="font-size:0.8em;color:var(--text3);margin-bottom:12px;" id="progressText">No questions loaded</p>
    <div class="timer-display" id="timerDisplay">00:00</div>
    <div class="btn-row" style="margin-top:12px;">
      <button class="btn btn-primary btn-lg btn-block" id="btnTestStarter" disabled>🚀 Test Starter</button>
    </div>
    <div class="btn-row">
      <button class="btn btn-secondary" id="btnLoadQ">📋 Load Questions</button>
      <button class="btn btn-secondary" id="btnAutoGen">⚡ Auto-Generate</button>
      <button class="btn btn-danger hidden" id="btnSubmit">📤 Submit Test</button>
    </div>
  </div>

  <!-- LOAD QUESTIONS PANEL -->
  <div class="card hidden" id="loadPanel">
    <div class="card-title">📋 Load Questions JSON</div>
    <textarea class="json-area" id="jsonInput" placeholder='Paste your JSON here...\n[\n  {"type":"mcq","question":"...","options":["A","B","C","D"],"answer":"A"},\n  {"type":"numeric","question":"...","answer":42},\n  {"type":"voice","question":"..."}\n]'></textarea>
    <div class="btn-row">
      <button class="btn btn-primary" id="btnParseJson">✅ Parse & Load</button>
      <button class="btn btn-secondary" id="btnCancelLoad">Cancel</button>
    </div>
  </div>

  <!-- QUESTION DISPLAY -->
  <div class="card hidden fade-in" id="questionPanel">
    <div class="question-box">
      <div class="q-number" id="qNumber">Question 1 of 5</div>
      <div style="display:flex;align-items:center;flex-wrap:wrap;">
        <span class="q-text" id="qText">Loading question...</span>
        <span class="q-type-badge" id="qBadge">MCQ</span>
      </div>
    </div>
    <!-- MCQ Options -->
    <div id="mcqArea" class="hidden">
      <div class="options-grid" id="optionsGrid"></div>
      <p style="font-size:0.75em;color:var(--text3);text-align:center;margin-top:8px;">Press A/B/C/D on the keypad to select</p>
    </div>
    <!-- Numeric Input -->
    <div id="numericArea" class="hidden">
      <div class="numeric-display empty" id="numDisplay">Use keypad 0-9</div>
      <p style="font-size:0.75em;color:var(--text3);text-align:center;margin-top:8px;">Use keypad digits • * to clear • # to confirm</p>
    </div>
    <!-- Voice Recording -->
    <div id="voiceArea" class="hidden">
      <div class="voice-area">
        <div class="voice-indicator idle" id="voiceIndicator">🎤</div>
        <p class="voice-status" id="voiceStatus">Touch sensor to start recording</p>
        <p style="font-size:0.75em;color:var(--text3);margin-top:8px;">Touch GPIO4 to toggle recording</p>
      </div>
    </div>
    <!-- HW Feedback -->
    <div class="hw-status" id="hwFeedback">
      <div class="hw-chip" id="hwKeypad">⌨️ Keypad</div>
      <div class="hw-chip" id="hwBuzzer">🔔 Buzzer</div>
      <div class="hw-chip" id="hwTouch">👆 Touch</div>
    </div>
  </div>

  <!-- RESULTS PANEL -->
  <div class="card hidden" id="resultsPanel">
    <div class="card-title">📊 Test Results</div>
    <div class="score-display">
      <div class="score-big" id="scoreBig">0/0</div>
      <div class="score-label">Questions Correct</div>
    </div>
    <div id="resultsBody"></div>
  </div>
</div>

<script>
// ============== STATE ==============
let questions = [];
let currentQ = -1;    // -1 = not started
let answers = [];     // user answers per question
let timers = [];      // time per question in seconds
let voiceBlobs = [];  // audio blobs for voice questions
let testActive = false;
let testFinished = false;
let timerInterval = null;
let currentTimerSec = 0;
let pollInterval = null;
let mediaRecorder = null;
let audioChunks = [];
let isRecording = false;

// ============== DOM REFS ==============
const $ = id => document.getElementById(id);
const btnTest = $('btnTestStarter');
const btnLoad = $('btnLoadQ');
const btnAuto = $('btnAutoGen');
const btnSubmit = $('btnSubmit');
const btnParse = $('btnParseJson');
const btnCancel = $('btnCancelLoad');
const loadPanel = $('loadPanel');
const questionPanel = $('questionPanel');
const controlPanel = $('controlPanel');
const resultsPanel = $('resultsPanel');
const progressFill = $('progressFill');
const progressText = $('progressText');
const timerDisplay = $('timerDisplay');
const modeChip = $('modeChip');
const hwDot = $('hwDot');

// ============== POLLING ==============
function startPolling() {
  if (pollInterval) clearInterval(pollInterval);
  pollInterval = setInterval(pollESP, 300);
}

async function pollESP() {
  try {
    const r = await fetch('/api/state');
    const d = await r.json();
    handleESPState(d);
  } catch(e) {}
}

function handleESPState(d) {
  // Update HW status
  hwDot.className = 'dot active';
  $('hwStatus').textContent = 'HW Active';

  // Key press
  if (d.key && d.key !== '' && testActive && currentQ >= 0) {
    processKeyPress(d.key);
  }

  // Touch
  if (d.touch_event && testActive && currentQ >= 0 && questions[currentQ].type === 'voice') {
    toggleVoiceRecording();
  }

  // Timer from ESP
  if (d.timer !== undefined) {
    currentTimerSec = d.timer;
    updateTimerDisplay();
  }
}

function processKeyPress(key) {
  if (!testActive || currentQ < 0 || currentQ >= questions.length) return;
  const q = questions[currentQ];

  if (q.type === 'mcq') {
    const keyUpper = key.toUpperCase();
    if (['A','B','C','D'].includes(keyUpper)) {
      const idx = keyUpper.charCodeAt(0) - 65;
      if (idx < q.options.length) {
        answers[currentQ] = keyUpper;
        renderMCQSelection(idx);
        flashHW('hwKeypad');
      }
    }
  } else if (q.type === 'numeric') {
    if (key >= '0' && key <= '9') {
      if (answers[currentQ] === null || answers[currentQ] === undefined) answers[currentQ] = '';
      answers[currentQ] += key;
      renderNumericDisplay();
      flashHW('hwKeypad');
    } else if (key === '*') {
      // Clear
      answers[currentQ] = '';
      renderNumericDisplay();
    } else if (key === '#') {
      // Confirm numeric
      flashHW('hwBuzzer');
    }
  }
}

function flashHW(id) {
  const el = $(id);
  el.classList.add('active');
  setTimeout(() => el.classList.remove('active'), 300);
}

// ============== VOICE RECORDING ==============
async function toggleVoiceRecording() {
  if (!isRecording) {
    try {
      const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
      mediaRecorder = new MediaRecorder(stream);
      audioChunks = [];
      mediaRecorder.ondataavailable = e => audioChunks.push(e.data);
      mediaRecorder.onstop = () => {
        const blob = new Blob(audioChunks, { type: 'audio/webm' });
        voiceBlobs[currentQ] = blob;
        answers[currentQ] = 'RECORDED';
        stream.getTracks().forEach(t => t.stop());
        $('voiceIndicator').className = 'voice-indicator idle';
        $('voiceStatus').textContent = '✅ Recording saved! Touch to re-record.';
      };
      mediaRecorder.start();
      isRecording = true;
      $('voiceIndicator').className = 'voice-indicator recording';
      $('voiceStatus').textContent = '🔴 Recording... Touch to stop';
      flashHW('hwTouch');
    } catch(e) {
      $('voiceStatus').textContent = '❌ Microphone access denied';
    }
  } else {
    mediaRecorder.stop();
    isRecording = false;
    flashHW('hwTouch');
  }
}

// ============== RENDERING ==============
function renderQuestion() {
  if (currentQ < 0 || currentQ >= questions.length) return;
  const q = questions[currentQ];

  questionPanel.classList.remove('hidden');
  questionPanel.classList.add('fade-in');
  $('qNumber').textContent = `Question ${currentQ + 1} of ${questions.length}`;
  $('qText').textContent = q.question;

  const badge = $('qBadge');
  badge.textContent = q.type.toUpperCase();
  badge.className = 'q-type-badge ' + q.type;

  // Update progress
  progressFill.style.width = ((currentQ + 1) / questions.length * 100) + '%';
  progressText.textContent = `Question ${currentQ + 1} / ${questions.length}`;

  // Update mode chip
  let modeText = 'IDLE';
  if (q.type === 'mcq') modeText = 'MCQ';
  else if (q.type === 'numeric') modeText = 'NUM';
  else if (q.type === 'voice') modeText = 'VOICE';
  modeChip.querySelector('span').textContent = 'Mode: ' + modeText;

  // Notify ESP of mode
  fetch('/api/mode?m=' + modeText).catch(()=>{});

  // Hide all input areas
  $('mcqArea').classList.add('hidden');
  $('numericArea').classList.add('hidden');
  $('voiceArea').classList.add('hidden');

  if (q.type === 'mcq') {
    $('mcqArea').classList.remove('hidden');
    renderMCQOptions(q);
  } else if (q.type === 'numeric') {
    $('numericArea').classList.remove('hidden');
    renderNumericDisplay();
  } else if (q.type === 'voice') {
    $('voiceArea').classList.remove('hidden');
    isRecording = false;
    $('voiceIndicator').className = 'voice-indicator idle';
    $('voiceStatus').textContent = 'Touch sensor to start recording';
  }

  // Reset timer for this question
  fetch('/api/reset_timer').catch(()=>{});

  // Button text
  if (currentQ < questions.length - 1) {
    btnTest.textContent = '⏭️ Next Question';
  } else {
    btnTest.textContent = '🏁 Finish Test';
  }
}

function renderMCQOptions(q) {
  const grid = $('optionsGrid');
  grid.innerHTML = '';
  const labels = ['A','B','C','D'];
  q.options.forEach((opt, i) => {
    const div = document.createElement('div');
    div.className = 'option-card';
    div.id = 'opt_' + i;
    div.innerHTML = `<span class="option-label">${labels[i]}</span>${opt}`;
    grid.appendChild(div);
  });
  if (answers[currentQ]) {
    const selIdx = answers[currentQ].charCodeAt(0) - 65;
    renderMCQSelection(selIdx);
  }
}

function renderMCQSelection(idx) {
  document.querySelectorAll('.option-card').forEach((el, i) => {
    el.classList.toggle('selected', i === idx);
  });
}

function renderNumericDisplay() {
  const disp = $('numDisplay');
  const val = answers[currentQ];
  if (!val || val === '') {
    disp.textContent = 'Use keypad 0-9';
    disp.className = 'numeric-display empty';
  } else {
    disp.textContent = val;
    disp.className = 'numeric-display';
  }
}

function updateTimerDisplay() {
  const mins = Math.floor(currentTimerSec / 60);
  const secs = currentTimerSec % 60;
  timerDisplay.textContent = String(mins).padStart(2,'0') + ':' + String(secs).padStart(2,'0');
}

// ============== FLOW ==============
btnTest.addEventListener('click', () => {
  if (testFinished) return;

  if (!testActive) {
    // Start test
    testActive = true;
    currentQ = 0;
    answers = new Array(questions.length).fill(null);
    timers = new Array(questions.length).fill(0);
    voiceBlobs = new Array(questions.length).fill(null);
    btnSubmit.classList.remove('hidden');
    fetch('/api/start_test').catch(()=>{});
    renderQuestion();
    startPolling();
  } else {
    // Save current timer
    timers[currentQ] = currentTimerSec;

    // Stop recording if active
    if (isRecording && mediaRecorder) {
      mediaRecorder.stop();
      isRecording = false;
    }

    currentQ++;
    if (currentQ >= questions.length) {
      // End of test
      finishTest();
    } else {
      renderQuestion();
    }
  }
});

function finishTest() {
  testActive = false;
  testFinished = true;
  if (pollInterval) clearInterval(pollInterval);
  questionPanel.classList.add('hidden');
  btnTest.disabled = true;
  btnTest.textContent = '✅ Test Completed';
  btnSubmit.classList.remove('hidden');
  fetch('/api/mode?m=DONE').catch(()=>{});
  modeChip.querySelector('span').textContent = 'Mode: DONE';
}

btnSubmit.addEventListener('click', () => {
  showResults();
});

function showResults() {
  resultsPanel.classList.remove('hidden');
  resultsPanel.classList.add('fade-in');
  btnSubmit.classList.add('hidden');
  controlPanel.style.display = 'none';

  let correct = 0;
  let total = 0;
  const body = $('resultsBody');
  body.innerHTML = '';

  questions.forEach((q, i) => {
    const card = document.createElement('div');
    card.className = 'results-card fade-in';

    let isCorrect = false;
    let userAns = answers[i] || 'No answer';
    let correctAns = q.answer !== undefined ? String(q.answer) : 'N/A';

    if (q.type === 'mcq') {
      total++;
      isCorrect = (answers[i] === q.answer);
      if (isCorrect) correct++;
    } else if (q.type === 'numeric') {
      total++;
      isCorrect = (String(answers[i]) === String(q.answer));
      if (isCorrect) correct++;
    } else if (q.type === 'voice') {
      correctAns = 'Voice response';
      userAns = answers[i] === 'RECORDED' ? 'Recorded' : 'Not recorded';
    }

    const markClass = q.type === 'voice' ? 'na' : (isCorrect ? 'correct' : 'wrong');
    const markText = q.type === 'voice' ? 'VOICE' : (isCorrect ? '✓ CORRECT' : '✗ WRONG');

    let html = `
      <div class="q-header">
        <div>
          <span class="q-number">Q${i+1}</span>
          <span class="q-type-badge ${q.type}">${q.type.toUpperCase()}</span>
        </div>
        <span class="mark-badge ${markClass}">${markText}</span>
      </div>
      <p style="margin:8px 0;font-size:0.95em;">${q.question}</p>
      <p style="font-size:0.8em;color:var(--text3);">Your answer: <strong style="color:var(--text)">${userAns}</strong></p>
      <p style="font-size:0.8em;color:var(--text3);">Correct: <strong style="color:var(--success)">${correctAns}</strong></p>
      <p style="font-size:0.8em;color:var(--text3);">Time: ${timers[i]}s</p>
    `;

    if (q.type === 'voice' && voiceBlobs[i]) {
      const url = URL.createObjectURL(voiceBlobs[i]);
      html += `<audio controls src="${url}"></audio>`;
    }

    card.innerHTML = html;
    body.appendChild(card);
  });

  $('scoreBig').textContent = `${correct}/${total}`;
}

// ============== LOAD QUESTIONS ==============
btnLoad.addEventListener('click', () => {
  loadPanel.classList.toggle('hidden');
});

btnCancel.addEventListener('click', () => {
  loadPanel.classList.add('hidden');
});

btnParse.addEventListener('click', () => {
  try {
    const raw = $('jsonInput').value.trim();
    const parsed = JSON.parse(raw);
    if (!Array.isArray(parsed) || parsed.length === 0) throw new Error('Must be a non-empty array');
    parsed.forEach((q, i) => {
      if (!q.type || !q.question) throw new Error(`Q${i+1}: missing type or question`);
      if (!['mcq','numeric','voice'].includes(q.type)) throw new Error(`Q${i+1}: invalid type "${q.type}"`);
      if (q.type === 'mcq' && (!q.options || q.options.length < 2)) throw new Error(`Q${i+1}: MCQ needs options`);
    });
    questions = parsed;
    loadPanel.classList.add('hidden');
    btnTest.disabled = false;
    progressText.textContent = `${questions.length} questions loaded — Click Test Starter to begin`;
    // Send to ESP
    fetch('/api/load_questions', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify({ count: questions.length, types: questions.map(q => q.type) })
    }).catch(()=>{});
  } catch(e) {
    alert('JSON Error: ' + e.message);
  }
});

// ============== AUTO-GENERATE ==============
btnAuto.addEventListener('click', () => {
  const sample = [
    {
      "type": "mcq",
      "question": "What is the capital of France?",
      "options": ["Berlin", "Madrid", "Paris", "Rome"],
      "answer": "C"
    },
    {
      "type": "mcq",
      "question": "Which planet is known as the Red Planet?",
      "options": ["Venus", "Mars", "Jupiter", "Saturn"],
      "answer": "B"
    },
    {
      "type": "mcq",
      "question": "What is the chemical symbol for water?",
      "options": ["CO2", "H2O", "NaCl", "O2"],
      "answer": "B"
    },
    {
      "type": "numeric",
      "question": "What is 15 × 7?",
      "answer": 105
    },
    {
      "type": "numeric",
      "question": "What is the square root of 144?",
      "answer": 12
    },
    {
      "type": "voice",
      "question": "Explain the concept of photosynthesis in your own words."
    },
    {
      "type": "mcq",
      "question": "Which programming language is used for ESP32 with Arduino framework?",
      "options": ["Python", "Java", "C++", "Ruby"],
      "answer": "C"
    },
    {
      "type": "numeric",
      "question": "How many bits are in a byte?",
      "answer": 8
    },
    {
      "type": "voice",
      "question": "Describe how a microcontroller differs from a microprocessor."
    },
    {
      "type": "mcq",
      "question": "What does HTTP stand for?",
      "options": ["HyperText Transfer Protocol", "High Tech Transfer Process", "Hyper Transfer Text Protocol", "Home Tool Transfer Protocol"],
      "answer": "A"
    }
  ];
  questions = sample;
  btnTest.disabled = false;
  progressText.textContent = `${questions.length} sample questions loaded — Click Test Starter to begin`;
  loadPanel.classList.add('hidden');
  $('jsonInput').value = JSON.stringify(sample, null, 2);
  // Notify ESP
  fetch('/api/load_questions', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify({ count: questions.length, types: questions.map(q => q.type) })
  }).catch(()=>{});
});

// ============== INIT ==============
startPolling();
</script>
</body>
</html>
)rawliteral";

#endif // WEB_PAGE_H
