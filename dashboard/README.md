# Kubi Companion Web Dashboard

This is the React + Tailwind CSS dashboard for **Project Kubi** (ESP32 Desk Companion Cube).

## Overview
- **Framework**: React 19 + TypeScript + Vite + Tailwind CSS v4
- **State & Data Fetching**: TanStack React Query + Lucide Icons + Sonner Toasts
- **Host**: Embedded ESP32 web server running `ESPAsyncWebServer`, served from `LittleFS` at `http://kubi.local`.

## Development

```bash
# Install dependencies
npm install

# Start local dev server with proxy to http://kubi.local
npm run dev
```

## Production Build

```bash
# Compiles and bundles static assets directly into ../firmware/data
npm run build
```

When building for production, Vite outputs minified HTML, JS, and CSS directly to `../firmware/data`, ready to be uploaded to the ESP32's LittleFS partition.

