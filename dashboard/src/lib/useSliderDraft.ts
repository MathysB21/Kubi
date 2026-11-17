import { useEffect, useReducer, useRef, useState } from "react";

// A range slider bound to a value that lives on Kubi.
//
// While the finger moves, the slider shows the dragged value (a draft) instead
// of Kubi's, so it never snaps back mid-drag. The value is sent once: when the
// slider goes still for `delayMs`, or straight away on release. Release events
// alone are not enough: some phone browsers never report them for sliders.
// The draft is dropped once Kubi reports the value back, or after a few
// seconds if it never does (a failed save), so the slider shows the truth.
export function useSliderDraft(server: number, commit: (value: number) => void, delayMs = 450) {
  const [draft, setDraft] = useState<number | null>(null);
  const [, rerender] = useReducer((n: number) => n + 1, 0);
  const pending = useRef<number | null>(null);
  const timer = useRef<number | undefined>(undefined);
  const commitRef = useRef(commit);
  commitRef.current = commit;

  const flush = () => {
    if (timer.current !== undefined) window.clearTimeout(timer.current);
    timer.current = undefined;
    if (pending.current === null) return;
    const value = pending.current;
    pending.current = null;
    commitRef.current(value);
    rerender(); // re-run the effects below now that nothing is pending
  };

  const change = (value: number) => {
    setDraft(value);
    pending.current = value;
    if (timer.current !== undefined) window.clearTimeout(timer.current);
    timer.current = window.setTimeout(flush, delayMs);
  };

  // Kubi has the value: show Kubi's again
  useEffect(() => {
    if (draft !== null && pending.current === null && server === draft) setDraft(null);
  });

  // Saved but never reported back: give up on the draft after 5 s
  useEffect(() => {
    if (draft === null || pending.current !== null) return;
    const t = window.setTimeout(() => setDraft(null), 5000);
    return () => window.clearTimeout(t);
  }, [draft, server]);

  useEffect(() => () => window.clearTimeout(timer.current), []);

  return {
    value: draft ?? server,
    /** true while the slider shows a value Kubi has not confirmed yet */
    drafting: draft !== null,
    /** spread onto the <input type="range"> */
    input: {
      value: draft ?? server,
      onChange: (e: React.ChangeEvent<HTMLInputElement>) => change(Number(e.target.value)),
      onPointerUp: flush,
      onTouchEnd: flush,
      onBlur: flush,
    },
  };
}
