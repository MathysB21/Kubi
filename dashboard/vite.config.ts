import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";
import tailwindcss from "@tailwindcss/vite";

// https://vite.dev/config/
export default defineConfig({
  plugins: [react(), tailwindcss()],
  server: {
    proxy: {
      "/api": {
        target: "http://kubi.local", // The permanent mDNS hostname!
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
