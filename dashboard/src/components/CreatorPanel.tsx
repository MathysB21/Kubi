// Hidden "Creator" panel: the brother's backdoor. Opened by tapping the KUBI
// title five times; remembered per browser (localStorage), so Kubi's owner
// never sees it on his own phone. Nothing here is saved on Kubi: scenes are
// timed previews, banners go away on a tap, chimes just play. The saved
// birthday date (NVS "bday") is never touched.
import { useState } from "react";
import { useQueryClient } from "@tanstack/react-query";
import { toast } from "sonner";
import { Cake, Wand2, Megaphone, Music2, Image, X, Undo2 } from "lucide-react";

// Every scene, by firmware index (ALL_KUBI_SCENES), Easter eggs included
const ALL_SCENES = [
  { id: 1, title: "Birthday" },
  { id: 2, title: "Christmas" },
  { id: 0, title: "Beach" },
  { id: 3, title: "Flower field" },
  { id: 4, title: "Music" },
  { id: 5, title: "Cafe" },
  { id: 6, title: "Bedtime" },
  { id: 7, title: "Sunset" },
  { id: 8, title: "Working" },
  { id: 9, title: "Gym" },
];

const HOLDS = [
  { min: 10, label: "10 min" },
  { min: 60, label: "1 hour" },
  { min: 180, label: "3 hours" },
  { min: 720, label: "12 hours" },
];

const CHIMES = [
  { id: "fanfare", label: "Fanfare" },
  { id: "done", label: "Arpeggio" },
  { id: "ping", label: "Ping" },
  { id: "tap", label: "Tap" },
  { id: "ouch", label: "Ouch" },
];

const MODE_MASCOT = 2;

async function post(path: string, body: unknown) {
  const res = await fetch(path, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(body),
  });
  if (!res.ok) throw new Error(`${path} failed`);
}

interface Props {
  mascotScene?: number;
  mascotPreview?: boolean;
  mascotPreviewLeftMin?: number;
  onClose: () => void;
}

export function CreatorPanel({ mascotScene, mascotPreview, mascotPreviewLeftMin, onClose }: Props) {
  const qc = useQueryClient();
  const [hold, setHold] = useState(60);
  const [toFace3, setToFace3] = useState(true);
  const [title, setTitle] = useState("Happy 21st, Wilhelm!");
  const [message, setMessage] = useState("Crafted with care by your brother");
  const [busy, setBusy] = useState(false);

  const run = async (what: string, steps: () => Promise<void>) => {
    setBusy(true);
    try {
      await steps();
      toast.success(what);
      qc.invalidateQueries({ queryKey: ["kubiState"] });
    } catch {
      toast.error("Could not reach Kubi");
    } finally {
      setBusy(false);
    }
  };

  const showScene = (id: number, minutes: number, face3: boolean) =>
    post("/api/settings", {
      ...(face3 ? { mode: MODE_MASCOT } : {}),
      mascotScene: id,
      mascotPreviewMin: minutes,
    });

  const surprise = () =>
    run("Surprise sent", async () => {
      await showScene(1, hold, true);
      await post("/api/chime", { chime: "fanfare" });
      await post("/api/override", { title, message });
    });

  const heldName = mascotPreview ? ALL_SCENES.find((s) => s.id === mascotScene)?.title : undefined;
  const btn =
    "px-3 py-2 rounded-xl border text-xs font-medium transition-colors cursor-pointer disabled:opacity-50";
  const idle = "bg-zinc-950/60 border-zinc-800 text-zinc-300 hover:border-zinc-600";
  const input =
    "w-full bg-zinc-950/60 border border-zinc-800 rounded-xl px-3 py-2 text-sm text-zinc-200 placeholder-zinc-600 focus:outline-none focus:border-fuchsia-500/60";

  return (
    <section className="bg-zinc-900 border border-fuchsia-500/30 rounded-3xl p-6 shadow-xl space-y-6">
      <div className="flex justify-between items-start">
        <div>
          <p className="text-fuchsia-400 text-xs font-semibold tracking-wider mb-1 flex items-center gap-1.5">
            <Wand2 size={13} /> CREATOR
          </p>
          <h2 className="text-lg font-medium text-zinc-200">Backstage</h2>
          <p className="text-xs text-zinc-500 mt-1">Nothing here is saved on Kubi. Tap the title 5 times to open or hide.</p>
        </div>
        <button onClick={onClose} className="p-2 text-zinc-500 hover:text-zinc-300 cursor-pointer" aria-label="Hide Creator">
          <X size={18} />
        </button>
      </div>

      {/* Birthday surprise: scene + fanfare + banner in one tap */}
      <div className="space-y-3 rounded-2xl border border-fuchsia-500/20 bg-fuchsia-500/5 p-4">
        <p className="text-sm font-medium text-zinc-200 flex items-center gap-2">
          <Cake size={16} className="text-fuchsia-400" /> Birthday surprise
        </p>
        <p className="text-xs text-zinc-500">
          Switches Kubi to Face 3 with the Birthday scene, plays the fanfare and puts this banner up. A tap on Kubi
          clears the banner and leaves the party scene.
        </p>
        <input className={input} value={title} maxLength={30} onChange={(e) => setTitle(e.target.value)} placeholder="Banner title" />
        <input className={input} value={message} maxLength={90} onChange={(e) => setMessage(e.target.value)} placeholder="Banner message" />
        <button
          disabled={busy || !message.trim()}
          onClick={surprise}
          className="w-full py-2.5 rounded-xl bg-fuchsia-500/20 border border-fuchsia-500/50 text-fuchsia-300 text-sm font-semibold hover:bg-fuchsia-500/30 cursor-pointer disabled:opacity-50"
        >
          Surprise him ({HOLDS.find((h) => h.min === hold)?.label} of Birthday)
        </button>
      </div>

      {/* Any scene, any length */}
      <div className="space-y-3">
        <div className="flex items-center justify-between">
          <p className="text-sm font-medium text-zinc-200 flex items-center gap-2">
            <Image size={16} className="text-zinc-400" /> Scenes
          </p>
          {heldName && (
            <span className="text-[11px] text-fuchsia-300">
              Showing {heldName}
              {mascotPreviewLeftMin !== undefined ? `, ${mascotPreviewLeftMin} min left` : ""}
            </span>
          )}
        </div>
        <div className="flex flex-wrap gap-1.5">
          {HOLDS.map((h) => (
            <button
              key={h.min}
              onClick={() => setHold(h.min)}
              className={`${btn} ${hold === h.min ? "bg-fuchsia-500/15 border-fuchsia-500/50 text-fuchsia-300" : idle}`}
            >
              {h.label}
            </button>
          ))}
        </div>
        <label className="flex items-center gap-2 text-xs text-zinc-400">
          <input type="checkbox" checked={toFace3} onChange={(e) => setToFace3(e.target.checked)} className="accent-fuchsia-500" />
          Switch Kubi to Face 3 to show it
        </label>
        <div className="grid grid-cols-2 sm:grid-cols-5 gap-1.5">
          {ALL_SCENES.map((s) => (
            <button
              key={s.id}
              disabled={busy}
              onClick={() => run(`${s.title} for ${HOLDS.find((h) => h.min === hold)?.label}`, () => showScene(s.id, hold, toFace3))}
              className={`${btn} ${mascotPreview && mascotScene === s.id ? "bg-fuchsia-500/15 border-fuchsia-500/50 text-fuchsia-300" : idle}`}
            >
              {s.title}
            </button>
          ))}
        </div>
        <button
          disabled={busy || !mascotPreview}
          onClick={() => run("Kubi picks his own scene again", () => post("/api/settings", { mascotScene: -1 }))}
          className={`${btn} ${idle} flex items-center gap-1.5`}
        >
          <Undo2 size={13} /> Hand back to Kubi
        </button>
      </div>

      {/* Banner */}
      <div className="space-y-3">
        <p className="text-sm font-medium text-zinc-200 flex items-center gap-2">
          <Megaphone size={16} className="text-zinc-400" /> Banner
        </p>
        <p className="text-xs text-zinc-500">Uses the title and message above. Stays until someone taps Kubi.</p>
        <div className="flex gap-2">
          <button
            disabled={busy || !message.trim()}
            onClick={() => run("Banner up", () => post("/api/override", { title, message }))}
            className={`${btn} ${idle} flex-1`}
          >
            Show banner
          </button>
          <button
            disabled={busy}
            onClick={() => run("Banner down", () => post("/api/override", { message: "" }))}
            className={`${btn} ${idle} flex-1`}
          >
            Take it down
          </button>
        </div>
      </div>

      {/* Chimes */}
      <div className="space-y-3">
        <p className="text-sm font-medium text-zinc-200 flex items-center gap-2">
          <Music2 size={16} className="text-zinc-400" /> Chimes
        </p>
        <div className="grid grid-cols-3 sm:grid-cols-5 gap-1.5">
          {CHIMES.map((c) => (
            <button
              key={c.id}
              disabled={busy}
              onClick={() => run(`${c.label} played`, () => post("/api/chime", { chime: c.id }))}
              className={`${btn} ${idle}`}
            >
              {c.label}
            </button>
          ))}
        </div>
      </div>
    </section>
  );
}
