import type { ReactNode } from "react";

export function Section({ title, children, right }: { title: string; children: ReactNode; right?: ReactNode }) {
  return (
    <section className="border-b border-zinc-800/80 px-5 py-4">
      <div className="mb-3 flex items-center justify-between gap-2">
        <h3 className="text-xs font-semibold uppercase tracking-wider text-zinc-400">{title}</h3>
        {right}
      </div>
      {children}
    </section>
  );
}

export function Slider({ label, value, onChange, min = 0, max = 1, step = 0.01, format }: {
  label: string;
  value: number;
  onChange: (v: number) => void;
  min?: number;
  max?: number;
  step?: number;
  format?: (v: number) => string;
}) {
  return (
    <label className="block">
      <div className="mb-1.5 flex justify-between text-xs text-zinc-400">
        <span>{label}</span>
        <span className="font-mono text-zinc-300">{format ? format(value) : value.toFixed(2)}</span>
      </div>
      <input
        type="range"
        min={min}
        max={max}
        step={step}
        value={value}
        onChange={(e) => onChange(Number(e.target.value))}
        className="w-full cursor-pointer accent-amber-500"
      />
    </label>
  );
}

export function Toggle({ label, checked, onChange, swatch }: { label: string; checked: boolean; onChange: (v: boolean) => void; swatch?: string }) {
  return (
    <label className="flex cursor-pointer items-center gap-2.5 py-1 text-sm text-zinc-300 select-none">
      <input type="checkbox" checked={checked} onChange={(e) => onChange(e.target.checked)} className="h-4 w-4 accent-amber-500" />
      {swatch && <span className="h-3 w-3 rounded-sm border border-white/10" style={{ background: swatch }} />}
      {label}
    </label>
  );
}

export function Button({ children, onClick, active = false }: { children: ReactNode; onClick: () => void; active?: boolean }) {
  return (
    <button
      onClick={onClick}
      className={`rounded-lg border px-3 py-1.5 text-xs font-medium transition-colors cursor-pointer ${
        active
          ? "border-amber-500/60 bg-amber-500/10 text-amber-400"
          : "border-zinc-700 bg-zinc-900 text-zinc-300 hover:border-zinc-500 hover:text-zinc-100"
      }`}
    >
      {children}
    </button>
  );
}

export function Segmented<T extends string>({ value, options, onChange }: { value: T; options: { id: T; label: string }[]; onChange: (v: T) => void }) {
  return (
    <div className="inline-flex rounded-lg border border-zinc-800 bg-zinc-950 p-0.5">
      {options.map((o) => (
        <button
          key={o.id}
          onClick={() => onChange(o.id)}
          className={`rounded-md px-3 py-1.5 text-xs font-medium transition-colors cursor-pointer ${
            value === o.id ? "bg-zinc-800 text-amber-400" : "text-zinc-400 hover:text-zinc-200"
          }`}
        >
          {o.label}
        </button>
      ))}
    </div>
  );
}

export function Hint({ children }: { children: ReactNode }) {
  return <p className="text-xs leading-relaxed text-zinc-500">{children}</p>;
}
