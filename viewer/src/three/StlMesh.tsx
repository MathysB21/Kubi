import { useLoader, type ThreeEvent } from "@react-three/fiber";
import { STLLoader } from "three/addons/loaders/STLLoader.js";

interface Props {
  url: string;
  color: string;
  opacity?: number;
  highlight?: boolean;
  onSelect?: () => void;
  onHover?: (over: boolean) => void;
}

export function StlMesh({ url, color, opacity = 1, highlight = false, onSelect, onHover }: Props) {
  const geometry = useLoader(STLLoader, url);
  const ghost = opacity < 1;
  return (
    <mesh
      geometry={geometry}
      onClick={(e: ThreeEvent<MouseEvent>) => {
        if (!onSelect) return;
        e.stopPropagation();
        onSelect();
      }}
      onPointerOver={(e) => {
        if (!onHover) return;
        e.stopPropagation();
        onHover(true);
      }}
      onPointerOut={() => onHover?.(false)}
      raycast={ghost && !onSelect ? () => null : undefined}
    >
      <meshStandardMaterial
        color={color}
        roughness={0.72}
        metalness={0.02}
        transparent={ghost}
        opacity={opacity}
        depthWrite={!ghost}
        emissive={highlight ? "#f59e0b" : "#000000"}
        emissiveIntensity={highlight ? 0.22 : 0}
      />
    </mesh>
  );
}
