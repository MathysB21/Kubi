import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";
import tailwindcss from "@tailwindcss/vite";

// The viewer loads the enclosure STLs straight from ../enclosure/stl/assembly,
// so the dev server is allowed to read the repo root. No copies to keep in sync.
export default defineConfig({
  plugins: [react(), tailwindcss()],
  server: {
    port: 5174,
    fs: { allow: [".."] },
  },
  assetsInclude: ["**/*.stl"],
  // three.js alone is ~1 MB; this is a local build tool, not a page on the cube
  build: { chunkSizeWarningLimit: 1600 },
});
