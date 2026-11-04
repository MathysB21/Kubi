import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";
import tailwindcss from "@tailwindcss/vite";

// The enclosure STLs are served straight from ../enclosure/stl/assembly as the
// public directory, so there are no copies to keep in sync. Importing them from
// outside the root would go through /@fs/ URLs instead, which get a 403 when the
// dev server is started through a junction on another drive (C:\Development is
// one, pointing at D:\Development).
export default defineConfig({
  plugins: [react(), tailwindcss()],
  publicDir: "../enclosure/stl/assembly",
  server: {
    port: 5174,
  },
  // three.js alone is ~1 MB; this is a local build tool, not a page on the cube
  build: { chunkSizeWarningLimit: 1600 },
});
