import { StrictMode, lazy, Suspense } from "react";
import { createRoot } from "react-dom/client";
import { BrowserRouter, Route, Routes } from "react-router";
import "./index.css";
import App from "./App.tsx";
import Manual from "./pages/Manual.tsx";

// Dev-server only: the constant-false branch (and its chunk) is dropped from
// the cube build. See __KUBI_SIM__ in vite.config.ts.
const Simulator = __KUBI_SIM__ ? lazy(() => import("./pages/Simulator.tsx")) : null;

createRoot(document.getElementById("root")!).render(
  <StrictMode>
    <BrowserRouter>
      <Routes>
        <Route path="/" element={<App />} />
        <Route path="/manual" element={<Manual />} />
        {Simulator && (
          <Route
            path="/sim"
            element={
              <Suspense fallback={<div className="min-h-screen bg-zinc-950 text-amber-500 font-mono flex items-center justify-center">Loading Kubi Digital Twin...</div>}>
                <Simulator />
              </Suspense>
            }
          />
        )}
      </Routes>
    </BrowserRouter>
  </StrictMode>,
);
