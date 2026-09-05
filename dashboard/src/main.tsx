import { StrictMode, lazy, Suspense } from "react";
import { createRoot } from "react-dom/client";
import { BrowserRouter, Route, Routes } from "react-router";
import "./index.css";
import App from "./App.tsx";
import Manual from "./pages/Manual.tsx";

const Simulator = lazy(() => import("./pages/Simulator.tsx"));

createRoot(document.getElementById("root")!).render(
  <StrictMode>
    <BrowserRouter>
      <Routes>
        <Route path="/" element={<App />} />
        <Route path="/manual" element={<Manual />} />
        <Route
          path="/sim"
          element={
            <Suspense fallback={<div className="min-h-screen bg-zinc-950 text-amber-500 font-mono flex items-center justify-center">Loading Kubi Digital Twin...</div>}>
              <Simulator />
            </Suspense>
          }
        />
      </Routes>
    </BrowserRouter>
  </StrictMode>,
);
