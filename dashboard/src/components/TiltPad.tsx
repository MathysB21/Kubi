import { useCallback, useEffect, useRef, useState } from "react";

// Continuous tilt control for the /sim workbench. Streams tiltRoll/tiltPitch
// (degrees, relative to the current face resting flat) to /sim/inject at up to
// ~30 Hz. The firmware sim turns that into an accelerometer vector with the
// same frame the maze steers with (FaceMap.h), so no tilt maths lives here.
//
// Drag the puck, or hold arrow keys / WASD. Letting go springs back to level.

const MAX_TILT_DEG = 30;
const KEY_TILT_DEG = 18;
const SEND_INTERVAL_MS = 33;

type Tilt = { roll: number; pitch: number };

export function TiltPad({ send }: { send: (tilt: Tilt) => Promise<void> }) {
  const padRef = useRef<HTMLDivElement | null>(null);
  const [tilt, setTilt] = useState<Tilt>({ roll: 0, pitch: 0 });
  const [dragging, setDragging] = useState(false);
  const target = useRef<Tilt>({ roll: 0, pitch: 0 });
  const lastSent = useRef<Tilt>({ roll: 0, pitch: 0 });
  const inFlight = useRef(false);
  const keys = useRef(new Set<string>());
  const sendRef = useRef(send);
  useEffect(() => {
    sendRef.current = send;
  }, [send]);

  const update = useCallback((t: Tilt) => {
    target.current = t;
    setTilt(t);
  }, []);

  // Throttled streaming: one request in flight, latest value wins
  useEffect(() => {
    const id = window.setInterval(async () => {
      const t = target.current;
      if (inFlight.current) return;
      if (t.roll === lastSent.current.roll && t.pitch === lastSent.current.pitch) return;
      inFlight.current = true;
      lastSent.current = t;
      try {
        await sendRef.current(t);
      } finally {
        inFlight.current = false;
      }
    }, SEND_INTERVAL_MS);
    return () => window.clearInterval(id);
  }, []);

  const fromPointer = (e: React.PointerEvent) => {
    const rect = padRef.current?.getBoundingClientRect();
    if (!rect) return;
    const r = rect.width / 2;
    let dx = (e.clientX - rect.left - r) / r;
    let dy = (e.clientY - rect.top - r) / r;
    const len = Math.hypot(dx, dy);
    if (len > 1) {
      dx /= len;
      dy /= len;
    }
    // Right = right side down (+roll); up on the pad = top edge away (+pitch)
    update({ roll: Math.round(dx * MAX_TILT_DEG * 10) / 10, pitch: Math.round(-dy * MAX_TILT_DEG * 10) / 10 });
  };

  // Keyboard: arrows / WASD, ignored while typing in a field
  useEffect(() => {
    const apply = () => {
      const k = keys.current;
      const roll = (k.has("right") ? KEY_TILT_DEG : 0) - (k.has("left") ? KEY_TILT_DEG : 0);
      const pitch = (k.has("up") ? KEY_TILT_DEG : 0) - (k.has("down") ? KEY_TILT_DEG : 0);
      update({ roll, pitch });
    };
    const KEY_DIRS: Record<string, string> = {
      ArrowLeft: "left", a: "left", ArrowRight: "right", d: "right",
      ArrowUp: "up", w: "up", ArrowDown: "down", s: "down",
    };
    const dir = (key: string): string | undefined => KEY_DIRS[key];
    const typing = (e: KeyboardEvent) => {
      const el = e.target as HTMLElement | null;
      return !!el && (el.tagName === "INPUT" || el.tagName === "TEXTAREA" || el.isContentEditable);
    };
    const down = (e: KeyboardEvent) => {
      const d = dir(e.key);
      if (!d || typing(e)) return;
      e.preventDefault();
      keys.current.add(d);
      apply();
    };
    const up = (e: KeyboardEvent) => {
      const d = dir(e.key);
      if (!d) return;
      keys.current.delete(d);
      apply();
    };
    window.addEventListener("keydown", down);
    window.addEventListener("keyup", up);
    return () => {
      window.removeEventListener("keydown", down);
      window.removeEventListener("keyup", up);
    };
  }, [update]);

  const puckX = (tilt.roll / MAX_TILT_DEG) * 50;
  const puckY = (-tilt.pitch / MAX_TILT_DEG) * 50;

  return (
    <div className="flex items-center gap-5">
      <div
        ref={padRef}
        onPointerDown={(e) => {
          e.currentTarget.setPointerCapture(e.pointerId);
          setDragging(true);
          fromPointer(e);
        }}
        onPointerMove={(e) => dragging && fromPointer(e)}
        onPointerUp={() => {
          setDragging(false);
          update({ roll: 0, pitch: 0 });
        }}
        className="relative w-36 h-36 shrink-0 rounded-full bg-zinc-900/80 border border-zinc-700 cursor-grab active:cursor-grabbing touch-none select-none"
        title="Drag to tilt the cube (arrow keys / WASD work too)"
      >
        <div className="absolute inset-1/4 rounded-full border border-zinc-800" />
        <div className="absolute left-1/2 top-2 bottom-2 w-px bg-zinc-800" />
        <div className="absolute top-1/2 left-2 right-2 h-px bg-zinc-800" />
        <div
          className="absolute w-7 h-7 -ml-3.5 -mt-3.5 rounded-full bg-amber-500 shadow-[0_0_14px_rgba(245,158,11,0.5)] transition-[left,top] duration-75"
          style={{ left: `${50 + puckX}%`, top: `${50 + puckY}%` }}
        />
      </div>
      <div className="space-y-1.5 text-xs text-zinc-400">
        <div>
          Roll <span className="font-mono text-amber-400">{tilt.roll.toFixed(1)}°</span>
          <span className="text-zinc-600"> (+ right side down)</span>
        </div>
        <div>
          Pitch <span className="font-mono text-amber-400">{tilt.pitch.toFixed(1)}°</span>
          <span className="text-zinc-600"> (+ top away)</span>
        </div>
        <div className="text-zinc-500 pt-1">Drag the puck, or hold arrow keys / WASD. Release to level.</div>
      </div>
    </div>
  );
}
