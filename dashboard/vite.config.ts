import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";
import tailwindcss from "@tailwindcss/vite";

// https://vite.dev/config/
export default defineConfig({
  plugins: [react(), tailwindcss()],
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
    outDir: '../firmware/data',
    emptyOutDir: true, // Empty folder before build
  }
});
