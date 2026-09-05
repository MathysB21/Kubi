import { Link } from "react-router";

function Manual() {
  return (
    <div className="max-w-screen w-full min-h-screen h-full pt-12 pb-16 px-6 bg-zinc-950 text-zinc-100">
      <article className="prose dark:prose-invert max-w-md lg:max-w-xl mx-auto">
        <div className="mb-8">
          <Link
            to="/"
            className="text-xs text-amber-500 hover:text-amber-400 no-underline font-mono uppercase tracking-wider"
          >
            ← Back to Dashboard
          </Link>
        </div>

        <h1 className="text-3xl font-bold tracking-tight text-zinc-100">Kubi Manual</h1>
        <p className="lead text-zinc-400">
          Welcome to the companion guide for <strong>Kubi</strong> — your custom physical desk companion cube.
        </p>

        <section className="space-y-4 border-t border-zinc-800 pt-6">
          <h2 className="text-xl font-semibold text-amber-500">1. Overview &amp; The "No-Button" Rule</h2>
          <p className="text-zinc-400">
            Kubi is built around physical interaction. There are no tactile buttons on the exterior. All navigation,
            acknowledgments, and mode changes are driven entirely by orientation and physical gestures.
          </p>
        </section>

        <section className="space-y-4 border-t border-zinc-800 pt-6">
          <h2 className="text-xl font-semibold text-amber-500">2. Physical Gestures</h2>
          <ul className="text-zinc-400 list-disc pl-5 space-y-2">
            <li><strong>Gentle Tap:</strong> Wakes screen from sleep, toggles view elements, or silences an active chime.</li>
            <li><strong>Shake:</strong> Snoozes reminders, pauses or resets the Pomodoro timer.</li>
            <li><strong>Desk Slam:</strong> Detects rage slams on the desk and triggers Kubi's reaction.</li>
          </ul>
        </section>

        <section className="space-y-4 border-t border-zinc-800 pt-6">
          <h2 className="text-xl font-semibold text-amber-500">3. Operating Faces &amp; Modes</h2>
          <div className="space-y-3 text-zinc-400">
            <p>Rotating the cube changes the active operating mode automatically:</p>
            <ul className="list-disc pl-5 space-y-2">
              <li><strong>Face 1 Up (Focus Clock):</strong> Clean time and date display. Shake to toggle between digital and analog clock view.</li>
              <li><strong>Face 2 Up (Pomodoro Timer):</strong> Rotates display upright and starts your configured focus/break timer with 8-bit chimes.</li>
              <li><strong>Face 3 Up (Mascot &amp; Environment):</strong> Room temperature from the internal sensor and animated Kubi daily routines.</li>
              <li><strong>Face 4 Up (Schedule &amp; Agenda):</strong> Upcoming 3-day schedule summary synced from Google Calendar.</li>
            </ul>
          </div>
        </section>

        <section className="space-y-4 border-t border-zinc-800 pt-6">
          <h2 className="text-xl font-semibold text-amber-500">4. Google Calendar Integration</h2>
          <p className="text-zinc-400">
            To synchronize your agenda, paste your private/secret Google Calendar iCal (.ics) URL into the Kubi Web
            Dashboard. Kubi downloads and updates your agenda automatically in the background.
          </p>
        </section>

        <section className="space-y-4 border-t border-zinc-800 pt-6">
          <h2 className="text-xl font-semibold text-amber-500">5. Power &amp; Battery Care</h2>
          <p className="text-zinc-400">
            Powered by high-capacity 18650 lithium cells with pass-through USB-C power management. Kubi can run
            unplugged for days or remain plugged in safely on your desk.
          </p>
        </section>

        <footer className="mt-16 pt-8 border-t border-zinc-900 text-center text-xs text-zinc-500">
          Happy 21st Birthday Wilhelm! Crafted with care by your brother.
        </footer>
      </article>
    </div>
  );
}

export default Manual;
