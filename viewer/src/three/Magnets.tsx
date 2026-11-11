import { MAGNET } from "../model/enclosure";

// Four magnets for one side (the +x side; the caller rotates for the others).
export function MagnetSet({ x }: { x: number }) {
  return (
    <>
      {MAGNET.ys.flatMap((y) =>
        MAGNET.zs.map((z) => (
          <mesh key={`${y}-${z}`} position={[x, y, z]} rotation={[0, 0, Math.PI / 2]}>
            <cylinderGeometry args={[MAGNET.d / 2, MAGNET.d / 2, MAGNET.h, 24]} />
            <meshStandardMaterial color="#d4d4d8" metalness={0.85} roughness={0.25} emissive="#7c2d12" emissiveIntensity={0.15} />
          </mesh>
        )),
      )}
    </>
  );
}
