/**
 * wm_strings_en.h
 * PREMIUM SUNRISE OVERHAUL V2 - The Card UI
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
"  if (typeof savedNetworks !== 'undefined' && savedNetworks[ssid]) {"
"    pInput.value = savedNetworks[ssid];"
"  } else {"
"    pInput.value = '';"
"  }"
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
"    g.innerHTML = '<label>AVAILABLE NETWORKS</label>';"
"    g.appendChild(w);"
"    i[0].parentNode.insertBefore(g, i[0]);"
"    i.forEach(function(e) { w.appendChild(e); });"
"  }"
"});"
"</script>";

const char HTTP_HEAD_END[]         PROGMEM = "</head><body class='{c}'><div class='wrap'>"; 

// THE LOGO FIX: Changed %23f59e0b back to #f59e0b
const char HTTP_ROOT_MAIN[]        PROGMEM = "<div class='header-wrap'>"
"<svg class='logo' xmlns='http://www.w3.org/2000/svg' viewBox='0 0 90 90'><path fill='#f59e0b' d='M 56.784 80.59 L 33.214 80.59 C 32.659 80.59 32.214 80.144 32.214 79.59 C 32.214 79.035 32.659 78.59 33.214 78.59 L 56.78 78.59 C 57.335 78.59 57.78 79.035 57.78 79.59 C 57.784 80.14 57.335 80.59 56.784 80.59 L 56.784 80.59 Z M 66.636 72.691 L 23.363 72.691 C 22.808 72.691 22.363 72.246 22.363 71.691 C 22.363 71.137 22.808 70.691 23.363 70.691 L 66.636 70.691 C 67.19 70.691 67.636 71.137 67.636 71.691 C 67.636 72.242 67.19 72.691 66.636 72.691 Z M 79.757 64.793 L 10.241 64.793 C 9.686 64.793 9.241 64.347 9.241 63.793 C 9.241 63.238 9.686 62.793 10.241 62.793 L 25.565 62.793 C 24.244 59.965 23.549 56.847 23.549 53.703 C 23.549 41.875 33.17 32.25 45.002 32.25 C 56.83 32.25 66.455 41.871 66.455 53.703 C 66.455 56.847 65.764 59.965 64.439 62.793 L 79.763 62.793 C 80.318 62.793 80.763 63.238 80.763 63.793 C 80.756 64.344 80.31 64.793 79.756 64.793 L 79.757 64.793 Z M 27.796 62.793 L 62.198 62.793 C 63.67 60.012 64.448 56.875 64.448 53.703 C 64.448 42.976 55.721 34.25 44.995 34.25 C 34.268 34.25 25.542 42.977 25.542 53.703 C 25.545 56.871 26.323 60.008 27.795 62.793 L 27.796 62.793 Z M 74.995 54.703 C 74.534 54.703 74.116 54.383 74.018 53.91 C 73.905 53.371 74.248 52.84 74.788 52.723 L 87.792 49.957 C 88.327 49.844 88.862 50.187 88.979 50.726 C 89.092 51.266 88.749 51.797 88.21 51.914 L 75.206 54.68 C 75.131 54.695 75.065 54.703 74.995 54.703 L 74.995 54.703 Z M 15.003 54.703 C 14.932 54.703 14.862 54.695 14.792 54.683 L 1.792 51.914 C 1.253 51.801 0.905 51.269 1.022 50.726 C 1.135 50.187 1.667 49.844 2.21 49.957 L 15.214 52.723 C 15.753 52.836 16.1 53.367 15.983 53.91 C 15.882 54.379 15.464 54.703 15.003 54.703 L 15.003 54.703 Z M 72.714 43.223 C 72.319 43.223 71.948 42.993 71.788 42.606 C 71.577 42.094 71.819 41.508 72.327 41.297 L 76.675 39.496 C 77.182 39.285 77.772 39.528 77.983 40.035 C 78.194 40.547 77.952 41.133 77.444 41.344 L 73.096 43.145 C 72.967 43.2 72.839 43.223 72.714 43.223 L 72.714 43.223 Z M 17.284 43.223 C 17.155 43.223 17.026 43.2 16.901 43.149 L 12.553 41.348 C 12.041 41.137 11.799 40.551 12.014 40.039 C 12.225 39.528 12.815 39.285 13.323 39.5 L 17.67 41.301 C 18.182 41.512 18.424 42.098 18.209 42.61 C 18.049 42.989 17.678 43.223 17.284 43.223 L 17.284 43.223 Z M 66.21 33.492 C 65.952 33.492 65.698 33.395 65.503 33.2 C 65.112 32.809 65.112 32.176 65.503 31.785 L 74.901 22.387 C 75.292 21.996 75.924 21.996 76.315 22.387 C 76.706 22.778 76.706 23.41 76.315 23.801 L 66.917 33.2 C 66.721 33.395 66.467 33.492 66.21 33.492 L 66.21 33.492 Z M 23.788 33.492 C 23.53 33.492 23.276 33.395 23.081 33.2 L 13.682 23.801 C 13.292 23.411 13.292 22.778 13.682 22.387 C 14.073 21.996 14.706 21.996 15.096 22.387 L 24.495 31.785 C 24.885 32.176 24.885 32.809 24.495 33.2 C 24.299 33.395 24.045 33.492 23.788 33.492 L 23.788 33.492 Z M 56.479 26.989 C 56.35 26.989 56.221 26.965 56.096 26.914 C 55.584 26.703 55.342 26.118 55.557 25.606 L 57.358 21.258 C 57.568 20.746 58.158 20.504 58.666 20.719 C 59.178 20.93 59.42 21.516 59.205 22.028 L 57.404 26.375 C 57.24 26.758 56.869 26.989 56.479 26.989 L 56.479 26.989 Z M 33.518 26.989 C 33.123 26.989 32.752 26.758 32.592 26.371 L 30.791 22.024 C 30.58 21.512 30.822 20.926 31.33 20.715 C 31.838 20.504 32.428 20.746 32.639 21.254 L 34.439 25.602 C 34.65 26.114 34.408 26.7 33.9 26.91 C 33.775 26.965 33.646 26.989 33.518 26.989 L 33.518 26.989 Z M 44.998 24.707 C 44.443 24.707 43.998 24.262 43.998 23.707 L 43.998 10.41 C 43.998 9.856 44.443 9.41 44.998 9.41 C 45.552 9.41 45.998 9.856 45.998 10.41 L 45.998 23.703 C 45.998 24.258 45.552 24.707 44.998 24.707 L 44.998 24.707 Z'/></svg>"
"<h1>{t}</h1><p class='subtitle'>Smart Ambient Lighting</p></div>";

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
const char HTTP_FORM_END[]         PROGMEM = "<button type='submit' class='primary-btn mt-4'>Connect to Network</button></form>";
const char HTTP_FORM_LABEL[]       PROGMEM = "<label for='{i}'>{t}</label>";
const char HTTP_FORM_PARAM_HEAD[]  PROGMEM = "";
const char HTTP_FORM_PARAM[]       PROGMEM = "<br/><input id='{i}' name='{n}' maxlength='{l}' value='{v}' {c}>\n";

const char HTTP_SCAN_LINK[]        PROGMEM = "<form action='/wifi?refresh=1' method='POST'><button class='ghost-btn' name='refresh' value='1'><svg width='16' height='16' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'><polyline points='23 4 23 10 17 10'/><polyline points='1 20 1 14 7 14'/><path d='M3.51 9a9 9 0 0 1 14.85-3.36L23 10M1 14l4.64 4.36A9 9 0 0 0 20.49 15'/></svg> Refresh Networks</button></form>";
const char HTTP_SAVED[]            PROGMEM = "<div class='msg card'><strong>Saving Credentials</strong><p class='subtitle mt-2'>Attempting to connect. The lamp will breathe amber while connecting. If it fails, this portal will reappear.</p></div>";
const char HTTP_PARAMSAVED[]       PROGMEM = "<div class='msg card'>Saved</div>";
const char HTTP_END[]              PROGMEM = "</div></body></html>";
const char HTTP_ERASEBTN[]         PROGMEM = "";
const char HTTP_UPDATEBTN[]        PROGMEM = "";
const char HTTP_BACKBTN[]          PROGMEM = "<form action='/' method='get'><button class='ghost-btn'>Cancel</button></form>";

const char HTTP_STATUS_ON[]        PROGMEM = "<div class='msg card'><strong>Connected</strong> to {v}</div>";
const char HTTP_STATUS_OFF[]       PROGMEM = "<div class='msg card {c}'><strong>Not connected</strong> to {v}{r}</div>"; 
const char HTTP_STATUS_OFFPW[]     PROGMEM = "<br/><span class='err'>Authentication failure</span>"; 
const char HTTP_STATUS_OFFNOAP[]   PROGMEM = "<br/><span class='err'>Network not found</span>"; 
const char HTTP_STATUS_OFFFAIL[]   PROGMEM = "<br/><span class='err'>Could not connect</span>"; 
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

const char S_brand[]              PROGMEM = "Sunrise Setup";
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