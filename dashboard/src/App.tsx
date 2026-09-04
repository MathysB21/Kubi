import {
  Sun,
  Moon,
  Shield,
  Settings2,
  Waves,
  Clock,
  Activity,
  RefreshCw,
  MapPin,
  Globe,
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
import { useEffect, useState } from "react";
import { SkyArc } from "./components/SkyArc";
import { Link } from "react-router";

// Define the shape of our API payload
interface LampPayload {
  [key: string]: number | boolean | string | number[] | undefined;
}

const MODE_MAP = [
  "Auto Day",
  "Auto Night",
  "Manual Day",
  "Manual Night",
  "Night Light",
  "Sunrise",
  "Sundown",
  "Proximity",
  "Away",
];

// Helper to translate WWA Temp to Text
const getTempName = (temp: number) => {
  if (temp === 0) return "Amber";
  if (temp === 1) return "Warm White";
  if (temp === 2) return "Cool White";
  return "Unknown";
};

const queryClient = new QueryClient();

const DayPicker = ({
  days,
  onChange,
  color,
}: {
  days: number[];
  onChange: (d: number[]) => void;
  color: "amber" | "sky";
}) => {
  // SAFETY NET: If API misses the array, fall back to a valid 7-day array
  const safeDays =
    Array.isArray(days) && days.length === 7 ? days : [0, 1, 1, 1, 1, 1, 0];

  const toggleDay = (index: number) => {
    const newDays = [...safeDays];
    newDays[index] = (newDays[index] + 1) % 3; // Cycles 0 -> 1 -> 2 -> 0
    onChange(newDays);
  };

  const getStyles = (state: number) => {
    if (state === 0)
      return "bg-zinc-950 text-zinc-500 border border-zinc-800 hover:text-zinc-400"; // OFF
    if (state === 1)
      return `bg-transparent text-${color}-500 border border-dashed border-${color}-500/50`; // AUTO
    return `bg-${color}-500 text-zinc-950 shadow-[0_0_12px_rgba(var(--${color}-500-rgb),0.4)] border border-${color}-500`; // MANUAL
  };

  const getStateLabel = (state: number) => {
    if (state === 0) return "Off";
    if (state === 1) return "Auto (Planet Sync)";
    return "Manual (Target Time)";
  };

  return (
    <div className="flex justify-between items-center pt-2">
      {["S", "M", "T", "W", "T", "F", "S"].map((day, index) => (
        <button
          key={index}
          onClick={() => toggleDay(index)}
          title={getStateLabel(safeDays[index])}
          className={`w-10 h-10 rounded-full text-sm font-medium transition-all cursor-pointer ${getStyles(days[index])}`}
        >
          {day}
        </button>
      ))}
    </div>
  );
};

interface ConfigSliderProps {
  label: string;
  value: number;
  min: number;
  max: number;
  onChange: (val: number) => void;
  onRelease: (val: number) => void;
}

const ConfigSlider = ({
  label,
  value,
  min,
  max,
  onChange,
  onRelease,
}: ConfigSliderProps) => (
  <div className="space-y-2 mt-4">
    <div className="flex justify-between text-sm">
      <span className="text-zinc-300">{label}</span>
      <span className="text-amber-500 font-mono">{value}</span>
    </div>
    <input
      type="range"
      min={min}
      max={max}
      value={value}
      onChange={(e) => onChange(Number(e.target.value))}
      onMouseUp={() => onRelease(value)}
      onTouchEnd={() => onRelease(value)}
      className="w-full h-2 bg-zinc-950 rounded-lg appearance-none cursor-pointer accent-amber-500"
    />
  </div>
);

function LampDashboard() {
  const qc = useQueryClient();

  // --- LOCATION SEARCH STATE ---
  const [citySearch, setCitySearch] = useState("");
  const [isSearching, setIsSearching] = useState(false);

  // --- HARDWARE DIAGNOSTICS POLLING ---
  const {
    data: diagnostics,
    isFetching: isFetchingDiag,
    refetch: refetchDiagnostics,
    isError: isDiagError,
    error: diagError,
  } = useQuery({
    queryKey: ["diagnostics"],
    queryFn: async () => {
      const res = await fetch("/api/diagnostics");
      if (!res.ok) throw new Error("Failed to fetch diagnostics");
      return res.json();
    },
    refetchOnMount: true,
  });

  // Fire the toast when the error state changes
  useEffect(() => {
    if (isDiagError && diagError) {
      toast.error(`Diagnostics Error`, { description: diagError.message });
    }
  }, [isDiagError, diagError]);

  // --- BACKGROUND POLLING ---
  const {
    data: state,
    isLoading,
    isError: isStateError,
    error: stateError,
  } = useQuery({
    queryKey: ["lampState"],
    queryFn: async () => {
      const res = await fetch("/api/state");
      if (!res.ok) throw new Error("Network response was not ok");
      return res.json();
    },
    refetchInterval: 5000, // Poll every 5 seconds automatically
    refetchOnMount: true,
  });

  // Fire the toast when the error state changes
  useEffect(() => {
    if (isStateError && stateError) {
      toast.error(`Lamp State Error`, { description: stateError.message });
    }
  }, [isStateError, stateError]);

  // --- THE MUTATION ENGINE (Optimistic Updates) ---
  const mutation = useMutation({
    mutationFn: async (payload: LampPayload) => {
      const res = await fetch("/api/settings", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(payload),
      });
      if (!res.ok) throw new Error("Failed to update ESP32");
      return res.json();
    },
    // This runs INSTANTLY when you click a button, before the network request finishes
    onMutate: async (newSettings) => {
      // Cancel background refetches so they don't overwrite our optimistic update
      await qc.cancelQueries({ queryKey: ["lampState"] });

      // Snapshot the previous value in case we need to roll back
      const previousState = qc.getQueryData(["lampState"]);

      // Optimistically overwrite the cache with the new value
      qc.setQueryData(["lampState"], (old: any) => ({
        ...old,
        ...newSettings,
      }));

      return { previousState };
    },
    onSuccess: () => {
      toast.success("Saved Successfully");
    },
    // If the ESP32 is offline or crashes, roll the UI back to the snapshot
    onError: (err, _newSettings, context) => {
      qc.setQueryData(["lampState"], context?.previousState);
      console.error("Hardware Sync Failed:", err);
      toast.error("Hardware Sync Failed", {
        description: "Sunrise might be offline",
      });
    },
    // Once everything is done, force a fresh pull from the ESP32 just to be certain
    onSettled: () => {
      qc.invalidateQueries({ queryKey: ["lampState"] });
    },
  });

  // --- MASTER DEFAULT STATE ---
  const defaultData = {
    brightness: 150,
    alarmHour: 7,
    alarmMinute: 30,
    mode: 0,
    temp: 0, // Replaced 'hue' with 'temp' (0=Amber, 1=Warm, 2=Cool)
    dawnH: 6,
    dawnM: 0,
    duskH: 18,
    duskM: 0,
    nightLightBright: 128,
    cfgMaxGlow: 255,
    cfgTouchThreshold: 35,
    cfgProxThreshold: 30,
    cfgAmbientThreshold: 100,
    isSunriseEnabled: true,
    isSundownEnabled: true,
    cfgSundownHour: 18,
    cfgSundownMinute: 0,
    cfgModeTimeout: 40,
    cfgBrightMode: 2, // 0=Var, 1=Step, 2=Hybrid
    sunriseDuration: 30,
    sundownDuration: 30,
    geoLat: -33.9321,
    geoLon: 18.8602,
    tzOffset: 2,
    planetDawnH: 6,
    planetDawnM: 0,
    planetDuskH: 18,
    planetDuskM: 0,
    cfgProxEnabled: true,
    sunriseDays: [0, 1, 1, 1, 1, 1, 0],
    sundownDays: [1, 1, 1, 1, 1, 1, 1],
  };

  // Fallback state while the initial fetch happens
  const data = state ? { ...defaultData, ...state } : defaultData;

  // --- HANDLERS ---
  const handleTimeChange = (type: "hour" | "minute", value: string) => {
    const num = Number(value);
    if (type === "hour") {
      mutation.mutate({ alarmHour: num, alarmMinute: data.alarmMinute });
    } else {
      mutation.mutate({ alarmHour: data.alarmHour, alarmMinute: num });
    }
  };

  const handleCitySearch = async () => {
    if (!citySearch) return;
    setIsSearching(true);
    try {
      // Free, no-auth forward geocoding API
      const res = await fetch(
        `https://nominatim.openstreetmap.org/search?format=json&q=${encodeURIComponent(citySearch)}&limit=1`,
      );
      const geoData = await res.json();

      if (geoData && geoData.length > 0) {
        const lat = parseFloat(parseFloat(geoData[0].lat).toFixed(4));
        const lon = parseFloat(parseFloat(geoData[0].lon).toFixed(4));

        mutation.mutate({ geoLat: lat, geoLon: lon });
        setCitySearch(""); // Clear the input on success
      } else {
        alert(
          "City not found. Try adding the country name (e.g., 'Stellenbosch, South Africa').",
        );
      }
    } catch (error) {
      console.error("Geocoding failed", error);
      alert("Failed to search location.");
    } finally {
      setIsSearching(false);
    }
  };

  const handleSyncTimezone = () => {
    // Silently grabs the timezone from the phone's browser (e.g., SAST is 2)
    const localTzOffset = -(new Date().getTimezoneOffset() / 60);
    mutation.mutate({ tzOffset: localTzOffset });
  };

  // Smooth UI dragging without spamming the ESP32
  const handleDrag = (key: string, value: number) => {
    qc.setQueryData(["lampState"], (old: any) => ({
      ...old,
      [key]: value,
    }));
  };

  // Only fire the network request when the slider is released
  const handleRelease = (key: string, value: number) => {
    mutation.mutate({ [key]: value });
  };

  const accordionItems = [
    // Sequencing & Durations
    {
      headerContent: (
        <div className="flex items-center gap-3 font-medium text-zinc-100">
          <Clock size={18} className="text-amber-500" /> Sequencing & Durations
        </div>
      ),
      bodyContent: (
        <div>
          <p className="text-sm text-zinc-500 mb-4">
            Set transition times (in minutes) for the automated fades.
          </p>
          <ConfigSlider
            label="Sunrise Duration (Mins)"
            value={(data as any)["sunriseDuration"]}
            onChange={(v) => handleDrag("sunriseDuration", v)}
            onRelease={(v) => handleRelease("sunriseDuration", v)}
            min={1}
            max={120}
          />
          <ConfigSlider
            label="Sundown Duration (Mins)"
            value={(data as any)["sundownDuration"]}
            onChange={(v) => handleDrag("sundownDuration", v)}
            onRelease={(v) => handleRelease("sundownDuration", v)}
            min={1}
            max={120}
          />

          <div className="border-t border-zinc-800/50 mt-6 pt-4">
            <p className="text-xs text-zinc-500 mb-2 font-semibold tracking-wider uppercase">
              Failsafe Timeout
            </p>
            <p className="text-xs text-zinc-500 mb-4 leading-relaxed">
              If you leave the house during a sequence, the lamp will
              automatically drop to Auto Mode after this limit.
            </p>
            <ConfigSlider
              label="Exit Sequence After (Mins)"
              value={(data as any)["cfgModeTimeout"]}
              onChange={(v) => handleDrag("cfgModeTimeout", v)}
              onRelease={(v) => handleRelease("cfgModeTimeout", v)}
              min={10}
              max={180}
            />
          </div>
        </div>
      ),
    },
    // Location & Timezone
    {
      headerContent: (
        <div className="flex items-center gap-3 font-medium text-zinc-100">
          <Globe size={18} className="text-amber-500" /> Location & Timezone
        </div>
      ),
      bodyContent: (
        <div className="space-y-6">
          <p className="text-sm text-zinc-500">
            Sync planetary data to your current city.
          </p>

          {/* CITY SEARCH BAR */}
          <div className="space-y-2">
            <label className="text-xs font-semibold tracking-widest text-zinc-500 uppercase">
              Search City
            </label>
            <div className="flex gap-2">
              <input
                type="text"
                placeholder="e.g. Stellenbosch..."
                value={citySearch}
                onChange={(e) => setCitySearch(e.target.value)}
                onKeyDown={(e) => e.key === "Enter" && handleCitySearch()}
                className="w-full bg-zinc-950 border border-zinc-800 rounded-lg p-2.5 text-sm text-amber-500 focus:outline-none focus:border-amber-500/50 transition-colors"
              />
              <button
                onClick={handleCitySearch}
                disabled={isSearching}
                className="px-4 bg-amber-500/10 hover:bg-amber-500/20 text-amber-500 border border-amber-500/50 rounded-lg text-sm font-medium transition-colors disabled:opacity-50"
              >
                {isSearching ? "..." : "Find"}
              </button>
            </div>
          </div>

          {/* EDITABLE COORDS & TIMEZONE */}
          <div className="grid grid-cols-2 gap-4 border-t border-zinc-800/50 pt-4">
            <div className="space-y-1">
              <label className="text-xs text-zinc-400">Latitude</label>
              <input
                type="number"
                step="0.0001"
                value={data.geoLat}
                onChange={(e) =>
                  handleDrag("geoLat", parseFloat(e.target.value) || 0)
                }
                onBlur={() => handleRelease("geoLat", data.geoLat)}
                className="w-full bg-zinc-950 border border-zinc-800 rounded-lg p-2 text-sm text-amber-500 focus:outline-none focus:border-amber-500/50"
              />
            </div>
            <div className="space-y-1">
              <label className="text-xs text-zinc-400">Longitude</label>
              <input
                type="number"
                step="0.0001"
                value={data.geoLon}
                onChange={(e) =>
                  handleDrag("geoLon", parseFloat(e.target.value) || 0)
                }
                onBlur={() => handleRelease("geoLon", data.geoLon)}
                className="w-full bg-zinc-950 border border-zinc-800 rounded-lg p-2 text-sm text-amber-500 focus:outline-none focus:border-amber-500/50"
              />
            </div>
            <div className="col-span-2 space-y-1 pt-2">
              <div className="flex justify-between items-center pb-1">
                <label className="text-xs text-zinc-400">
                  UTC Offset (Hours)
                </label>
                <button
                  onClick={handleSyncTimezone}
                  className="flex items-center gap-1.5 px-2 py-1 bg-zinc-900 hover:bg-zinc-800 text-zinc-300 text-[10px] uppercase tracking-wider rounded-md transition-colors"
                >
                  <MapPin size={12} className="text-amber-500" />
                  Sync Phone
                </button>
              </div>
              <input
                type="number"
                step="1"
                value={data.tzOffset}
                onChange={(e) =>
                  handleDrag("tzOffset", parseInt(e.target.value) || 0)
                }
                onBlur={() => handleRelease("tzOffset", data.tzOffset)}
                className="w-full bg-zinc-950 border border-zinc-800 rounded-lg p-2 text-sm text-amber-500 focus:outline-none focus:border-amber-500/50"
              />
            </div>
          </div>
        </div>
      ),
    },
    // Light Output Limits
    {
      headerContent: (
        <div className="flex items-center gap-3 font-medium text-zinc-100">
          <Settings2 size={18} className="text-amber-500" /> Light Output Limits
        </div>
      ),
      bodyContent: (
        <div>
          <p className="text-sm text-zinc-500 mb-4">
            Set the absolute maximum hardware bounds for automated states.
          </p>
          <ConfigSlider
            label="Night Light Brightness (Default)"
            value={(data as any)["nightLightBright"]}
            onChange={(v) => handleDrag("nightLightBright", v)}
            onRelease={(v) => handleRelease("nightLightBright", v)}
            min={13}
            max={255}
          />
          <ConfigSlider
            label="Proximity Glow Output"
            value={(data as any)["cfgMaxGlow"]}
            onChange={(v) => handleDrag("cfgMaxGlow", v)}
            onRelease={(v) => handleRelease("cfgMaxGlow", v)}
            min={50}
            max={255}
          />
          <div className="border-t border-zinc-800/50 mt-6 pt-4">
            <p className="text-xs text-zinc-500 mb-2 font-semibold tracking-wider uppercase">
              Intensity Button Behavior
            </p>
            <p className="text-xs text-zinc-500 mb-4 leading-relaxed">
              Customize how the physical brightness button reacts to taps and
              holds. Minimum hardware brightness is hard-locked to 5%.
            </p>
            <div className="grid grid-cols-3 gap-2">
              {[
                { label: "Variable", val: 0, desc: "Hold only" },
                { label: "Stepped", val: 1, desc: "8 Levels" },
                { label: "Hybrid", val: 2, desc: "Default" },
              ].map((mode) => (
                <button
                  key={mode.val}
                  onClick={() => mutation.mutate({ cfgBrightMode: mode.val })}
                  className={`p-2 flex flex-col items-center justify-center rounded-xl transition-colors border ${
                    data.cfgBrightMode === mode.val
                      ? "bg-amber-500/10 border-amber-500/50 text-amber-500"
                      : "bg-zinc-950 border-zinc-800 text-zinc-500 hover:text-zinc-300"
                  }`}
                >
                  <span className="text-sm font-medium">{mode.label}</span>
                  <span className="text-[10px] opacity-70">{mode.desc}</span>
                </button>
              ))}
            </div>
          </div>
        </div>
      ),
    },
    // Sensor Calibration
    {
      headerContent: (
        <div className="flex items-center gap-3 font-medium text-zinc-100">
          <Waves size={18} className="text-amber-500" /> Sensor Calibration
        </div>
      ),
      bodyContent: (
        <div>
          <p className="text-sm text-zinc-500 mb-4">
            Fine-tune the analog sensors to the Avodire wood density and room
            location.
          </p>
          <ConfigSlider
            label="Touch Sensitivity Threshold"
            value={(data as any)["cfgTouchThreshold"]}
            onChange={(v) => handleDrag("cfgTouchThreshold", v)}
            onRelease={(v) => handleRelease("cfgTouchThreshold", v)}
            min={10}
            max={200}
          />
          <ConfigSlider
            label="Proximity Sensitivity Threshold"
            value={(data as any)["cfgProxThreshold"]}
            onChange={(v) => handleDrag("cfgProxThreshold", v)}
            onRelease={(v) => handleRelease("cfgProxThreshold", v)}
            min={10}
            max={200}
          />
          <ConfigSlider
            label="Ambient Darkness Trigger"
            value={(data as any)["cfgAmbientThreshold"]}
            onChange={(v) => handleDrag("cfgAmbientThreshold", v)}
            onRelease={(v) => handleRelease("cfgAmbientThreshold", v)}
            min={10}
            max={1000}
          />
          <div className="flex justify-between items-center mt-6 pt-4 border-t border-zinc-800/50">
            <div>
              <p className="text-sm text-zinc-300 font-medium">
                Proximity Sensing
              </p>
              <p className="text-xs text-zinc-500 mt-0.5">
                Hand-hover glow when lamp is in Auto mode
              </p>
            </div>
            <button
              onClick={() =>
                mutation.mutate({
                  cfgProxEnabled: !(data as any).cfgProxEnabled,
                })
              }
              className={`w-12 h-6 rounded-full transition-colors relative shrink-0 ${
                (data as any).cfgProxEnabled ? "bg-amber-500" : "bg-zinc-800"
              }`}
            >
              <div
                className={`w-4 h-4 rounded-full bg-white absolute top-1 transition-transform ${
                  (data as any).cfgProxEnabled
                    ? "translate-x-7"
                    : "translate-x-1"
                }`}
              />
            </button>
          </div>
        </div>
      ),
    },
    // Hardware & Diagnostics
    {
      headerContent: (
        <div className="flex items-center gap-3 font-medium text-zinc-100">
          <Activity size={18} className="text-amber-500" /> Hardware Diagnostics
        </div>
      ),
      bodyContent: (
        <div className="space-y-4">
          <div className="flex justify-between items-center mb-4">
            <p className="text-sm text-zinc-500">Live telemetry from Core 0</p>
            <button
              onClick={() => refetchDiagnostics()}
              className="flex items-center gap-2 px-3 py-1.5 bg-zinc-800 hover:bg-zinc-700 text-zinc-300 text-xs rounded-lg transition-colors"
            >
              <RefreshCw
                size={14}
                className={isFetchingDiag ? "animate-spin" : ""}
              />
              Resync
            </button>
          </div>

          {diagnostics ? (
            <div className="grid grid-cols-2 gap-3 text-sm">
              <div className="bg-zinc-950 p-3 rounded-xl border border-zinc-800/50">
                <span className="text-zinc-500 text-xs block mb-1">
                  PROXIMITY (GPIO 4)
                </span>
                <span className="font-mono text-amber-500">
                  {diagnostics.prox}
                </span>
              </div>
              <div className="bg-zinc-950 p-3 rounded-xl border border-zinc-800/50">
                <span className="text-zinc-500 text-xs block mb-1">
                  AMBIENT LUX
                </span>
                <span className="font-mono text-amber-500">
                  {diagnostics.ambient}
                </span>
              </div>
              <div className="bg-zinc-950 p-3 rounded-xl border border-zinc-800/50">
                <span className="text-zinc-500 text-xs block mb-1">
                  BTN: BRIGHT (GPIO 15)
                </span>
                <span className="font-mono text-zinc-300">
                  {diagnostics.b1}
                </span>
              </div>
              <div className="bg-zinc-950 p-3 rounded-xl border border-zinc-800/50">
                <span className="text-zinc-500 text-xs block mb-1">
                  BTN: TEMP (GPIO 14)
                </span>
                <span className="font-mono text-zinc-300">
                  {diagnostics.b2}
                </span>
              </div>
              <div className="bg-zinc-950 p-3 rounded-xl border border-zinc-800/50">
                <span className="text-zinc-500 text-xs block mb-1">
                  BTN: MODE (GPIO 27)
                </span>
                <span className="font-mono text-zinc-300">
                  {diagnostics.b3}
                </span>
              </div>
              <div className="bg-zinc-950 p-3 rounded-xl border border-zinc-800/50">
                <span className="text-zinc-500 text-xs block mb-1">
                  RAW FSM ID
                </span>
                <span className="font-mono text-zinc-300">
                  {diagnostics.stateCode} ({MODE_MAP[diagnostics.stateCode]})
                </span>
              </div>
            </div>
          ) : (
            <div className="text-center py-6 text-zinc-600 text-sm border border-dashed border-zinc-800 rounded-xl">
              Awaiting telemetry sync...
            </div>
          )}
        </div>
      ),
    },
  ];

  if (isLoading)
    return (
      <div className="min-h-screen bg-zinc-950 text-amber-500 flex items-center justify-center tracking-widest uppercase text-sm">
        Initializing System...
      </div>
    );

  return (
    <div className="min-h-screen bg-zinc-950 text-zinc-100 p-6 pb-24 font-montserrat">
      <div className="max-w-md mx-auto space-y-6">
        {/* SKY ARC */}
        <SkyArc data={data} />

        {/* HEADER */}
        <header className="space-y-1 mb-8 mt-4">
          {/* Which name is cooler? */}
          <h1 className="text-4xl font-medium tracking-[0.3em]! text-center">
            SUNRISE
          </h1>
          {/* <h1 className="text-3xl font-medium tracking-[0.3em]!">SOLARIS</h1> */}
          <p className="text-zinc-500 text-sm tracking-wide text-center">
            Smart Ambient Lighting
          </p>
        </header>

        {/* SYSTEM STATUS CARD */}
        <div className="grid grid-cols-2 gap-4 relative mb-8">
          <div className="bg-zinc-900 border border-zinc-800 rounded-3xl p-5 shadow-xl relative flex flex-col justify-center items-center overflow-hidden">
            <Sun
              className="text-amber-500/40 absolute -top-8 -left-8"
              size={100}
            />
            <div className="z-10">
              <p className="text-zinc-300 text-center text-xs font-semibold tracking-wider mb-1">
                SUNRISE
              </p>
              <p className="text-3xl font-light tabular-nums">
                {String(data.dawnH).padStart(2, "0")}:
                {String(data.dawnM).padStart(2, "0")}
              </p>
            </div>
          </div>
          <div className="bg-zinc-900 border border-zinc-800 rounded-3xl p-5 shadow-xl relative flex flex-col justify-center items-center overflow-hidden">
            <Moon
              className="absolute -top-5 -left-7 text-sky-400/40"
              size={100}
            />
            <div className="z-10">
              <p className="text-zinc-300 text-center text-xs font-semibold tracking-wider mb-1">
                SUNDOWN
              </p>
              <p className="text-3xl font-light tabular-nums">
                {String(data.duskH).padStart(2, "0")}:
                {String(data.duskM).padStart(2, "0")}
              </p>
            </div>
          </div>
          {data.dawnH === data.alarmHour && data.dawnM === data.alarmMinute && (
            <p className="text-zinc-600 text-xs absolute -bottom-5 left-1">
              Note: Planetary sunrise overwritten by user alarm
            </p>
          )}
        </div>

        {/* MODE CONTROLS */}
        <section className="bg-zinc-900 border border-zinc-800 rounded-3xl p-6 shadow-xl">
          <div className="flex justify-between items-center mb-6">
            <div className="text-left">
              <p className="text-zinc-500 text-xs font-semibold tracking-wider mb-1">
                ACTIVE STATE
              </p>
              <h2 className="text-xl font-medium text-amber-500">
                {MODE_MAP[data.mode]}
              </h2>
            </div>
            <Shield
              className={data.mode === 8 ? "text-red-500" : "text-zinc-700"}
            />
          </div>
          <div className="grid grid-cols-3 gap-3">
            <button
              onClick={() => mutation.mutate({ mode: 0 })}
              className={`py-3 cursor-pointer rounded-xl text-sm transition-colors border ${data.mode === 0 || data.mode === 1 ? "bg-amber-500/10 border-amber-500/50 text-amber-500" : "bg-zinc-950 border-zinc-800 text-zinc-400"}`}
            >
              Auto
            </button>
            <button
              onClick={() => mutation.mutate({ mode: 2 })}
              className={`py-3 cursor-pointer rounded-xl text-sm transition-colors border ${data.mode === 2 || data.mode === 3 ? "bg-amber-500/10 border-amber-500/50 text-amber-500" : "bg-zinc-950 border-zinc-800 text-zinc-400"}`}
            >
              Manual
            </button>
            <button
              onClick={() => mutation.mutate({ mode: 8 })}
              className={`py-3 cursor-pointer rounded-xl text-sm transition-colors border ${data.mode === 8 ? "bg-red-500/10 border-red-500/50 text-red-500" : "bg-zinc-950 border-zinc-800 text-zinc-400"}`}
            >
              Away
            </button>
          </div>
        </section>

        {/* AUTOMATION CONFIG CARD */}
        <section className="bg-zinc-900 border border-zinc-800 rounded-3xl p-6 shadow-xl space-y-6">
          {/* ======================= */}
          {/* SUNRISE ENGINE       */}
          {/* ======================= */}
          <div className="space-y-5">
            <div className="flex justify-between items-start">
              <div>
                <div className="flex items-center gap-2 mb-1">
                  <Clock
                    size={18}
                    className={
                      data.isSunriseEnabled ? "text-amber-500" : "text-zinc-600"
                    }
                  />
                  <h2
                    className={`text-lg font-medium ${data.isSunriseEnabled ? "text-zinc-100" : "text-zinc-600"}`}
                  >
                    Sunrise Engine
                  </h2>
                </div>
                <p className="text-zinc-500 text-sm">
                  Automated dawn transitions
                </p>
              </div>
              <button
                onClick={() =>
                  mutation.mutate({
                    isSunriseEnabled: !data.isSunriseEnabled,
                  })
                }
                className={`w-12 h-6 rounded-full transition-colors relative ${data.isSunriseEnabled ? "bg-amber-500" : "bg-zinc-800"}`}
              >
                <div
                  className={`w-4 h-4 rounded-full bg-white absolute top-1 transition-transform ${data.isSunriseEnabled ? "translate-x-7" : "translate-x-1"}`}
                />
              </button>
            </div>

            <div
              className={`space-y-5 transition-opacity duration-300 ${data.isSunriseEnabled ? "opacity-100" : "opacity-30 pointer-events-none"}`}
            >
              <div className="flex justify-between items-center">
                <span className="text-sm font-medium text-zinc-400">
                  Target Time (Manual)
                </span>
                <div className="flex gap-1 text-xl font-light tabular-nums bg-zinc-950 p-2 rounded-xl border border-zinc-800 focus-within:border-amber-500/50 transition-colors">
                  <input
                    type="number"
                    min="0"
                    max="23"
                    value={String(data.alarmHour).padStart(2, "0")}
                    onChange={(e) => handleTimeChange("hour", e.target.value)}
                    className="bg-transparent text-center w-12 outline-none focus:text-amber-500"
                  />
                  <span className="text-zinc-600">:</span>
                  <input
                    type="number"
                    min="0"
                    max="59"
                    value={String(data.alarmMinute).padStart(2, "0")}
                    onChange={(e) => handleTimeChange("minute", e.target.value)}
                    className="bg-transparent text-center w-12 outline-none focus:text-amber-500"
                  />
                </div>
              </div>
              <DayPicker
                days={data.sunriseDays}
                onChange={(newDays) =>
                  mutation.mutate({ sunriseDays: newDays })
                }
                color="amber"
              />
            </div>
          </div>

          <div className="border-t border-zinc-800/50"></div>

          {/* ======================= */}
          {/* SUNDOWN ENGINE       */}
          {/* ======================= */}
          <div className="space-y-5">
            <div className="flex justify-between items-start">
              <div>
                <div className="flex items-center gap-2 mb-1">
                  <Moon
                    size={18}
                    className={
                      data.isSundownEnabled ? "text-sky-500" : "text-zinc-600"
                    }
                  />
                  <h2
                    className={`text-lg font-medium ${data.isSundownEnabled ? "text-zinc-100" : "text-zinc-600"}`}
                  >
                    Sundown Engine
                  </h2>
                </div>
                <p className="text-zinc-500 text-sm">
                  Automated dusk sequences
                </p>
              </div>
              <button
                onClick={() =>
                  mutation.mutate({
                    isSundownEnabled: !data.isSundownEnabled,
                  })
                }
                className={`w-12 h-6 rounded-full transition-colors relative ${data.isSundownEnabled ? "bg-sky-500" : "bg-zinc-800"}`}
              >
                <div
                  className={`w-4 h-4 rounded-full bg-white absolute top-1 transition-transform ${data.isSundownEnabled ? "translate-x-7" : "translate-x-1"}`}
                />
              </button>
            </div>

            <div
              className={`space-y-5 transition-opacity duration-300 ${data.isSundownEnabled ? "opacity-100" : "opacity-30 pointer-events-none"}`}
            >
              <div className="flex justify-between items-center">
                <span className="text-sm font-medium text-zinc-400">
                  Target Time (Manual)
                </span>
                <div className="flex gap-1 text-xl font-light tabular-nums bg-zinc-950 p-2 rounded-xl border border-zinc-800 focus-within:border-sky-500/50 transition-colors">
                  <input
                    type="number"
                    min="0"
                    max="23"
                    value={String(data.cfgSundownHour).padStart(2, "0")}
                    onChange={(e) =>
                      mutation.mutate({
                        cfgSundownHour: Number(e.target.value),
                        cfgSundownMinute: data.cfgSundownMinute,
                      })
                    }
                    className="bg-transparent text-center w-12 outline-none focus:text-sky-500"
                  />
                  <span className="text-zinc-600">:</span>
                  <input
                    type="number"
                    min="0"
                    max="59"
                    value={String(data.cfgSundownMinute).padStart(2, "0")}
                    onChange={(e) =>
                      mutation.mutate({
                        cfgSundownHour: data.cfgSundownHour,
                        cfgSundownMinute: Number(e.target.value),
                      })
                    }
                    className="bg-transparent text-center w-12 outline-none focus:text-sky-500"
                  />
                </div>
              </div>
              <DayPicker
                days={data.sundownDays}
                onChange={(newDays) =>
                  mutation.mutate({ sundownDays: newDays })
                }
                color="sky"
              />
            </div>
          </div>
        </section>

        {/* MASTER CONTROL CARD */}
        <section className="bg-zinc-900 border border-zinc-800 rounded-3xl p-6 shadow-xl">
          <div className="flex justify-between items-center mb-6">
            <h2 className="text-lg font-medium">Master Output</h2>
            <div className="flex items-center gap-3">
              <span className="text-xs font-semibold tracking-widest text-zinc-500 uppercase px-2 py-1 bg-zinc-950 rounded-md border border-zinc-800">
                {getTempName(data.temp)}
              </span>
              <div className="h-3 w-3 rounded-full bg-amber-500 shadow-[0_0_12px_rgba(245,158,11,0.6)]"></div>
            </div>
          </div>

          <div className="space-y-6">
            {/* INTENSITY SLIDER */}
            <div className="space-y-3">
              <label className="text-sm text-zinc-400 flex justify-between">
                <span>
                  {data.mode === 4 ? "Night Light Brightness" : "Intensity"}
                </span>
                <span>
                  {Math.round(
                    ((data.mode === 4
                      ? data.nightLightBright
                      : data.brightness) /
                      255) *
                      100,
                  )}
                  %
                </span>
              </label>
              <input
                type="range"
                min="13"
                max={data.mode === 4 ? 255 : 255}
                value={
                  data.mode === 4 ? data.nightLightBright : data.brightness
                }
                onChange={(e) =>
                  data.mode === 4
                    ? handleDrag("nightLightBright", Number(e.target.value))
                    : handleDrag("brightness", Number(e.target.value))
                }
                onMouseUp={() =>
                  data.mode === 4
                    ? handleRelease("nightLightBright", data.nightLightBright)
                    : handleRelease("brightness", data.brightness)
                }
                onTouchEnd={() =>
                  data.mode === 4
                    ? handleRelease("nightLightBright", data.nightLightBright)
                    : handleRelease("brightness", data.brightness)
                }
                className="w-full h-2 bg-zinc-950 rounded-lg appearance-none cursor-pointer accent-amber-500"
              />
            </div>

            {/* NEW: TEMPERATURE SELECTOR */}
            <div className="space-y-3 pt-2">
              <label className="text-sm text-zinc-400">Color Temperature</label>
              <div className="grid grid-cols-3 gap-2">
                {[
                  { label: "Amber", val: 0 },
                  { label: "Warm White", val: 1 },
                  { label: "Cool White", val: 2 },
                ].map((t) => (
                  <button
                    key={t.val}
                    onClick={() => mutation.mutate({ temp: t.val })}
                    className={`py-2 text-xs rounded-lg transition-colors border ${
                      data.temp === t.val
                        ? "bg-amber-500/10 border-amber-500/50 text-amber-500"
                        : "bg-zinc-950 border-zinc-800 text-zinc-500 hover:text-zinc-300"
                    }`}
                  >
                    {t.label}
                  </button>
                ))}
              </div>
            </div>
          </div>
        </section>

        {/* ADVANCED CONFIG ACCORDION */}
        <div className="pt-4">
          <Accordion items={accordionItems} />
        </div>

        {/* MANUAL LINK */}
        <div className="pt-20 flex items-center justify-center">
          <Link to="/manual" className="text-zinc-400 underline tracking-wider">
            Manual
          </Link>
        </div>
      </div>
    </div>
  );
}

export default function App() {
  return (
    <QueryClientProvider client={queryClient}>
      <LampDashboard />
      <Toaster position="bottom-center" richColors closeButton />
    </QueryClientProvider>
  );
}
