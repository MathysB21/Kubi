import { useEffect, useMemo, useState } from "react";
import { NET_COLORS } from "../model/electronics";
import { BOARD_WIRES, cutLength, isPigtail, NET_NAMES, OCCUPIED, R2, wirePoints, type BoardWire } from "../model/perfboard";
import { backHole, COL_LETTERS, COLS, parseHole, ROWS } from "../model/scad";
import { Button, Hint, Section, Toggle } from "../ui";

// One wire at a time, drawn on the solder side exactly as the board is held:
// turned over like a page, row 01 at the bottom. Every hole name shown here is the
// one printed on this side of the board (see backHole in scad.ts).

const P = 36;
const EDGE_X = 2.31 * P; // board edge beyond columns A and T
const EDGE_Y = 1.37 * P; // board edge beyond rows 01 and 14
const LABEL = 30; // band outside the board for the column letters and row numbers
const W = 2 * LABEL + 2 * EDGE_X + (COLS - 1) * P;
const H = 2 * LABEL + 2 * EDGE_Y + (ROWS - 1) * P;

const colX = (col: number) => LABEL + EDGE_X + (COLS - col) * P;
const rowY = (row: number) => LABEL + EDGE_Y + (ROWS - row) * P;

// The tape labels from the assembly guide
const PIGTAIL_NAMES: Record<string, string> = {
  S2: "5V IN", T2: "GND IN", S4: "AMP 5V", T4: "AMP GND", O14: "AMP L", P14: "AMP R", Q14: "AMP G",
};

const STEP_KEY = "kubi-viewer:wire-guide-step";
const FIRST_UNSOLDERED = Math.max(0, BOARD_WIRES.findIndex((w) => w.id === "s1"));

function loadStep() {
  try {
    const raw = localStorage.getItem(STEP_KEY);
    const n = raw === null ? FIRST_UNSOLDERED : Number(raw);
    return Number.isInteger(n) && n >= 0 && n < BOARD_WIRES.length ? n : FIRST_UNSOLDERED;
  } catch {
    return FIRST_UNSOLDERED;
  }
}

// Which holes end up joined once every wire is on the board
function buildGroups() {
  const parent = new Map<string, string>();
  const find = (h: string): string => {
    if (!parent.has(h)) parent.set(h, h);
    const p = parent.get(h)!;
    if (p === h) return h;
    const root = find(p);
    parent.set(h, root);
    return root;
  };
  for (const w of BOARD_WIRES) parent.set(find(w.from), find(w.to));
  return find;
}
const groupOf = buildGroups();

function pathOf(w: BoardWire) {
  return wirePoints(w).map(([c, r], i) => `${i ? "L" : "M"} ${colX(c)} ${rowY(r)}`).join(" ");
}

function holeName(ref: string) {
  const name = OCCUPIED[ref];
  return name ? `${backHole(ref)} (${name})` : backHole(ref);
}

// Plain directions as the solder side is held: column letters run backwards
function direction(w: BoardWire) {
  const a = parseHole(w.from);
  const b = parseHole(w.to);
  const across = a.col - b.col; // a higher column is further left on this side
  const up = b.row - a.row;
  const parts = [];
  if (across) parts.push(`${Math.abs(across)} column${Math.abs(across) > 1 ? "s" : ""} to the ${across > 0 ? "right" : "left"}`);
  if (up) parts.push(`${Math.abs(up)} row${Math.abs(up) > 1 ? "s" : ""} ${up > 0 ? "up" : "down"}`);
  return parts.join(", ");
}

function neighbours(hole: string) {
  const h = parseHole(hole);
  return [h.col - 1, h.col + 1]
    .filter((c) => c >= 1 && c <= COLS)
    .map((c) => `${COL_LETTERS[c - 1]}${h.row}`)
    .filter((n) => OCCUPIED[n] && groupOf(n) !== groupOf(hole));
}

function silentChecks(w: BoardWire) {
  const pairs = new Set<string>();
  for (const end of [w.from, w.to]) for (const n of neighbours(end)) pairs.add(`${backHole(end)} - ${backHole(n)}`);
  if (w.net === "gnd") {
    pairs.add(`${backHole(w.to)} - ${backHole("O2")} (5 V)`);
    pairs.add(`${backHole(w.to)} - ${backHole("O12")} (3.3 V)`);
  } else {
    pairs.add(`${backHole(w.to)} - ${backHole("N12")} (GND)`);
  }
  return [...pairs];
}

// The outside wires that must be through the board before this wire can land on them
function firstSteps(index: number) {
  const earlier = new Set(BOARD_WIRES.slice(0, index).flatMap((w) => [w.from, w.to]));
  const w = BOARD_WIRES[index];
  return [w.from, w.to].filter((h) => isPigtail(h) && !earlier.has(h));
}

export function WireGuideView({ active }: { active: boolean }) {
  const [step, setStepRaw] = useState(loadStep);
  const [showAhead, setShowAhead] = useState(false);
  const setStep = (n: number) => {
    const next = Math.min(BOARD_WIRES.length - 1, Math.max(0, n));
    setStepRaw(next);
    try {
      localStorage.setItem(STEP_KEY, String(next));
    } catch {
      // ignore: the step just won't be remembered
    }
  };

  useEffect(() => {
    if (!active) return;
    const onKey = (e: KeyboardEvent) => {
      if (e.key === "ArrowRight") setStep(step + 1);
      if (e.key === "ArrowLeft") setStep(step - 1);
    };
    window.addEventListener("keydown", onKey);
    return () => window.removeEventListener("keydown", onKey);
  });

  const wire = BOARD_WIRES[step];
  const color = NET_COLORS[wire.net];
  const a = parseHole(wire.from);
  const b = parseHole(wire.to);
  const pigtailsFirst = useMemo(() => firstSteps(step), [step]);

  const occupied = Object.keys(OCCUPIED).map((ref) => ({ ref, ...parseHole(ref) }));
  const activeCols = new Set([a.col, b.col]);
  const activeRows = new Set([a.row, b.row]);
  // The end nearer the top gets its label above the hole, the other below
  const fromAbove = a.row > b.row;

  return (
    <div className="flex h-full min-h-0 flex-col overflow-y-auto lg:flex-row lg:overflow-hidden">
      <div className="shrink-0 p-5 lg:min-h-0 lg:flex-1 lg:shrink lg:overflow-auto">
        <div className="mb-3 flex flex-wrap items-center justify-between gap-3">
          <Hint>Back of the board, turned over like a page: row 01 at the bottom. Letters are the ones printed on this side, A on the left.</Hint>
          <Toggle label="Show wires still to come" checked={showAhead} onChange={setShowAhead} />
        </div>

        <svg viewBox={`0 0 ${W} ${H}`} className="w-full max-w-[1100px] select-none" role="img"
          aria-label={`Solder side of the perfboard. Current wire runs from ${backHole(wire.from)} to ${backHole(wire.to)}.`}>
          <rect x={LABEL} y={LABEL} width={W - 2 * LABEL} height={H - 2 * LABEL} rx={6} fill="#2b2418" stroke="#6b5636" />

          {/* Guide lines from the two holes out to their printed labels */}
          {[...activeCols].map((c) => (
            <rect key={c} x={colX(c) - P / 2} y={LABEL} width={P} height={H - 2 * LABEL} fill="#fbbf24" opacity={0.09} />
          ))}
          {[...activeRows].map((r) => (
            <rect key={r} x={LABEL} y={rowY(r) - P / 2} width={W - 2 * LABEL} height={P} fill="#fbbf24" opacity={0.09} />
          ))}

          {/* Printed coordinates on all four sides */}
          {Array.from({ length: COLS }, (_, i) => i + 1).map((c) => {
            const on = activeCols.has(c);
            return [17, H - 13].map((y) => (
              <text key={`${c}-${y}`} x={colX(c)} y={y + 4} textAnchor="middle" fontSize={on ? 18 : 13} fontWeight={on ? 700 : 400}
                fill={on ? "#fbbf24" : "#a1a1aa"}>{COL_LETTERS[COLS - c]}</text>
            ));
          })}
          {Array.from({ length: ROWS }, (_, i) => i + 1).map((r) => {
            const on = activeRows.has(r);
            return [15, W - 15].map((x) => (
              <text key={`${r}-${x}`} x={x} y={rowY(r) + 4.5} textAnchor="middle" fontSize={on ? 16 : 12} fontWeight={on ? 700 : 400}
                fill={on ? "#fbbf24" : "#a1a1aa"}>{String(r).padStart(2, "0")}</text>
            ));
          })}

          {/* Holes */}
          {Array.from({ length: ROWS }, (_, ri) => Array.from({ length: COLS }, (_, ci) => (
            <circle key={`${ri}-${ci}`} cx={colX(ci + 1)} cy={rowY(ri + 1)} r={5} fill="#17120b" stroke="#a87a38" strokeWidth={2} />
          ))).flat()}

          {/* Wires still to come */}
          {showAhead && BOARD_WIRES.slice(step + 1).map((w) => (
            <path key={w.id} d={pathOf(w)} fill="none" stroke={NET_COLORS[w.net]} strokeWidth={1.5} strokeDasharray="4 5" opacity={0.55} />
          ))}

          {/* Wires already on the board */}
          {BOARD_WIRES.slice(0, step).map((w) => (
            <path key={w.id} d={pathOf(w)} fill="none" stroke={NET_COLORS[w.net]} strokeWidth={3} strokeLinecap="round" strokeLinejoin="round" opacity={0.42} />
          ))}

          {/* Occupied holes: the stubs a wire solders to or must keep clear of */}
          {occupied.map((h) => (
            <g key={h.ref}>
              <circle cx={colX(h.col)} cy={rowY(h.row)} r={6.5} fill="#d4d4d8" stroke="#71717a" />
              <text x={colX(h.col)} y={rowY(h.row) + 17} textAnchor="middle" fontSize={8.5} fill="#a1a1aa" fontFamily="ui-monospace, monospace">
                {OCCUPIED[h.ref].replace(/^(screen|IMU) /, "")}
              </text>
            </g>
          ))}

          {/* R2 sits on the other side of the board */}
          <line x1={colX(parseHole(R2.from).col)} y1={rowY(parseHole(R2.from).row)} x2={colX(parseHole(R2.to).col)} y2={rowY(parseHole(R2.to).row)}
            stroke="#d6b88a" strokeWidth={1} strokeDasharray="2 4" />

          {/* The wire to solder now */}
          <path d={pathOf(wire)} fill="none" stroke="#0a0a0a" strokeWidth={10} strokeLinecap="round" strokeLinejoin="round" />
          <path d={pathOf(wire)} fill="none" stroke={color} strokeWidth={6.5} strokeLinecap="round" strokeLinejoin="round" />

          {[{ ref: wire.from, h: a, above: fromAbove, tag: "start" }, { ref: wire.to, h: b, above: !fromAbove, tag: "end" }].map(({ ref, h, above, tag }) => {
            const x = colX(h.col);
            const y = rowY(h.row);
            const text = `${backHole(ref)}  ${OCCUPIED[ref] ?? ""}`.trim();
            const width = text.length * 7.7 + 16;
            const bx = Math.min(W - LABEL - width - 2, Math.max(LABEL + 2, x - width / 2));
            const by = above ? y - 40 : y + 19;
            return (
              <g key={tag}>
                <circle cx={x} cy={y} r={12} fill="none" stroke="#fbbf24" strokeWidth={3} className="animate-pulse" />
                <rect x={bx} y={by} width={width} height={22} rx={5} fill="#fbbf24" />
                <text x={bx + width / 2} y={by + 15.5} textAnchor="middle" fontSize={13} fontWeight={700} fill="#18181b" fontFamily="ui-monospace, monospace">{text}</text>
              </g>
            );
          })}
        </svg>
      </div>

      <aside className="w-full shrink-0 border-t border-zinc-800 bg-zinc-950/80 lg:w-[400px] lg:overflow-y-auto lg:border-l lg:border-t-0">
        <Section title={`Wire ${step + 1} of ${BOARD_WIRES.length}: ${NET_NAMES[wire.net]}`}>
          <p className="font-mono text-2xl font-semibold text-amber-400">{backHole(wire.from)} → {backHole(wire.to)}</p>
          <p className="mt-1 text-sm text-zinc-200">{wire.label}</p>
          <p className="mt-0.5 text-xs text-zinc-500">On the ESP32 side these holes are marked {wire.from} and {wire.to}.</p>

          <div className="mt-3 flex gap-2">
            <Button onClick={() => setStep(step - 1)}>Back</Button>
            <Button active onClick={() => setStep(step + 1)}>Soldered, next wire</Button>
          </div>

          <dl className="mt-4 space-y-2.5 text-sm">
            {pigtailsFirst.length > 0 && (
              <div className="rounded-md border border-amber-500/40 bg-amber-500/10 p-2.5 text-amber-200">
                <dt className="font-semibold">First</dt>
                {pigtailsFirst.map((h) => (
                  <dd key={h}>
                    {h === "S6"
                      ? `Push the SENSE wire from R1 in from the ESP32 side, through the hole marked ${backHole(h)} on this side, and solder it.`
                      : `Push a ~12 cm wire labelled "${PIGTAIL_NAMES[h]}" in from the ESP32 side, through the hole marked ${backHole(h)} on this side, and solder it.`}{" "}
                    Then solder this wire to its end.
                  </dd>
                ))}
              </div>
            )}
            <Row label="Start">{holeName(wire.from)}</Row>
            <Row label="End">{holeName(wire.to)}{direction(wire) ? `: ${direction(wire)} from the start` : ""}</Row>
            {!wire.bridge && <Row label="Cut">about {cutLength(wire)} mm</Row>}
            <Row label="Beep">{backHole(wire.from)} - {backHole(wire.to)} must beep</Row>
            <Row label="Silent">{silentChecks(wire).join(", ")}</Row>
          </dl>
        </Section>

        <Section title="All wires">
          <ul className="space-y-0.5">
            {BOARD_WIRES.map((w, i) => (
              <li key={w.id}>
                <button onClick={() => setStep(i)}
                  className={`flex w-full items-center gap-2.5 rounded-md px-2 py-1.5 text-left text-sm transition-colors ${i === step ? "bg-zinc-800" : "hover:bg-zinc-900"}`}>
                  <span className="h-2.5 w-2.5 shrink-0 rounded-full" style={{ background: NET_COLORS[w.net] }} />
                  <span className={`font-mono text-xs ${i < step ? "text-zinc-500 line-through" : "text-amber-400"}`}>{backHole(w.from)} → {backHole(w.to)}</span>
                  <span className={`truncate ${i < step ? "text-zinc-500" : "text-zinc-300"}`}>{w.label}</span>
                </button>
              </li>
            ))}
          </ul>
        </Section>
      </aside>
    </div>
  );
}

function Row({ label, children }: { label: string; children: React.ReactNode }) {
  return (
    <div className="flex gap-3">
      <dt className="w-12 shrink-0 text-zinc-500">{label}</dt>
      <dd className="text-zinc-200">{children}</dd>
    </div>
  );
}
