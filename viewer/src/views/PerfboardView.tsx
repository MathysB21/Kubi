import { useState } from "react";
import { NET_COLORS } from "../model/electronics";
import { BOARD_WIRES, NET_NAMES, OFFBOARD_WIRES, PADS, R2, SOCKET_HIGH, SOCKET_LOW, SOCKET_ROW, wirePoints, type BoardWire, type Net } from "../model/perfboard";
import { backHole, COL_LETTERS, COLS, parseHole, ROWS } from "../model/scad";
import { usePersistentSet } from "../hooks";
import { Button, Hint, Section, Segmented, Toggle } from "../ui";

type Side = "component" | "solder";

// Drawing grid: one hole pitch = P px. Columns run -3 .. 23 so the ESP32's
// antenna overhang (beyond the left board edge) fits in the drawing.
const P = 28;
const ML = 34;
const MT = 30;
const W = ML + 27 * P;
const HGT = MT + 17 * P;

const PAD_TAGS: Record<string, string> = { S2: "5i", T2: "Gi", S4: "5a", T4: "Ga", S6: "Bt" };

export function PerfboardView() {
  const [side, setSide] = useState<Side>("component");
  const [outline, setOutline] = useState(true);
  const [hover, setHover] = useState<string | null>(null);
  const done = usePersistentSet("kubi-viewer:wires-done");

  const colX = (col: number) => ML + ((side === "solder" ? COLS + 1 - col : col) + 3) * P;
  // Row 14 at the top, row 1 at the bottom, as the board is printed
  const rowY = (r: number) => MT + (ROWS - r + 1.6) * P;
  const at = (ref: string) => {
    const h = parseHole(ref);
    return { x: colX(h.col), y: rowY(h.row) };
  };
  // Board edges, in hole units: 2.31 pitches beyond columns A/T, 1.37 beyond rows 1/14
  const boardX0 = Math.min(colX(1 - 2.31), colX(COLS + 2.31));
  const boardX1 = Math.max(colX(1 - 2.31), colX(COLS + 2.31));
  // ESP32 outline: x -33.1..17.9 mm and y -15.27..12.73 mm (rows 1.49..12.51), in hole units
  const espX0 = Math.min(colX(-2.53), colX(17.55));
  const espX1 = Math.max(colX(-2.53), colX(17.55));
  // The back of the board has its own printed letters
  const nm = (hole: string) => (side === "solder" ? backHole(hole) : hole);
  const hovered = BOARD_WIRES.find((w) => w.id === hover);
  const boardDone = BOARD_WIRES.filter((w) => done.set.has(w.id)).length;
  const offDone = OFFBOARD_WIRES.filter((w) => done.set.has(w.id)).length;

  return (
    <div className="flex h-full min-h-0 flex-col overflow-y-auto lg:flex-row lg:overflow-hidden">
      <div className="shrink-0 p-5 lg:min-h-0 lg:flex-1 lg:shrink lg:overflow-auto">
        <div className="mb-4 flex flex-wrap items-center gap-4">
          <Segmented<Side>
            value={side}
            onChange={setSide}
            options={[
              { id: "component", label: "Component side (top)" },
              { id: "solder", label: "Solder side (underneath)" },
            ]}
          />
          <Toggle label="ESP32 outline" checked={outline} onChange={setOutline} />
        </div>

        <svg viewBox={`0 0 ${W} ${HGT}`} className="w-full max-w-[1100px] select-none">
          {/* board */}
          <rect x={boardX0} y={rowY(ROWS + 1.37)} width={boardX1 - boardX0} height={rowY(-0.37) - rowY(ROWS + 1.37)} rx={6}
            fill={side === "component" ? "#3b2f20" : "#2b2418"} stroke="#6b5636" />
          {/* column letters and row numbers, as printed on the board */}
          {Array.from({ length: COLS }, (_, i) => (
            <text key={i} x={colX(i + 1)} y={MT + 0.6 * P} textAnchor="middle" fontSize={10} fill="#a1a1aa">{side === "solder" ? COL_LETTERS[COLS - 1 - i] : COL_LETTERS[i]}</text>
          ))}
          {Array.from({ length: ROWS }, (_, i) => (
            <text key={i} x={Math.min(colX(-2), colX(23)) - 8} y={rowY(i + 1) + 3.5} textAnchor="middle" fontSize={10} fill="#a1a1aa">
              {String(i + 1).padStart(2, "0")}
            </text>
          ))}
          {/* holes */}
          {Array.from({ length: ROWS }, (_, ri) => Array.from({ length: COLS }, (_, ci) => (
            <circle key={`${ri}-${ci}`} cx={colX(ci + 1)} cy={rowY(ri + 1)} r={4.2} fill="#c08a3e" stroke="#7a5524" strokeWidth={0.8} />
          ))).flat()}

          {/* ESP32 sockets (rows 2 and 12, columns A-O); labels on the side away from row 14's headers */}
          {[{ row: SOCKET_ROW.low, pins: SOCKET_LOW, above: false }, { row: SOCKET_ROW.high, pins: SOCKET_HIGH, above: false }].map(({ row, pins, above }) => (
            <g key={row}>
              {side === "component" && (
                <rect x={Math.min(colX(1), colX(15)) - P / 2} y={rowY(row) - P / 2.4} width={15 * P} height={P / 1.2} rx={3}
                  fill="#0a0a0a" stroke="#3f3f46" opacity={0.85} />
              )}
              {pins.map((name, i) => (
                <g key={name + i}>
                  <circle cx={colX(i + 1)} cy={rowY(row)} r={3} fill={pinColor(name)} />
                  <text x={colX(i + 1)} y={rowY(row) + (above ? -P * 0.55 : P * 0.72)} textAnchor="middle" fontSize={8.5}
                    fill={pinColor(name)} fontFamily="ui-monospace, monospace">{name}</text>
                </g>
              ))}
            </g>
          ))}

          {/* ESP32 outline */}
          {outline && (
            <g pointerEvents="none">
              <rect x={espX0} y={rowY(12.51)} width={espX1 - espX0} height={rowY(1.49) - rowY(12.51)} rx={6}
                fill="none" stroke="#a1a1aa" strokeDasharray="6 5" strokeWidth={1.2} />
              <text x={(espX0 + espX1) / 2} y={rowY(9) + 4} textAnchor="middle" fontSize={11} fill="#71717a">
                ESP32 sits ~10 mm above (antenna end at column A)
              </text>
            </g>
          )}

          {/* R2 */}
          {(() => {
            const a = at(R2.from);
            const b = at(R2.to);
            return (
              <g>
                {side === "component" ? (
                  <>
                    <line x1={a.x} y1={a.y} x2={b.x} y2={b.y} stroke="#d4d4d8" strokeWidth={1.5} />
                    <rect x={a.x - 5} y={Math.min(a.y, b.y) + Math.abs(b.y - a.y) * 0.25} width={10} height={Math.abs(b.y - a.y) * 0.5} rx={4}
                      fill="#d6b88a" stroke="#8a6d3b" />
                  </>
                ) : (
                  <line x1={a.x} y1={a.y} x2={b.x} y2={b.y} stroke="#d6b88a" strokeWidth={1} strokeDasharray="2 3" />
                )}
                <text x={a.x + 9} y={(a.y + b.y) / 2 + 3} fontSize={10} fill="#d6b88a">R2 10k</text>
              </g>
            );
          })()}

          {/* headers and pads */}
          {PADS.map((p) => {
            const c = at(p.hole);
            const tag = PAD_TAGS[p.hole];
            return (
              <g key={p.hole}>
                <rect x={c.x - 6} y={c.y - 6} width={12} height={12} rx={p.kind === "pigtail" ? 6 : 2}
                  fill={p.kind === "female" ? "#0a0a0a" : "none"} stroke={p.kind === "pigtail" ? "#fbbf24" : "#e4e4e7"} strokeWidth={1.4} />
                {tag ? (
                  <text x={c.x} y={c.y - 9} textAnchor="middle" fontSize={9} fill="#fbbf24" fontFamily="ui-monospace, monospace">{tag}</text>
                ) : (
                  <text x={c.x} y={c.y + 19} textAnchor="middle" fontSize={8.5} fill="#e4e4e7" fontFamily="ui-monospace, monospace">{p.label}</text>
                )}
              </g>
            );
          })}

          {/* wires */}
          {BOARD_WIRES.map((w) => {
            const a = at(w.from);
            const b = at(w.to);
            const isHover = hover === w.id;
            const isDone = done.set.has(w.id);
            const color = NET_COLORS[w.net];
            if (w.bridge) {
              return (
                <line key={w.id} x1={a.x} y1={a.y} x2={b.x} y2={b.y} stroke={color} strokeWidth={isHover ? 7 : 5} strokeLinecap="round"
                  opacity={hover && !isHover ? 0.25 : isDone ? 0.45 : 0.95} onMouseEnter={() => setHover(w.id)} onMouseLeave={() => setHover(null)} />
              );
            }
            // The route it is actually laid along (see the Wire by wire view)
            const d = wirePoints(w).map(([c, r], n) => `${n ? "L" : "M"} ${colX(c)} ${rowY(r)}`).join(" ");
            return (
              <g key={w.id} onMouseEnter={() => setHover(w.id)} onMouseLeave={() => setHover(null)} className="cursor-pointer"
                onClick={() => done.toggle(w.id)}>
                <path d={d} fill="none" stroke="transparent" strokeWidth={12} />
                <path d={d} fill="none" stroke={color} strokeWidth={isHover ? 4.5 : 2.4} strokeLinecap="round" strokeLinejoin="round"
                  strokeDasharray={isDone ? "5 5" : undefined} opacity={hover && !isHover ? 0.18 : isDone ? 0.5 : 0.95} />
                <circle cx={a.x} cy={a.y} r={3.4} fill={color} opacity={hover && !isHover ? 0.2 : 1} />
                <circle cx={b.x} cy={b.y} r={3.4} fill={color} opacity={hover && !isHover ? 0.2 : 1} />
              </g>
            );
          })}
        </svg>

        <div className="mt-3 min-h-[44px] max-w-[1100px]">
          {hovered ? <WireCaption w={hovered} nm={nm} /> : (
            <Hint>
              Hover a wire to trace it, click it to tick it off. Tags: 5i / Gi = 5 V and GND in from the MT3608, 5a / Ga = 5 V and GND out to the
              PAM8403, Bt = battery sense from R1. Round amber pads are wires soldered straight into the board; square ones are header pins.
            </Hint>
          )}
        </div>

        <div className="mt-2 flex max-w-[1100px] flex-wrap gap-x-4 gap-y-1.5">
          {(Object.keys(NET_NAMES) as Net[]).filter((n) => BOARD_WIRES.some((w) => w.net === n)).map((n) => (
            <span key={n} className="flex items-center gap-1.5 text-xs text-zinc-400">
              <span className="h-2.5 w-2.5 rounded-full" style={{ background: NET_COLORS[n] }} /> {NET_NAMES[n]}
            </span>
          ))}
        </div>
      </div>

      <aside className="w-full shrink-0 border-t border-zinc-800 bg-zinc-950/80 lg:w-[400px] lg:overflow-y-auto lg:border-l lg:border-t-0">
        <Section title={`On the board: ${boardDone} / ${BOARD_WIRES.length}`} right={<Button onClick={done.clear}>Reset ticks</Button>}>
          <Hint>
            Solder side, in this order. Strip 2-3 mm, tin, keep wires flat and away from the antenna end (column A). Leave the
            pigtail holes (S2, T2, S4, T4, S6, O14-Q14, ESP32-side names) empty until their outside wire is through.
          </Hint>
          <ul className="mt-3 space-y-1">
            {BOARD_WIRES.map((w) => (
              <li key={w.id}>
                <label
                  className={`flex cursor-pointer items-start gap-2.5 rounded-md px-2 py-1.5 text-sm transition-colors ${hover === w.id ? "bg-zinc-800" : "hover:bg-zinc-900"}`}
                  onMouseEnter={() => setHover(w.id)}
                  onMouseLeave={() => setHover(null)}
                >
                  <input type="checkbox" checked={done.set.has(w.id)} onChange={() => done.toggle(w.id)} className="mt-0.5 h-4 w-4 accent-amber-500" />
                  <span className="mt-1 h-2.5 w-2.5 shrink-0 rounded-full" style={{ background: NET_COLORS[w.net] }} />
                  <span className={done.set.has(w.id) ? "text-zinc-500 line-through" : "text-zinc-200"}>
                    <span className="font-mono text-xs text-amber-400">{nm(w.from)} → {nm(w.to)}</span>
                    <span className="ml-2">{w.label}</span>
                  </span>
                </label>
              </li>
            ))}
          </ul>
        </Section>

        <Section title={`Off the board: ${offDone} / ${OFFBOARD_WIRES.length}`}>
          <ul className="space-y-1">
            {OFFBOARD_WIRES.map((w) => (
              <li key={w.id}>
                <label className="flex cursor-pointer items-start gap-2.5 rounded-md px-2 py-1.5 text-sm hover:bg-zinc-900">
                  <input type="checkbox" checked={done.set.has(w.id)} onChange={() => done.toggle(w.id)} className="mt-0.5 h-4 w-4 accent-amber-500" />
                  <span className="mt-1 h-2.5 w-2.5 shrink-0 rounded-full" style={{ background: NET_COLORS[w.net] }} />
                  <span className={done.set.has(w.id) ? "text-zinc-500 line-through" : "text-zinc-200"}>
                    {w.from} <span className="text-zinc-500">→</span> {w.to}
                    {w.note && <span className="block text-xs text-zinc-500">{w.note}</span>}
                  </span>
                </label>
              </li>
            ))}
          </ul>
        </Section>

        <Section title="Before power-up">
          <ul className="list-disc space-y-1.5 pl-4 text-sm text-zinc-300">
            <li>ESP32 out: every wire beeps end to end.</li>
            <li>No beep between 5 V, 3.3 V and GND (ESP32-side names): O2-N2, O12-N12, O2-O12.</li>
            <li>No beep between neighbouring socket holes.</li>
            <li>S6 to GND reads about 10 kohm (R2).</li>
          </ul>
        </Section>
      </aside>
    </div>
  );
}

function WireCaption({ w, nm }: { w: BoardWire; nm: (hole: string) => string }) {
  return (
    <div className="inline-flex items-center gap-2 rounded-lg border border-zinc-700 bg-zinc-900 px-3 py-2 text-sm text-zinc-100">
      <span className="h-2.5 w-2.5 rounded-full" style={{ background: NET_COLORS[w.net] }} />
      <span className="font-mono text-amber-400">{nm(w.from)} → {nm(w.to)}</span>
      <span>{w.label}</span>
      <span className="text-zinc-500">({NET_NAMES[w.net]})</span>
    </div>
  );
}

function pinColor(name: string) {
  if (name === "GND") return NET_COLORS.gnd;
  if (name === "VIN") return NET_COLORS.v5;
  if (name === "3V3") return NET_COLORS.v33;
  if (["D23", "D18", "D5", "D2", "D4"].includes(name)) return NET_COLORS.spi;
  if (name === "D32") return NET_COLORS.bl;
  if (["D21", "D22"].includes(name)) return NET_COLORS.i2c;
  if (["D25", "D26"].includes(name)) return NET_COLORS.audio;
  if (name === "D34") return NET_COLORS.sense;
  return "#52525b";
}
