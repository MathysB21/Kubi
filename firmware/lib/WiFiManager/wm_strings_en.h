/**
 * wm_strings_en.h
 * Kubi setup portal (card UI, adapted from Project Sunrise)
 */

#ifndef _WM_STRINGS_EN_H_
#define _WM_STRINGS_EN_H_

#ifndef WIFI_MANAGER_OVERRIDE_STRINGS

#include "wm_consts_en.h" 

const char WM_LANGUAGE[] PROGMEM = "en-US"; 

const char HTTP_HEAD_START[]       PROGMEM = "<!DOCTYPE html>"
"<html lang='en'><head>"
"<meta name='format-detection' content='telephone=no'>"
"<meta charset='UTF-8'>"
"<meta name='viewport' content='width=device-width,initial-scale=1,user-scalable=no,maximum-scale=1'/>"
"<title>{v}</title>";

// THE SCRIPT FIX: Scoops up the networks and wraps them beautifully
const char HTTP_SCRIPT[]           PROGMEM = "<script>"
"function c(el){"
"  var ssid = el.getAttribute('data-ssid');"
"  document.getElementById('s').value = ssid;"
"  document.querySelectorAll('.net-item').forEach(function(e){e.classList.remove('selected')});"
"  el.classList.add('selected');"
"  var pInput = document.getElementById('p');"
"  pInput.value = '';"
"  var known = (typeof knownNetworks !== 'undefined') && knownNetworks.indexOf(ssid) >= 0;"
"  pInput.placeholder = known ? 'Saved on Kubi - leave blank' : 'Enter password...';"
"  pInput.focus();"
"}"
"function togglePw(){"
"  var p = document.getElementById('p');"
"  var eye = document.getElementById('eye');"
"  if(p.type === 'password') {"
"    p.type = 'text';"
"    eye.setAttribute('stroke', 'var(--amber)');"
"  } else {"
"    p.type = 'password';"
"    eye.setAttribute('stroke', 'var(--muted)');"
"  }"
"}"
"window.addEventListener('DOMContentLoaded', function() {"
"  var i = document.querySelectorAll('.net-item');"
"  if (i.length > 0) {"
"    var w = document.createElement('div'); w.className = 'card list-card';"
"    var g = document.createElement('div'); g.className = 'input-group';"
"    g.innerHTML = '<label>CHOOSE YOUR WIFI</label>';"
"    g.appendChild(w);"
"    i[0].parentNode.insertBefore(g, i[0]);"
"    i.forEach(function(e) { w.appendChild(e); });"
"  }"
"});"
"</script>";

const char HTTP_HEAD_END[]         PROGMEM = "</head><body class='{c}'><div class='wrap'>"; 

// Kubi: isometric cube logo in the dashboard's amber
const char HTTP_ROOT_MAIN[]        PROGMEM = "<div class='header-wrap'>"
"<svg class='logo' xmlns='http://www.w3.org/2000/svg' viewBox='0 0 90 90'><g fill='none' stroke='#f59e0b' stroke-width='4' stroke-linejoin='round'><path d='M45 10 L78 28 L78 64 L45 82 L12 64 L12 28 Z'/><path d='M12 28 L45 46 L78 28'/><path d='M45 46 L45 82'/></g></svg>"
"<h1>{t}</h1><p class='subtitle'>Let's get Kubi onto your WiFi</p></div>";

const char * const HTTP_PORTAL_MENU[] PROGMEM = {
"<form action='/wifi' method='get'><button class='primary-btn'><svg width='18' height='18' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'><path d='M5 12.55a11 11 0 0 1 14.08 0'/><path d='M1.42 9a16 16 0 0 1 21.16 0'/><path d='M8.53 16.11a6 6 0 0 1 6.95 0'/><line x1='12' y1='20' x2='12.01' y2='20'/></svg> Configure WiFi</button></form><br/>\n",
"", "", "", "", "", "", "", "", "" 
};

const char HTTP_PORTAL_OPTIONS[]   PROGMEM = "";
const char HTTP_ITEM_QI[]          PROGMEM = "<div class='sig sig-{q}'></div>"; 
const char HTTP_ITEM_QP[]          PROGMEM = ""; 

// The bare-metal network item. JS wraps it later.
const char HTTP_ITEM[]             PROGMEM = "<div class='net-item {i}' onclick='c(this)' data-ssid='{V}'><div class='net-name'><svg class='tick' viewBox='0 0 24 24' fill='none' stroke='#f59e0b' stroke-width='3' stroke-linecap='round' stroke-linejoin='round'><polyline points='20 6 9 17 4 12'></polyline></svg>{v}</div><div class='q-wrap'>{qi}</div></div>"; 

// THE FORM FIX: This just opens the form and holds the hidden input
const char HTTP_FORM_START[]       PROGMEM = "<form method='POST' action='{v}'><input type='hidden' id='s' name='s'>";

// THE PASSWORD CARD: Contains ONLY the password input now
const char HTTP_FORM_WIFI[]        PROGMEM = "<div class='input-group'><label for='p'>PASSWORD</label><div class='card pass-card' style='display:flex;align-items:center;'><input id='p' name='p' type='password' placeholder='Enter password...' style='flex:1;'><svg id='eye' onclick='togglePw()' width='20' height='20' viewBox='0 0 24 24' fill='none' stroke='var(--muted)' stroke-width='2' stroke-linecap='round' stroke-linejoin='round' style='cursor:pointer;margin-right:0.5rem;'><path d='M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z'></path><circle cx='12' cy='12' r='3'></circle></svg></div></div>";

const char HTTP_FORM_WIFI_END[]    PROGMEM = "";
const char HTTP_FORM_STATIC_HEAD[] PROGMEM = "";
const char HTTP_FORM_END[]         PROGMEM = "<button type='submit' class='primary-btn mt-4'>Connect Kubi</button></form>";
const char HTTP_FORM_LABEL[]       PROGMEM = "<label for='{i}'>{t}</label>";
const char HTTP_FORM_PARAM_HEAD[]  PROGMEM = "";
const char HTTP_FORM_PARAM[]       PROGMEM = "<br/><input id='{i}' name='{n}' maxlength='{l}' value='{v}' {c}>\n";

const char HTTP_SCAN_LINK[]        PROGMEM = "<form action='/wifi?refresh=1' method='POST'><button class='ghost-btn' name='refresh' value='1'><svg width='16' height='16' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'><polyline points='23 4 23 10 17 10'/><polyline points='1 20 1 14 7 14'/><path d='M3.51 9a9 9 0 0 1 14.85-3.36L23 10M1 14l4.64 4.36A9 9 0 0 0 20.49 15'/></svg> Refresh Networks</button></form>";
const char HTTP_SAVED[]            PROGMEM = "<div class='msg card'><strong>Connecting Kubi...</strong><p class='subtitle mt-2'>Kubi is joining your WiFi and will restart. When it's online, its screen shows the address to open next. You can close this page. If Kubi can't connect, the Kubi-Setup network comes back so you can try again.</p></div>";
const char HTTP_PARAMSAVED[]       PROGMEM = "<div class='msg card'>Saved</div>";
const char HTTP_END[]              PROGMEM = "</div></body></html>";
const char HTTP_ERASEBTN[]         PROGMEM = "";
const char HTTP_UPDATEBTN[]        PROGMEM = "";
const char HTTP_BACKBTN[]          PROGMEM = "<form action='/' method='get'><button class='ghost-btn'>Cancel</button></form>";

const char HTTP_STATUS_ON[]        PROGMEM = "<div class='msg card'><strong>Connected</strong> to {v}</div>";
const char HTTP_STATUS_OFF[]       PROGMEM = "<div class='msg card {c}'><strong>Not connected</strong> to {v}{r}</div>"; 
const char HTTP_STATUS_OFFPW[]     PROGMEM = "<br/><span class='err'>That password didn't work. Try again?</span>"; 
const char HTTP_STATUS_OFFNOAP[]   PROGMEM = "<br/><span class='err'>Kubi couldn't find that network.</span>"; 
const char HTTP_STATUS_OFFFAIL[]   PROGMEM = "<br/><span class='err'>Kubi couldn't connect. Try again?</span>"; 
const char HTTP_STATUS_NONE[]      PROGMEM = "";
const char HTTP_BR[]               PROGMEM = "";

// THE BRAND NEW CSS INJECTION
const char HTTP_STYLE[]            PROGMEM = "<style>"
":root { color-scheme: dark; --amber: #f59e0b; --bg: #09090b; --card: #18181b; --border: #27272a; --text: #f4f4f5; --muted: #a1a1aa; }"
"body { font-family: 'Montserrat', system-ui, -apple-system, sans-serif; background: var(--bg); color: var(--text); margin: 0; padding: 1.5rem 1rem; display: flex; align-items: center; justify-content: center; min-height: 100dvh; box-sizing: border-box; }"
".wrap { width: 100%; max-width: 420px; }"
".header-wrap { text-align: center; margin-bottom: 2rem; }"
".logo { width: 64px; height: 64px; margin: 0 auto 0.5rem; display: block; filter: drop-shadow(0 0 12px rgba(245, 158, 11, 0.3)); }"
"h1 { font-size: 1.75rem; font-weight: 500; margin: 0 0 0.25rem 0; letter-spacing: -0.025em; }"
".subtitle { color: var(--muted); font-size: 0.65rem; font-weight: 600; letter-spacing: 0.1em; text-transform: uppercase; margin: 0; }"
".mt-2 { margin-top: 0.5rem; }"
".card { background: var(--card); border: 1px solid var(--border); border-radius: 1rem; padding: 1.25rem; margin-bottom: 1rem; }"
".list-card { padding: 0.5rem; max-height: 280px; overflow-y: auto; }"
".pass-card { padding: 0.25rem; display: flex; }"
"form { margin: 0; }"
""
"/* Scrollbar for Network List */"
".list-card::-webkit-scrollbar { width: 6px; }"
".list-card::-webkit-scrollbar-track { background: var(--card); border-radius: 4px; }"
".list-card::-webkit-scrollbar-thumb { background: var(--border); border-radius: 4px; }"
""
"/* Typography & Inputs */"
".input-group { margin-bottom: 1.25rem; display: flex; flex-direction: column; gap: 0.5rem; }"
"label { font-size: 0.65rem; font-weight: 600; letter-spacing: 0.1em; color: var(--muted); padding-left: 0.25rem; }"
".pass-card input { width: 100%; background: transparent; border: none; color: var(--text); padding: 0.875rem 0.75rem; font-family: inherit; font-size: 0.95rem; outline: none; }"
""
"/* Buttons */"
"button { display: flex; align-items: center; justify-content: center; gap: 0.5rem; width: 100%; font-family: inherit; font-size: 0.95rem; font-weight: 600; border-radius: 0.75rem; padding: 0.875rem 1rem; cursor: pointer; border: none; transition: all 0.2s; }"
".primary-btn { background: var(--amber); color: var(--bg); }"
".primary-btn:active { opacity: 0.8; transform: scale(0.98); }"
".ghost-btn { background: transparent; color: var(--muted); border: 1px solid var(--border); margin-top: 1rem; }"
".ghost-btn:active { background: var(--card); color: var(--text); }"
""
"/* The Network List Items */"
".net-item { display: flex; align-items: center; justify-content: space-between; padding: 0.875rem 0.5rem; border-bottom: 1px solid var(--border); cursor: pointer; transition: background 0.2s; border-radius: 0.5rem; }"
".net-item:last-child { border-bottom: none; }"
".net-name { display: flex; align-items: center; font-weight: 500; font-size: 0.95rem; pointer-events: none; }"
".tick { width: 18px; height: 18px; opacity: 0; transform: scale(0.5); transition: all 0.2s; margin-right: 0.75rem; }"
".net-item.selected { background: rgba(245, 158, 11, 0.1); }"
".net-item.selected .tick { opacity: 1; transform: scale(1); }"
".q-wrap { padding-right: 0.5rem; opacity: 0.5; pointer-events: none; }"
".err { color: #ef4444; font-size: 0.85rem; font-weight: 500; }"
".msg { text-align: center; line-height: 1.5; }"
"</style>";

#ifndef WM_NOHELP
const char HTTP_HELP[]             PROGMEM = "";
#else
const char HTTP_HELP[]             PROGMEM = "";
#endif

const char HTTP_UPDATE[] PROGMEM = "";
const char HTTP_UPDATE_FAIL[] PROGMEM = "";
const char HTTP_UPDATE_SUCCESS[] PROGMEM = "";

#ifdef WM_JSTEST
const char HTTP_JS[] PROGMEM = "";
#endif

// Info html 
#ifdef ESP32
    const char HTTP_INFO_esphead[]    PROGMEM = "";
    const char HTTP_INFO_chiprev[]    PROGMEM = "";
    const char HTTP_INFO_lastreset[]  PROGMEM = "";
    const char HTTP_INFO_aphost[]     PROGMEM = "";
    const char HTTP_INFO_psrsize[]    PROGMEM = "";
    const char HTTP_INFO_temp[]       PROGMEM = "";
    const char HTTP_INFO_hall[]       PROGMEM = "";
#else
    const char HTTP_INFO_esphead[]    PROGMEM = "";
#endif

const char HTTP_INFO_memsmeter[]  PROGMEM = "";
const char HTTP_INFO_memsketch[]  PROGMEM = "";
const char HTTP_INFO_freeheap[]   PROGMEM = "";
const char HTTP_INFO_wifihead[]   PROGMEM = "";
const char HTTP_INFO_uptime[]     PROGMEM = "";
const char HTTP_INFO_chipid[]     PROGMEM = "";
const char HTTP_INFO_idesize[]    PROGMEM = "";
const char HTTP_INFO_sdkver[]     PROGMEM = "";
const char HTTP_INFO_cpufreq[]    PROGMEM = "";
const char HTTP_INFO_apip[]       PROGMEM = "";
const char HTTP_INFO_apmac[]      PROGMEM = "";
const char HTTP_INFO_apssid[]     PROGMEM = "";
const char HTTP_INFO_apbssid[]    PROGMEM = "";
const char HTTP_INFO_stassid[]    PROGMEM = "";
const char HTTP_INFO_staip[]      PROGMEM = "";
const char HTTP_INFO_stagw[]      PROGMEM = "";
const char HTTP_INFO_stasub[]     PROGMEM = "";
const char HTTP_INFO_dnss[]       PROGMEM = "";
const char HTTP_INFO_host[]       PROGMEM = "";
const char HTTP_INFO_stamac[]     PROGMEM = "";
const char HTTP_INFO_conx[]       PROGMEM = "";
const char HTTP_INFO_autoconx[]   PROGMEM = "";

const char HTTP_INFO_aboutver[]     PROGMEM = "";
const char HTTP_INFO_aboutarduino[] PROGMEM = "";
const char HTTP_INFO_aboutsdk[]     PROGMEM = "";
const char HTTP_INFO_aboutdate[]    PROGMEM = "";

const char S_brand[]              PROGMEM = "Kubi Setup";
const char S_debugPrefix[]        PROGMEM = "*wm:";
const char S_y[]                  PROGMEM = "Yes";
const char S_n[]                  PROGMEM = "No";
const char S_enable[]             PROGMEM = "Enabled";
const char S_disable[]            PROGMEM = "Disabled";
const char S_GET[]                PROGMEM = "GET";
const char S_POST[]               PROGMEM = "POST";
const char S_NA[]                 PROGMEM = "Unknown";
const char S_passph[]             PROGMEM = "********";
const char S_titlewifisaved[]     PROGMEM = "Connecting...";
const char S_titlewifisettings[]  PROGMEM = "Settings saved";
const char S_titlewifi[]          PROGMEM = "Select Network";
const char S_titleinfo[]          PROGMEM = "Info";
const char S_titleparam[]         PROGMEM = "Setup";
const char S_titleparamsaved[]    PROGMEM = "Setup saved";
const char S_titleexit[]          PROGMEM = "Exit";
const char S_titlereset[]         PROGMEM = "Reset";
const char S_titleerase[]         PROGMEM = "Erase";
const char S_titleclose[]         PROGMEM = "Close";
const char S_options[]            PROGMEM = "options";
// Stripped the card class here so it doesn't nest awkwardly
const char S_nonetworks[]         PROGMEM = "<div class='msg'>No networks found. Tap refresh to scan again.</div>";
const char S_staticip[]           PROGMEM = "Static IP";
const char S_staticgw[]           PROGMEM = "Static gateway";
const char S_staticdns[]          PROGMEM = "Static DNS";
const char S_subnet[]             PROGMEM = "Subnet";
const char S_exiting[]            PROGMEM = "Exiting";
const char S_resetting[]          PROGMEM = "Rebooting...";
const char S_closing[]            PROGMEM = "You can close this page.";
const char S_error[]              PROGMEM = "An error occured";
const char S_notfound[]           PROGMEM = "File not found\n\n";
const char S_uri[]                PROGMEM = "URI: ";
const char S_method[]             PROGMEM = "\nMethod: ";
const char S_args[]               PROGMEM = "\nArguments: ";
const char S_parampre[]           PROGMEM = "param_";

const char D_HR[]                 PROGMEM = "--------------------";

#ifdef ESP8266
    const char S_ssidpre[]        PROGMEM = "ESP";
#elif defined(ESP32)
    const char S_ssidpre[]        PROGMEM = "ESP32";
#else
    const char S_ssidpre[]        PROGMEM = "WM";
#endif

#endif
#endif