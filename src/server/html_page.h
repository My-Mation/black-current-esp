#pragma once
#include <pgmspace.h>
// =============================================================
//  html_page.h — Full web UI assembled from separate components
// =============================================================

#include "web_css.h"
#include "web_js.h"

const char HTML_PART1[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1.0">
<title>ESP32 Test Station</title>
<style>
)rawliteral";

const char HTML_PART2[] PROGMEM = R"rawliteral(
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

<!-- =========== ROLL INPUT CARD =========== -->
<div class="card hidden fade" id="rollCard">
  <div class="card-ttl">🆔 Student Registration</div>
  <p class="prog-lbl">Enter your Roll Number to begin</p>
  <div class="num-display empty" id="rollDisp">Use keypad 0–9</div>
  <p class="hint">Press # to confirm and start test</p>
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
      <p class="v-status" id="vStatus">Press <b>1</b> to start recording</p>
      
      <!-- WhatsApp Style Playback -->
      <div id="voicePlayback" class="hidden" style="margin-top:15px;">
        <div class="wa-audio">
          <div class="wa-play" id="waPlayBtn" onclick="toggleLocalPlayback()">▶</div>
          <div class="wa-info">
            <div class="wa-title"><span>Voice Recording</span> <span id="waTime">0:00</span></div>
            <div class="wa-prog"><div class="wa-fill" id="waFill"></div></div>
            <div class="wa-dur">Press <b>0</b>=Play · <b>D</b>=Delete · <b>#</b>=Confirm</div>
          </div>
        </div>
        <audio id="voicePreview" class="hidden"></audio>
      </div>

      <div class="v-stage">
        <div class="vstep active" id="vs1">1 · Rec (1)</div>
        <div class="vstep" id="vs2">2 · Stop (2)</div>
        <div class="vstep" id="vs3">3 · Done (#)</div>
      </div>
      <p class="hint" id="vHint" style="margin-top:15px">Keypad: 1=Start · 2=Stop · D=Delete · #=Submit</p>
    </div>
  </div>
</div>

<!-- =========== REVIEW PROMPT =========== -->
<div class="card hidden fade" id="reviewCard">
  <div class="card-ttl">🏁 All Questions Answered</div>
  <p style="margin-bottom:15px; color:var(--txt2);">You can revisit questions by pressing the <strong>Star (*)</strong> button. <br><br>If you are finished, press the <strong>Test Starter</strong> (Hardware Button) to submit your results to the server.</p>
  <div class="row">
    <button class="btn btn-p btn-blk" id="btnHWSubmit">📤 Submit Test (Hardware Button)</button>
    <button class="btn btn-s btn-blk" id="btnGoBackReview">🔍 Review Previous (*) </button>
  </div>
</div>

<!-- =========== RESULTS CARD =========== -->
<div class="card hidden fade" id="resCard">
  <div class="card-ttl">📊 Results</div>
  <div class="score-hdr">
    <div class="score-big" id="scoreBig">0/0</div>
    <div class="score-lbl">Questions Answered Correctly</div>
  </div>
  <div id="uploadStatus" class="prog-lbl" style="margin-top:10px; text-align:center; color:var(--p1); font-weight:bold;"></div>
  <div id="resBody"></div>
</div>

</div><!-- /wrap -->

<script>
)rawliteral";

const char HTML_PART3[] PROGMEM = R"rawliteral(
</script>
</body>
</html>
)rawliteral";
