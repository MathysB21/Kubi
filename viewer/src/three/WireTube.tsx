import { useMemo } from "react";
import * as THREE from "three";
import type { Vec3 } from "../model/scad";

interface Props {
  points: Vec3[];
  color: string;
  r?: number;
  highlight?: boolean;
  dim?: boolean;
  onHover?: (over: boolean) => void;
}

// A wire as a smooth tube through its route points.
export function WireTube({ points, color, r = 0.55, highlight = false, dim = false, onHover }: Props) {
  const geometry = useMemo(() => {
    const curve = new THREE.CatmullRomCurve3(points.map((p) => new THREE.Vector3(...p)), false, "centripetal");
    return new THREE.TubeGeometry(curve, Math.max(24, points.length * 16), highlight ? r * 1.8 : r, 8, false);
  }, [points, r, highlight]);

  return (
    <mesh
      geometry={geometry}
      onPointerOver={(e) => {
        if (!onHover) return;
        e.stopPropagation();
        onHover(true);
      }}
      onPointerOut={() => onHover?.(false)}
    >
      <meshStandardMaterial
        color={color}
        roughness={0.45}
        emissive={highlight ? color : "#000000"}
        emissiveIntensity={highlight ? 0.6 : 0}
        transparent={dim}
        opacity={dim ? 0.18 : 1}
      />
    </mesh>
  );
}
