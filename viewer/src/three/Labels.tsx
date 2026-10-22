import { useFrame } from "@react-three/fiber";
import { useRef, type MutableRefObject } from "react";
import * as THREE from "three";

// Screen-space labels without drei's <Html>: that gives every label its own
// React root, which React 19 tears down synchronously (console errors, and a
// removeChild crash when the canvas unmounts). Here anchors are plain 3D
// objects in the scene, labels are ordinary divs over the canvas, and a frame
// hook moves each div to its anchor's projected position.

export type Registry<T> = MutableRefObject<Map<string, T>>;

export function useRegistry<T>() {
  return useRef(new Map<string, T>());
}

// Place inside the scene where a label should point.
export function LabelAnchor({ id, position, anchors }: { id: string; position: [number, number, number]; anchors: Registry<THREE.Object3D> }) {
  return (
    <object3D
      position={position}
      ref={(o) => {
        if (o) anchors.current.set(id, o);
        else anchors.current.delete(id);
      }}
    />
  );
}

// Place inside the <Canvas>: projects every anchor onto its label each frame.
export function LabelProjector({ anchors, labels }: { anchors: Registry<THREE.Object3D>; labels: Registry<HTMLElement> }) {
  const v = useRef(new THREE.Vector3());
  useFrame(({ camera, size }) => {
    for (const [id, el] of labels.current) {
      const anchor = anchors.current.get(id);
      if (!anchor) {
        el.style.visibility = "hidden";
        continue;
      }
      anchor.getWorldPosition(v.current);
      v.current.project(camera);
      const behind = v.current.z > 1;
      const x = (v.current.x * 0.5 + 0.5) * size.width;
      const y = (-v.current.y * 0.5 + 0.5) * size.height;
      el.style.visibility = behind ? "hidden" : "visible";
      el.style.transform = `translate(${x}px, ${y}px) translate(-50%, -50%)`;
    }
  });
  return null;
}
