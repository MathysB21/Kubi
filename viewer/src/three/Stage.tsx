import { Canvas } from "@react-three/fiber";
import { ContactShadows, OrbitControls } from "@react-three/drei";
import { Suspense, type ReactNode } from "react";
import { H } from "../model/scad";

// Maps the OpenSCAD frame into the scene: a half turn about the vertical axis
// puts the front face (scad z = 0) towards the default camera, and the cube's
// centre (0, 0, 45) at the origin.
export function ScadSpace({ children }: { children: ReactNode }) {
  return (
    <group rotation={[0, Math.PI, 0]} position={[0, 0, H]}>
      {children}
    </group>
  );
}

export function Stage({ children, onMiss }: { children: ReactNode; onMiss?: () => void }) {
  return (
    <Canvas
      camera={{ position: [215, 135, 250], fov: 35, near: 1, far: 4000 }}
      onPointerMissed={onMiss}
      dpr={[1, 2]}
    >
      <color attach="background" args={["#0c0c0f"]} />
      <hemisphereLight args={["#f4efe6", "#1c1917", 0.9]} />
      <directionalLight position={[120, 180, 140]} intensity={1.8} />
      <directionalLight position={[-140, 60, -120]} intensity={0.6} />
      <directionalLight position={[0, -120, 80]} intensity={0.25} />
      <Suspense fallback={null}>{children}</Suspense>
      <ContactShadows position={[0, -H - 0.5, 0]} opacity={0.45} scale={320} blur={2.6} far={120} />
      <OrbitControls makeDefault enableDamping dampingFactor={0.12} minDistance={60} maxDistance={900} />
    </Canvas>
  );
}
