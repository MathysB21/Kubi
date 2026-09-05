// Web Audio 8-bit synthesizer matching firmware AudioManager.cpp
let audioCtx: AudioContext | null = null;

function getAudioContext(): AudioContext {
  if (!audioCtx) {
    audioCtx = new (window.AudioContext || (window as any).webkitAudioContext)();
  }
  if (audioCtx.state === "suspended") {
    audioCtx.resume();
  }
  return audioCtx;
}

interface Note {
  freq: number;
  durationMs: number;
}

const CHIMES: Record<string, Note[]> = {
  CHIME_TAP_FEEDBACK: [{ freq: 880.0, durationMs: 35 }],
  CHIME_WAKE_PING: [
    { freq: 587.33, durationMs: 60 },
    { freq: 880.0, durationMs: 100 },
  ],
  CHIME_POMODORO_DONE: [
    { freq: 523.25, durationMs: 100 }, // C5
    { freq: 659.25, durationMs: 100 }, // E5
    { freq: 783.99, durationMs: 100 }, // G5
    { freq: 1046.5, durationMs: 250 }, // C6
  ],
  CHIME_POMODORO_LONG_BREAK: [
    { freq: 523.25, durationMs: 90 },   // C5
    { freq: 659.25, durationMs: 90 },   // E5
    { freq: 783.99, durationMs: 90 },   // G5
    { freq: 1046.50, durationMs: 130 }, // C6
    { freq: 1174.66, durationMs: 100 }, // D6
    { freq: 1318.51, durationMs: 120 }, // E6
    { freq: 1567.98, durationMs: 350 }, // G6
  ],
  CHIME_SLAM_OUCH: [
    { freq: 349.23, durationMs: 80 },  // F4
    { freq: 261.63, durationMs: 80 },  // C4
    { freq: 174.61, durationMs: 160 }, // F3
  ],
};

export function playSimChime(chimeName: string) {
  const notes = CHIMES[chimeName];
  if (!notes || notes.length === 0) return;

  try {
    const ctx = getAudioContext();
    let startTime = ctx.currentTime;

    for (const note of notes) {
      const osc = ctx.createOscillator();
      const gain = ctx.createGain();

      osc.type = "square"; // Authentic 8-bit square wave matching AudioManager.cpp
      osc.frequency.setValueAtTime(note.freq, startTime);

      // Attack & Decay envelope
      const durationSec = note.durationMs / 1000;
      gain.gain.setValueAtTime(0.12, startTime);
      gain.gain.exponentialRampToValueAtTime(0.001, startTime + durationSec);

      osc.connect(gain);
      gain.connect(ctx.destination);

      osc.start(startTime);
      osc.stop(startTime + durationSec);

      startTime += durationSec;
    }
  } catch (err) {
    console.warn("Web Audio chime playback failed:", err);
  }
}
