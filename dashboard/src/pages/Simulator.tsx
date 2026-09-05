import { useState, useEffect, useRef } from "react";
import { Link } from "react-router";
import {
  Clock,
  Timer,
  Smile,
  Calendar,
  Volume2,
  VolumeX,
  RotateCw,
  Hand,
  Zap,
  Flame,
  Thermometer,
  Battery,
  ArrowLeft,
  Sliders,
  Send,
  Compass,
} from "lucide-react";
import { playSimChime } from "../lib/audioChimes";
import KubiDashboardApp from "../App";

interface SimState {
  accelX: number;
  accelY: number;
  accelZ: number;
  temp: number;
  battery: number;
  rotation: number;
  backlight: number;
  lastChime: string;
  lastChimeTime: number;
  isSleeping: boolean;
  mode: number;
  hour?: number;
  minute?: number;
}

export default function Simulator() {
  const canvasRef = useRef<HTMLCanvasElement | null>(null);
  const [simState, setSimState] = useState<SimState | null>(null);
  const [isConnected, setIsConnected] = useState(false);
  const [isAudioEnabled, setIsAudioEnabled] = useState(true);
  const [lastHandledChimeTime, setLastHandledChimeTime] = useState(0);
  const [autoRotate, setAutoRotate] = useState(true);
  const [timeMode, setTimeMode] = useState<"real" | "manual">("real");

  const getRotationDeg = (rot: number) => {
    if (!autoRotate) return 0;
    switch (rot) {
      case 1: return -90;
      case 2: return 180;
      case 3: return 90;
      default: return 0;
    }
  };

  // Local interactive controls
  const [tempInput, setTempInput] = useState(22.0);
  const [batteryInput, setBatteryInput] = useState(100);
  const [hourInput, setHourInput] = useState(14);
  const [minuteInput, setMinuteInput] = useState(30);
  const [accelX, setAccelX] = useState(0.0);
  const [accelY, setAccelY] = useState(0.0);
  const [accelZ, setAccelZ] = useState(9.8);
  const [activeFace, setActiveFace] = useState(0);

  const [overrideMsg, setOverrideMsg] = useState("");
  const [activeTab, setActiveTab] = useState<"controls" | "dashboard">("controls");

  // Robust fetch with automatic direct-connection fallback to 127.0.0.1:8080
  const apiFetch = async (path: string, init?: RequestInit): Promise<Response> => {
    try {
      const res = await fetch(path, init);
      if (res.ok) return res;
    } catch {
      // Fallback below
    }
    return fetch(`http://127.0.0.1:8080${path}`, init);
  };

  // 1. Frame rendering loop (~25 FPS)
  useEffect(() => {
    let active = true;
    let frameReq: number;

    const renderLoop = async () => {
      if (!active) return;
      try {
        const res = await apiFetch("/sim/frame");
        if (res.ok) {
          const buffer = await res.arrayBuffer();
          if (buffer.byteLength === 240 * 320 * 4 && canvasRef.current) {
            const ctx = canvasRef.current.getContext("2d");
            if (ctx) {
              const imgData = ctx.createImageData(240, 320);
              imgData.data.set(new Uint8ClampedArray(buffer));
              ctx.putImageData(imgData, 0, 0);
            }
          }
        }
      } catch (err) {
        console.warn("Frame stream error:", err);
      }

      if (active) {
        frameReq = window.setTimeout(renderLoop, 40);
      }
    };

    renderLoop();

    return () => {
      active = false;
      clearTimeout(frameReq);
    };
  }, []);

  // 2. Poll simulator telemetry & heartbeat (~3Hz)
  useEffect(() => {
    let isMounted = true;

    const pollState = async () => {
      try {
        const res = await apiFetch("/sim/state");
        if (res.ok) {
          if (isMounted) setIsConnected(true);
          const data: SimState = await res.json();
          if (isMounted) {
            setSimState(data);
            setActiveFace(data.mode);

            if (timeMode === "real" && data.hour !== undefined && data.minute !== undefined) {
              setHourInput(data.hour);
              setMinuteInput(data.minute);
            }

            // Audio chime detection
            if (
              isAudioEnabled &&
              data.lastChime &&
              data.lastChime !== "NONE" &&
              data.lastChimeTime > lastHandledChimeTime
            ) {
              setLastHandledChimeTime(data.lastChimeTime);
              playSimChime(data.lastChime);
            }
          }
        } else {
          if (isMounted) setIsConnected(false);
        }
      } catch {
        if (isMounted) setIsConnected(false);
      }
    };

    pollState();
    const interval = setInterval(pollState, 300);

    return () => {
      isMounted = false;
      clearInterval(interval);
    };
  }, [isAudioEnabled, lastHandledChimeTime]);

  // Inject helper
  const inject = async (payload: any) => {
    try {
      const res = await apiFetch("/sim/inject", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(payload),
      });
      if (res.ok) {
        const data = await res.json();
        if (
          isAudioEnabled &&
          data.lastChime &&
          data.lastChime !== "NONE" &&
          data.lastChimeTime > lastHandledChimeTime
        ) {
          setLastHandledChimeTime(data.lastChimeTime);
          playSimChime(data.lastChime);
        }
      }
    } catch (e) {
      console.error("Simulation injection failed:", e);
    }
  };

  // Face change
  const setFace = (faceIdx: number) => {
    setActiveFace(faceIdx);
    setSimState((prev) => (prev ? { ...prev, rotation: faceIdx, mode: faceIdx } : null));
    inject({ face: faceIdx });
  };

  // Gestures
  const triggerGesture = (gesture: "tap" | "shake" | "slam") => {
    inject({ gesture });
  };

  // Secret Override
  const sendOverride = async () => {
    if (!overrideMsg.trim()) return;
    try {
      await apiFetch("/api/override", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ message: overrideMsg }),
      });
      setOverrideMsg("");
    } catch (e) {
      console.error("Override failed:", e);
    }
  };

  return (
    <div className="min-h-screen bg-zinc-950 text-zinc-100 flex flex-col selection:bg-amber-500/30">
      {/* HEADER */}
      <header className="border-b border-zinc-800/80 bg-zinc-900/50 backdrop-blur px-6 py-3.5 flex items-center justify-between">
        <div className="flex items-center gap-4">
          <Link
            to="/"
            className="p-1.5 rounded-lg bg-zinc-800/80 hover:bg-zinc-700 text-zinc-300 transition-colors"
            title="Back to Dashboard"
          >
            <ArrowLeft className="w-4 h-4" />
          </Link>
          <div>
            <div className="flex items-center gap-2.5">
              <span className="font-mono font-bold tracking-widest text-amber-500 text-sm uppercase">
                KUBI DIGITAL TWIN
              </span>
              <span
                className={`w-2 h-2 rounded-full ${
                  isConnected ? "bg-emerald-500 animate-pulse" : "bg-red-500"
                }`}
              />
              <span className="text-xs text-zinc-400 font-mono">
                {isConnected ? "C++ RUNNER ACTIVE (127.0.0.1:8080)" : "RUNNER DISCONNECTED"}
              </span>
            </div>
            <p className="text-[11px] text-zinc-400 mt-0.5">
              Executing actual ESP32 C++ firmware routines in native desktop software
            </p>
          </div>
        </div>

        <div className="flex items-center gap-3">
          <button
            onClick={() => setIsAudioEnabled(!isAudioEnabled)}
            className={`px-3 py-1.5 rounded-lg text-xs font-medium flex items-center gap-1.5 border transition-all ${
              isAudioEnabled
                ? "bg-amber-500/10 border-amber-500/30 text-amber-400"
                : "bg-zinc-800/50 border-zinc-700 text-zinc-400"
            }`}
          >
            {isAudioEnabled ? <Volume2 className="w-3.5 h-3.5" /> : <VolumeX className="w-3.5 h-3.5" />}
            <span>8-Bit Audio {isAudioEnabled ? "ON" : "MUTED"}</span>
          </button>

          <div className="flex bg-zinc-900 rounded-lg p-0.5 border border-zinc-800 text-xs">
            <button
              onClick={() => setActiveTab("controls")}
              className={`px-3 py-1 rounded-md transition-colors ${
                activeTab === "controls"
                  ? "bg-amber-500 text-black font-semibold"
                  : "text-zinc-400 hover:text-zinc-200"
              }`}
            >
              Hardware Rig
            </button>
            <button
              onClick={() => setActiveTab("dashboard")}
              className={`px-3 py-1 rounded-md transition-colors ${
                activeTab === "dashboard"
                  ? "bg-amber-500 text-black font-semibold"
                  : "text-zinc-400 hover:text-zinc-200"
              }`}
            >
              Side-by-Side Dashboard
            </button>
          </div>
        </div>
      </header>

      {/* MAIN CONTENT AREA */}
      <div className="flex-1 flex overflow-hidden">
        {/* LEFT COLUMN: THE PHYSICAL KUBI CUBE & IPS DISPLAY */}
        <div className="w-full lg:w-[480px] border-r border-zinc-800/80 bg-zinc-950/80 p-8 flex flex-col items-center justify-center shrink-0">
          <div className="text-center mb-5 flex items-center justify-between w-full max-w-[370px]">
            <div className="text-left">
              <span className="text-[11px] font-mono tracking-widest text-zinc-400 uppercase">
                {autoRotate ? "Desk Companion View" : "Raw Panel View (240x320)"}
              </span>
              <p className="text-xs text-zinc-400 mt-0.5">Click screen to simulate direct tap</p>
            </div>
            <button
              onClick={() => setAutoRotate(!autoRotate)}
              className={`px-2.5 py-1 rounded-md text-[11px] font-mono flex items-center gap-1.5 border transition-all ${
                autoRotate
                  ? "bg-amber-500/20 border-amber-500/40 text-amber-300"
                  : "bg-zinc-800 border-zinc-700 text-zinc-400"
              }`}
              title="Automatically rotate the companion cube when tilted onto different faces"
            >
              <RotateCw className="w-3 h-3" />
              <span>{autoRotate ? "Auto-Orient: ON" : "Auto-Orient: OFF"}</span>
            </button>
          </div>

          {/* WOODEN CUBE BEZEL (10cm x 10cm DESK COMPANION CHASSIS) */}
          <div className="w-[370px] h-[370px] flex items-center justify-center relative select-none">
            <div
              className="relative w-[360px] h-[360px] rounded-3xl shadow-2xl border-4 flex flex-col items-center justify-center"
              style={{
                background: "linear-gradient(145deg, #2a1f18 0%, #17110e 100%)",
                borderColor: "#3d2e24",
                boxShadow: "0 25px 50px -12px rgba(0, 0, 0, 0.8), inset 0 2px 4px rgba(255,255,255,0.05)",
                transform: `rotate(${getRotationDeg(simState ? simState.rotation : 0)}deg)`,
                transition: "transform 0.6s cubic-bezier(0.34, 1.56, 0.64, 1)",
              }}
            >
              {/* Brass Corner Machine Screws */}
              <div className="absolute top-2.5 left-2.5 w-2.5 h-2.5 rounded-full bg-amber-700/60 border border-amber-600/80 shadow-inner" />
              <div className="absolute top-2.5 right-2.5 w-2.5 h-2.5 rounded-full bg-amber-700/60 border border-amber-600/80 shadow-inner" />
              <div className="absolute bottom-2.5 left-2.5 w-2.5 h-2.5 rounded-full bg-amber-700/60 border border-amber-600/80 shadow-inner" />
              <div className="absolute bottom-2.5 right-2.5 w-2.5 h-2.5 rounded-full bg-amber-700/60 border border-amber-600/80 shadow-inner" />

              {/* Inner Display Bezel */}
              <div
                onClick={() => triggerGesture("tap")}
                className="relative cursor-pointer group rounded-xl overflow-hidden p-1 bg-black border-2 border-zinc-800 shadow-inner transition-transform active:scale-[0.98]"
                title="Click to trigger gentle tap!"
              >
                {/* Glass Reflection Glare */}
                <div className="absolute inset-0 bg-gradient-to-tr from-transparent via-white/5 to-transparent pointer-events-none z-10" />

                {/* The Actual Canvas (240x320) */}
                <canvas
                  ref={canvasRef}
                  width={240}
                  height={320}
                  className="rounded-lg block transition-opacity duration-300"
                  style={{
                    imageRendering: "pixelated",
                    opacity: simState ? (simState.isSleeping ? 0.08 : simState.backlight / 255) : 1,
                    boxShadow: "0 0 20px rgba(0,0,0,0.9)",
                  }}
                />

                {/* Tap feedback indicator */}
                <div className="absolute inset-0 flex items-center justify-center pointer-events-none opacity-0 group-hover:opacity-100 transition-opacity">
                  <span className="bg-black/60 backdrop-blur text-amber-400 font-mono text-[10px] px-2 py-1 rounded-full border border-amber-500/30">
                    Tap to Interact
                  </span>
                </div>
              </div>

              {/* Logo on lower chassis */}
              <div className="text-center mt-1">
                <span className="font-mono text-[9px] tracking-[0.3em] text-amber-600/50 uppercase font-bold">
                  K U B I
                </span>
              </div>
            </div>
          </div>

          {/* Quick Hardware Status */}
          <div className="mt-6 w-full max-w-[280px] grid grid-cols-2 gap-2 text-xs font-mono">
            <div className="bg-zinc-900/80 border border-zinc-800/80 rounded-lg p-2.5">
              <div className="text-zinc-400 text-[10px]">Backlight PWM</div>
              <div className="text-zinc-200 font-bold mt-0.5">
                {simState ? `${Math.round((simState.backlight / 255) * 100)}%` : "100%"}
              </div>
            </div>
            <div className="bg-zinc-900/80 border border-zinc-800/80 rounded-lg p-2.5">
              <div className="text-zinc-400 text-[10px]">Active Rotation</div>
              <div className="text-zinc-200 font-bold mt-0.5">
                {simState ? `${simState.rotation * 90}°` : `${activeFace * 90}°`}
              </div>
            </div>
          </div>
        </div>

        {/* RIGHT AREA: TOGGLEABLE BETWEEN CONTROLS AND DASHBOARD */}
        <div className="flex-1 overflow-y-auto p-8">
          {activeTab === "controls" ? (
            <div className="max-w-4xl mx-auto space-y-8">
              {/* 1. PHYSICAL ORIENTATION / ACTIVE FACE SELECTION */}
              <section className="space-y-3">
                <div className="flex items-center justify-between">
                  <h3 className="text-sm font-semibold tracking-wide uppercase text-zinc-300 flex items-center gap-2">
                    <RotateCw className="w-4 h-4 text-amber-500" />
                    Cube Orientation (Face Up Selector)
                  </h3>
                  <span className="text-xs text-zinc-400">
                    Triggers ADXL345 tilt detection + 400ms debounce
                  </span>
                </div>

                <div className="grid grid-cols-2 sm:grid-cols-4 gap-3">
                  {[
                    {
                      id: 0,
                      title: "Face 1 UP",
                      mode: "Focus Clock",
                      desc: "Minimal time, calendar & ticker",
                      icon: Clock,
                    },
                    {
                      id: 1,
                      title: "Face 2 UP",
                      mode: "Pomodoro Timer",
                      desc: "Countdown with auto-start",
                      icon: Timer,
                    },
                    {
                      id: 2,
                      title: "Face 3 UP",
                      mode: "Mascot & Temp",
                      desc: "Kubi jelly & BMP280 room temp",
                      icon: Smile,
                    },
                    {
                      id: 3,
                      title: "Face 4 UP",
                      mode: "3-Day Schedule",
                      desc: "Google Calendar agenda",
                      icon: Calendar,
                    },
                  ].map((face) => {
                    const Icon = face.icon;
                    const isActive = activeFace === face.id;
                    return (
                      <button
                        key={face.id}
                        onClick={() => setFace(face.id)}
                        className={`p-4 rounded-xl text-left border transition-all ${
                          isActive
                            ? "bg-amber-500/10 border-amber-500 text-amber-400 shadow-lg shadow-amber-500/10"
                            : "bg-zinc-900/60 border-zinc-800 text-zinc-300 hover:border-zinc-700"
                        }`}
                      >
                        <div className="flex items-center justify-between mb-2">
                          <Icon className={`w-5 h-5 ${isActive ? "text-amber-400" : "text-zinc-400"}`} />
                          <span className="font-mono text-[10px] px-1.5 py-0.5 rounded bg-zinc-800 text-zinc-400">
                            {face.title}
                          </span>
                        </div>
                        <div className="font-semibold text-sm text-zinc-100">{face.mode}</div>
                        <div className="text-[11px] text-zinc-400 mt-1 line-clamp-2">{face.desc}</div>
                      </button>
                    );
                  })}
                </div>
              </section>

              {/* 2. PHYSICAL GESTURES TRIGGER */}
              <section className="space-y-3">
                <div className="flex items-center justify-between">
                  <h3 className="text-sm font-semibold tracking-wide uppercase text-zinc-300 flex items-center gap-2">
                    <Hand className="w-4 h-4 text-amber-500" />
                    Physical Manipulation Gestures
                  </h3>
                  <span className="text-xs text-zinc-400">
                    ADXL345 acceleration vectors &amp; transients
                  </span>
                </div>

                <div className="grid grid-cols-1 sm:grid-cols-3 gap-3">
                  <button
                    onClick={() => triggerGesture("tap")}
                    className="p-4 rounded-xl bg-zinc-900/60 border border-zinc-800 hover:border-amber-500/50 hover:bg-zinc-800/60 transition-all flex items-center gap-3 text-left"
                  >
                    <div className="w-10 h-10 rounded-lg bg-amber-500/10 border border-amber-500/20 flex items-center justify-center text-amber-400 shrink-0">
                      <Hand className="w-5 h-5" />
                    </div>
                    <div>
                      <div className="font-semibold text-sm text-zinc-100">Gentle Tap</div>
                      <div className="text-[11px] text-zinc-400">
                        Toggle details / Pause Pomodoro
                      </div>
                    </div>
                  </button>

                  <button
                    onClick={() => triggerGesture("shake")}
                    className="p-4 rounded-xl bg-zinc-900/60 border border-zinc-800 hover:border-amber-500/50 hover:bg-zinc-800/60 transition-all flex items-center gap-3 text-left"
                  >
                    <div className="w-10 h-10 rounded-lg bg-sky-500/10 border border-sky-500/20 flex items-center justify-center text-sky-400 shrink-0">
                      <Zap className="w-5 h-5" />
                    </div>
                    <div>
                      <div className="font-semibold text-sm text-zinc-100">Shake Cube</div>
                      <div className="text-[11px] text-zinc-400">
                        Skip Pomodoro phase immediately
                      </div>
                    </div>
                  </button>

                  <button
                    onClick={() => triggerGesture("slam")}
                    className="p-4 rounded-xl bg-zinc-900/60 border border-zinc-800 hover:border-rose-500/50 hover:bg-zinc-800/60 transition-all flex items-center gap-3 text-left"
                  >
                    <div className="w-10 h-10 rounded-lg bg-rose-500/10 border border-rose-500/20 flex items-center justify-center text-rose-400 shrink-0">
                      <Flame className="w-5 h-5" />
                    </div>
                    <div>
                      <div className="font-semibold text-sm text-zinc-100">Desk Slam!</div>
                      <div className="text-[11px] text-zinc-400">
                        Easter Egg: &quot;Ouch!&quot; chime &amp; reaction
                      </div>
                    </div>
                  </button>
                </div>
              </section>

              {/* 3. SENSOR & ENVIRONMENT SLIDERS */}
              <section className="space-y-4">
                <h3 className="text-sm font-semibold tracking-wide uppercase text-zinc-300 flex items-center gap-2">
                  <Sliders className="w-4 h-4 text-amber-500" />
                  Environmental Feeds &amp; Time Machine
                </h3>

                <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                  {/* LEFT COLUMN: Environment & Telemetry & Secret Alert */}
                  <div className="space-y-4">
                    {/* Temperature (BMP280) */}
                    <div className="bg-zinc-900/60 border border-zinc-800 rounded-xl p-4 space-y-3">
                      <div className="flex items-center justify-between text-xs">
                        <span className="font-medium text-zinc-300 flex items-center gap-1.5">
                          <Thermometer className="w-4 h-4 text-amber-400" />
                          BMP280 Room Temperature
                        </span>
                        <span className="font-mono text-amber-400 font-bold text-sm">
                          {tempInput.toFixed(1)} °C
                        </span>
                      </div>
                      <input
                        type="range"
                        min="10.0"
                        max="34.0"
                        step="0.5"
                        value={tempInput}
                        onChange={(e) => {
                          const val = parseFloat(e.target.value);
                          setTempInput(val);
                          inject({ temp: val });
                        }}
                        className="w-full accent-amber-500 cursor-pointer"
                      />
                      <div className="flex justify-between text-[10px] text-zinc-400 font-mono">
                        <span>10.0°C (Chilly)</span>
                        <span>22.0°C (Standard)</span>
                        <span>34.0°C (Hot)</span>
                      </div>
                    </div>

                    {/* Battery Level */}
                    <div className="bg-zinc-900/60 border border-zinc-800 rounded-xl p-4 space-y-3">
                      <div className="flex items-center justify-between text-xs">
                        <span className="font-medium text-zinc-300 flex items-center gap-1.5">
                          <Battery className="w-4 h-4 text-emerald-400" />
                          Battery Telemetry
                        </span>
                        <span className="font-mono text-emerald-400 font-bold text-sm">
                          {batteryInput}%
                        </span>
                      </div>
                      <input
                        type="range"
                        min="5"
                        max="100"
                        value={batteryInput}
                        onChange={(e) => {
                          const b = parseInt(e.target.value);
                          setBatteryInput(b);
                          inject({ battery: b });
                        }}
                        className="w-full accent-emerald-500 cursor-pointer"
                      />
                    </div>

                    {/* Secret Override Alert */}
                    <div className="bg-zinc-900/60 border border-zinc-800 rounded-xl p-4 space-y-3">
                      <div className="text-xs font-medium text-zinc-300 flex items-center gap-1.5">
                        <Send className="w-4 h-4 text-rose-400" />
                        Push Secret Alert Banner (/api/override)
                      </div>
                      <div className="flex gap-2">
                        <input
                          type="text"
                          value={overrideMsg}
                          onChange={(e) => setOverrideMsg(e.target.value)}
                          placeholder="e.g. Time for dinner brother!"
                          className="flex-1 bg-zinc-950 border border-zinc-800 rounded-lg px-3 py-1.5 text-xs text-zinc-200 placeholder-zinc-400 focus:outline-none focus:border-rose-500"
                          onKeyDown={(e) => e.key === "Enter" && sendOverride()}
                        />
                        <button
                          onClick={sendOverride}
                          className="px-3 py-1.5 rounded-lg bg-rose-500 hover:bg-rose-600 text-white text-xs font-semibold transition-colors shrink-0"
                        >
                          Push Alert
                        </button>
                      </div>
                    </div>
                  </div>

                  {/* RIGHT COLUMN: Virtual Clock Time Scrubber */}
                  <div className="bg-zinc-900/60 border border-zinc-800 rounded-xl p-4 flex flex-col justify-between space-y-3">
                    <div className="space-y-3">
                      <div className="flex items-center justify-between text-xs">
                        <span className="font-medium text-zinc-300 flex items-center gap-1.5">
                          <Clock className="w-4 h-4 text-sky-400" />
                          Time of Day Scrubber
                        </span>
                        <div className="flex items-center gap-2">
                          <button
                            onClick={() => {
                              if (timeMode === "real") {
                                setTimeMode("manual");
                                inject({ hour: hourInput, minute: minuteInput });
                              } else {
                                setTimeMode("real");
                                inject({ hour: -1, minute: -1 });
                              }
                            }}
                            className={`px-2 py-0.5 rounded text-[10px] font-mono border transition-all ${
                              timeMode === "manual"
                                ? "bg-sky-500/20 border-sky-500/50 text-sky-300 font-semibold"
                                : "bg-zinc-800 border-zinc-700 text-zinc-400 hover:text-zinc-200"
                            }`}
                            title="Toggle between live PC clock and manual simulation scrubber"
                          >
                            {timeMode === "manual" ? "Manual Override: ON" : "System Time: LIVE"}
                          </button>
                          <span className="font-mono text-sky-400 font-bold text-sm">
                            {String(hourInput).padStart(2, "0")}:{String(minuteInput).padStart(2, "0")}
                          </span>
                        </div>
                      </div>

                      <p className="text-[11px] text-zinc-400 leading-relaxed">
                        Controls virtual time for <strong>Face 1 (Clock Face)</strong> and <strong>Face 3 (Mascot Routine)</strong>: Sleep (Zzz) 23:00–06:00, Morning Coffee 06:00–10:00, Focus 10:00–23:00.
                      </p>

                      <div className="space-y-1.5">
                        <div className="text-[10px] text-zinc-400 font-mono">Hour: {hourInput}</div>
                        <input
                          type="range"
                          min="0"
                          max="23"
                          value={hourInput}
                          onChange={(e) => {
                            const h = parseInt(e.target.value);
                            setHourInput(h);
                            setTimeMode("manual");
                            inject({ hour: h, minute: minuteInput });
                          }}
                          className="w-full accent-sky-500 cursor-pointer"
                        />
                      </div>
                      <div className="space-y-1.5">
                        <div className="text-[10px] text-zinc-400 font-mono">Minute: {minuteInput}</div>
                        <input
                          type="range"
                          min="0"
                          max="59"
                          value={minuteInput}
                          onChange={(e) => {
                            const m = parseInt(e.target.value);
                            setMinuteInput(m);
                            setTimeMode("manual");
                            inject({ hour: hourInput, minute: m });
                          }}
                          className="w-full accent-sky-500 cursor-pointer"
                        />
                      </div>
                    </div>

                    {/* Quick Routine Presets */}
                    <div className="flex flex-wrap gap-1.5 pt-2 border-t border-zinc-800/60">
                      <button
                        onClick={() => {
                          setTimeMode("manual");
                          setHourInput(2);
                          setMinuteInput(0);
                          inject({ hour: 2, minute: 0 });
                        }}
                        className="px-2 py-1 rounded bg-zinc-800/80 hover:bg-zinc-700 text-zinc-300 border border-zinc-700/60 text-[10px] font-mono transition-colors"
                      >
                        02:00 (Mascot Sleep)
                      </button>
                      <button
                        onClick={() => {
                          setTimeMode("manual");
                          setHourInput(8);
                          setMinuteInput(0);
                          inject({ hour: 8, minute: 0 });
                        }}
                        className="px-2 py-1 rounded bg-zinc-800/80 hover:bg-zinc-700 text-zinc-300 border border-zinc-700/60 text-[10px] font-mono transition-colors"
                      >
                        08:00 (Coffee Morning)
                      </button>
                      <button
                        onClick={() => {
                          setTimeMode("manual");
                          setHourInput(14);
                          setMinuteInput(0);
                          inject({ hour: 14, minute: 0 });
                        }}
                        className="px-2 py-1 rounded bg-zinc-800/80 hover:bg-zinc-700 text-zinc-300 border border-zinc-700/60 text-[10px] font-mono transition-colors"
                      >
                        14:00 (Focus Reading)
                      </button>
                      {timeMode === "manual" && (
                        <button
                          onClick={() => {
                            setTimeMode("real");
                            inject({ hour: -1, minute: -1 });
                          }}
                          className="px-2 py-1 rounded bg-amber-500/10 hover:bg-amber-500/20 text-amber-400 border border-amber-500/30 text-[10px] font-mono transition-colors ml-auto"
                        >
                          Reset to Live PC Time
                        </button>
                      )}
                    </div>
                  </div>

                  {/* 3-Axis Accelerometer (ADXL345) */}
                  <div className="bg-zinc-900/60 border border-zinc-800 rounded-xl p-4 space-y-3 md:col-span-2">
                    <div className="flex items-center justify-between text-xs">
                      <span className="font-medium text-zinc-300 flex items-center gap-1.5">
                        <Compass className="w-4 h-4 text-amber-400" />
                        ADXL345 3-Axis Accelerometer Vector Injection
                      </span>
                      <span className="font-mono text-xs text-zinc-400">
                        X: {accelX.toFixed(1)} | Y: {accelY.toFixed(1)} | Z: {accelZ.toFixed(1)} m/s²
                      </span>
                    </div>
                    <div className="grid grid-cols-3 gap-4 text-xs font-mono">
                      <div>
                        <div className="text-[10px] text-zinc-400 mb-1">X-Axis ({accelX.toFixed(1)})</div>
                        <input
                          type="range"
                          min="-15.0"
                          max="15.0"
                          step="0.5"
                          value={accelX}
                          onChange={(e) => {
                            const val = parseFloat(e.target.value);
                            setAccelX(val);
                            inject({ accelX: val, accelY, accelZ });
                          }}
                          className="w-full accent-amber-500 cursor-pointer"
                        />
                      </div>
                      <div>
                        <div className="text-[10px] text-zinc-400 mb-1">Y-Axis ({accelY.toFixed(1)})</div>
                        <input
                          type="range"
                          min="-15.0"
                          max="15.0"
                          step="0.5"
                          value={accelY}
                          onChange={(e) => {
                            const val = parseFloat(e.target.value);
                            setAccelY(val);
                            inject({ accelX, accelY: val, accelZ });
                          }}
                          className="w-full accent-amber-500 cursor-pointer"
                        />
                      </div>
                      <div>
                        <div className="text-[10px] text-zinc-400 mb-1">Z-Axis ({accelZ.toFixed(1)})</div>
                        <input
                          type="range"
                          min="-15.0"
                          max="15.0"
                          step="0.5"
                          value={accelZ}
                          onChange={(e) => {
                            const val = parseFloat(e.target.value);
                            setAccelZ(val);
                            inject({ accelX, accelY, accelZ: val });
                          }}
                          className="w-full accent-amber-500 cursor-pointer"
                        />
                      </div>
                    </div>
                  </div>
                </div>
              </section>
            </div>
          ) : (
            /* SIDE-BY-SIDE LIVE DASHBOARD */
            <div className="h-full">
              <div className="mb-4 flex items-center justify-between">
                <div>
                  <h3 className="text-sm font-semibold text-zinc-200">
                    Live React Companion Dashboard
                  </h3>
                  <p className="text-xs text-zinc-400">
                    Changes here communicate directly with your running C++ firmware!
                  </p>
                </div>
              </div>
              <div className="border border-zinc-800 rounded-2xl p-6 bg-zinc-900/40">
                <KubiDashboardApp />
              </div>
            </div>
          )}
        </div>
      </div>
    </div>
  );
}
