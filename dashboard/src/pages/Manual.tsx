import type { ReactNode } from "react";
import { Link } from "react-router";

// The manual is told by Kubi himself, with Axel (his ADXL345 accelerometer,
// the bit that feels every tap) heckling in the margins. In the style of the
// Sunrise lamp's manual. Keep it true: every joke sits on a real behaviour,
// so update it when a face, gesture or setting changes. Never mention the
// special-day scenes by name (they are Easter eggs, see AGENTS.md).

// Kubi's own little asides
function Aside({ children }: { children: ReactNode }) {
  return <span className="text-xs text-zinc-500 italic">{children}</span>;
}

// Axel heckling from the sidelines
function Axel({ children }: { children: ReactNode }) {
  return <p className="text-xs text-zinc-500">{children}</p>;
}

// Words that need a *bleep*
function Bleep({ children, title = "*bleeep*" }: { children: ReactNode; title?: string }) {
  return (
    <span title={title} className="inline-block filter-[url(#pixelate)]">
      {children}
    </span>
  );
}

function Code({ children }: { children: ReactNode }) {
  return (
    <span className="font-mono text-amber-400 bg-zinc-800 px-2 py-0.5 rounded-md not-prose text-sm">{children}</span>
  );
}

function Manual() {
  return (
    <div className="max-w-screen w-full min-h-screen h-full pt-12 pb-16 px-6 bg-zinc-950 text-zinc-100">
      <article className="prose prose-invert prose-headings:text-zinc-100 prose-h1:text-amber-500 prose-strong:text-zinc-100 max-w-md lg:max-w-xl mx-auto">
        <svg className="hidden">
          <filter id="pixelate" x="0%" y="0%" width="100%" height="100%">
            <feTurbulence type="fractalNoise" baseFrequency="0.5" numOctaves="1" result="noise" />
            <feDisplacementMap in="SourceGraphic" in2="noise" scale="10" xChannelSelector="R" yChannelSelector="G" />
          </filter>
        </svg>
        <title>Kubi - Manual</title>

        <div className="not-prose mb-8">
          <Link
            to="/"
            className="text-xs text-amber-500 hover:text-amber-400 no-underline font-mono uppercase tracking-wider"
          >
            ← Back to Dashboard
          </Link>
        </div>

        <h1>Manual</h1>
        <p>Hey Wilhelm! 👋</p>
        <p>
          Happy 21st, boet. Your brother started building me on the 4th of September, and he's been soldering, sawing,
          sanding and swearing at me ever since. All for you.
        </p>
        <p>
          So please give this a read. It's not as long as it looks, and there's a lot of useful stuff hiding between my
          jokes. <Aside>They're good jokes. Trust me.</Aside>
        </p>
        <p>Enjoy your brand new, custom, one-of-a-kind desk companion.</p>
        <p className="font-bold text-center">- Welcome to Project Kubi -</p>

        <section>
          <h1>Hi, I'm Kubi</h1>
          <p>
            That's me. The wooden cube with the brass screws and the screen. I'm a desk buddy: I tell the time, I run your
            focus sessions, I have a whole daily routine of my own, I glow at night, and I've got a game in me too.
          </p>
          <p>
            You might notice I have no buttons. That's on purpose. Everything I do, you do by{" "}
            <strong>turning me over</strong>, <strong>tapping me</strong> or <strong>shaking me</strong>.
          </p>
          <p>
            The one who actually feels all of that is Axel, my accelerometer. He lives in my belly and he'll chip in now
            and then. Sorry in advance.
          </p>
          <Axel>Hi. I feel everything. Everything.</Axel>
        </section>

        <section>
          <h1>Setup</h1>
          <h2>Charging Up</h2>
          <p>
            Round the back there's a little slot with a <strong>USB-C</strong> port in it. That's where I eat. Any phone
            charger with a USB-C cable will do.
          </p>
          <p>
            Charging from empty takes about 5 to 6 hours, a bit longer if you keep using me while I charge, which is
            perfectly fine.
          </p>
          <p>
            There's another USB port somewhere on my insides. That one's Mathys's maintenance hatch: it can't charge
            me. <Aside>Don't go poking around in there.</Aside>
          </p>

          <h2>WiFi Setup</h2>
          <p>
            I need your WiFi to know what time it is. Without it I don't know if it's breakfast or bedtime, and I take my
            routine very seriously.
          </p>
          <p>
            The first time I wake up somewhere new, I'll say <strong>Hello!</strong> on my screen and make a little WiFi
            network of my own. Grab your phone, open your WiFi settings and join:
          </p>
          <p className="text-center">
            <Code>Kubi-Setup</Code>
          </p>
          <p>
            Give me a second or two and a page should pop up on your phone by itself. If it doesn't, open your browser and
            go to <Code>192.168.4.1</Code>.
          </p>
          <p>
            Pick your home WiFi from the list, type in the password and hit save. I'll handle the rest.{" "}
            <span className="font-bold italic">Automagically.</span>
          </p>
          <p>
            Once I'm on, my screen says <strong>You're connected!</strong> and shows you where my app lives. Write it
            down, then give me a tap and I'll get on with my life.
          </p>
          <Axel>If nobody joins in 3 minutes he restarts and tries again. He's patient like that.</Axel>
          <p>
            I remember every WiFi I've been on, so taking me to a place I already know just works. A new place means a
            new round of the above.
          </p>
        </section>

        <section>
          <h1>The App</h1>
          <p>
            Every{" "}
            <Bleep>damn</Bleep> thing has an app these days, and I'm no exception. Except mine lives inside me, so there's
            nothing to install.
          </p>
          <p>On your phone (on the same WiFi as me), open your browser and type:</p>
          <p className="text-center">
            <Code>http://kubi.local</Code>
          </p>
          <p>
            If your phone shrugs at that, use the number my screen showed you when I connected. It'll look something like{" "}
            <Code>192.168.x.x</Code>.
          </p>
          <p>
            In there you can see and change pretty much everything about me. Mathys loves his settings.{" "}
            <Aside>Nerd.</Aside>
          </p>
          <p>Go on, give it a scroll. This manual is in there too, as you've clearly worked out.</p>
        </section>

        <section>
          <h1>My Faces</h1>
          <p>
            Here's the big one. I have four sides, and whichever side I'm standing on decides what I do. Roll me onto
            another side, let go, and in under half a second my screen turns the right way up and I switch.
          </p>
          <Axel>Roll him gently. I'm on padded mounts but I'm not a stress ball.</Axel>

          <h2>The Clock</h2>
          <p>
            Big, clean time with the date underneath. The face you'll probably leave me on the most.
          </p>
          <p>
            <strong>Shake</strong> me and I swap between a digital and an analog clock. I remember which one you like,
            even after a restart.
          </p>

          <h2>The Pomodoro Timer</h2>
          <p>
            For when you need to actually get some work done. I count down a focus session, then a short break, and
            after a few rounds you get a long break.
          </p>
          <p>
            When it's ready, a slow flashing <strong>START</strong> waits for you. <strong>Tap</strong> me to start.
            Tap again to pause, and again to carry on. <strong>Shake</strong> me to skip to the next phase.
          </p>
          <p>
            I chime when a phase ends, and when you've finished all your rounds I play a proper little fanfare. You
            earned it.
          </p>
          <p>
            Wander off to another face and come back, and a paused timer stays paused. How long everything lasts and
            what colours I use are all in the app.
          </p>

          <h2>Kubi's World</h2>
          <p>This is my favourite face, because it's all about me.</p>
          <p>
            I have a life, you know. On weekdays I'm at the gym early, then working from home, with a café lunch around
            midday. I always stop to watch the sunset, then put some music on after dark, and I'm in bed by ten. On
            weekends I sleep in, do coffee in the morning, and spend the afternoon on the beach in summer or in a flower
            field when it's not.
          </p>
          <p>
            I pick the scene myself from the time and the day. <strong>Tap</strong> or <strong>shake</strong> me and
            I'll blink at you. That's all you get, I'm busy.
          </p>
          <p>
            Want to see a scene right now? The app lets you peek at any of them for 10 minutes, then I go back to my
            routine.
          </p>
          <p>
            The little badge in the corner is the temperature, measured by a sensor inside me.
          </p>
          <p>
            Some days are special, by the way. Keep an eye on me.{" "}
            <Aside>That's all I'm saying.</Aside>
          </p>
          <p>
            If nobody touches me for a while, my screen takes a nap (5 minutes, unless you change it in the app). A tap
            or a nudge wakes it, and that tap doesn't count for anything else.
          </p>

          <h2>The Glow</h2>
          <p>
            My night light. A soft, breathing pixel orb that slowly drifts through night colours, with my screen turned
            right down. Made for a dark room.
          </p>
          <p>
            <strong>Tap</strong> me to skip to the next colour. I do that one silently, because it's probably bedtime.
            This face never sleeps.
          </p>

          <h2>Screen Up: The Maze</h2>
          <p>Okay, this is the secret level.</p>
          <p>
            Lay me on my back with my screen facing the ceiling, and I turn into a marble maze. Hold still for a moment
            while the ball waits on the start, then <strong>tilt me</strong> to roll it. Whichever way is down, the ball
            goes. Just like the real thing.
          </p>
          <p>
            Get the ball to the gold checkered square without falling in any of the holes. A hole sends you back to the
            start, and the clock keeps running. <Aside>Brutal, I know.</Aside> Finish and I'll show your time and your
            best, then load the next maze. There are fifty, and they get mean.
          </p>
          <p>
            The maze faces whoever tipped me back, so it always reads the right way round for you. While you're playing
            I won't switch faces at the slightest wobble: stand me up and hold me there for 2 seconds to leave. If you
            wander off mid-game, I let go by myself after 45 seconds.
          </p>
          <Axel>This is my time to shine. Literally everything in this game is me.</Axel>
          <p>Lying face down does nothing, by the way. I just keep showing whatever I was showing.</p>
        </section>

        <section>
          <h1>Night Mode</h1>
          <p>
            Even desk buddies need their beauty sleep. From 11 at night until half past six in the morning I go into
            night mode.
          </p>
          <p>
            Only the clock stays on, in a deep red with my screen turned right down, so you can check the time at 3 am
            without lighting up the whole room. Every other face goes dark.
          </p>
          <p>
            Want to see a dark face anyway? <strong>Tap</strong> me and I'll show it for 10 seconds. Roll me onto
            another face and I'll show that one for 10 seconds too, then lights out again.
          </p>
          <p>
            "Can you switch the backlight off and just show the numbers?" Mathys asked that too. Sadly no: my screen
            is an LCD, and without its backlight it shows nothing at all. So I do the next best thing: red digits,
            with the backlight as low as it goes. <Aside>Physics. Can't argue with it.</Aside>
          </p>
          <p>
            All of it is yours to change in the app: the hours, which faces stay on, and how bright my night glow is.
            There's even a button to preview night mode in broad daylight.
          </p>
          <Axel>Battery reminders keep quiet during night hours too. You're welcome.</Axel>
        </section>

        <section>
          <h1>Taps, Shakes and Slams</h1>
          <h2>Tap</h2>
          <p>
            A gentle finger tap on any side. <strong>Gentle.</strong> Soft taps work just fine, I promise. There's no
            need to knock on me like I owe you money.
          </p>
          <p>
            Tapping also dismisses any note on my screen, like a battery reminder. And if my screen is asleep, the first
            tap just wakes me up.
          </p>
          <h2>Shake</h2>
          <p>
            A quick, short shake back and forth, like you're getting the last bit of sauce out of the bottle. One shake
            counts once, so there's no need to keep going.
          </p>
          <h2>Slam</h2>
          <p>
            If someone slams the desk hard enough (and I do mean <em>hard</em>), I'll say <strong>ouch</strong>. That's
            it. That's the feature. Please don't test it too often.
          </p>
          <Axel>
            Every single time, I feel it. Every. Single. <Bleep>Time</Bleep>.
          </Axel>
          <p>
            Picking me up, putting me down or rolling me to another face doesn't count as a tap. Mathys recorded loads of
            taps and fumbles on me to teach me the difference.
          </p>
        </section>

        <section>
          <h1>Battery</h1>
          <p>
            I've got two big lithium cells inside, so I can sit anywhere without a cable. A full charge lasts me about
            15 hours. <Aside>I'm a desk buddy, not a phone. Different career.</Aside>
          </p>
          <p>
            The simplest way to live with me is to keep me plugged in on your desk and unplug me when you want to carry me
            around. Being plugged in all the time is fine.
          </p>

          <h2>Reminders</h2>
          <p>
            In the app you can ask me to remind you to unplug me when I'm full enough, or to plug me in when I'm running
            low. I'll chime and show a note. Tap me to make it go away.
          </p>
          <p>
            During night hours (11 pm to 6:30 am, unless you change them) I still show the note, but I keep my mouth
            shut. I learned that one the hard way.
          </p>

          <h2>Running on Empty</h2>
          <p>
            If you forget about me and my battery runs right down, I'll say <strong>Battery empty - charge me!</strong>{" "}
            and go to sleep to protect my cells. Picking me up won't wake me then. Just plug me in, and within a minute
            or two I'll notice and start up again by myself.
          </p>

          <h2>Naps</h2>
          <p>
            Going away for a while? There's a <strong>Power</strong> section in the app that puts me to sleep: screen,
            sound and WiFi all off. I wake up when someone picks me up. A full battery lasts about a week like that, and I
            still charge while I nap.
          </p>
        </section>

        <section>
          <h1>Settings</h1>
          <p>Alright, nearly done. A quick tour of the app, top to bottom.</p>

          <h2>Pomodoro Configuration</h2>
          <p>
            How long a focus session, a short break and a long break last, how many rounds before the long one, and the
            colours I use for each.
          </p>

          <h2>Kubi's Scenes</h2>
          <p>
            Peek at any of my everyday scenes for 10 minutes. After that, or when you tap "Let Kubi choose", I go back to
            my routine.
          </p>

          <h2>Battery Reminders</h2>
          <p>
            The percentages for "unplug me" and "plug me in". Tap the percentage next to one to switch it off, tap it
            again (or drag the slider) to switch it back on.
          </p>

          <h2>Power</h2>
          <p>The nap button from the battery section.</p>

          <h2>Screen Sleep</h2>
          <p>
            How long I wait before my screen naps on the faces that do that (my scenes and the maze). The clock, the
            timer and the glow stay on (except at night, see Night Mode).
          </p>

          <h2>Night Mode</h2>
          <p>
            When night starts and ends, which faces stay on, and how bright I glow (1 is barely there). Changing the
            brightness shows it for 30 seconds, and there's a button to preview the whole thing.
          </p>

          <h2>Maze</h2>
          <p>
            Which board you're on and how many you've got a best time for. "Reset maze progress" sends you back to
            board 1 and forgets every best time, in case someone else wants a go from the start.
          </p>

          <h2>Sound</h2>
          <p>
            How loud my chimes are. I play a little chime at the new volume when you let go, so you can hear it.
          </p>

          <h2>Hardware &amp; Telemetry</h2>
          <p>
            The nerd corner. Temperature, air pressure, my battery level and voltage, and whether I'm charging. You
            probably won't need it, but it's fun to look at.
          </p>

          <h2>Factory Reset</h2>
          <p>
            Wipes all my settings and every WiFi I know, and sends me back to the Hello! screen as if I were brand new.
            Only press this if you really have to.{" "}
            <Aside>I'd rather keep my memories, thanks.</Aside>
          </p>
        </section>

        <section>
          <h1>Looking After Me</h1>
          <ul>
            <li>I'm made of wood. Keep me away from water, and don't leave me baking in the sun.</li>
            <li>
              Those brass screws are soft. Use a screwdriver that fits properly, if you ever need to open me at all.
            </li>
            <li>Roll me, don't drop me. Axel says thanks.</li>
            <li>Don't cover me while I'm charging. I get a bit warm, and that's normal.</li>
          </ul>
          <hr />
          <h2>The End</h2>
          <p>
            And that's me. Thanks for reading all the way down here. Now go put me on your desk and let's get to work.{" "}
            <Aside>Or play the maze. I won't tell.</Aside>
          </p>
          <p className="text-sm mt-6">
            Cheers from Kubi and Axel <br />
            <span className="text-xs text-zinc-500">The cube and the bit of him that feels everything</span>
            <br />
            <span>🧊 &amp; 📳</span>
          </p>
        </section>

        <section>
          <h1>Stats for Nerds</h1>
          <p>Started: 4 September 2026</p>
          <p>Commits: 97 and counting</p>
          <hr />
          <p>Lines of code (Firmware): 5,512 lines</p>
          <p>Lines of code (App): 3,520 lines</p>
          <p>Lines of code (Simulator): 2,454 lines</p>
          <p className="font-bold">Grand Total Lines: 11,486 lines</p>
          <hr />
          <p>
            Yes, a simulator. Mathys built a digital twin of me on his PC, so he could test me without picking me up a
            thousand times. <Aside>There are two of me. Think about that.</Aside>
          </p>
          <hr />
          <p>
            Money spent:{" "}
            <Bleep title="*priceless*">Priceless</Bleep>
          </p>
          <p>Maze boards: 50, every one proven solvable by a robot before you got them</p>
          <p>Amount of effort: Worth every second</p>
        </section>

        <section>
          <h1>Future Plans</h1>
          <p>Believe it or not, Mathys isn't done with me yet. For Kubi Revision 2:</p>
          <Axel>Gimme the list.</Axel>
          <ol>
            <li>A power switch (so I can actually be switched off)</li>
            <li>Smarter power circuitry, so a charge lasts a lot longer</li>
            <li>Switching my speaker amp off between chimes, for even more battery</li>
            <li>A wire to my charger, so I know the moment I'm plugged in instead of working it out</li>
          </ol>
        </section>

        <p className="text-xs max-w-1/2 mx-auto text-center mt-20 text-zinc-500">
          Made with lots of love, time and attention to detail by your brother. Happy 21st Birthday, Wilhelm! 🎉
        </p>
      </article>
    </div>
  );
}

export default Manual;
