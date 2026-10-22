import { TriangleAlert } from "lucide-react";
import { SAFETY, STAGES } from "../model/steps";
import { usePersistentSet } from "../hooks";
import { Button } from "../ui";

export function StepsView() {
  const done = usePersistentSet("kubi-viewer:steps-done");
  const total = STAGES.reduce((n, s) => n + s.steps.length, 0);

  return (
    <div className="h-full overflow-y-auto">
      <div className="mx-auto max-w-3xl px-6 py-6">
        <div className="mb-5 flex items-center justify-between">
          <p className="text-sm text-zinc-400">
            Each stage ends with a check, so a mistake is caught before the next stage builds on it. {done.set.size} / {total} done.
          </p>
          <Button onClick={done.clear}>Reset ticks</Button>
        </div>

        <div className="mb-6 rounded-xl border border-amber-500/30 bg-amber-500/5 p-4">
          <h3 className="mb-2 flex items-center gap-2 text-sm font-semibold text-amber-400">
            <TriangleAlert size={16} /> Battery rules for the whole build
          </h3>
          <ul className="list-disc space-y-1 pl-5 text-sm text-zinc-300">
            {SAFETY.map((s) => <li key={s}>{s}</li>)}
          </ul>
        </div>

        {STAGES.map((stage) => {
          const n = stage.steps.filter((s) => done.set.has(s.id)).length;
          return (
            <section key={stage.id} className="mb-6 rounded-xl border border-zinc-800 bg-zinc-900/40">
              <header className="flex items-center justify-between border-b border-zinc-800 px-4 py-3">
                <h3 className="font-semibold text-zinc-100">{stage.title}</h3>
                <span className={`text-xs ${n === stage.steps.length ? "text-emerald-400" : "text-zinc-500"}`}>{n} / {stage.steps.length}</span>
              </header>
              <div className="px-4 py-3">
                {stage.intro && <p className="mb-3 text-sm text-zinc-400">{stage.intro}</p>}
                <ol className="space-y-1">
                  {stage.steps.map((s, i) => (
                    <li key={s.id}>
                      <label className="flex cursor-pointer items-start gap-3 rounded-md px-2 py-1.5 hover:bg-zinc-900">
                        <input type="checkbox" checked={done.set.has(s.id)} onChange={() => done.toggle(s.id)} className="mt-0.5 h-4 w-4 shrink-0 accent-amber-500" />
                        <span className="w-5 shrink-0 text-right font-mono text-xs leading-6 text-zinc-500">{i + 1}.</span>
                        <span className={`text-sm leading-6 ${done.set.has(s.id) ? "text-zinc-500 line-through" : s.warn ? "text-amber-200" : "text-zinc-200"}`}>
                          {s.warn && <TriangleAlert size={13} className="mr-1.5 inline -translate-y-px text-amber-400" />}
                          {s.text}
                        </span>
                      </label>
                    </li>
                  ))}
                </ol>
                {stage.check && (
                  <p className="mt-3 rounded-lg border border-emerald-500/20 bg-emerald-500/5 px-3 py-2 text-sm text-emerald-200">
                    <span className="font-semibold">Check: </span>{stage.check}
                  </p>
                )}
              </div>
            </section>
          );
        })}
      </div>
    </div>
  );
}
