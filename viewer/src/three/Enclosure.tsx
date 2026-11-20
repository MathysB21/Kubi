import { ENCLOSURE_PARTS, MAGNET, PANEL_OUT, SIDE_ANGLES, type EnclosurePart } from "../model/enclosure";
import { EXPLODE_MM } from "../model/scad";
import { MagnetSet } from "./Magnets";
import { StlMesh } from "./StlMesh";

interface Props {
  explode: number; // 0..1
  opacity?: number;
  visible?: Partial<Record<EnclosurePart["id"], boolean>>;
  showMagnets?: boolean;
  selected?: string | null;
  hovered?: string | null;
  onSelect?: (id: EnclosurePart["id"]) => void;
  onHover?: (id: EnclosurePart["id"] | null) => void;
}

// The four printed parts in their assembled positions, pulled apart along the
// roll axis (and the side panels outwards) by `explode`.
export function Enclosure({ explode, opacity = 1, visible = {}, showMagnets = false, selected, hovered, onSelect, onHover }: Props) {
  const shift = (layer: number) => layer * EXPLODE_MM * explode;
  const side = ENCLOSURE_PARTS.find((p) => p.id === "side")!;
  const panelOut = PANEL_OUT * EXPLODE_MM * explode;
  const interactive = (id: EnclosurePart["id"]) => ({
    highlight: selected === id || hovered === id,
    onSelect: onSelect ? () => onSelect(id) : undefined,
    onHover: onHover ? (over: boolean) => onHover(over ? id : null) : undefined,
  });

  return (
    <>
      {ENCLOSURE_PARTS.filter((p) => p.id !== "side" && visible[p.id] !== false).map((p) => (
        <group key={p.id} position={[0, 0, shift(p.layer)]}>
          <StlMesh url={p.url} color={p.color} opacity={opacity} {...interactive(p.id)} />
        </group>
      ))}
      {SIDE_ANGLES.map((a) => (
        <group key={a} rotation={[0, 0, a]}>
          {visible.side !== false && (
            <group position={[panelOut, 0, 0]}>
              <StlMesh url={side.url} color={side.color} opacity={opacity} {...interactive("side")} />
              {showMagnets && <MagnetSet x={MAGNET.panelX} />}
            </group>
          )}
          {showMagnets && visible.front !== false && <MagnetSet x={MAGNET.frameX} />}
        </group>
      ))}
    </>
  );
}
