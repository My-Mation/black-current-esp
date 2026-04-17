#pragma once
#include <pgmspace.h>
// =============================================================
//  web_css.h — CSS styles for the web UI
// =============================================================

const char WEB_CSS[] PROGMEM = R"rawliteral(
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

/* ===================== WHATSAPP AUDIO STYLE ===================== */
.wa-audio{background:var(--s2);border:1px solid var(--bdr2);border-radius:24px;
  padding:10px 15px;display:flex;align-items:center;gap:12px;margin:12px auto;
  box-shadow:0 3px 10px rgba(0,0,0,0.15);max-width:320px;transition:all .3s;}
.wa-play{width:36px;height:36px;border-radius:50%;background:var(--acc);color:white;
  display:flex;align-items:center;justify-content:center;cursor:pointer;font-size:1.1em;flex-shrink:0;}
.wa-play:hover{background:var(--acc2);transform:scale(1.05);}
.wa-info{flex:1;text-align:left;overflow:hidden;}
.wa-title{font-size:.82em;font-weight:600;color:var(--txt);margin-bottom:2px;display:flex;justify-content:space-between;}
.wa-prog{height:3px;background:var(--s3);border-radius:2px;position:relative;margin:4px 0;}
.wa-fill{height:100%;background:var(--acc2);border-radius:2px;width:0%;transition:width .1s linear;}
.wa-dur{font-size:.7em;color:var(--txt3);margin-top:2px;}

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
)rawliteral";
