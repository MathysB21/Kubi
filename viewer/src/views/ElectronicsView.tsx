import { useMemo, useState } from "react";
import type * as THREE from "three";
import { COMPONENTS, WIRES_3D, type Component } from "../model/electronics";
import { EXPLODE_MM, type Vec3 } from "../model/scad";
import { Enclosure } from "../three/Enclosure";
import { LabelAnchor, LabelProjector, useRegistry, type Registry } from "../three/Labels";
import { ShapeMesh } from "../three/ShapeMesh";
import { ScadSpace, Stage } from "../three/Stage";
import { WireTube } from "../three/WireTube";
import { useEased } from "../hooks";
import { Button, Hint, Section, Segmented, Slider, Toggle } from "../ui";
import { PerfboardView } from "./PerfboardView";
import { StepsView } from "./StepsView";

type Mode = "fit" | "perfboard" | "steps";

export function ElectronicsView() {
  const [mode, setMode] = useState<Mode>("fit");
  return (
    <div className="flex h-full min-h-0 flex-col">
      <div className="flex items-center gap-4 border-b border-zinc-800 bg-zinc-950 px-5 py-2.5">
        <Segmented<Mode>
          value={mode}
          onChange={setMode}
          options={[
            { id: "fit", label: "Fit in the cube (3D)" },
            { id: "perfboard", label: "Perfboard & wiring" },
            { id: "steps", label: "Assembly steps" },
          ]}
        />
      </div>
      {/* All three stay mounted and are hidden instead: unmounting the 3D view
          tears down drei's <Html> label roots mid-render (React 19 errors), and
          keeping them also keeps each view's camera and scroll position. */}
      <div className={`min-h-0 flex-1 ${mode === "fit" ? "" : "hidden"}`}><FitView /></div>
      <div className={`min-h-0 flex-1 ${mode === "perfboard" ? "" : "hidden"}`}><PerfboardView /></div>
      <div className={`min-h-0 flex-1 ${mode === "steps" ? "" : "hidden"}`}><StepsView /></div>
    </div>
  );
}

function FitView() {
  const explode = useEased(0);
  const [shell, setShell] = useState(0.14);
  const [showShell, setShowShell] = useState(true);
  const [showSides, setShowSides] = useState(false);
  const [wires, setWires] = useState(true);
  const [labels, setLabels] = useState(true);
  const [selected, setSelected] = useState<string | null>(null);
  const [hovered, setHovered] = useState<string | null>(null);
  const [hoverWire, setHoverWire] = useState<string | null>(null);
  const info = COMPONENTS.find((c) => c.id === (selected ?? hovered));
  const wireInfo = WIRES_3D.find((w) => w.id === hoverWire);
  const anchors = useRegistry<THREE.Object3D>();
  const labelEls = useRegistry<HTMLElement>();

  return (
    <div className="flex h-full min-h-0 flex-col overflow-y-auto lg:flex-row lg:overflow-hidden">
      <div className="relative h-[62vh] shrink-0 overflow-hidden lg:h-auto lg:flex-1 lg:shrink">
        <Stage onMiss={() => setSelected(null)}>
          <LabelProjector anchors={anchors} labels={labelEls} />
          <ScadSpace>
            {showShell && (
              <Enclosure explode={explode.value} opacity={shell} visible={{ side: showSides }} />
            )}
            {COMPONENTS.map((c) => (
              <ComponentMesh
                key={c.id}
                c={c}
                explode={explode.value}
                highlight={selected === c.id || hovered === c.id}
                anchors={anchors}
                onSelect={() => setSelected((s) => (s === c.id ? null : c.id))}
                onHover={(over) => setHovered(over ? c.id : null)}
              />
            ))}
            {wires && WIRES_3D.map((w) => (
              <ExplodedWire key={w.id} wire={w} explode={explode.value} highlight={hoverWire === w.id} dim={!!hoverWire && hoverWire !== w.id} onHover={(o) => setHoverWire(o ? w.id : null)} />
            ))}
          </ScadSpace>
        </Stage>
        {labels && (
          <div className="pointer-events-none absolute inset-0">
            {COMPONENTS.map((c) => {
              const lit = selected === c.id || hovered === c.id;
              return (
                <div
                  key={c.id}
                  ref={(el) => {
                    if (el) labelEls.current.set(c.id, el);
                    else labelEls.current.delete(c.id);
                  }}
                  className={`absolute left-0 top-0 whitespace-nowrap rounded-md border px-2 py-0.5 text-[11px] font-medium shadow ${
                    lit ? "z-10 border-amber-500/70 bg-amber-500/20 text-amber-200" : "border-zinc-700 bg-zinc-900/85 text-zinc-200"
                  }`}
                  style={{ visibility: "hidden" }}
                >
                  {c.name.split(" (")[0]}
                </div>
              );
            })}
          </div>
        )}
        {wireInfo && (
          <div className="pointer-events-none absolute left-4 top-4 rounded-lg border border-zinc-700 bg-zinc-900/95 px-3 py-2 text-sm text-zinc-100 shadow-lg">
            <span className="mr-2 inline-block h-2.5 w-2.5 rounded-full" style={{ background: wireInfo.color }} />
            {wireInfo.label}
          </div>
        )}
        <div className="pointer-events-none absolute bottom-3 left-4 text-xs text-zinc-500">
          Click a part for details, hover a wire to trace it. Wire routes and the screen plug position are approximate.
        </div>
      </div>

      <aside className="w-full shrink-0 border-t border-zinc-800 bg-zinc-950/80 lg:w-[400px] lg:overflow-y-auto lg:border-l lg:border-t-0">
        <Section title="Exploded view">
          <Slider label="Explode" value={explode.value} onChange={explode.set} format={(v) => `${Math.round(v * 100)}%`} />
          <div className="mt-3 flex gap-2">
            <Button onClick={() => explode.animateTo(1)} active={explode.value > 0.95}>Explode</Button>
            <Button onClick={() => explode.animateTo(0)} active={explode.value < 0.05}>Assemble</Button>
          </div>
        </Section>

        <Section title="Show">
          <Toggle label="Enclosure (see-through)" checked={showShell} onChange={setShowShell} />
          {showShell && (
            <div className="mb-1 mt-1 pl-6">
              <Slider label="Enclosure opacity" value={shell} onChange={setShell} min={0.04} max={0.6} format={(v) => `${Math.round(v * 100)}%`} />
              <Toggle label="Side panels" checked={showSides} onChange={setShowSides} />
            </div>
          )}
          <Toggle label="Wires" checked={wires} onChange={setWires} />
          <Toggle label="Labels" checked={labels} onChange={setLabels} />
        </Section>

        <Section title="Parts">
          <div className="flex flex-col gap-1">
            {COMPONENTS.map((c) => (
              <button
                key={c.id}
                onClick={() => setSelected((s) => (s === c.id ? null : c.id))}
                onMouseEnter={() => setHovered(c.id)}
                onMouseLeave={() => setHovered(null)}
                className={`rounded-md px-2.5 py-1.5 text-left text-sm transition-colors cursor-pointer ${
                  selected === c.id ? "bg-amber-500/10 text-amber-400" : "text-zinc-300 hover:bg-zinc-900"
                }`}
              >
                {c.name}
              </button>
            ))}
          </div>
        </Section>

        <Section title={info ? info.name : "Details"}>
          {info ? <ComponentDetails c={info} /> : <Hint>Hover or click a part in the list or in the 3D view.</Hint>}
        </Section>
      </aside>
    </div>
  );
}

function ComponentMesh({ c, explode, highlight, anchors, onSelect, onHover }: {
  c: Component;
  explode: number;
  highlight: boolean;
  anchors: Registry<THREE.Object3D>;
  onSelect: () => void;
  onHover: (over: boolean) => void;
}) {
  return (
    <group
      position={[0, 0, c.layer * EXPLODE_MM * explode]}
      onClick={(e) => {
        e.stopPropagation();
        onSelect();
      }}
      onPointerOver={(e) => {
        e.stopPropagation();
        onHover(true);
      }}
      onPointerOut={() => onHover(false)}
    >
      {c.shapes.map((s, i) => <ShapeMesh key={i} shape={s} highlight={highlight} />)}
      <LabelAnchor id={c.id} position={c.label} anchors={anchors} />
    </group>
  );
}

function ExplodedWire({ wire, explode, highlight, dim, onHover }: {
  wire: (typeof WIRES_3D)[number];
  explode: number;
  highlight: boolean;
  dim: boolean;
  onHover: (over: boolean) => void;
}) {
  const points = useMemo<Vec3[]>(
    () => wire.points.map(({ p, layer }) => [p[0], p[1], p[2] + layer * EXPLODE_MM * explode]),
    [wire, explode],
  );
  return <WireTube points={points} color={wire.color} r={wire.r} highlight={highlight} dim={dim} onHover={onHover} />;
}

function ComponentDetails({ c }: { c: Component }) {
  return (
    <div className="space-y-3 text-sm">
      <p className="text-xs text-zinc-400">{c.size}</p>
      <div>
        <h4 className="mb-1 text-xs font-semibold text-zinc-400">Where</h4>
        <p className="text-zinc-300">{c.where}</p>
      </div>
      <div>
        <h4 className="mb-1 text-xs font-semibold text-zinc-400">Connections</h4>
        <ul className="list-disc space-y-1 pl-4 text-zinc-300">
          {c.connections.map((x) => <li key={x}>{x}</li>)}
        </ul>
      </div>
      {c.notes.length > 0 && (
        <div>
          <h4 className="mb-1 text-xs font-semibold text-zinc-400">Notes</h4>
          <ul className="list-disc space-y-1 pl-4 text-zinc-300">
            {c.notes.map((x) => <li key={x}>{x}</li>)}
          </ul>
        </div>
      )}
    </div>
  );
}
