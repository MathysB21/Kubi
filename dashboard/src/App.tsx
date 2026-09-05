import {
  Clock,
  Timer,
  Smile,
  Calendar,
  Activity,
  RefreshCw,
  Wifi,
  Battery,
  Thermometer,
  BellRing,
  Play,
  Pause,
  SkipForward,
  RotateCcw,
  Sliders,
  Palette,
} from "lucide-react";
import { Accordion } from "./components/Accordion";
import {
  useQuery,
  useMutation,
  useQueryClient,
  QueryClient,
  QueryClientProvider,
} from "@tanstack/react-query";
import { Toaster, toast } from "sonner";
import { useState } from "react";
import { SkyArc } from "./components/SkyArc";
import { Link } from "react-router";

// Detailed Pomodoro state
interface PomodoroState {
  phase: number;
  phaseName: string;
  remaining: number;
  total: number;
  isPaused: boolean;
  hasChimed: boolean;
  cycle: number;
  cycleTarget: number;
  focusMin: number;
  shortBreakMin: number;
  longBreakMin: number;
  colorWork: string;
  colorShort: string;
  colorLong: string;
}

// Shape of Kubi API state
interface KubiState {
  mode: number;
  temp: number;
  battery: number;
  icalUrl: string;
  isNightMode: boolean;
  planetDawnH: number;
  planetDawnM: number;
  planetDuskH: number;
  planetDuskM: number;
  alarmHour: number;
  alarmMinute: number;
  cfgSundownHour: number;
  cfgSundownMinute: number;
  pomodoro: PomodoroState;
  [key: string]: any;
}

const DEFAULT_STATE: KubiState = {
  mode: 0,
  temp: 22.0,
  battery: 100,
  icalUrl: "",
  isNightMode: false,
  planetDawnH: 6,
  planetDawnM: 0,
  planetDuskH: 18,
  planetDuskM: 0,
  alarmHour: 8,
  alarmMinute: 0,
  cfgSundownHour: 18,
  cfgSundownMinute: 0,
  pomodoro: {
    phase: 0,
    phaseName: "FOCUS",
    remaining: 25 * 60,
    total: 25 * 60,
    isPaused: false,
    hasChimed: false,
    cycle: 0,
    cycleTarget: 4,
    focusMin: 25,
    shortBreakMin: 5,
    longBreakMin: 15,
    colorWork: "#F59E0B",
    colorShort: "#10B981",
    colorLong: "#0EA5E9",
  },
};

const FACE_MODES = [
  { id: 0, name: "Face 1: Focus Clock", desc: "Minimal digital & analog clock with date", icon: Clock },
  { id: 1, name: "Face 2: Pomodoro", desc: "Auto focus timer with 8-bit chimes", icon: Timer },
  { id: 2, name: "Face 3: Mascot & Temp", desc: "Kubi routine & room telemetry", icon: Smile },
  { id: 3, name: "Face 4: Schedule", desc: "3-day Google Calendar agenda", icon: Calendar },
];

const PRESET_COLORS = [
  "#F59E0B", // Amber
  "#10B981", // Emerald
  "#0EA5E9", // Sky
  "#F43F5E", // Rose
  "#8B5CF6", // Violet
  "#06B6D4", // Cyan
  "#EAB308", // Yellow
  "#EC4899", // Pink
];

const queryClient = new QueryClient();

function formatTime(totalSecs: number) {
  const mins = Math.floor(totalSecs / 60);
  const secs = totalSecs % 60;
  return `${String(mins).padStart(2, "0")}:${String(secs).padStart(2, "0")}`;
}

function KubiDashboard() {
  const qc = useQueryClient();
  const [icalInput, setIcalInput] = useState("");

  // 1. Fetch system state
  const { data = DEFAULT_STATE, isLoading } = useQuery<KubiState>({
    queryKey: ["kubiState"],
    queryFn: async () => {
      try {
        const res = await fetch("/api/state");
        if (!res.ok) return DEFAULT_STATE;
        const json = await res.json();
        return {
          ...DEFAULT_STATE,
          ...json,
          pomodoro: {
            ...DEFAULT_STATE.pomodoro,
            ...(json.pomodoro || {}),
          },
        };
      } catch {
        return DEFAULT_STATE;
      }
    },
    refetchInterval: 1000, // 1Hz live polling for smooth timer sync
  });

  // 2. Settings mutation
  const mutation = useMutation({
    mutationFn: async (payload: Partial<KubiState> & Record<string, any>) => {
      const res = await fetch("/api/settings", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(payload),
      });
      if (!res.ok) throw new Error("Failed to update settings");
      return res.json();
    },
    onSuccess: () => {
      qc.invalidateQueries({ queryKey: ["kubiState"] });
      toast.success("Settings saved to Kubi");
    },
    onError: () => {
      toast.error("Could not communicate with Kubi");
    },
  });

  // 3. Direct Pomodoro actions mutation
  const pomodoroActionMutation = useMutation({
    mutationFn: async (action: "play" | "pause" | "toggle" | "skip" | "reset") => {
      const res = await fetch("/api/pomodoro/action", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ action }),
      });
      if (!res.ok) throw new Error("Action failed");
      return res.json();
    },
    onSuccess: () => {
      qc.invalidateQueries({ queryKey: ["kubiState"] });
    },
    onError: () => {
      toast.error("Pomodoro action failed");
    },
  });

  const activeFace = FACE_MODES[data.mode] || FACE_MODES[0];
  const pomo = data.pomodoro || DEFAULT_STATE.pomodoro;

  // Resolve current active color for Pomodoro
  let activePhaseColor = pomo.colorWork || "#F59E0B";
  if (pomo.phase === 1) activePhaseColor = pomo.colorShort || "#10B981";
  if (pomo.phase === 2) activePhaseColor = pomo.colorLong || "#0EA5E9";

  const accordionItems = [
    // POMODORO CONFIGURATION ITEM
    {
      headerContent: (
        <div className="flex items-center gap-3 font-medium text-zinc-100">
          <Sliders size={18} className="text-amber-500" /> Pomodoro Configuration
        </div>
      ),
      bodyContent: (
        <div className="space-y-5 text-sm">
          <p className="text-xs text-zinc-500 leading-relaxed">
            Customize phase durations, session targets, and the countdown text colors rendered on Kubi's display.
          </p>

          {/* DURATION SLIDERS */}
          <div className="space-y-4">
            <div className="space-y-1.5">
              <div className="flex justify-between text-xs">
                <span className="text-zinc-300 font-medium">Work / Focus Duration</span>
                <span className="font-mono text-amber-500">{pomo.focusMin} mins</span>
              </div>
              <input
                type="range"
                min="1"
                max="90"
                value={pomo.focusMin}
                onChange={(e) => mutation.mutate({ focusMin: Number(e.target.value) })}
                className="w-full h-2 bg-zinc-950 rounded-lg appearance-none cursor-pointer accent-amber-500"
              />
            </div>

            <div className="space-y-1.5">
              <div className="flex justify-between text-xs">
                <span className="text-zinc-300 font-medium">Short Break Duration</span>
                <span className="font-mono text-emerald-400">{pomo.shortBreakMin} mins</span>
              </div>
              <input
                type="range"
                min="1"
                max="30"
                value={pomo.shortBreakMin}
                onChange={(e) => mutation.mutate({ shortBreakMin: Number(e.target.value) })}
                className="w-full h-2 bg-zinc-950 rounded-lg appearance-none cursor-pointer accent-emerald-500"
              />
            </div>

            <div className="space-y-1.5">
              <div className="flex justify-between text-xs">
                <span className="text-zinc-300 font-medium">Long Break Duration</span>
                <span className="font-mono text-sky-400">{pomo.longBreakMin} mins</span>
              </div>
              <input
                type="range"
                min="5"
                max="60"
                value={pomo.longBreakMin}
                onChange={(e) => mutation.mutate({ longBreakMin: Number(e.target.value) })}
                className="w-full h-2 bg-zinc-950 rounded-lg appearance-none cursor-pointer accent-sky-500"
              />
            </div>

            <div className="space-y-1.5">
              <div className="flex justify-between text-xs">
                <span className="text-zinc-300 font-medium">Sessions Before Long Break</span>
                <span className="font-mono text-zinc-300">{pomo.cycleTarget} sessions</span>
              </div>
              <input
                type="range"
                min="2"
                max="8"
                value={pomo.cycleTarget}
                onChange={(e) => mutation.mutate({ cycleTarget: Number(e.target.value) })}
                className="w-full h-2 bg-zinc-950 rounded-lg appearance-none cursor-pointer accent-zinc-400"
              />
            </div>
          </div>

          {/* COLOR PICKERS */}
          <div className="pt-4 border-t border-zinc-800/80 space-y-3">
            <div className="flex items-center gap-2 mb-1">
              <Palette size={14} className="text-amber-500" />
              <span className="text-xs font-semibold text-zinc-300 uppercase tracking-wider">
                Display Text Colors
              </span>
            </div>

            {/* Work Color */}
            <div className="space-y-1.5">
              <div className="flex justify-between text-xs items-center">
                <span className="text-zinc-400">Work Mode Text</span>
                <span className="w-3.5 h-3.5 rounded-full border border-zinc-700" style={{ backgroundColor: pomo.colorWork }} />
              </div>
              <div className="flex gap-2">
                {PRESET_COLORS.map((col) => (
                  <button
                    key={col}
                    onClick={() => mutation.mutate({ colorWork: col })}
                    className={`w-6 h-6 rounded-full border cursor-pointer transition-transform hover:scale-110 ${
                      pomo.colorWork === col ? "border-white ring-2 ring-white/40 scale-105" : "border-transparent"
                    }`}
                    style={{ backgroundColor: col }}
                  />
                ))}
              </div>
            </div>

            {/* Short Break Color */}
            <div className="space-y-1.5 pt-1">
              <div className="flex justify-between text-xs items-center">
                <span className="text-zinc-400">Short Break Text</span>
                <span className="w-3.5 h-3.5 rounded-full border border-zinc-700" style={{ backgroundColor: pomo.colorShort }} />
              </div>
              <div className="flex gap-2">
                {PRESET_COLORS.map((col) => (
                  <button
                    key={col}
                    onClick={() => mutation.mutate({ colorShort: col })}
                    className={`w-6 h-6 rounded-full border cursor-pointer transition-transform hover:scale-110 ${
                      pomo.colorShort === col ? "border-white ring-2 ring-white/40 scale-105" : "border-transparent"
                    }`}
                    style={{ backgroundColor: col }}
                  />
                ))}
              </div>
            </div>

            {/* Long Break Color */}
            <div className="space-y-1.5 pt-1">
              <div className="flex justify-between text-xs items-center">
                <span className="text-zinc-400">Long Break Text</span>
                <span className="w-3.5 h-3.5 rounded-full border border-zinc-700" style={{ backgroundColor: pomo.colorLong }} />
              </div>
              <div className="flex gap-2">
                {PRESET_COLORS.map((col) => (
                  <button
                    key={col}
                    onClick={() => mutation.mutate({ colorLong: col })}
                    className={`w-6 h-6 rounded-full border cursor-pointer transition-transform hover:scale-110 ${
                      pomo.colorLong === col ? "border-white ring-2 ring-white/40 scale-105" : "border-transparent"
                    }`}
                    style={{ backgroundColor: col }}
                  />
                ))}
              </div>
            </div>
          </div>
        </div>
      ),
    },
    // HARDWARE & TELEMETRY ITEM
    {
      headerContent: (
        <div className="flex items-center gap-3 font-medium text-zinc-100">
          <Activity size={18} className="text-amber-500" /> Hardware & Telemetry
        </div>
      ),
      bodyContent: (
        <div className="space-y-4">
          <div className="flex justify-between items-center mb-2">
            <p className="text-xs text-zinc-500">Live telemetry from Core 0 / Core 1</p>
            <button
              onClick={() => qc.invalidateQueries({ queryKey: ["kubiState"] })}
              className="flex items-center gap-1.5 px-2.5 py-1 bg-zinc-800 hover:bg-zinc-700 text-zinc-300 text-xs rounded-lg transition-colors cursor-pointer"
            >
              <RefreshCw size={12} />
              Resync
            </button>
          </div>
          <div className="grid grid-cols-2 gap-3 text-sm">
            <div className="bg-zinc-950 p-3 rounded-xl border border-zinc-800/50">
              <span className="text-zinc-500 text-xs block mb-1 flex items-center gap-1">
                <Thermometer size={12} className="text-amber-500" /> ROOM TEMP
              </span>
              <span className="font-mono text-amber-500">{data.temp.toFixed(1)} °C</span>
            </div>
            <div className="bg-zinc-950 p-3 rounded-xl border border-zinc-800/50">
              <span className="text-zinc-500 text-xs block mb-1 flex items-center gap-1">
                <Battery size={12} className="text-amber-500" /> BATTERY
              </span>
              <span className="font-mono text-zinc-300">{data.battery}%</span>
            </div>
            <div className="bg-zinc-950 p-3 rounded-xl border border-zinc-800/50">
              <span className="text-zinc-500 text-xs block mb-1 flex items-center gap-1">
                <Wifi size={12} className="text-amber-500" /> NETWORK
              </span>
              <span className="font-mono text-zinc-300">kubi.local</span>
            </div>
            <div className="bg-zinc-950 p-3 rounded-xl border border-zinc-800/50">
              <span className="text-zinc-500 text-xs block mb-1 flex items-center gap-1">
                <BellRing size={12} className="text-amber-500" /> ALERTS API
              </span>
              <span className="font-mono text-zinc-300">/api/override</span>
            </div>
          </div>
        </div>
      ),
    },
  ];

  if (isLoading) {
    return (
      <div className="min-h-screen bg-zinc-950 text-amber-500 flex items-center justify-center tracking-widest uppercase text-sm">
        Connecting to Kubi...
      </div>
    );
  }

  return (
    <div className="min-h-screen bg-zinc-950 text-zinc-100 p-6 pb-24 font-montserrat">
      <div className="max-w-md mx-auto space-y-6">
        {/* SKY ARC COMPONENT */}
        <SkyArc data={data} />

        {/* HEADER */}
        <header className="space-y-1 mb-6 mt-4 text-center">
          <h1 className="text-4xl font-medium tracking-[0.3em] text-center">
            KUBI
          </h1>
          <p className="text-zinc-500 text-sm tracking-wide text-center">
            Desk Companion
          </p>
        </header>

        {/* ================================================================= */}
        {/* DYNAMIC CONTEXTUAL SECTION (RESPONDS TO ACTIVE STATE OF CUBE)   */}
        {/* ================================================================= */}
        {data.mode === 1 ? (
          // --- POMODORO CONTEXTUAL HERO ---
          <section className="bg-zinc-900 border border-zinc-800 rounded-3xl p-6 shadow-2xl relative overflow-hidden transition-all">
            {/* Ambient Background Glow matching active phase color */}
            <div
              className="absolute -top-12 -right-12 w-40 h-40 rounded-full blur-3xl opacity-25 pointer-events-none transition-all duration-500"
              style={{ backgroundColor: activePhaseColor }}
            />

            <div className="flex justify-between items-center mb-2">
              <div className="flex items-center gap-2">
                <span
                  className="w-2.5 h-2.5 rounded-full animate-pulse"
                  style={{ backgroundColor: activePhaseColor }}
                />
                <span
                  className="text-xs font-bold tracking-widest uppercase"
                  style={{ color: activePhaseColor }}
                >
                  {pomo.phaseName}
                </span>
              </div>
              <span className="text-xs font-medium text-zinc-400 bg-zinc-950 px-2.5 py-1 rounded-full border border-zinc-800">
                Session {pomo.cycle + 1} of {pomo.cycleTarget}
              </span>
            </div>

            {/* Countdown */}
            <div className="text-center py-4">
              <div
                className="text-6xl font-light tabular-nums tracking-tight font-mono transition-colors"
                style={{ color: activePhaseColor }}
              >
                {formatTime(pomo.remaining)}
              </div>
              <p className="text-xs text-zinc-500 mt-2 font-medium tracking-wide">
                {pomo.isPaused ? "PAUSED (TAP CUBE TO RESUME)" : "TICKING ON KUBI SCREEN"}
              </p>
            </div>

            {/* Progress Bar */}
            <div className="w-full bg-zinc-950 h-2 rounded-full overflow-hidden mb-6 border border-zinc-800">
              <div
                className="h-full rounded-full transition-all duration-300"
                style={{
                  width: `${Math.min(100, Math.max(0, ((pomo.total - pomo.remaining) / (pomo.total || 1)) * 100))}%`,
                  backgroundColor: activePhaseColor,
                }}
              />
            </div>

            {/* Action Buttons: Play/Pause, Skip, Reset */}
            <div className="grid grid-cols-3 gap-2.5">
              <button
                onClick={() => pomodoroActionMutation.mutate(pomo.isPaused ? "play" : "pause")}
                className="py-2.5 flex items-center justify-center gap-1.5 rounded-xl text-xs font-medium bg-zinc-950 hover:bg-zinc-800 text-zinc-200 transition-colors cursor-pointer border border-zinc-800"
              >
                {pomo.isPaused ? <Play size={14} className="fill-current text-amber-500" /> : <Pause size={14} className="text-amber-500" />}
                {pomo.isPaused ? "Resume" : "Pause"}
              </button>
              <button
                onClick={() => pomodoroActionMutation.mutate("skip")}
                className="py-2.5 flex items-center justify-center gap-1.5 rounded-xl text-xs font-medium bg-zinc-950 hover:bg-zinc-800 text-zinc-200 transition-colors cursor-pointer border border-zinc-800"
              >
                <SkipForward size={14} className="text-sky-400" />
                Skip
              </button>
              <button
                onClick={() => pomodoroActionMutation.mutate("reset")}
                className="py-2.5 flex items-center justify-center gap-1.5 rounded-xl text-xs font-medium bg-zinc-950 hover:bg-zinc-800 text-zinc-200 transition-colors cursor-pointer border border-zinc-800"
              >
                <RotateCcw size={14} className="text-zinc-400" />
                Reset
              </button>
            </div>
          </section>
        ) : (
          // --- NON-POMODORO CONTEXTUAL CARD ---
          <section className="bg-zinc-900 border border-zinc-800 rounded-3xl p-5 shadow-xl flex items-center justify-between">
            <div className="flex items-center gap-3.5">
              <div className="p-3 bg-amber-500/10 border border-amber-500/30 rounded-2xl text-amber-500">
                <activeFace.icon size={22} />
              </div>
              <div>
                <p className="text-[11px] font-semibold uppercase tracking-wider text-zinc-500">
                  Current Face Orientation
                </p>
                <h2 className="text-base font-medium text-zinc-100">{activeFace.name}</h2>
              </div>
            </div>
            <button
              onClick={() => mutation.mutate({ mode: 1 })}
              className="px-3 py-1.5 bg-zinc-800 hover:bg-zinc-700 text-amber-500 border border-amber-500/30 rounded-xl text-xs font-medium transition-colors cursor-pointer"
            >
              Start Pomodoro
            </button>
          </section>
        )}

        {/* 1. OPERATING MODE SELECTOR (MANUAL OVERRIDE) */}
        <section className="bg-zinc-900 border border-zinc-800 rounded-3xl p-6 shadow-xl space-y-4">
          <div className="flex justify-between items-center">
            <div>
              <p className="text-zinc-500 text-xs font-semibold tracking-wider mb-1">
                ORIENTATION FACES
              </p>
              <h2 className="text-lg font-medium text-zinc-200">
                Cube Modes
              </h2>
            </div>
            <span className="px-2.5 py-1 text-xs font-medium rounded-full bg-amber-500/10 text-amber-500 border border-amber-500/30">
              Face {data.mode + 1}
            </span>
          </div>

          <div className="grid grid-cols-2 gap-2.5 pt-2">
            {FACE_MODES.map((mode) => {
              const Icon = mode.icon;
              const isActive = data.mode === mode.id;
              return (
                <button
                  key={mode.id}
                  onClick={() => mutation.mutate({ mode: mode.id })}
                  className={`p-3 text-left rounded-2xl transition-all border cursor-pointer ${
                    isActive
                      ? "bg-amber-500/10 border-amber-500/50 text-amber-500 shadow-[0_0_15px_rgba(245,158,11,0.15)]"
                      : "bg-zinc-950/60 border-zinc-800 text-zinc-400 hover:text-zinc-200 hover:border-zinc-700"
                  }`}
                >
                  <div className="flex items-center gap-2 mb-1">
                    <Icon size={16} />
                    <span className="text-xs font-semibold">{mode.name.split(":")[1] || mode.name}</span>
                  </div>
                  <p className="text-[11px] text-zinc-500 leading-tight line-clamp-1">{mode.desc}</p>
                </button>
              );
            })}
          </div>
        </section>

        {/* 2. GOOGLE CALENDAR ICAL SYNC */}
        <section className="bg-zinc-900 border border-zinc-800 rounded-3xl p-6 shadow-xl space-y-4">
          <div className="flex items-center gap-2 mb-1">
            <Calendar size={18} className="text-amber-500" />
            <h2 className="text-lg font-medium text-zinc-100">Google Calendar Sync</h2>
          </div>
          <p className="text-zinc-500 text-xs leading-relaxed">
            Paste your private iCal (.ics) link from Google Calendar settings. Kubi syncs upcoming events every hour.
          </p>
          <div className="space-y-2">
            <input
              type="text"
              placeholder="https://calendar.google.com/calendar/ical/.../basic.ics"
              value={icalInput || data.icalUrl}
              onChange={(e) => setIcalInput(e.target.value)}
              className="w-full bg-zinc-950 border border-zinc-800 rounded-xl p-3 text-xs text-zinc-200 placeholder-zinc-700 focus:outline-none focus:border-amber-500/50"
            />
            <button
              onClick={() => {
                mutation.mutate({ icalUrl: icalInput });
                toast.success("Calendar URL saved");
              }}
              className="w-full py-2.5 bg-amber-500/10 hover:bg-amber-500/20 text-amber-500 border border-amber-500/30 rounded-xl text-xs font-medium transition-colors cursor-pointer"
            >
              Save &amp; Sync Calendar
            </button>
          </div>
        </section>

        {/* 3. ACCORDION (POMODORO SETTINGS + HARDWARE TELEMETRY) */}
        <div className="pt-2">
          <Accordion items={accordionItems} />
        </div>

        {/* 4. MANUAL & SIMULATOR LINKS */}
        <div className="pt-12 flex items-center justify-center gap-6">
          <Link to="/sim" className="text-amber-500 hover:text-amber-400 text-xs font-semibold tracking-widest uppercase transition-colors flex items-center gap-1.5">
            <span className="w-2 h-2 rounded-full bg-amber-500 animate-pulse" />
            Launch Virtual Hardware Workbench
          </Link>
          <span className="text-zinc-700">•</span>
          <Link to="/manual" className="text-zinc-400 hover:text-amber-500 text-xs underline tracking-widest uppercase transition-colors">
            Kubi User Manual &amp; Gestures Guide
          </Link>
        </div>
      </div>
    </div>
  );
}

export default function App() {
  return (
    <QueryClientProvider client={queryClient}>
      <KubiDashboard />
      <Toaster position="bottom-center" richColors closeButton />
    </QueryClientProvider>
  );
}
