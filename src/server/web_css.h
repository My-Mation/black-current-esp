#pragma once
#include <pgmspace.h>
// =============================================================
//  web_css.h — CSS styles for the web UI
// =============================================================

const char WEB_CSS[] PROGMEM = R"rawliteral(
/* ===================== RESET & TOKENS ===================== */
*{margin:0;padding:0;box-sizing:border-box;}
:root{
  --bg:#111215;
  --s1:#17191e;--s2:#1e2026;--s3:#24272e;
  --bdr:#2c2f38;--bdr2:#373b48;
  --txt:#d8dae0;--txt2:#8b909e;--txt3:#555b6b;
  --acc:#4a90d9;--acc2:#72aae3;--acc3:#2e6fb5;
  --ok:#3da87a;--err:#c0392b;--warn:#d4a017;--info:#5b9bd5;
  --rad:10px;--rad2:7px;--rad3:4px;
}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',system-ui,sans-serif;
  background:var(--bg);color:var(--txt);min-height:100vh;overflow-x:hidden;
  -webkit-font-smoothing:antialiased;}

/* ===================== LAYOUT ===================== */
.wrap{max-width:700px;margin:0 auto;padding:16px 16px 48px;}

/* ===================== HEADER ===================== */
.hdr{padding:24px 0 16px;border-bottom:1px solid var(--bdr);margin-bottom:16px;}
.hdr h1{font-size:1.25em;font-weight:700;letter-spacing:-0.3px;color:var(--txt);}
.hdr p{color:var(--txt3);font-size:.8em;margin-top:4px;}
.chips{display:flex;gap:6px;flex-wrap:wrap;margin-top:12px;}
.chip{background:var(--s2);border:1px solid var(--bdr);
  border-radius:5px;padding:3px 10px;font-size:.71em;color:var(--txt2);
  display:flex;align-items:center;gap:5px;}
.dot{width:6px;height:6px;border-radius:50%;background:var(--err);}
.dot.on{background:var(--ok);}
@keyframes blink{0%,100%{opacity:1}50%{opacity:.25}}
.dot.rec{background:var(--err);animation:blink .8s infinite;}

/* ===================== CARD ===================== */
.card{background:var(--s1);border:1px solid var(--bdr);border-radius:var(--rad);
  padding:18px;margin-bottom:12px;}
.card-ttl{font-size:.78em;font-weight:700;color:var(--txt2);margin-bottom:14px;
  text-transform:uppercase;letter-spacing:.8px;}

/* ===================== BUTTONS ===================== */
.btn{border:none;border-radius:var(--rad2);padding:10px 20px;
  font-family:inherit;font-weight:600;font-size:.85em;cursor:pointer;
  transition:background .15s,opacity .15s;display:inline-flex;align-items:center;gap:6px;outline:none;}
.btn:active{opacity:.8;}
.btn-p{background:var(--acc3);color:#fff;}
.btn-p:hover{background:var(--acc);}
.btn-s{background:var(--s2);color:var(--txt2);border:1px solid var(--bdr);}
.btn-s:hover{background:var(--s3);color:var(--txt);}
.btn-ok{background:var(--ok);color:#fff;}
.btn-ok:hover{opacity:.88;}
.btn-err{background:var(--err);color:#fff;}
.btn-err:hover{opacity:.88;}
.btn-warn{background:var(--warn);color:#000;}
.btn-blk{width:100%;justify-content:center;}
.btn-lg{padding:13px 28px;font-size:.95em;border-radius:var(--rad);}
.btn-sm{padding:6px 12px;font-size:.76em;}
.btn:disabled{opacity:.3;cursor:not-allowed;}
.row{display:flex;gap:8px;margin-top:10px;flex-wrap:wrap;}
.debug-row{border-top:1px solid var(--bdr);padding-top:12px;margin-top:18px!important;justify-content:center;opacity:0.5;}

/* ===================== PROGRESS ===================== */
.prog-bar{height:3px;background:var(--s3);border-radius:2px;overflow:hidden;margin:8px 0;}
.prog-fill{height:100%;background:var(--acc);border-radius:2px;transition:width .5s ease;}
.prog-lbl{font-size:.75em;color:var(--txt3);margin-bottom:4px;}

/* ===================== TIMER ===================== */
.timer{font-family:'SF Mono','Fira Code','Courier New',monospace;font-size:1.25em;font-weight:700;
  color:var(--warn);text-align:center;padding:6px 10px;
  background:var(--s2);border:1px solid var(--bdr);
  border-radius:var(--rad2);margin:8px 0;letter-spacing:4px;}

/* ===================== QUESTION BOX ===================== */
.qbox{background:var(--s2);border:1px solid var(--bdr2);border-radius:var(--rad);
  padding:16px;margin:10px 0;}
.qno{font-size:.72em;color:var(--txt3);font-weight:600;text-transform:uppercase;
  letter-spacing:.8px;margin-bottom:6px;}
.qrow{display:flex;align-items:flex-start;gap:8px;flex-wrap:wrap;}
.qtxt{font-size:1.05em;font-weight:400;line-height:1.65;flex:1;color:var(--txt);}
.badge{padding:3px 9px;border-radius:var(--rad3);font-size:.65em;font-weight:700;
  text-transform:uppercase;letter-spacing:.6px;white-space:nowrap;}
.badge.mcq{background:rgba(74,144,217,.15);color:var(--acc2);border:1px solid rgba(74,144,217,.3);}
.badge.numeric{background:rgba(212,160,23,.13);color:var(--warn);border:1px solid rgba(212,160,23,.25);}
.badge.voice{background:rgba(61,168,122,.13);color:var(--ok);border:1px solid rgba(61,168,122,.25);}

/* ===================== MCQ OPTIONS ===================== */
.opts{display:grid;grid-template-columns:1fr 1fr;gap:8px;margin-top:12px;}
.opt{background:var(--s1);border:1px solid var(--bdr);border-radius:var(--rad2);
  padding:12px 13px;font-size:.86em;text-align:left;transition:border-color .2s,background .2s;
  display:flex;align-items:flex-start;gap:10px;cursor:pointer;}
.opt:hover{background:var(--s2);border-color:var(--bdr2);}
.opt-lbl{font-weight:700;color:var(--txt3);font-size:.78em;min-width:16px;margin-top:1px;}
.opt.sel{border-color:var(--acc3);background:rgba(74,144,217,.08);}
.opt.sel .opt-lbl{color:var(--acc);}
.opt.ok{border-color:var(--ok);background:rgba(61,168,122,.09);}
.opt.wrong{border-color:var(--err);background:rgba(192,57,43,.09);}
.hint{font-size:.71em;color:var(--txt3);text-align:center;margin-top:8px;}

/* ===================== NUMERIC ===================== */
.num-display{background:var(--s2);border:1px solid var(--bdr2);border-radius:var(--rad);
  padding:18px;text-align:center;font-size:2em;font-weight:700;
  font-family:'SF Mono','Fira Code','Courier New',monospace;color:var(--txt);min-height:70px;
  margin-top:14px;letter-spacing:6px;transition:border-color .2s;}
.num-display.empty{font-size:.95em;color:var(--txt3);font-weight:400;letter-spacing:0;}
.num-display.has-val{border-color:var(--acc3);}

/* ===================== VOICE ===================== */
.v-area{text-align:center;padding:20px 16px 12px;}
.v-ring{width:80px;height:80px;border-radius:50%;margin:0 auto 14px;
  display:flex;align-items:center;justify-content:center;
  font-size:1.6em;transition:all .3s;position:relative;}
.v-ring.idle{background:var(--s2);border:2px solid var(--bdr2);color:var(--txt3);}
.v-ring.rec{background:rgba(192,57,43,.12);border:2px solid var(--err);color:var(--err);}
.v-ring.rec::after{content:'';position:absolute;inset:-7px;border-radius:50%;
  border:1px solid var(--err);animation:ripple 1.4s infinite;}
.v-ring.done{background:rgba(61,168,122,.12);border:2px solid var(--ok);color:var(--ok);}
@keyframes ripple{from{transform:scale(1);opacity:.6}to{transform:scale(1.4);opacity:0}}
.v-status{font-size:.88em;color:var(--txt2);line-height:1.5;}
.v-stage{display:flex;justify-content:center;gap:6px;margin-top:12px;flex-wrap:wrap;}
.vstep{padding:3px 11px;border-radius:4px;font-size:.71em;font-weight:600;
  background:var(--s2);border:1px solid var(--bdr);color:var(--txt3);}
.vstep.active{background:rgba(74,144,217,.1);border-color:var(--acc3);color:var(--acc2);}
.vstep.done2{background:rgba(61,168,122,.1);border-color:var(--ok);color:var(--ok);}

/* ===================== LOAD JSON ===================== */
.json-area{width:100%;min-height:160px;background:var(--bg);color:var(--txt);
  border:1px solid var(--bdr);border-radius:var(--rad2);padding:11px;
  font-family:'SF Mono','Fira Code','Courier New',monospace;font-size:.77em;resize:vertical;
  outline:none;transition:border-color .2s;}
.json-area:focus{border-color:var(--acc3);}
.err-msg{color:var(--err);font-size:.78em;margin-top:6px;display:none;}

/* ===================== RESULTS ===================== */
.score-hdr{text-align:center;padding:18px 10px;}
.score-big{font-size:3em;font-weight:800;color:var(--ok);}
.score-lbl{color:var(--txt3);font-size:.8em;margin-top:4px;}

.res-card{border:1px solid var(--bdr);border-radius:var(--rad);padding:18px;
  margin-bottom:12px;background:var(--s1);box-shadow: 0 4px 12px rgba(0,0,0,0.1);}
.res-head{display:flex;justify-content:space-between;align-items:center;
  margin-bottom:12px;gap:6px;}
.q-header{display:flex;align-items:center;gap:8px;}
.q-idx{font-size:0.75em;font-weight:700;color:var(--txt3);text-transform:uppercase;letter-spacing:1px;}
.time-taken{font-size:0.75em;color:var(--warn);background:rgba(212,160,23,0.1);padding:2px 8px;border-radius:4px;font-family:monospace;}

.res-qtxt{font-size:1em;color:var(--txt);margin-bottom:12px;line-height:1.5;font-weight:500;}

.res-ans-row{display:flex;gap:8px;font-size:0.85em;margin-top:4px;align-items:baseline;}
.res-ans-lbl{color:var(--txt3);min-width:60px;}
.res-ans-val{color:var(--txt);font-weight:600;}

/* ===================== AUDIO PLAYER ===================== */
.wa-audio{background:var(--s2);border:1px solid var(--bdr2);border-radius:12px;
  padding:12px 16px;display:flex;align-items:center;gap:12px;margin:12px 0;
  transition: transform 0.2s ease, border-color 0.2s;}
.wa-audio:hover{border-color:var(--acc3);transform: translateY(-1px);}

.wa-play{width:38px;height:38px;border-radius:50%;background:var(--acc3);color:white;
  display:flex;align-items:center;justify-content:center;cursor:pointer;font-size:1.1em;flex-shrink:0;
  box-shadow: 0 2px 6px rgba(46,111,181,0.3);}
.wa-play:hover{background:var(--acc);transform: scale(1.05);}
.wa-info{flex:1;text-align:left;overflow:hidden;}
.wa-title{font-size:.82em;font-weight:600;color:var(--txt);margin-bottom:4px;display:flex;justify-content:space-between;}
.wa-prog{height:4px;background:var(--s3);border-radius:2px;position:relative;margin:6px 0;}
.wa-fill{height:100%;background:var(--acc);border-radius:2px;width:0%;transition:width .1s linear;}
.wa-dur{font-size:.7em;color:var(--txt3);margin-top:2px;letter-spacing:0.3px;}

/* ===================== ANIM ===================== */
.fade{animation:fa .25s ease;}
@keyframes fa{from{opacity:0;transform:translateY(6px)}to{opacity:1;transform:translateY(0)}}
.hidden{display:none!important;}

/* ===================== RESPONSIVE ===================== */
@media(max-width:480px){
  .opts{grid-template-columns:1fr;}
  .row{flex-direction:column;}
  .row .btn{width:100%;justify-content:center;}
  .hdr h1{font-size:1.1em;}
}
)rawliteral";
