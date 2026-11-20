import { useState } from "react";
import { Box, Cpu } from "lucide-react";
import { EnclosureView } from "./views/EnclosureView";
import { ElectronicsView } from "./views/ElectronicsView";

type Tab = "enclosure" | "electronics";

const TABS: { id: Tab; label: string; icon: typeof Box }[] = [
  { id: "enclosure", label: "Enclosure", icon: Box },
  { id: "electronics", label: "Electronics", icon: Cpu },
];

export default function App() {
  const [tab, setTab] = useState<Tab>("enclosure");

  return (
    <div className="flex h-screen flex-col bg-zinc-950 text-zinc-100">
      <header className="flex items-center gap-6 border-b border-zinc-800 px-5 py-3">
        <div>
          <h1 className="text-base font-semibold tracking-tight">Kubi build viewer</h1>
          <p className="text-xs text-zinc-500">90 mm cube, PETG, from enclosure/kubi_enclosure.scad</p>
        </div>
        <nav className="flex gap-1">
          {TABS.map(({ id, label, icon: Icon }) => (
            <button
              key={id}
              onClick={() => setTab(id)}
              className={`flex items-center gap-2 rounded-lg px-3.5 py-2 text-sm font-medium transition-colors cursor-pointer ${
                tab === id ? "bg-amber-500/10 text-amber-400" : "text-zinc-400 hover:bg-zinc-900 hover:text-zinc-200"
              }`}
            >
              <Icon size={16} /> {label}
            </button>
          ))}
        </nav>
      </header>
      {/* Both tabs stay mounted (see ElectronicsView): switching is instant and
          each 3D view keeps its camera. */}
      <main className={`min-h-0 flex-1 ${tab === "enclosure" ? "" : "hidden"}`}><EnclosureView /></main>
      <main className={`min-h-0 flex-1 ${tab === "electronics" ? "" : "hidden"}`}><ElectronicsView /></main>
    </div>
  );
}
