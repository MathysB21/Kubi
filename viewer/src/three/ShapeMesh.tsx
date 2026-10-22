import { useMemo } from "react";
import * as THREE from "three";
import type { Shape } from "../model/electronics";

// A small clock face for the screen's visible area, so it reads as a display.
function useScreenTexture() {
  return useMemo(() => {
    const c = document.createElement("canvas");
    c.width = 240;
    c.height = 320;
    const g = c.getContext("2d")!;
    g.fillStyle = "#050507";
    g.fillRect(0, 0, 240, 320);
    g.fillStyle = "#f8fafc";
    g.font = "600 64px ui-monospace, Consolas, monospace";
    g.textAlign = "center";
    g.fillText("12:34", 120, 175);
    g.fillStyle = "#94a3b8";
    g.font = "500 22px ui-monospace, Consolas, monospace";
    g.fillText("Thu, 22 Oct", 120, 215);
    const tex = new THREE.CanvasTexture(c);
    tex.colorSpace = THREE.SRGBColorSpace;
    return tex;
  }, []);
}

export function ShapeMesh({ shape, highlight, opacity = 1 }: { shape: Shape; highlight: boolean; opacity?: number }) {
  const screenTex = useScreenTexture();
  const ghost = opacity < 1;
  const emissive = highlight ? "#f59e0b" : "#000000";
  const emissiveIntensity = highlight ? 0.35 : 0;

  if (shape.kind === "screen") {
    const [x0, y0, z0] = shape.min;
    const [x1, y1, z1] = shape.max;
    // The scene looks at the front, i.e. down scad -z: face the plane that way.
    return (
      <mesh position={[(x0 + x1) / 2, (y0 + y1) / 2, Math.min(z0, z1)]} rotation={[0, Math.PI, 0]}>
        <planeGeometry args={[x1 - x0, y1 - y0]} />
        <meshBasicMaterial map={screenTex} toneMapped={false} transparent={ghost} opacity={opacity} />
      </mesh>
    );
  }

  const material = (
    <meshStandardMaterial
      color={shape.color}
      roughness={shape.metal ? 0.3 : 0.65}
      metalness={shape.metal ? 0.7 : 0.05}
      emissive={emissive}
      emissiveIntensity={emissiveIntensity}
      transparent={ghost}
      opacity={opacity}
    />
  );

  if (shape.kind === "box") {
    const [x0, y0, z0] = shape.min;
    const [x1, y1, z1] = shape.max;
    return (
      <mesh position={[(x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2]}>
        <boxGeometry args={[x1 - x0, y1 - y0, z1 - z0]} />
        {material}
      </mesh>
    );
  }

  // Cylinder geometry runs along y; turn it onto the requested axis
  const rotation: [number, number, number] =
    shape.axis === "x" ? [0, 0, Math.PI / 2] : shape.axis === "z" ? [Math.PI / 2, 0, 0] : [0, 0, 0];
  return (
    <mesh position={shape.center} rotation={rotation}>
      <cylinderGeometry args={[shape.r, shape.r, shape.len, 28]} />
      {material}
    </mesh>
  );
}
