import { useEffect, useRef } from "react";

// Cube vertices in normalized local coordinates [-1, 1]
const VERTICES: [number, number, number][] = [
  [-1, -1, -1],
  [1, -1, -1],
  [1, 1, -1],
  [-1, 1, -1],
  [-1, -1, 1],
  [1, -1, 1],
  [1, 1, 1],
  [-1, 1, 1],
];

// 12 edges connecting the 8 vertices
const EDGES: [number, number][] = [
  // Back face
  [0, 1],
  [1, 2],
  [2, 3],
  [3, 0],
  // Front face
  [4, 5],
  [5, 6],
  [6, 7],
  [7, 4],
  // Connecting edges
  [0, 4],
  [1, 5],
  [2, 6],
  [3, 7],
];

export const SpinningCube = () => {
  const canvasRef = useRef<HTMLCanvasElement | null>(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext("2d");
    if (!ctx) return;

    let animId: number;
    let angleX = 0.4;
    let angleY = 0;
    let angleZ = 0.15;
    let bobTime = 0;

    const render = () => {
      const width = canvas.width;
      const height = canvas.height;

      ctx.clearRect(0, 0, width, height);

      // Slow continuous rotations
      angleY += 0.008;
      angleX += 0.003;
      bobTime += 0.02;

      // Slow gentle vertical bobbing (up and down)
      const bobOffset = Math.sin(bobTime) * 6;

      const cosX = Math.cos(angleX);
      const sinX = Math.sin(angleX);
      const cosY = Math.cos(angleY);
      const sinY = Math.sin(angleY);
      const cosZ = Math.cos(angleZ);
      const sinZ = Math.sin(angleZ);

      // Camera / projection setup
      const fov = 180;
      const cameraDistance = 3.2;
      const scale = 58;

      // Project vertices
      const projected: { x: number; y: number; z: number }[] = [];

      for (let i = 0; i < VERTICES.length; i++) {
        let [x, y, z] = VERTICES[i];

        // Rotate Y
        const x1 = x * cosY + z * sinY;
        const z1 = -x * sinY + z * cosY;

        // Rotate X
        const y2 = y * cosX - z1 * sinX;
        const z2 = y * sinX + z1 * cosX;

        // Rotate Z
        const x3 = x1 * cosZ - y2 * sinZ;
        const y3 = x1 * sinZ + y2 * cosZ;

        // Perspective divide
        const depth = z2 + cameraDistance;
        const factor = fov / Math.max(depth, 0.1);

        const projX = width / 2 + (x3 * factor * scale) / fov;
        const projY = 65 + (y3 * factor * scale) / fov + bobOffset;

        projected.push({ x: projX, y: projY, z: z2 });
      }

      // Physical floor shadow:
      // In screen coords, higher bobOffset (+6) means cube is down (closer to the floor).
      // Lower bobOffset (-6) means cube is up (further from the floor).
      // When close to floor: shadow is tighter, sharper, and higher opacity.
      // When far from floor: shadow is more diffused, wider, and softer/fainter.
      const shadowY = 138;
      // Proximity factor: 0 when highest, 1 when lowest
      const proximity = (bobOffset + 6) / 12; // 0 to 1

      // Radius: 32px when close, expanding to 48px when high up
      const radiusX = 32 + (1 - proximity) * 16;
      const radiusY = radiusX * 0.22;

      // Opacity: stronger when close, softer when far
      const baseAlpha = 0.12 + proximity * 0.18; // 0.12 to 0.30

      ctx.save();
      ctx.translate(width / 2, shadowY);
      ctx.scale(1, radiusY / radiusX);

      // Radial gradient in scaled circle space with soft falloff
      const shadowGradient = ctx.createRadialGradient(0, 0, 0, 0, 0, radiusX);
      shadowGradient.addColorStop(0, `rgba(245, 158, 11, ${baseAlpha.toFixed(3)})`);
      shadowGradient.addColorStop(0.3, `rgba(245, 158, 11, ${(baseAlpha * 0.55).toFixed(3)})`);
      shadowGradient.addColorStop(0.65, `rgba(245, 158, 11, ${(baseAlpha * 0.18).toFixed(3)})`);
      shadowGradient.addColorStop(0.88, `rgba(245, 158, 11, ${(baseAlpha * 0.04).toFixed(3)})`);
      shadowGradient.addColorStop(1, "rgba(245, 158, 11, 0)");

      ctx.fillStyle = shadowGradient;
      ctx.beginPath();
      ctx.arc(0, 0, radiusX, 0, Math.PI * 2);
      ctx.fill();
      ctx.restore();

      // Draw edges with amber-500 (#F59E0B / rgb(245, 158, 11))
      ctx.lineCap = "round";
      ctx.lineJoin = "round";

      // Soft ambient outer glow for wireframe edges
      ctx.strokeStyle = "rgba(245, 158, 11, 0.22)";
      ctx.lineWidth = 4.0;
      ctx.beginPath();
      for (let i = 0; i < EDGES.length; i++) {
        const [p1, p2] = EDGES[i];
        ctx.moveTo(projected[p1].x, projected[p1].y);
        ctx.lineTo(projected[p2].x, projected[p2].y);
      }
      ctx.stroke();

      // Crisp core wireframe edges in amber-500
      ctx.strokeStyle = "rgb(245, 158, 11)";
      ctx.lineWidth = 1.8;
      ctx.beginPath();
      for (let i = 0; i < EDGES.length; i++) {
        const [p1, p2] = EDGES[i];
        ctx.moveTo(projected[p1].x, projected[p1].y);
        ctx.lineTo(projected[p2].x, projected[p2].y);
      }
      ctx.stroke();

      animId = requestAnimationFrame(render);
    };

    render();

    return () => {
      cancelAnimationFrame(animId);
    };
  }, []);

  return (
    <div className="w-full flex flex-col items-center justify-center -mb-2 select-none pointer-events-none">
      <canvas
        ref={canvasRef}
        width={240}
        height={165}
        className="block"
        style={{ width: "240px", height: "165px" }}
      />
    </div>
  );
};
