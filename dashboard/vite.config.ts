import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";
import tailwindcss from "@tailwindcss/vite";

// https://vite.dev/config/
export default defineConfig(({ command }) => ({
  plugins: [react(), tailwindcss()],
  define: {
    // The /sim workbench only works against kubi_sim.exe on 127.0.0.1:8080.
    // It exists in the dev server (npm run dev / npm run sim) and is dropped
    // from the production build that gets flashed to the cube.
    __KUBI_SIM__: JSON.stringify(command === "serve"),
  },
  server: {
    proxy: {
      "/api": {
        target: process.env.VITE_BACKEND_URL || "http://127.0.0.1:8080",
        changeOrigin: true,
        secure: false,
      },
      "/sim/frame": {
        target: process.env.VITE_BACKEND_URL || "http://127.0.0.1:8080",
        changeOrigin: true,
        secure: false,
      },
      "/sim/state": {
        target: process.env.VITE_BACKEND_URL || "http://127.0.0.1:8080",
        changeOrigin: true,
        secure: false,
      },
      "/sim/inject": {
        target: process.env.VITE_BACKEND_URL || "http://127.0.0.1:8080",
        changeOrigin: true,
        secure: false,
      },
    },
  },
  build: {
    // Served by the firmware from LittleFS /www only, so nothing else on the
    // filesystem (NVS-adjacent files, uploads) is reachable over HTTP.
    outDir: "../firmware/data/www",
    emptyOutDir: true, // Empty folder before build
  },
}));
