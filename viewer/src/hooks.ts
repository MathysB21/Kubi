import { useCallback, useEffect, useRef, useState } from "react";

// A set of ticked ids that survives reloads (per browser). Storage can be
// unavailable (private windows, blocked site data); the set then just lives in memory.
export function usePersistentSet(key: string) {
  const [set, setSet] = useState<Set<string>>(() => {
    try {
      const raw = localStorage.getItem(key);
      return new Set(raw ? (JSON.parse(raw) as string[]) : []);
    } catch {
      return new Set();
    }
  });

  useEffect(() => {
    try {
      localStorage.setItem(key, JSON.stringify([...set]));
    } catch {
      // ignore: ticks just won't persist
    }
  }, [key, set]);

  const toggle = useCallback((id: string) => {
    setSet((prev) => {
      const next = new Set(prev);
      if (next.has(id)) next.delete(id);
      else next.add(id);
      return next;
    });
  }, []);

  const clear = useCallback(() => setSet(new Set()), []);
  return { set, toggle, clear };
}

// A number that eases towards `target` (for the explode / assemble buttons),
// while `set` jumps straight to a value (for dragging the slider).
export function useEased(initial: number) {
  const [value, setValue] = useState(initial);
  const current = useRef(initial);
  const target = useRef(initial);
  const frame = useRef<number | null>(null);

  const animateTo = useCallback((to: number) => {
    target.current = to;
    if (frame.current !== null) cancelAnimationFrame(frame.current);
    const step = () => {
      const next = current.current + (target.current - current.current) * 0.12;
      if (Math.abs(target.current - next) < 0.002) {
        current.current = target.current;
        frame.current = null;
      } else {
        current.current = next;
        frame.current = requestAnimationFrame(step);
      }
      setValue(current.current);
    };
    frame.current = requestAnimationFrame(step);
  }, []);

  const set = useCallback((to: number) => {
    if (frame.current !== null) cancelAnimationFrame(frame.current);
    frame.current = null;
    current.current = to;
    target.current = to;
    setValue(to);
  }, []);

  useEffect(() => () => {
    if (frame.current !== null) cancelAnimationFrame(frame.current);
  }, []);

  return { value, set, animateTo };
}
