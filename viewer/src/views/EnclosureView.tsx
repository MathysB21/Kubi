import { useState } from "react";
import { ENCLOSURE_PARTS, type EnclosurePart } from "../model/enclosure";
import { Enclosure } from "../three/Enclosure";
import { ScadSpace, Stage } from "../three/Stage";
import { useEased } from "../hooks";
import { Button, Hint, Section, Slider, Toggle } from "../ui";

type PartId = EnclosurePart["id"];

export function EnclosureView() {
  const explode = useEased(0.55);
  const [visible, setVisible] = useState<Record<PartId, boolean>>({ front: true, midplate: true, back: true, side: true });
  const [magnets, setMagnets] = useState(true);
  const [selected, setSelected] = useState<PartId | null>(null);
  const [hovered, setHovered] = useState<PartId | null>(null);
  const info = ENCLOSURE_PARTS.find((p) => p.id === (selected ?? hovered));

  return (
    <div className="flex h-full min-h-0 flex-col overflow-y-auto lg:flex-row lg:overflow-hidden">
      <div className="relative h-[62vh] shrink-0 lg:h-auto lg:flex-1 lg:shrink">
        <Stage onMiss={() => setSelected(null)}>
          <ScadSpace>
            <Enclosure
              explode={explode.value}
              visible={visible}
              showMagnets={magnets}
              selected={selected}
              hovered={hovered}
              onSelect={(id) => setSelected((s) => (s === id ? null : id))}
              onHover={setHovered}
            />
          </ScadSpace>
        </Stage>
        <div className="pointer-events-none absolute bottom-3 left-4 text-xs text-zinc-500">
          Drag to orbit, scroll to zoom, right-drag to pan. Click a part for details.
        </div>
      </div>

      <aside className="w-full shrink-0 border-t border-zinc-800 bg-zinc-950/80 lg:w-[380px] lg:overflow-y-auto lg:border-l lg:border-t-0">
        <Section title="Exploded view">
          <Slider label="Explode" value={explode.value} onChange={explode.set} format={(v) => `${Math.round(v * 100)}%`} />
          <div className="mt-3 flex gap-2">
            <Button onClick={() => explode.animateTo(1)} active={explode.value > 0.95}>Explode</Button>
            <Button onClick={() => explode.animateTo(0)} active={explode.value < 0.05}>Assemble</Button>
          </div>
        </Section>

        <Section title="Parts">
          {ENCLOSURE_PARTS.map((p) => (
            <Toggle
              key={p.id}
              label={`${p.name}${p.count > 1 ? ` (x${p.count})` : ""}`}
              swatch={p.color}
              checked={visible[p.id]}
              onChange={(v) => setVisible((s) => ({ ...s, [p.id]: v }))}
            />
          ))}
          <Toggle label="Magnets (5 x 2 mm, 32)" swatch="#d4d4d8" checked={magnets} onChange={setMagnets} />
        </Section>

        <Section title={info ? info.name : "Details"}>
          {info ? (
            <div className="space-y-3 text-sm">
              <div className="flex gap-2 text-xs">
                <span className="rounded bg-zinc-800 px-2 py-0.5 text-zinc-300">Print x{info.count}</span>
                <span className="rounded bg-zinc-800 px-2 py-0.5 text-zinc-300">{info.print}</span>
              </div>
              <ul className="list-disc space-y-1.5 pl-4 text-zinc-300">
                {info.notes.map((n) => <li key={n}>{n}</li>)}
              </ul>
            </div>
          ) : (
            <Hint>Hover or click a part.</Hint>
          )}
        </Section>

        <Section title="Printing">
          <ul className="list-disc space-y-1.5 pl-4 text-sm text-zinc-300">
            <li>PETG, 0.2 mm layers, 4 walls, 4 top/bottom, 20 % gyroid.</li>
            <li>No supports. Brim on the front and back.</li>
            <li>Print one side panel first to check the fit and the magnet pockets.</li>
            <li>STLs in print orientation: <span className="font-mono text-xs text-zinc-400">enclosure/stl/</span></li>
          </ul>
        </Section>

        <Section title="Hardware">
          <ul className="list-disc space-y-1.5 pl-4 text-sm text-zinc-300">
            <li>8 x M3 x 10 countersunk: 4 for the back, 4 for the mid-plate.</li>
            <li>32 magnets, 5 x 2 mm. Frame all one pole out, panels the other.</li>
            <li>2 zip ties (up to 4 mm) round the battery holder.</li>
            <li>Perfboard: glue to the pads, or 4 x M2 x 5 after drilling 1.6 mm.</li>
          </ul>
        </Section>
      </aside>
    </div>
  );
}
