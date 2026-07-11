#include "web_interface.h"

#include <WebServer.h>
#include <WiFi.h>
#include <Update.h>
#include "config.h"
#include "app_state.h"
#include "settings.h"
#include "matrix_display.h"
#include "wifi_manager.h"
#include "ota_update.h"

static WebServer server(80);

static String htmlEscape(const String &input) {
  String output = input;
  output.replace("&", "&amp;");
  output.replace("\"", "&quot;");
  output.replace("'", "&#39;");
  output.replace("<", "&lt;");
  output.replace(">", "&gt;");
  return output;
}

static String jsEscape(const String &input) {
  String output = input;
  output.replace("\\", "\\\\");
  output.replace("\"", "\\\"");
  output.replace("\r", "");
  output.replace("\n", "\\n");
  output.replace("</", "<\\/");
  return output;
}

static String urlEncode(const String &input) {
  const char *hex = "0123456789ABCDEF";
  String output;

  for (size_t i = 0; i < input.length(); i++) {
    uint8_t c = (uint8_t)input[i];

    if ((c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') ||
        c == '-' || c == '_' || c == '.' || c == '~') {
      output += (char)c;
    } else {
      output += '%';
      output += hex[(c >> 4) & 0x0F];
      output += hex[c & 0x0F];
    }
  }

  return output;
}

static String getWifiSecurityName(wifi_auth_mode_t type) {
  switch (type) {
    case WIFI_AUTH_OPEN: return "Offen";
    case WIFI_AUTH_WEP: return "WEP";
    case WIFI_AUTH_WPA_PSK: return "WPA";
    case WIFI_AUTH_WPA2_PSK: return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2 Enterprise";
    default: return "Gesch&uuml;tzt";
  }
}

static String getWifiSignalName(int32_t rssi) {
  if (rssi >= -55) return "Sehr gut";
  if (rssi >= -67) return "Gut";
  if (rssi >= -75) return "OK";
  return "Schwach";
}

static String L(const char *deText, const char *enText) {
  return isGermanUi() ? String(deText) : String(enText);
}

static String activeLangClass(const char *lang) {
  return uiLanguage.equalsIgnoreCase(lang) ? " active" : "";
}

static String getAutoStatusText() {
  return autoModeDemo ? L("AKTIV", "ON") : L("AUS", "OFF");
}

static String getWifiSecurityNameLocalized(wifi_auth_mode_t type) {
  if (!isGermanUi()) {
    switch (type) {
      case WIFI_AUTH_OPEN: return "Open";
      case WIFI_AUTH_WEP: return "WEP";
      case WIFI_AUTH_WPA_PSK: return "WPA";
      case WIFI_AUTH_WPA2_PSK: return "WPA2";
      case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
      case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2 Enterprise";
      default: return "Protected";
    }
  }

  return getWifiSecurityName(type);
}

static String getWifiSignalNameLocalized(int32_t rssi) {
  if (isGermanUi()) {
    return getWifiSignalName(rssi);
  }

  if (rssi >= -55) return "Excellent";
  if (rssi >= -67) return "Good";
  if (rssi >= -75) return "OK";
  return "Weak";
}

static String htmlButton(const String &label, const String &url) {
  return "<a class='btn' href='" + url + "'>" + label + "</a>";
}

static bool isAjaxRequest() {
  return server.hasArg("_ajax");
}

static void redirectHome() {
  if (isAjaxRequest()) {
    server.sendHeader("Cache-Control", "no-store");
    server.send(204, "text/plain", "");
    return;
  }

  server.sendHeader("Location", "/");
  server.send(303);
}

static String htmlPage() {
  String autoStatus = getAutoStatusText();
  String wifiSsidValue = homeWifiSsid;
  bool wifiSsidFromScan = false;

  if (server.hasArg("ssid")) {
    wifiSsidValue = server.arg("ssid");
    wifiSsidValue.trim();
    wifiSsidFromScan = wifiSsidValue.length() > 0;
  }

  String page;
  page += "<!DOCTYPE html><html lang='" + uiLanguage + "'><head>";
  page += "<meta charset='UTF-8'>";
  page += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  page += "<meta name='theme-color' content='#050812'>";
  page += "<title>SmartFix Matrix</title>";
  page += R"rawliteral(
<style>
:root{--bg:#050812;--panel:#0d1320;--panel2:#111827;--line:#223149;--text:#e5e7eb;--muted:#94a3b8;--green:#22c55e;--green2:#16a34a;--blue:#38bdf8;--blue2:#2563eb;--danger:#ef4444;}
*{box-sizing:border-box}html{scroll-behavior:smooth}body{margin:0;font-family:Arial,Helvetica,sans-serif;background:radial-gradient(circle at 18% 0%,rgba(34,197,94,.20),transparent 34%),radial-gradient(circle at 88% 10%,rgba(56,189,248,.16),transparent 32%),linear-gradient(180deg,#050812 0%,#08111f 55%,#050812 100%);color:var(--text);min-height:100vh;}
a{color:inherit}.wrap{max-width:940px;margin:0 auto;padding:22px}.hero{position:relative;overflow:hidden;border:1px solid rgba(148,163,184,.22);border-radius:26px;background:linear-gradient(145deg,rgba(17,24,39,.93),rgba(2,6,23,.92));box-shadow:0 22px 70px rgba(0,0,0,.45);padding:22px;margin-bottom:16px}.hero:before{content:'';position:absolute;inset:0;background:linear-gradient(90deg,rgba(34,197,94,.10),transparent 40%,rgba(56,189,248,.10));pointer-events:none}.topbar{position:relative;display:flex;align-items:center;gap:15px;margin-bottom:18px;padding-right:120px}.logo-badge{width:52px;height:52px;border-radius:16px;display:grid;place-items:center;font-weight:900;font-size:20px;color:white;background:linear-gradient(135deg,var(--green),var(--blue));box-shadow:0 0 32px rgba(34,197,94,.28)}.title-block h1{margin:0;font-size:31px;letter-spacing:-.8px}.title-block .sub{margin:5px 0 0;color:var(--muted)}.version-chip{margin-left:auto;background:rgba(34,197,94,.12);border:1px solid rgba(34,197,94,.36);color:#bbf7d0;padding:8px 12px;border-radius:999px;font-size:13px;font-weight:bold;white-space:nowrap}.lang-switch{position:absolute;top:18px;right:18px;display:flex;gap:6px;background:rgba(2,6,23,.54);border:1px solid rgba(148,163,184,.18);border-radius:999px;padding:5px;z-index:2}.lang-btn{display:block;text-decoration:none;color:var(--muted);font-weight:900;font-size:12px;border-radius:999px;padding:7px 10px}.lang-btn.active{background:linear-gradient(135deg,var(--green2),var(--blue));color:white;box-shadow:0 0 18px rgba(56,189,248,.18)}.status{position:relative;display:grid;grid-template-columns:repeat(3,1fr);gap:10px}.pill{background:rgba(2,6,23,.72);border:1px solid rgba(148,163,184,.18);border-radius:16px;padding:12px;box-shadow:inset 0 1px 0 rgba(255,255,255,.03)}.pill.good{border-color:rgba(34,197,94,.32)}.pill.blue{border-color:rgba(56,189,248,.28)}.label{font-size:11px;letter-spacing:.5px;text-transform:uppercase;color:var(--muted)}.value{font-size:16px;font-weight:bold;color:#f8fafc;margin-top:5px;word-break:break-word}.card{background:linear-gradient(145deg,rgba(17,24,39,.92),rgba(12,18,30,.92));border:1px solid rgba(148,163,184,.18);border-radius:22px;margin-bottom:12px;box-shadow:0 12px 36px rgba(0,0,0,.28)}details.card{overflow:hidden}summary{cursor:pointer;list-style:none;padding:18px 20px;display:flex;align-items:center;gap:12px;user-select:none}summary::-webkit-details-marker{display:none}.section-icon{width:34px;height:34px;border-radius:12px;display:grid;place-items:center;background:rgba(56,189,248,.12);border:1px solid rgba(56,189,248,.25);color:#7dd3fc;font-size:18px}.summary-text{font-size:18px;font-weight:800;color:#f8fafc}.sumvalue{margin-left:auto;color:var(--muted);font-size:13px;font-weight:bold;text-align:right}.chev{margin-left:8px;color:var(--green);font-size:22px;line-height:1;transition:.18s transform}details[open] .chev{transform:rotate(45deg)}.detail-body{padding:0 20px 20px;border-top:1px solid rgba(148,163,184,.11)}h2{margin:18px 0 12px;font-size:15px;color:#7dd3fc;text-transform:uppercase;letter-spacing:.5px}.sub{color:var(--muted);font-size:14px;line-height:1.45}.hint{margin-top:12px;padding:12px;border:1px solid rgba(34,197,94,.24);background:rgba(34,197,94,.08);border-radius:14px;color:#bbf7d0}.buttons{display:grid;grid-template-columns:repeat(2,1fr);gap:10px}.btn{display:flex;align-items:center;justify-content:center;min-height:46px;text-align:center;text-decoration:none;border:1px solid rgba(148,163,184,.18);background:linear-gradient(135deg,rgba(37,99,235,.95),rgba(14,165,233,.82));color:white;padding:12px;border-radius:14px;font-weight:800;box-shadow:0 10px 22px rgba(37,99,235,.17);transition:.16s transform,.16s filter,.16s border-color}.btn:hover{filter:brightness(1.12);transform:translateY(-1px);border-color:rgba(125,211,252,.45)}.btn.green{background:linear-gradient(135deg,var(--green2),var(--green));box-shadow:0 10px 22px rgba(34,197,94,.17)}.btn.danger{background:linear-gradient(135deg,#b91c1c,var(--danger))}button.btn{border:0;width:100%;cursor:pointer;font-size:15px}input{width:100%;background:rgba(2,6,23,.78);color:var(--text);border:1px solid rgba(148,163,184,.22);border-radius:15px;padding:14px 15px;font-size:16px;margin-bottom:12px;outline:none}input[type=file]{cursor:pointer}input:focus{border-color:rgba(56,189,248,.65);box-shadow:0 0 0 3px rgba(56,189,248,.12)}.small{font-size:13px;color:var(--muted);margin:18px 0 4px;text-align:center}.row-title{display:flex;align-items:center;justify-content:space-between;margin:18px 0 10px}.mini-chip{font-size:12px;color:#bbf7d0;background:rgba(34,197,94,.10);border:1px solid rgba(34,197,94,.25);border-radius:999px;padding:5px 9px}.slider-card{background:rgba(2,6,23,.46);border:1px solid rgba(148,163,184,.16);border-radius:18px;padding:15px;margin-top:6px}.slider-head{display:flex;align-items:center;justify-content:space-between;gap:12px;margin-bottom:12px}.slider-head span{font-size:14px;color:var(--muted);font-weight:800}.slider-head strong{font-size:18px;color:#f8fafc}.range-slider{width:100%;appearance:none;-webkit-appearance:none;height:12px;border-radius:999px;padding:0;margin:4px 0 10px;background:linear-gradient(90deg,var(--green2),var(--blue));border:1px solid rgba(148,163,184,.24);outline:none}.range-slider::-webkit-slider-thumb{-webkit-appearance:none;appearance:none;width:28px;height:28px;border-radius:50%;background:#f8fafc;border:3px solid var(--blue);box-shadow:0 0 22px rgba(56,189,248,.55);cursor:pointer}.range-slider::-moz-range-thumb{width:24px;height:24px;border-radius:50%;background:#f8fafc;border:3px solid var(--blue);box-shadow:0 0 22px rgba(56,189,248,.55);cursor:pointer}.slider-scale{display:flex;justify-content:space-between;color:var(--muted);font-size:12px;font-weight:800;margin-top:2px}.preview-shell{display:flex;justify-content:center;align-items:center;background:radial-gradient(circle at 50% 40%,#07111f,#020617 70%);border:1px solid rgba(56,189,248,.25);border-radius:18px;padding:14px;box-shadow:inset 0 0 34px rgba(56,189,248,.10),0 14px 34px rgba(0,0,0,.28)}#matrixPreview{width:100%;max-width:640px;height:auto;border-radius:14px;background:#000;box-shadow:0 0 28px rgba(34,197,94,.16),inset 0 0 18px rgba(255,255,255,.04)}#section-preview .chev{display:none}#section-preview summary{cursor:default}.preview-row{display:flex;gap:10px;align-items:center;justify-content:space-between;flex-wrap:wrap;margin-top:12px}.preview-dot{width:10px;height:10px;border-radius:50%;display:inline-block;background:var(--green);box-shadow:0 0 12px var(--green);margin-right:7px}.sf-toast{position:fixed;left:50%;bottom:18px;transform:translateX(-50%) translateY(18px);opacity:0;z-index:99;background:rgba(2,6,23,.94);border:1px solid rgba(56,189,248,.34);box-shadow:0 18px 48px rgba(0,0,0,.45);color:#e5e7eb;border-radius:999px;padding:11px 16px;font-size:13px;font-weight:800;pointer-events:none;transition:.18s opacity,.18s transform}.sf-toast.show{opacity:1;transform:translateX(-50%) translateY(0)}@media(max-width:760px){.wrap{padding:14px}.hero{padding:18px;border-radius:22px}.topbar{align-items:flex-start;padding-top:34px;padding-right:0}.version-chip{display:none}.status{grid-template-columns:1fr 1fr}.buttons{grid-template-columns:1fr}.sumvalue{display:none}.title-block h1{font-size:27px}}@media(max-width:460px){.status{grid-template-columns:1fr}.logo-badge{width:46px;height:46px}.summary-text{font-size:16px}}
</style>
<script>
(function(){
  var key='sf_open_sections_v2';
  function ids(){
    var a=[];
    document.querySelectorAll('details.config-section').forEach(function(d){
      if(d.id==='section-preview'){return;}
      if(d.open&&d.id)a.push(d.id);
    });
    return a;
  }
  function save(){
    try{localStorage.setItem(key,JSON.stringify(ids()));sessionStorage.setItem('sf_scroll',String(window.scrollY||0));}catch(e){}
  }
  function forcePreviewOpen(){
    var p=document.getElementById('section-preview');
    if(!p)return;
    p.open=true;
    if(!p.dataset.locked){
      p.dataset.locked='1';
      p.addEventListener('toggle',function(){if(!p.open){setTimeout(function(){p.open=true;},0);}});
    }
  }
  function openByHash(){
    var h=location.hash?location.hash.substring(1):'';
    if(!h)return false;
    var el=document.getElementById(h);
    if(el&&el.tagName&&el.tagName.toLowerCase()==='details'){
      el.open=true;
      setTimeout(function(){el.scrollIntoView({block:'start'});},80);
      return true;
    }
    return false;
  }
  window.addEventListener('load',function(){
    var restored=false;
    try{
      var raw=localStorage.getItem(key);
      if(raw){
        var arr=JSON.parse(raw);
        document.querySelectorAll('details.config-section').forEach(function(d){
          if(d.id==='section-preview'){d.open=true;return;}
          d.open=arr.indexOf(d.id)>=0;
        });
        restored=true;
      }
    }catch(e){}
    if(!restored){var d=document.getElementById('section-mode');if(d)d.open=true;}
    forcePreviewOpen();
    var hashOpened=openByHash();
    forcePreviewOpen();
    document.querySelectorAll('details.config-section').forEach(function(d){d.addEventListener('toggle',save);});
    document.querySelectorAll('a.btn,button.btn,form,.lang-btn').forEach(function(el){el.addEventListener(el.tagName.toLowerCase()==='form'?'submit':'click',save);});
    if(!hashOpened){
      var y=sessionStorage.getItem('sf_scroll');
      if(y!==null){setTimeout(function(){window.scrollTo(0,parseInt(y)||0);sessionStorage.removeItem('sf_scroll');},40);}
    }
  });
  window.addEventListener('beforeunload',save);
})();

(function(){
  function fastAction(href){return /^(\/mode|\/brightness|\/speed|\/logo-speed|\/text-color|\/scroll-effect|\/logo-effect|\/logo-color|\/panels|\/auto)(\?|$)/.test(href||'');}
  function withAjax(href){return href+(href.indexOf('?')>=0?'&':'?')+'_ajax=1&_t='+(Date.now());}
  function toast(msg){var t=document.getElementById('sfToast');if(!t){return;}t.textContent=msg;t.classList.add('show');clearTimeout(t._timer);t._timer=setTimeout(function(){t.classList.remove('show');},1250);}
  function updatePreviewFromUrl(href){
    try{var u=new URL(href,location.origin),p=window.sfPreview||{};
      if(u.pathname==='/brightness'){
        p.brightness=u.searchParams.get('v')||p.brightness;
        var bv=document.getElementById('brightnessStatusValue');
        if(bv){var v=parseInt(p.brightness,10)||0;bv.textContent=v+' / 255';}
      }
      else if(u.pathname==='/speed')p.speed=u.searchParams.get('v')||p.speed;
      else if(u.pathname==='/logo-speed')p.logoSpeed=u.searchParams.get('v')||p.logoSpeed;
      else if(u.pathname==='/text-color')p.scrollColor=u.searchParams.get('c')||p.scrollColor;
      else if(u.pathname==='/scroll-effect')p.scrollEffect=u.searchParams.get('e')||p.scrollEffect;
      else if(u.pathname==='/logo-effect')p.logoEffect=u.searchParams.get('e')||p.logoEffect;
      else if(u.pathname==='/logo-color')p.logoColor=u.searchParams.get('c')||p.logoColor;
      else if(u.pathname==='/panels')p.panelCount=u.searchParams.get('n')||p.panelCount;
      window.sfPreview=p;
    }catch(e){}
  }
  function ajaxGet(href){
    window.sfPreviewPaused=true;toast('Speichere...');
    fetch(withAjax(href),{cache:'no-store'}).then(function(){updatePreviewFromUrl(href);toast('Gespeichert');}).catch(function(){toast('Fehler beim Speichern');}).finally(function(){setTimeout(function(){window.sfPreviewPaused=false;},80);});
  }
  function setBrightnessUi(value){
    var v=Math.max(5,Math.min(255,parseInt(value,10)||70));
    var label=document.getElementById('brightnessValue');
    var status=document.getElementById('brightnessStatusValue');
    if(label){label.textContent=v+' / 255 ('+Math.round(v*100/255)+'%)';}
    if(status){status.textContent=v+' / 255';}
    var p=window.sfPreview||{};p.brightness=v;window.sfPreview=p;
  }
  function setupBrightnessSlider(){
    var slider=document.getElementById('brightnessSlider');
    if(!slider)return;
    var timer=null,lastSaved=String(slider.value||'');
    setBrightnessUi(slider.value);
    function saveNow(){
      var v=String(slider.value||'70');
      if(v===lastSaved)return;
      lastSaved=v;
      ajaxGet('/brightness?v='+encodeURIComponent(v));
    }
    slider.addEventListener('input',function(){setBrightnessUi(slider.value);clearTimeout(timer);timer=setTimeout(saveNow,220);});
    slider.addEventListener('change',function(){setBrightnessUi(slider.value);clearTimeout(timer);saveNow();});
  }
  window.addEventListener('load',function(){
    document.body.insertAdjacentHTML('beforeend','<div id="sfToast" class="sf-toast"></div>');
    setupBrightnessSlider();
    document.addEventListener('click',function(e){
      var a=e.target.closest&&e.target.closest('a.btn');if(!a)return;var href=a.getAttribute('href')||'';
      if(!fastAction(href))return;
      e.preventDefault();ajaxGet(href);
    });
    document.addEventListener('submit',function(e){
      var f=e.target;if(!f||!f.getAttribute)return;var action=f.getAttribute('action')||'';
      if(action!=='/set-text'&&action!=='/set-logo-text')return;
      e.preventDefault();
      var params=new URLSearchParams(new FormData(f));
      ajaxGet(action+'?'+params.toString());
    });
  });
})();

(function(){
  var W=64,H=32,S=10,FPS=22,FRAME_MS=1000/FPS,lastFrame=0;
  var colors=['#f8fafc','#22c55e','#38bdf8','#facc15','#ef4444'];
  var wordColors=['#22c55e','#38bdf8','#facc15','#ef4444','#f8fafc'];
  var off=document.createElement('canvas'); off.width=W; off.height=H;
  var oc=off.getContext('2d'); oc.imageSmoothingEnabled=false;
  var baseCanvas=null,baseReady=false,spriteCache={};

  // 5x7 pixel font, drawn directly to the 64x32 pixel buffer.
  // This is much closer to the ESP32/Adafruit-GFX style than browser text rendering.
  var F={
    ' ':[0,0,0,0,0],'!':[0,0,95,0,0],'.':[0,96,96,0,0],',':[0,80,48,0,0],':':[0,54,54,0,0],';':[0,86,54,0,0],
    '-' :[8,8,8,8,8],'_':[64,64,64,64,64],'+':[8,8,62,8,8],'/' :[32,16,8,4,2],
    '(':[0,28,34,65,0],')':[0,65,34,28,0],'?':[2,1,81,9,6],'&':[54,73,85,34,80],
    '0':[62,81,73,69,62],'1':[0,66,127,64,0],'2':[66,97,81,73,70],'3':[33,65,69,75,49],'4':[24,20,18,127,16],
    '5':[39,69,69,69,57],'6':[60,74,73,73,48],'7':[1,113,9,5,3],'8':[54,73,73,73,54],'9':[6,73,73,41,30],
    'A':[126,17,17,17,126],'B':[127,73,73,73,54],'C':[62,65,65,65,34],'D':[127,65,65,34,28],'E':[127,73,73,73,65],
    'F':[127,9,9,9,1],'G':[62,65,73,73,122],'H':[127,8,8,8,127],'I':[0,65,127,65,0],'J':[32,64,65,63,1],
    'K':[127,8,20,34,65],'L':[127,64,64,64,64],'M':[127,2,12,2,127],'N':[127,4,8,16,127],'O':[62,65,65,65,62],
    'P':[127,9,9,9,6],'Q':[62,65,81,33,94],'R':[127,9,25,41,70],'S':[70,73,73,73,49],'T':[1,1,127,1,1],
    'U':[63,64,64,64,63],'V':[31,32,64,32,31],'W':[63,64,56,64,63],'X':[99,20,8,20,99],'Y':[7,8,112,8,7],'Z':[97,81,73,69,67],
    'a':[32,84,84,84,120],'b':[127,72,68,68,56],'c':[56,68,68,68,32],'d':[56,68,68,72,127],'e':[56,84,84,84,24],
    'f':[8,126,9,1,2],'g':[12,82,82,82,62],'h':[127,8,4,4,120],'i':[0,68,125,64,0],'j':[32,64,68,61,0],
    'k':[127,16,40,68,0],'l':[0,65,127,64,0],'m':[124,4,24,4,120],'n':[124,8,4,4,120],'o':[56,68,68,68,56],
    'p':[124,20,20,20,8],'q':[8,20,20,24,124],'r':[124,8,4,4,8],'s':[72,84,84,84,32],'t':[4,63,68,64,32],
    'u':[60,64,64,32,124],'v':[28,32,64,32,28],'w':[60,64,48,64,60],'x':[68,40,16,40,68],'y':[12,80,80,80,60],'z':[68,100,84,76,68]
  };
  function state(){return window.sfPreview||{};}
  function syncLayout(cv){
    var p=state(), panels=parseInt(p.panelCount||1);
    if(!isFinite(panels)||panels<1)panels=1;
    if(panels>2)panels=2;
    var nextW=64*panels, nextS=panels>1?5:10;
    if(nextW!==W||nextS!==S){
      W=nextW; S=nextS; off.width=W; off.height=H; oc=off.getContext('2d'); oc.imageSmoothingEnabled=false; baseReady=false; spriteCache={};
    }
    var cw=W*S, ch=H*S;
    if(cv&&(cv.width!==cw||cv.height!==ch)){cv.width=cw;cv.height=ch;baseReady=false;}
  }
  function glyphs(t){return Array.from(t||'');}
  function prefix(t,n){return glyphs(t).slice(0,Math.max(0,n)).join('');}
  function isSmartFixBrand(t){return String(t||'').toLowerCase().replace(/[\s\-_]/g,'')==='smartfix';}
  function clamp(v,a,b){return Math.max(a,Math.min(b,v));}
  function hexToRgb(h){h=h.replace('#','');return {r:parseInt(h.substr(0,2),16),g:parseInt(h.substr(2,2),16),b:parseInt(h.substr(4,2),16)};}
  function rgba(hex,scale){var c=hexToRgb(hex),s=clamp(scale==null?255:scale,0,255)/255;return 'rgb('+Math.round(c.r*s)+','+Math.round(c.g*s)+','+Math.round(c.b*s)+')';}
  function rgbObj(hex,scale){var c=hexToRgb(hex),s=clamp(scale==null?255:scale,0,255)/255;return {r:Math.round(c.r*s),g:Math.round(c.g*s),b:Math.round(c.b*s)};}
  function colorByIndex(i){return colors[i]||colors[0];}
  function colToObj(color){if(color.charAt(0)==='#')return rgbObj(color,255);var m=color.match(/\d+/g)||[0,0,0];return {r:+m[0],g:+m[1],b:+m[2]};}
  function logoMain(part,scale){
    var p=state(), m=parseInt(p.logoColor||0);
    if(m===1)return rgba(colors[1],scale); if(m===2)return rgba(colors[2],scale); if(m===3)return rgba(colors[0],scale);
    if(m===4)return rgba(colors[3],scale); if(m===5)return rgba(colors[4],scale);
    if(m===7)return rgba(wordColors[part%wordColors.length],scale);
    return rgba(part%2?colors[2]:colors[1],scale);
  }
  function brandMain(part,scale){return logoMain(part,scale);}
  function logoShadow(part,scale){return part%2?rgba('#06162f',scale):rgba('#062412',scale);}
  function logoHighlight(part,scale){return part%2?rgba('#a7e8ff',scale):rgba('#bbffd0',scale);}
  function px(ctx,x,y,color){x=Math.round(x);y=Math.round(y);if(x<0||x>=W||y<0||y>=H)return;ctx.fillStyle=color;ctx.fillRect(x,y,1,1);}
  function baseChar(ch){var m={'ä':'a','ö':'o','ü':'u','Ä':'A','Ö':'O','Ü':'U'};return m[ch]||ch;}
  function drawGlyph(ctx,ch,x,y,color){
    if(ch==='ß'){drawGlyph(ctx,'s',x,y,color);drawGlyph(ctx,'s',x+6,y,color);return 12;}
    var uml='äöüÄÖÜ'.indexOf(ch)>=0, b=baseChar(ch), g=F[b]||F['?']||F[' '];
    for(var cx=0;cx<5;cx++){var bits=g[cx]||0;for(var cy=0;cy<7;cy++){if(bits&(1<<cy))px(ctx,x+cx,y+cy,color);}}
    if(uml){var dy=y-2;if(dy<0)dy=y;px(ctx,x+1,dy,color);px(ctx,x+3,dy,color);}
    return 6;
  }
  function txt(ctx,t,x,y,color){var cx=Math.round(x), chars=glyphs(t);for(var i=0;i<chars.length;i++){cx+=drawGlyph(ctx,chars[i],cx,Math.round(y),color);}}
  function tw(t){return glyphs(t).reduce(function(w,ch){return w+(ch==='ß'?12:6);},0);}
  function lineV(ctx,x,y,h,color){for(var i=0;i<h;i++)px(ctx,x,y+i,color);}
  function brand(ctx,x,y,reveal,scale,shimmer){
    var smart=prefix('Smart',Math.min(reveal,5)), fix=prefix('Fix',Math.max(0,reveal-5)), fx=x+32;
    txt(ctx,smart,x+1,y+1,logoShadow(0,scale)); txt(ctx,fix,fx+1,y+1,logoShadow(1,scale));
    txt(ctx,smart,x,y,brandMain(0,scale)); txt(ctx,fix,fx,y,brandMain(1,scale));
    if(reveal>=8&&scale>90){px(ctx,x+2,y,logoHighlight(0,scale));px(ctx,x+3,y,logoHighlight(0,scale));px(ctx,fx+1,y,logoHighlight(1,scale));px(ctx,fx+2,y,logoHighlight(1,scale));}
    if(shimmer>=0&&reveal>=8){if(shimmer<5)txt(ctx,'Smart'[shimmer],x+shimmer*6,y,logoHighlight(0,scale));else txt(ctx,'Fix'[shimmer-5],fx+(shimmer-5)*6,y,logoHighlight(1,scale));}
  }
  function genericLogo(ctx,t,x,y,reveal,scale,shimmer){
    var visible=prefix(t,reveal), chars=glyphs(visible), cx=x, word=0;
    for(var i=0;i<chars.length;i++){var ch=chars[i];if(ch===' '||ch==='-'||ch==='_'){txt(ctx,ch,cx,y,logoMain(word,scale));cx+=6;if(ch!==' ')word++;continue;}txt(ctx,ch,cx+1,y+1,logoShadow(word,scale));txt(ctx,ch,cx,y,logoMain(word,scale));if(shimmer===i)txt(ctx,ch,cx,y,rgba(colors[0],scale));cx+=(ch==='ß'?12:6);}
  }
  function triangle(phase,amp){phase=phase%24;if(phase>12)phase=24-phase;return Math.round((phase*amp*2/12)-amp);}
  function logoStep(mult){var p=state(),s=parseInt(p.logoSpeed||35)*(mult||1);return clamp(s,8,240);}
  function waveLogo(ctx,t,x,y,brandMode,scale,bounce,now){
    var chars=glyphs(t),cx=x,global=bounce?triangle(Math.floor(now/logoStep(2)),2):0;
    for(var i=0;i<chars.length;i++){var ch=chars[i],part=brandMode?(i<5?0:1):0,yy=y+global;if(!bounce)yy=y+triangle(Math.floor(now/logoStep(2))+i*3,2);txt(ctx,ch,cx+1,yy+1,logoShadow(part,scale));txt(ctx,ch,cx,yy,brandMode?brandMain(part,scale):logoMain(part,scale));cx+=(ch==='ß'?12:6);if(brandMode&&i===4)cx+=2;}
  }
  function sparkles(ctx,x,y,now){var pts=[[0,0],[8,1],[15,-1],[25,0],[35,-1],[46,1],[52,0],[4,9],[12,10],[29,9],[39,10],[48,9],[57,10]],f=Math.floor(now/95)%16;pts.forEach(function(pt,i){if(((i+f)%5)===0)px(ctx,x+pt[0],y+pt[1],logoHighlight(i,180));});}
  function drawLogo(ctx,now){
    var p=state(),t=(p.logoText||'SmartFix').trim()||'SmartFix',brandMode=isSmartFixBrand(t),total=brandMode?8:glyphs(t).length;
    var baseX=brandMode?Math.max(0,Math.round((W-50)/2)):Math.max(0,Math.round((W-tw(t))/2)),baseY=3,reveal=total,scale=255,shimmer=-1,effect=parseInt(p.logoEffect||0);
    if(effect===1){var ph=Math.floor(now/logoStep(5))%(total+8);reveal=Math.min(ph,total);} 
    else if(effect===2){var ph2=Math.floor(now/logoStep())%512;if(ph2>255)ph2=511-ph2;scale=50+Math.round(ph2*205/255);} 
    else if(effect===3){var ph3=Math.floor(now/logoStep())%170,target=baseX;if(ph3<55)baseX=W-((W-target)*ph3/55);else if(ph3>125)baseX=target-((ph3-125)*(target+56)/45);} 
    else if(effect===11){var ph11=Math.floor(now/logoStep())%220,target11=baseX,w=brandMode?50:tw(t);if(ph11<55)baseX=W-((W-target11)*ph11/55);else if(ph11<110)baseX=target11;else if(ph11<165)baseX=target11+((ph11-110)*(W-target11+2)/55);else baseX=-w+(((target11+w)*(ph11-165))/55);} 
    else if(effect===4){var sh=Math.floor(now/logoStep(3))%(total+4);if(sh<total)shimmer=sh;} 
    else if(effect===6){var ph6=Math.floor(now/logoStep())%512;if(ph6>255)ph6=511-ph6;scale=150+Math.round(ph6*105/255);} 
    if(effect===7)waveLogo(ctx,brandMode?'SmartFix':t,baseX,baseY,brandMode,scale,false,now);
    else if(effect===8)waveLogo(ctx,brandMode?'SmartFix':t,baseX,baseY,brandMode,scale,true,now);
    else if(brandMode)brand(ctx,baseX,baseY,reveal,scale,shimmer);
    else genericLogo(ctx,t,baseX,baseY,reveal,scale,shimmer);
    if(effect===5||effect===6)sparkles(ctx,baseX,baseY,now);
    else if(effect===9){txt(ctx,brandMode?'SmartFix':t,baseX+((Math.floor(now/90)%3)-1),baseY,rgba(colors[4],170));txt(ctx,brandMode?'SmartFix':t,baseX-((Math.floor(now/110)%3)-1),baseY+1,rgba(colors[2],170));}
    else if(effect===10){var w2=brandMode?50:tw(t),phs=Math.floor(now/logoStep())%(w2+18),sx=baseX-8+phs;lineV(ctx,sx,baseY-1,10,rgba(colors[0],170));lineV(ctx,sx+1,baseY-1,10,rgba(colors[0],170));}
  }
  function scrollX(now,w,speed,dual){if(dual){var dist=W+w,pos=(now/speed)%(dist*2);return pos<dist?W-pos:-w+(pos-dist);}return W-((now/speed)%(w+W+8));}
  function drawScroll(ctx,now){
    var p=state(),t=p.scrollText||'SmartFix Matrix',effect=parseInt(p.scrollEffect||0),speed=Math.max(8,parseInt(p.speed||35)),x=scrollX(now,tw(t),speed,effect===6),y=20,color=colorByIndex(parseInt(p.scrollColor||0));
    if(effect===1){var chars=glyphs(t),cx=x;for(var i=0;i<chars.length;i++){txt(ctx,chars[i],cx,y,wordColors[(i+Math.floor(now/180))%wordColors.length]);cx+=(chars[i]==='ß'?12:6);}return;}
    if(effect===2){var cs=glyphs(t),cx2=x;for(var j=0;j<cs.length;j++){txt(ctx,cs[j],cx2,y+triangle(Math.floor(now/70)+j*3,2),color);cx2+=(cs[j]==='ß'?12:6);}return;}
    if(effect===3){txt(ctx,t,x,y,color);sparkles(ctx,0,15,now);return;}
    if(effect===4){txt(ctx,t,x+3,y,rgba(color,45));txt(ctx,t,x+2,y,rgba(color,75));txt(ctx,t,x+1,y,rgba(color,110));txt(ctx,t,x,y,color);return;}
    if(effect===5){txt(ctx,t,x,y,((Math.floor(now/350)%2)===0)?colors[0]:color);return;}
    txt(ctx,t,x,y,color);
  }
  function makeBase(){
    if(baseReady)return;
    baseCanvas=document.createElement('canvas');baseCanvas.width=W*S;baseCanvas.height=H*S;
    var ctx=baseCanvas.getContext('2d');ctx.imageSmoothingEnabled=false;
    var bg=ctx.createLinearGradient(0,0,0,baseCanvas.height);bg.addColorStop(0,'#020617');bg.addColorStop(.48,'#050916');bg.addColorStop(1,'#000205');ctx.fillStyle=bg;ctx.fillRect(0,0,baseCanvas.width,baseCanvas.height);
    for(var y=0;y<H;y++)for(var x=0;x<W;x++){
      var cx=x*S+S/2,cy=y*S+S/2;
      var cup=ctx.createRadialGradient(cx-1.6,cy-1.8,.4,cx,cy,S*.47);cup.addColorStop(0,'#1b2838');cup.addColorStop(.45,'#0b1220');cup.addColorStop(1,'#01040a');
      ctx.beginPath();ctx.arc(cx,cy,S*.43,0,Math.PI*2);ctx.fillStyle=cup;ctx.fill();ctx.strokeStyle='rgba(51,65,85,.30)';ctx.lineWidth=.55;ctx.stroke();
      ctx.beginPath();ctx.arc(cx,cy,S*.13,0,Math.PI*2);ctx.fillStyle='#0f172a';ctx.fill();
    }
    ctx.globalAlpha=.08;ctx.strokeStyle='#334155';ctx.lineWidth=1;
    for(var gx=0;gx<=W;gx+=8){ctx.beginPath();ctx.moveTo(gx*S+.5,0);ctx.lineTo(gx*S+.5,H*S);ctx.stroke();}
    for(var gy=0;gy<=H;gy+=8){ctx.beginPath();ctx.moveTo(0,gy*S+.5);ctx.lineTo(W*S,gy*S+.5);ctx.stroke();}
    ctx.globalAlpha=1;
    var glass=ctx.createLinearGradient(0,0,baseCanvas.width,baseCanvas.height);glass.addColorStop(0,'rgba(255,255,255,.07)');glass.addColorStop(.38,'rgba(255,255,255,.012)');glass.addColorStop(1,'rgba(255,255,255,0)');ctx.fillStyle=glass;ctx.fillRect(0,0,baseCanvas.width,baseCanvas.height);
    baseReady=true;
  }
  function ledSprite(r,g,b){
    var key=r+','+g+','+b,sp=spriteCache[key];if(sp)return sp;
    sp=document.createElement('canvas');sp.width=S;sp.height=S;var ctx=sp.getContext('2d');
    var cx=S/2,cy=S/2,led=ctx.createRadialGradient(cx-1.3,cy-1.4,.35,cx,cy,S*.36);
    led.addColorStop(0,'rgba(255,255,255,.98)');led.addColorStop(.24,'rgb('+r+','+g+','+b+')');led.addColorStop(1,'rgba('+r+','+g+','+b+',.62)');
    ctx.beginPath();ctx.arc(cx,cy,S*.30,0,Math.PI*2);ctx.fillStyle=led;ctx.fill();
    ctx.globalAlpha=.22;ctx.beginPath();ctx.arc(cx,cy,S*.54,0,Math.PI*2);ctx.fillStyle='rgb('+r+','+g+','+b+')';ctx.fill();ctx.globalAlpha=1;
    spriteCache[key]=sp;return sp;
  }
  function renderDots(cv){
    makeBase();
    var ctx=cv.getContext('2d');ctx.setTransform(1,0,0,1,0,0);ctx.clearRect(0,0,cv.width,cv.height);ctx.drawImage(baseCanvas,0,0);
    var img=oc.getImageData(0,0,W,H).data;
    for(var y=0;y<H;y++)for(var x=0;x<W;x++){
      var i=(y*W+x)*4,r=img[i],g=img[i+1],b=img[i+2],a=img[i+3];
      if(a>15&&(r+g+b)>20){ctx.drawImage(ledSprite(r,g,b),x*S,y*S);}
    }
  }
  function frame(ts){
    var cv=document.getElementById('matrixPreview');if(!cv){requestAnimationFrame(frame);return;}
    if(document.hidden||window.sfPreviewPaused){requestAnimationFrame(frame);return;}
    if(ts-lastFrame<FRAME_MS){requestAnimationFrame(frame);return;}
    syncLayout(cv);
    lastFrame=ts;
    var p=state(),si=document.getElementById('scrollTextInput'),li=document.getElementById('logoTextInput');if(si)p.scrollText=si.value;if(li)p.logoText=li.value;
    oc.setTransform(1,0,0,1,0,0);oc.clearRect(0,0,W,H);oc.fillStyle='#000';oc.fillRect(0,0,W,H);
    drawLogo(oc,ts);drawScroll(oc,ts);renderDots(cv);requestAnimationFrame(frame);
  }
  window.addEventListener('load',function(){requestAnimationFrame(frame);});
})();
</script>
)rawliteral";
  page += "<script>window.sfPreview={";
  page += "scrollText:\"" + jsEscape(scrollText) + "\",";
  page += "logoText:\"" + jsEscape(logoText) + "\",";
  page += "scrollColor:" + String(scrollTextColorMode) + ",";
  page += "scrollEffect:" + String(scrollTextEffectMode) + ",";
  page += "logoEffect:" + String(logoEffectMode) + ",";
  page += "logoColor:" + String(logoColorMode) + ",";
  page += "speed:" + String(scrollInterval) + ",";
  page += "logoSpeed:" + String(logoInterval) + ",";
  page += "brightness:" + String(matrixBrightness) + ",";
  page += "panelCount:" + String(panelCount);
  page += "};</script>";
  page += "</head><body><div class='wrap'>";

  page += "<div class='hero'>";
  page += "<div class='lang-switch'><a class='lang-btn" + activeLangClass("de") + "' href='/lang?l=de'>DE</a><a class='lang-btn" + activeLangClass("en") + "' href='/lang?l=en'>EN</a></div>";
  page += "<div class='topbar'><div class='logo-badge'>SF</div><div class='title-block'>";
  page += "<h1>SmartFix Matrix</h1>";
  page += "<div class='sub'>" + L("SmartFix Elektronikservice &bull; ESP32-S3 HUB75 Matrix", "SmartFix electronics service &bull; ESP32-S3 HUB75 Matrix") + "</div>";
  page += "</div><div class='version-chip'>Firmware v";
  page += FIRMWARE_VERSION;
  page += "</div></div>";

  page += "<div class='status'>";
  page += "<div class='pill good'><div class='label'>Firmware</div><div class='value'>v" + String(FIRMWARE_VERSION) + "</div></div>";
  page += "<div class='pill blue'><div class='label'>" + L("Display", "Display") + "</div><div class='value'>" + String(getPanelLayoutName()) + "</div></div>";
  page += "<div class='pill blue'><div class='label'>" + L("Modus", "Mode") + "</div><div class='value'>" + String(getModeName(currentMode)) + "</div></div>";
  page += "<div class='pill'><div class='label'>" + L("Auto Demo", "Auto demo") + "</div><div class='value'>" + autoStatus + "</div></div>";
  page += "<div class='pill'><div class='label'>" + L("Helligkeit", "Brightness") + "</div><div class='value' id='brightnessStatusValue'>" + String(matrixBrightness) + " / 255</div></div>";
  page += "<div class='pill'><div class='label'>" + L("Laufschrift", "Scroll") + "</div><div class='value'>" + String(getSpeedName()) + " / " + String(getScrollTextEffectName()) + "</div></div>";
  page += "<div class='pill'><div class='label'>Logo</div><div class='value'>" + String(getLogoSpeedName()) + " / " + String(getLogoEffectName()) + "</div></div>";
  page += "<div class='pill'><div class='label'>" + L("Textfarbe", "Text color") + "</div><div class='value'>" + String(getScrollTextColorName()) + "</div></div>";
  page += "<div class='pill'><div class='label'>" + L("Heim WLAN", "Home WiFi") + "</div><div class='value'>" + getWiFiStatusText() + "<br>" + htmlEscape(getStaIpText()) + "</div></div>";
  page += "<div class='pill'><div class='label'>" + L("mDNS Adresse", "mDNS address") + "</div><div class='value'>" + htmlEscape(getMdnsAddressText()) + "</div></div>";
  page += "<div class='pill'><div class='label'>" + L("Firmware Check", "Firmware check") + "</div><div class='value'>";
  if (latestFirmwareVersion == "-") {
    page += L("Nicht geprüft", "Not checked");
  } else {
    page += firmwareUpdateAvailable ? L("Update verfügbar", "Update available") : L("Aktuell", "Up to date");
  }
  page += "<br>" + htmlEscape(latestFirmwareVersion) + "</div></div>";
  page += "</div></div>";

  page += "<details class='card config-section' id='section-preview' open>";
  page += "<summary><span class='section-icon'>&#128161;</span><span class='summary-text'>" + L("LED Matrix Vorschau", "LED matrix preview") + "</span><span class='sumvalue'>" + String(getPanelLayoutName()) + "</span><span class='chev'>+</span></summary><div class='detail-body'>";
  page += "<div class='preview-shell'><canvas id='matrixPreview' width='" + String(getMatrixWidth() * (panelCount > 1 ? 5 : 10)) + "' height='320'></canvas></div>";
  page += "<div class='preview-row'><div class='sub'><span class='preview-dot'></span>" + L("Animierte Vorschau mit realistischen LED-Punkten. Logo-Text, Farben und Lauftext werden live mit den aktuellen Effekten simuliert.", "Animated preview with realistic LED dots. Logo text, colors and scrolling text are simulated live with the current effects.") + "</div></div>";
  page += "</div></details>";

  page += "<details class='card config-section' id='section-mode'>";
  page += "<summary><span class='section-icon'>&#9881;</span><span class='summary-text'>" + L("Modus ausw&auml;hlen", "Select mode") + "</span><span class='sumvalue'>" + String(getModeName(currentMode)) + "</span><span class='chev'>+</span></summary><div class='detail-body'>";
  page += "<div class='buttons'>";
  page += htmlButton(L("Laufschrift", "Scrolling text"), "/mode?m=0");
  page += htmlButton("Static Logo", "/mode?m=1");
  page += htmlButton("Pixel Art", "/mode?m=2");
  page += htmlButton("Random FX", "/mode?m=3");
  page += "</div></div></details>";

  page += "<details class='card config-section' id='section-scroll'>";
  page += "<summary><span class='section-icon'>&#9998;</span><span class='summary-text'>" + L("Laufschrift", "Scrolling text") + "</span><span class='sumvalue'>" + String(getScrollTextEffectName()) + "</span><span class='chev'>+</span></summary><div class='detail-body'>";
  page += "<div class='row-title'><h2>" + L("Text", "Text") + "</h2><span class='mini-chip'>" + L("max. 160 Zeichen", "max. 160 characters") + "</span></div>";
  page += "<form action='/set-text' method='GET'>";
  page += "<input id='scrollTextInput' name='t' maxlength='160' value='" + htmlEscape(scrollText) + "'>";
  page += "<button class='btn green' type='submit'>" + L("Text speichern", "Save text") + "</button>";
  page += "</form>";
  page += "<h2>" + L("Geschwindigkeit", "Speed") + "</h2><div class='buttons'>";
  page += htmlButton(L("Langsam", "Slow"), "/speed?v=70");
  page += htmlButton(L("Mittel", "Medium"), "/speed?v=35");
  page += htmlButton(L("Schnell", "Fast"), "/speed?v=18");
  page += htmlButton("Turbo", "/speed?v=8");
  page += "</div>";
  page += "<h2>" + L("Farbe", "Color") + "</h2><div class='buttons'>";
  page += htmlButton(L("Weiß", "White"), "/text-color?c=0");
  page += htmlButton(L("Grün", "Green"), "/text-color?c=1");
  page += htmlButton(L("Blau", "Blue"), "/text-color?c=2");
  page += htmlButton(L("Gelb", "Yellow"), "/text-color?c=3");
  page += htmlButton(L("Rot", "Red"), "/text-color?c=4");
  page += htmlButton("Refresh", "/");
  page += "</div>";
  page += "<h2>" + L("Text Effekt", "Text effect") + "</h2><div class='buttons'>";
  page += htmlButton("Normal", "/scroll-effect?e=0");
  page += htmlButton("Rainbow", "/scroll-effect?e=1");
  page += htmlButton("Wave", "/scroll-effect?e=2");
  page += htmlButton("Sparkle", "/scroll-effect?e=3");
  page += htmlButton("Comet Trail", "/scroll-effect?e=4");
  page += htmlButton("Flash", "/scroll-effect?e=5");
  page += htmlButton(L("Beidseitiges Sliden", "Two-way slide"), "/scroll-effect?e=6");
  page += "</div><div class='hint'>" + L("Effekte betreffen nur die laufende Textzeile unten.", "Effects apply only to the lower scrolling text line.") + "</div></div></details>";

  page += "<details class='card config-section' id='section-logo'>";
  page += "<summary><span class='section-icon'>SF</span><span class='summary-text'>Logo / Header</span><span class='sumvalue'>" + String(getLogoEffectName()) + "</span><span class='chev'>+</span></summary><div class='detail-body'>";
  page += "<div class='row-title'><h2>" + L("Logo Text", "Logo text") + "</h2><span class='mini-chip'>" + L("max. 160 Zeichen", "max. 160 characters") + "</span></div>";
  page += "<form action='/set-logo-text' method='GET'>";
  page += "<input id='logoTextInput' name='t' maxlength='160' value='" + htmlEscape(logoText) + "'>";
  page += "<button class='btn green' type='submit'>" + L("Logo Text speichern", "Save logo text") + "</button>";
  page += "</form>";
  page += "<div class='hint'>" + L("Der Logo-Text kann einfarbig, zweifarbig oder wortweise mehrfarbig dargestellt werden.", "The logo text can be displayed in one color, two colors, or word-by-word multicolor.") + "</div>";
  page += "<h2>" + L("Logo Effekt", "Logo effect") + "</h2><div class='buttons'>";
  page += htmlButton(L("Statisch", "Static"), "/logo-effect?e=0");
  page += htmlButton(L("Buchstabe", "Letter by letter"), "/logo-effect?e=1");
  page += htmlButton("Fade", "/logo-effect?e=2");
  page += htmlButton("Slide", "/logo-effect?e=3");
  page += htmlButton("Shimmer", "/logo-effect?e=4");
  page += htmlButton("Sparkle", "/logo-effect?e=5");
  page += htmlButton("Pulse", "/logo-effect?e=6");
  page += htmlButton("Wave", "/logo-effect?e=7");
  page += htmlButton("Bounce", "/logo-effect?e=8");
  page += htmlButton("Glitch", "/logo-effect?e=9");
  page += htmlButton("Scanline", "/logo-effect?e=10");
  page += htmlButton(L("Beidseitiges Sliden", "Two-way slide"), "/logo-effect?e=11");
  page += htmlButton("Refresh", "/");
  page += "</div><h2>" + L("Logo Geschwindigkeit", "Logo speed") + "</h2><div class='buttons'>";
  page += htmlButton(L("Langsam", "Slow"), "/logo-speed?v=70");
  page += htmlButton(L("Mittel", "Medium"), "/logo-speed?v=35");
  page += htmlButton(L("Schnell", "Fast"), "/logo-speed?v=18");
  page += htmlButton("Turbo", "/logo-speed?v=8");
  page += "</div><div class='hint'>" + L("Logo Geschwindigkeit und Laufschrift Geschwindigkeit sind jetzt getrennt gespeichert.", "Logo speed and scrolling text speed are now saved separately.") + "</div>";
  page += "<h2>" + L("Logo Farbe", "Logo color") + "</h2><div class='buttons'>";
  page += htmlButton("Auto / Brand", "/logo-color?c=0");
  page += htmlButton(L("Grün", "Green"), "/logo-color?c=1");
  page += htmlButton(L("Blau", "Blue"), "/logo-color?c=2");
  page += htmlButton(L("Weiß", "White"), "/logo-color?c=3");
  page += htmlButton(L("Gelb", "Yellow"), "/logo-color?c=4");
  page += htmlButton(L("Rot", "Red"), "/logo-color?c=5");
  page += htmlButton(L("2-farbig nach Wort", "Two-color by word"), "/logo-color?c=6");
  page += htmlButton(L("Mehrfarbig nach Wort", "Multicolor by word"), "/logo-color?c=7");
  page += "</div></div></details>";

  page += "<details class='card config-section' id='section-demo'>";
  page += "<summary><span class='section-icon'>&#9728;</span><span class='summary-text'>" + L("Auto Demo &amp; Helligkeit", "Auto demo &amp; brightness") + "</span><span class='sumvalue'>" + autoStatus + "</span><span class='chev'>+</span></summary><div class='detail-body'>";
  page += "<h2>Auto Demo</h2><div class='buttons'><a class='btn green' href='/auto'>" + L("Auto Demo starten", "Start auto demo") + "</a>";
  page += htmlButton("Refresh", "/");
  page += "</div><h2>" + L("Helligkeit", "Brightness") + "</h2>";
  page += "<div class='slider-card'>";
  page += "<div class='slider-head'><span>" + L("Display Helligkeit", "Display brightness") + "</span><strong id='brightnessValue'>" + String(matrixBrightness) + " / 255</strong></div>";
  page += "<input id='brightnessSlider' class='range-slider' type='range' min='5' max='255' step='1' value='" + String(matrixBrightness) + "'>";
  page += "<div class='slider-scale'><span>5</span><span>70</span><span>130</span><span>200</span><span>255</span></div>";
  page += "<div class='hint'>" + L("Der Wert wird beim Schieben automatisch gespeichert.", "The value is saved automatically while sliding.") + "</div>";
  page += "</div></div></details>";

  page += "<details class='card config-section' id='section-display'>";
  page += "<summary><span class='section-icon'>&#128250;</span><span class='summary-text'>" + L("Display Layout", "Display layout") + "</span><span class='sumvalue'>" + String(getPanelLayoutName()) + "</span><span class='chev'>+</span></summary><div class='detail-body'>";
  page += "<h2>" + L("64x32 Panels", "64x32 panels") + "</h2>";
  page += "<div class='buttons'>";
  page += htmlButton("1 Panel - 64x32", "/panels?n=1");
  page += htmlButton("2 Panels - 128x32", "/panels?n=2");
  page += "</div>";
  page += "<div class='hint'>" + L("Die Firmware ist f&uuml;r maximal 2 nebeneinander angeschlossene 64x32 Panels initialisiert. Die Auswahl bestimmt, wie breit Logo, Lauftext, Effekte und Vorschau gerendert werden.", "The firmware is initialized for up to 2 side-by-side 64x32 panels. This setting controls the rendered width for logo, scrolling text, effects and preview.") + "</div>";
  page += "</div></details>";

  page += "<details class='card config-section' id='wifi'>";
  page += "<summary><span class='section-icon'>&#128246;</span><span class='summary-text'>" + L("Heim WLAN", "Home WiFi") + "</span><span class='sumvalue'>" + getWiFiStatusText() + "</span><span class='chev'>+</span></summary><div class='detail-body'>";
  page += "<form action='/wifi-save' method='POST'>";
  page += "<input name='ssid' maxlength='64' placeholder='SSID' value='" + htmlEscape(wifiSsidValue) + "'>";
  page += "<input name='pass' maxlength='64' placeholder='" + L("WLAN Passwort", "WiFi password") + "' type='password' value='" + htmlEscape(homeWifiPassword) + "'>";
  page += "<button class='btn green' type='submit'>" + L("Mit Heim WLAN verbinden", "Connect to home WiFi") + "</button></form>";
  page += "<div class='buttons' style='margin-top:10px;'>";
  page += htmlButton(L("WLAN scannen", "Scan WiFi"), "/wifi-scan");
  page += htmlButton(L("Heim WLAN l&ouml;schen", "Forget home WiFi"), "/wifi-forget");
  page += htmlButton("Refresh", "/");
  page += "</div>";
  if (wifiSsidFromScan) page += "<div class='hint'>" + L("SSID aus Scan &uuml;bernommen. Bitte WLAN Passwort eingeben und speichern.", "SSID copied from scan. Enter the WiFi password and save.") + "</div>";
  page += "<div class='sub' style='margin-top:12px;'>" + L("Der SmartFix-Matrix Access Point bleibt zus&auml;tzlich aktiv.", "The SmartFix-Matrix access point remains active.") + "</div></div></details>";

  page += "<details class='card config-section' id='section-ota'>";
  page += "<summary><span class='section-icon'>&#9889;</span><span class='summary-text'>" + L("Firmware Update", "Firmware update") + "</span><span class='sumvalue'>" + htmlEscape(lastOtaStatus) + "</span><span class='chev'>+</span></summary><div class='detail-body'>";
  page += "<div class='hint'>" + L("Der GitHub Update-Check läuft automatisch, sobald Home WiFi verbunden ist. Die OTA URL wird fest aus dem Projekt verwendet und muss nicht konfiguriert werden.", "The GitHub update check runs automatically when Home WiFi is connected. The OTA URL is fixed by the project and does not need user configuration.") + "</div>";
  page += "<div class='sub' style='margin-top:12px;'>" + L("Letzter Check", "Last check") + ": " + htmlEscape(lastUpdateCheckText) + "<br>" + L("Neueste Version", "Latest version") + ": " + htmlEscape(latestFirmwareVersion) + "</div>";
  page += "<h2>" + L("GitHub OTA Update", "GitHub OTA update") + "</h2>";
  page += "<div class='buttons'>";
  page += htmlButton(L("GitHub Update jetzt installieren", "Install GitHub update now"), "/ota-github-start");
  page += htmlButton("Refresh", "/");
  page += "</div>";
  page += "<div class='hint'>" + L("Der Button prüft GitHub und flasht nur dann automatisch, wenn eine neuere SmartFix-Matrix-ota.bin verfügbar ist.", "The button checks GitHub and flashes automatically only when a newer SmartFix-Matrix-ota.bin is available.") + "</div>";
  page += "<h2>" + L("OTA BIN manuell flashen", "Flash OTA BIN manually") + "</h2>";
  page += "<form method='POST' action='/ota-upload' enctype='multipart/form-data'>";
  page += "<input type='file' name='firmware' accept='.bin' required>";
  page += "<button class='btn green' type='submit'>" + L("OTA BIN hochladen und flashen", "Upload and flash OTA BIN") + "</button></form>";
  page += "<div class='hint'>" + L("Wichtig: Hier nur die SmartFix-Matrix-ota.bin verwenden, nicht die USB-Full-BIN.", "Important: use only SmartFix-Matrix-ota.bin here, not the USB full BIN.") + "</div>";
  page += "<div class='hint'>" + L("Status", "Status") + ": " + htmlEscape(lastOtaStatus) + "</div></div></details>";

  page += "<details class='card config-section' id='section-system'>";
  page += "<summary><span class='section-icon'>&#128295;</span><span class='summary-text'>" + L("System", "System") + "</span><span class='sumvalue'>Reset</span><span class='chev'>+</span></summary><div class='detail-body'>";
  page += "<div class='buttons'><a class='btn danger' href='/factory-reset'>" + L("Werkseinstellungen", "Factory reset") + "</a>";
  page += htmlButton("Refresh", "/");
  page += "</div></div></details>";

  page += "<div class='small'>" + L("SmartFix Elektronikservice &bull; Entwickelt f&uuml;r ", "SmartFix electronics service &bull; Designed for ") + String(getPanelLayoutName()) + " HUB75 RGB Matrix</div>";
  page += "</div></body></html>";
  return page;
}

static void handleLanguageChange() {
  if (server.hasArg("l")) {
    String newLanguage = server.arg("l");
    newLanguage.toLowerCase();

    if (newLanguage == "de" || newLanguage == "en") {
      uiLanguage = newLanguage;
      saveLanguageSetting();
    }
  }

  redirectHome();
}

static void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

static void handleModeChange() {
  if (server.hasArg("m")) {
    int mode = server.arg("m").toInt();

    if (mode >= MODE_SCROLL_TEXT && mode <= MODE_RANDOM_FX) {
      autoModeDemo = false;
      setMode((DisplayMode)mode, true);
    }
  }

  redirectHome();
}

static void handleAutoDemo() {
  autoModeDemo = true;
  lastModeChange = millis();
  saveModeSettings();
  Serial.println("Auto mode demo enabled from web");
  redirectHome();
}

static void handleBrightness() {
  if (server.hasArg("v")) {
    int value = server.arg("v").toInt();

    if (value < 5) value = 5;
    if (value > 255) value = 255;

    matrixBrightness = value;
    display->setBrightness8(matrixBrightness);

    saveBrightnessSetting();

    Serial.print("Brightness changed to: ");
    Serial.println(matrixBrightness);
  }

  redirectHome();
}

static void handleTextColor() {
  if (server.hasArg("c")) {
    int value = server.arg("c").toInt();

    if (value < 0) value = 0;
    if (value > 4) value = 4;

    scrollTextColorMode = (uint8_t)value;
    saveTextColorSetting();

    autoModeDemo = false;
    setMode(MODE_SCROLL_TEXT, true);

    Serial.print("Text color changed to: ");
    Serial.println(getScrollTextColorName());
  }

  redirectHome();
}

static void handleScrollEffect() {
  if (server.hasArg("e")) {
    int value = server.arg("e").toInt();

    if (value < SCROLL_EFFECT_NORMAL) value = SCROLL_EFFECT_NORMAL;
    if (value > SCROLL_EFFECT_DUAL_SLIDE) value = SCROLL_EFFECT_DUAL_SLIDE;

    scrollTextEffectMode = (uint8_t)value;
    saveScrollEffectSetting();

    autoModeDemo = false;
    setMode(MODE_SCROLL_TEXT, true);

    Serial.print("Scroll effect changed to: ");
    Serial.println(getScrollTextEffectName());
  }

  redirectHome();
}

static void handleLogoEffect() {
  if (server.hasArg("e")) {
    int value = server.arg("e").toInt();

    if (value < LOGO_EFFECT_STATIC) value = LOGO_EFFECT_STATIC;
    if (value > LOGO_EFFECT_DUAL_SLIDE) value = LOGO_EFFECT_DUAL_SLIDE;

    logoEffectMode = (uint8_t)value;
    saveLogoEffectSetting();

    clearDisplay();

    Serial.print("Logo effect changed to: ");
    Serial.println(getLogoEffectName());
  }

  redirectHome();
}

static void handleLogoColor() {
  if (server.hasArg("c")) {
    int value = server.arg("c").toInt();

    if (value < LOGO_COLOR_BRAND) value = LOGO_COLOR_BRAND;
    if (value > LOGO_COLOR_RAINBOW) value = LOGO_COLOR_RAINBOW;

    logoColorMode = (uint8_t)value;
    saveLogoColorSetting();

    clearDisplay();

    Serial.print("Logo color changed to: ");
    Serial.println(getLogoColorName());
  }

  redirectHome();
}

static void handlePanelCount() {
  if (server.hasArg("n")) {
    int value = server.arg("n").toInt();
    if (value < MIN_PANEL_COUNT) value = MIN_PANEL_COUNT;
    if (value > MAX_PANEL_COUNT) value = MAX_PANEL_COUNT;

    setPanelCount((uint8_t)value, true);

    Serial.print("Panel count changed from web: ");
    Serial.println(panelCount);
  }

  redirectHome();
}

static void handleSpeed() {
  if (server.hasArg("v")) {
    int value = server.arg("v").toInt();

    if (value < 5) value = 5;
    if (value > 200) value = 200;

    scrollInterval = (uint16_t)value;
    saveSpeedSetting();

    autoModeDemo = false;
    setMode(MODE_SCROLL_TEXT, true);

    Serial.print("Scroll speed changed to: ");
    Serial.print(scrollInterval);
    Serial.println(" ms");
  }

  redirectHome();
}

static void handleLogoSpeed() {
  if (server.hasArg("v")) {
    int value = server.arg("v").toInt();

    if (value < 5) value = 5;
    if (value > 200) value = 200;

    logoInterval = (uint16_t)value;
    saveLogoSpeedSetting();

    autoModeDemo = false;
    clearDisplay();

    Serial.print("Logo speed changed to: ");
    Serial.print(logoInterval);
    Serial.println(" ms");
  }

  redirectHome();
}

static void handleSetText() {
  if (server.hasArg("t")) {
    String newText = server.arg("t");

    newText.trim();

    if (newText.length() == 0) {
      newText = "SMARTFIX ELEKTRONIKSERVICE";
    }

    if (newText.length() > MAX_SCROLL_TEXT_LEN) {
      newText = newText.substring(0, MAX_SCROLL_TEXT_LEN);
    }

    scrollText = newText + "   ";

    saveScrollTextSetting();

    autoModeDemo = false;
    setMode(MODE_SCROLL_TEXT, true);

    Serial.print("New scroll text from web: ");
    Serial.println(scrollText);
  }

  redirectHome();
}

static void handleSetLogoText() {
  if (server.hasArg("t")) {
    String newLogoText = server.arg("t");

    newLogoText.trim();

    if (newLogoText.length() == 0) {
      newLogoText = "SmartFix";
    }

    if (newLogoText.length() > MAX_LOGO_TEXT_LEN) {
      newLogoText = newLogoText.substring(0, MAX_LOGO_TEXT_LEN);
    }

    logoText = newLogoText;

    saveLogoTextSetting();
    clearDisplay();

    Serial.print("New logo text from web: ");
    Serial.println(logoText);
  }

  redirectHome();
}

static String wifiScanPage() {
  String page;
  page += "<!DOCTYPE html><html lang='" + uiLanguage + "'><head>";
  page += "<meta charset='UTF-8'>";
  page += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  page += "<meta name='theme-color' content='#050812'>";
  page += "<title>SmartFix Matrix WiFi Scan</title>";
  page += R"rawliteral(
<style>
:root{--bg:#050812;--panel:#0d1320;--line:#223149;--text:#e5e7eb;--muted:#94a3b8;--green:#22c55e;--blue:#38bdf8}*{box-sizing:border-box}body{margin:0;font-family:Arial,Helvetica,sans-serif;background:radial-gradient(circle at 18% 0%,rgba(34,197,94,.20),transparent 34%),radial-gradient(circle at 88% 10%,rgba(56,189,248,.16),transparent 32%),linear-gradient(180deg,#050812 0%,#08111f 55%,#050812 100%);color:var(--text);min-height:100vh}.wrap{max-width:940px;margin:0 auto;padding:22px}.card{background:linear-gradient(145deg,rgba(17,24,39,.93),rgba(2,6,23,.92));border:1px solid rgba(148,163,184,.22);border-radius:26px;padding:22px;margin-bottom:16px;box-shadow:0 22px 70px rgba(0,0,0,.45)}.topbar{display:flex;align-items:center;gap:15px;margin-bottom:18px}.logo-badge{width:52px;height:52px;border-radius:16px;display:grid;place-items:center;font-weight:900;font-size:20px;color:white;background:linear-gradient(135deg,var(--green),var(--blue));box-shadow:0 0 32px rgba(34,197,94,.28)}h1{margin:0;font-size:31px;letter-spacing:-.8px}h2{margin:0 0 14px;font-size:18px;color:#7dd3fc}.sub{color:var(--muted);line-height:1.45;margin-bottom:18px}.net{display:grid;grid-template-columns:1.4fr .7fr .8fr .8fr;gap:10px;align-items:center;background:rgba(2,6,23,.72);border:1px solid rgba(148,163,184,.18);border-radius:16px;padding:12px;margin-bottom:10px}.ssid{font-weight:bold;color:#f8fafc;word-break:break-word}.meta{font-size:13px;color:var(--muted)}.btn{display:flex;align-items:center;justify-content:center;text-align:center;text-decoration:none;min-height:42px;background:linear-gradient(135deg,#2563eb,#0ea5e9);color:white;padding:10px 12px;border-radius:13px;font-weight:800}.btn.green{background:linear-gradient(135deg,#16a34a,#22c55e)}.btn:hover{filter:brightness(1.12)}.actions{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-top:16px}@media(max-width:650px){.net{grid-template-columns:1fr}.actions{grid-template-columns:1fr}.wrap{padding:14px}}
</style>
)rawliteral";
  page += "</head><body><div class='wrap'><div class='card'>";
  page += "<div class='topbar'><div class='logo-badge'>SF</div><div><h1>SmartFix Matrix</h1><div class='sub' style='margin:5px 0 0;'>" + L("WLAN Scan", "WiFi scan") + "</div></div></div>";
  page += "<h2>" + L("Gefundene WLAN-Netzwerke", "Found WiFi networks") + "</h2>";
  page += "<div class='sub'>" + L("W&auml;hle eine SSID aus, gib danach das Passwort ein und speichere die Verbindung.", "Select an SSID, then enter the password and save the connection.") + "</div>";

  WiFi.scanDelete();
  int networkCount = WiFi.scanNetworks(false, true);

  if (networkCount <= 0) {
    page += "<div class='sub'>" + L("Keine WLAN-Netzwerke gefunden. Bitte pr&uuml;fe die Antenne/Position und scanne erneut.", "No WiFi networks found. Check antenna/position and scan again.") + "</div>";
  } else {
    for (int i = 0; i < networkCount; i++) {
      String ssid = WiFi.SSID(i);
      int32_t rssi = WiFi.RSSI(i);
      wifi_auth_mode_t security = WiFi.encryptionType(i);

      String ssidLabel = ssid.length() > 0 ? htmlEscape(ssid) : String("<i>") + L("Verstecktes Netzwerk", "Hidden network") + "</i>";
      String selectUrl = "/?ssid=" + urlEncode(ssid) + "#wifi";

      page += "<div class='net'>";
      page += "<div><div class='ssid'>" + ssidLabel + "</div><div class='meta'>" + L("Kanal", "Channel") + " ";
      page += String(WiFi.channel(i));
      page += "</div></div>";
      page += "<div class='meta'>" + String(rssi) + " dBm<br>" + getWifiSignalNameLocalized(rssi) + "</div>";
      page += "<div class='meta'>" + getWifiSecurityNameLocalized(security) + "</div>";
      if (ssid.length() > 0) {
        page += "<a class='btn green' href='" + selectUrl + "'>" + L("SSID &uuml;bernehmen", "Use SSID") + "</a>";
      } else {
        page += "<span class='meta'>" + L("Manuell eingeben", "Enter manually") + "</span>";
      }
      page += "</div>";
    }
  }

  WiFi.scanDelete();

  page += "<div class='actions'>";
  page += "<a class='btn green' href='/wifi-scan'>" + L("Erneut scannen", "Scan again") + "</a>";
  page += "<a class='btn' href='/#wifi'>" + L("Zur&uuml;ck", "Back") + "</a>";
  page += "</div>";
  page += "</div></div></body></html>";
  return page;
}

static void handleWifiScan() {
  server.send(200, "text/html", wifiScanPage());
}

static void handleWifiSave() {
  if (server.hasArg("ssid")) {
    homeWifiSsid = server.arg("ssid");
    homeWifiSsid.trim();

    homeWifiPassword = server.arg("pass");
    homeWifiEnabled = homeWifiSsid.length() > 0;

    saveWiFiSettings();

    server.send(200, "text/html",
                "<html><body style='background:#0b0f14;color:white;font-family:Arial;text-align:center;padding-top:40px;'>"
                "<h1>SmartFix Matrix</h1>"
                "<p>WLAN gespeichert. Neustart...</p>"
                "</body></html>");

    delay(1000);
    ESP.restart();
    return;
  }

  redirectHome();
}

static void handleWifiForget() {
  homeWifiEnabled = false;
  homeWifiSsid = "";
  homeWifiPassword = "";
  saveWiFiSettings();
  disconnectHomeWiFi();

  server.send(200, "text/html",
              "<html><body style='background:#0b0f14;color:white;font-family:Arial;text-align:center;padding-top:40px;'>"
              "<h1>SmartFix Matrix</h1>"
              "<p>Heim WLAN gel&ouml;scht. Neustart...</p>"
              "</body></html>");

  delay(1000);
  ESP.restart();
}

static void handleGithubOtaStart() {
  if (WiFi.status() != WL_CONNECTED) {
    lastOtaStatus = "GitHub OTA Fehler: Home WiFi ist nicht verbunden.";
    server.send(200, "text/html",
                "<html><body style='background:#0b0f14;color:white;font-family:Arial;text-align:center;padding-top:40px;'>"
                "<h1>SmartFix Matrix OTA</h1>"
                "<p>Home WiFi ist nicht verbunden.</p>"
                "<p><a style='color:#38bdf8' href='/#section-ota'>Zurück</a></p>"
                "</body></html>");
    return;
  }

  checkFirmwareUpdateFromGitHub(true);

  if (!firmwareUpdateAvailable) {
    lastOtaStatus = "Kein neueres GitHub Update gefunden.";
    server.send(200, "text/html",
                "<html><body style='background:#0b0f14;color:white;font-family:Arial;text-align:center;padding-top:40px;'>"
                "<h1>SmartFix Matrix OTA</h1>"
                "<p>Kein neueres GitHub Update gefunden.</p>"
                "<p><a style='color:#38bdf8' href='/#section-ota'>Zurück</a></p>"
                "</body></html>");
    return;
  }

  if (latestFirmwareUrl.length() == 0) {
    lastOtaStatus = "GitHub OTA Fehler: SmartFix-Matrix-ota.bin nicht gefunden.";
    server.send(500, "text/html",
                "<html><body style='background:#0b0f14;color:white;font-family:Arial;text-align:center;padding-top:40px;'>"
                "<h1>SmartFix Matrix OTA</h1>"
                "<p>SmartFix-Matrix-ota.bin wurde im GitHub Release nicht gefunden.</p>"
                "<p><a style='color:#38bdf8' href='/#section-ota'>Zurück</a></p>"
                "</body></html>");
    return;
  }

  otaUrl = latestFirmwareUrl;
  saveOtaSettings();
  lastOtaStatus = "GitHub OTA gestartet: " + latestFirmwareVersion;

  server.send(200, "text/html",
              "<html><body style='background:#0b0f14;color:white;font-family:Arial;text-align:center;padding-top:40px;'>"
              "<h1>SmartFix Matrix OTA</h1>"
              "<p>GitHub OTA Update startet jetzt.</p>"
              "<p>Bitte Stromversorgung nicht trennen.</p>"
              "</body></html>");

  delay(750);
  startOtaUpdateFromSavedUrl();
}

static void handleOtaUploadFinished() {
  if (Update.hasError()) {
    lastOtaStatus = "Manuelles OTA fehlgeschlagen.";
    server.send(500, "text/html",
                "<html><body style='background:#0b0f14;color:white;font-family:Arial;text-align:center;padding-top:40px;'>"
                "<h1>SmartFix Matrix OTA</h1>"
                "<p>Upload fehlgeschlagen.</p>"
                "<p>Bitte SmartFix-Matrix-ota.bin verwenden.</p>"
                "</body></html>");
    return;
  }

  lastOtaStatus = "Manuelles OTA OK. Neustart...";
  server.send(200, "text/html",
              "<html><body style='background:#0b0f14;color:white;font-family:Arial;text-align:center;padding-top:40px;'>"
              "<h1>SmartFix Matrix OTA</h1>"
              "<p>Firmware wurde hochgeladen.</p>"
              "<p>Neustart...</p>"
              "</body></html>");
  delay(1000);
  ESP.restart();
}

static void handleOtaFileUpload() {
  HTTPUpload &upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    Serial.print("Manual OTA upload: ");
    Serial.println(upload.filename);
    lastOtaStatus = "Manuelles OTA Upload gestartet.";

    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
      lastOtaStatus = "Manuelles OTA Fehler: Start fehlgeschlagen.";
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
      lastOtaStatus = "Manuelles OTA Fehler: Schreiben fehlgeschlagen.";
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.print("Manual OTA uploaded bytes: ");
      Serial.println(upload.totalSize);
      lastOtaStatus = "Manuelles OTA erfolgreich.";
    } else {
      Update.printError(Serial);
      lastOtaStatus = "Manuelles OTA Fehler: Update.end fehlgeschlagen.";
    }
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    Update.end();
    lastOtaStatus = "Manuelles OTA abgebrochen.";
  }
}

static void handleFactoryReset() {
  factoryResetSettings();

  server.send(200, "text/html",
              "<html><body style='background:#0b0f14;color:white;font-family:Arial;text-align:center;padding-top:40px;'>"
              "<h1>SmartFix Matrix</h1>"
              "<p>Einstellungen wurden gel&ouml;scht.</p>"
              "<p>Neustart...</p>"
              "</body></html>");

  delay(1000);
  ESP.restart();
}

void setupWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/lang", HTTP_GET, handleLanguageChange);
  server.on("/mode", HTTP_GET, handleModeChange);
  server.on("/auto", HTTP_GET, handleAutoDemo);
  server.on("/brightness", HTTP_GET, handleBrightness);
  server.on("/speed", HTTP_GET, handleSpeed);
  server.on("/logo-speed", HTTP_GET, handleLogoSpeed);
  server.on("/text-color", HTTP_GET, handleTextColor);
  server.on("/scroll-effect", HTTP_GET, handleScrollEffect);
  server.on("/logo-effect", HTTP_GET, handleLogoEffect);
  server.on("/logo-color", HTTP_GET, handleLogoColor);
  server.on("/panels", HTTP_GET, handlePanelCount);
  server.on("/set-text", HTTP_GET, handleSetText);
  server.on("/set-logo-text", HTTP_GET, handleSetLogoText);
  server.on("/wifi-scan", HTTP_GET, handleWifiScan);
  server.on("/wifi-save", HTTP_POST, handleWifiSave);
  server.on("/wifi-forget", HTTP_GET, handleWifiForget);
  server.on("/ota-github-start", HTTP_GET, handleGithubOtaStart);
  server.on("/ota-upload", HTTP_POST, handleOtaUploadFinished, handleOtaFileUpload);
  server.on("/factory-reset", HTTP_GET, handleFactoryReset);

  server.onNotFound([]() {
    server.send(404, "text/plain", "404 - Not found");
  });

  server.begin();
  Serial.println("Webserver started");
}

void handleWebServer() {
  server.handleClient();
}
