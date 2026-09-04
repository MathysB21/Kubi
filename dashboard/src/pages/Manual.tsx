function Manual() {
  return (
    <div className="max-w-screen w-full  min-h-screen h-full pt-20 pb-10">
      <article className="prose dark:prose-invert max-w-md lg:max-w-xl mx-auto">
        <svg className="hidden">
          <filter id="pixelate" x="0%" y="0%" width="100%" height="100%">
            <feTurbulence
              type="fractalNoise"
              baseFrequency="0.5"
              numOctaves="1"
              result="noise"
            />
            <feDisplacementMap
              in="SourceGraphic"
              in2="noise"
              scale="10"
              xChannelSelector="R"
              yChannelSelector="G"
            />
          </filter>
        </svg>
        <title>Sunrise - Manual</title>
        <h1>Manual</h1>
        <p>Hey there Honey! ♥️</p>
        <p>
          For the past three and a half years you've been a light in my life, so
          I decided to spend the last three and a half months to put a light in
          your life too!
        </p>
        <p>
          So, please do give this a read, there are many valuable bits of
          information that you might find useful.
        </p>
        <p>Enjoy your brand new, custom, one-of-a-kind, smart lamp.</p>
        <p className="font-bold text-center">- Welcome to Project Sunrise -</p>
        <section>
          <h1>Setup</h1>
          <p>What's this? You have a brand new Sunrise Smart lamp?</p>
          <p>
            Welll, that's fantastic news! You've come to the right guy. I'm
            Sunny, the only Sunrise Lamp expert in the world.
          </p>
          <p>Let's get you set up.</p>

          <h2>Powering Up</h2>
          <p>
            I believe you've been given a nice power supply, it should look like
            a beefy phone charger with a Type C plug on its end.
          </p>
          <p>
            Please only use this one that came with the lamp,{" "}
            <span className="text-xs text-gray-400 italic">
              I don't want your house to burn down
            </span>
          </p>
          <p>You can plug this wire into the "backdoor".</p>
          <span className="text-xs">hehehe... you get it?</span>
          <p>
            <span className="font-bold">*clears throat*</span> ahem, sorry about
            that, alter ego and all that. Just find the hole at the back.
          </p>
          <p>
            Your special plug also comes with a nice switch for if you ever
            wanna switch this thing off, but trust me, once you see this thing
            glow, you won't wanna switch it off.
          </p>

          <h2>Wifi Setup</h2>
          <p>
            Yes yes, I know I know, Chinese spy technology, computer chips and
            the CIA spying on your every move. I read conspiracies, I know
            what's up.
          </p>
          <p>
            But but but, don't worry now, this lamp won't spy on you,{" "}
            <span className="font-bold">Mathys</span> made sure of that.
          </p>
          <p>
            When you initially power on the light, you'll see this stunning
            breathing animation play on the lamp. This means the lamp is in its
            Wifi Setup mode{" "}
            <span className="text-xs text-gray-400">
              (I'll tell you a little more about this later)
            </span>
          </p>
          <p>
            When in this mode if your lamp has never been connected to your home
            Wifi network, it'll create its own network.
          </p>
          <p>
            So pull out your cellular device and pop open your Wifi settings.
            You'll see a network name{" "}
            <span className="font-mono bg-gray-700 px-2 py-0.5 rounded-md">
              Sunrise-Setup
            </span>
            . Connect to it. Your little lamp is smart, but not always so fast,
            give it a second or two to open a captive portal on your phone, it's
            magic I tell you.
          </p>
          <p>
            Select your home Wifi network from the list and type in your
            password. Once you've done this, click on the connect button, it'll{" "}
            <span className="font-bold italic">automagically</span> handle the
            rest.
          </p>
        </section>
        <section>
          <h1>App</h1>
          <p>
            Every{" "}
            <span
              title="*bleeep*"
              className="inline-block filter-[url(#pixelate)]"
            >
              fucking
            </span>{" "}
            thing has an app nowadays, and your smart lamp is no exception.
          </p>
          <p>
            Once you've done the setup, open your favourite browser. I know you
            like Safari, you Apple fanatic.{" "}
            <span className="text-xs text-gray-400">I see you</span>
          </p>
          <p>
            Type this into your URL bar:{" "}
            <span className="font-mono bg-gray-700 px-2 py-0.5 rounded-md">
              http://sunrise.local
            </span>
          </p>
          <p>
            Voila! You should be seeing an initialization screen on your screen.{" "}
            <span className="text-gray-400 text-xs">
              Screen inception, I tell you
            </span>
          </p>
          <p>
            On this app you'll be able to see and edit alllll of the settings.
            Every damn one. Mathys loves his settings.{" "}
            <span className="text-xs text-gray-400">
              What a nerd, am I right?
            </span>
          </p>
          <p>Go on, give it a scroll, see if you find something you like.</p>
        </section>
        <section>
          <h1>Buttons</h1>
          <p>
            <span className="font-bold">*whistle*</span> Dang, do you see that?
            Those buttons are shiny. That right there is some quality brass, but
            be careful, brass is incredibly soft and will scratch if you look at
            it too sharply.
          </p>
          <p className="text-sm text-gray-400">
            Stop it! I said don't look at it too sharply!
          </p>
          <p>
            Each of these buttons come with a distinct purpose, lemme give you a
            runthrough.
          </p>
          <h2>Power</h2>
          <p>
            Much like Eskom, you can turn this baby on or off at a moment's
            notice.
          </p>
          <p className="text-sm text-gray-400">
            Unfortunately. this model doesn't have battery capabilities, that
            model was out of stock.
          </p>
          <p>
            Tap the button to turn the lamp on or off, quite simple really. Give
            it a shot. This button <span className="font-bold">doesn't</span>{" "}
            have "hold" functionality.
          </p>
          <p>
            Note: Tapping this button will put the lamp in differing modes
            depending on the time of day. Pretty smart for a smart lamp hey.
            More details in the modes section.
          </p>
          <h2>Brightness</h2>
          <p>
            This button will make this bad boy shine brighter than the sun. Not
            really, but it would've been cool right?{" "}
            <span className="text-xs text-gray-400">
              Write that down, write that down.
            </span>
          </p>
          <p>
            Depending on whether you're in Hybrid, Stepped or Variable
            brightness modes (I'll mention more about these in the settings
            section), tapping or holding this button will do different things.
            The default is "Hybrid", try tapping it and then holding it to see
            what happens.
          </p>
          <p>
            Regardless of what brightness mode your lamp is in, if you tap and
            hold this button, then let go, and then within 5 seconds, tap and
            hold again, the dimming direction will switch.
          </p>
          <p className="text-xs text-gray-400">
            What the{" "}
            <span
              title="*bleeep*"
              className="inline-block filter-[url(#pixelate)]"
            >
              fuck
            </span>{" "}
            is a dimming direction? Ohh, right, that thing...
          </p>
          <p>
            So, dimming direction is just whether the lamp goes brighter or if
            it does more dim. You can switch the direction the brightness button
            is going by doing the 5 second thing I mentioned a few sentences
            earlier.
          </p>
          <h2>Temperature</h2>
          <p>
            No, this does not actually change the temperature of your room. Yes,
            technically the light does give off heat.{" "}
            <span className="text-xs text-gray-400">
              Shut up, smart ass. You're making me look bad.
            </span>
          </p>
          <p>
            This button toggles between the three colour temperatures that come
            with this lamp.
          </p>
          <p>
            You got <span className="text-amber-400">Amber</span>,{" "}
            <span className="text-amber-100">Warm White</span> and{" "}
            <span className="text-blue-100">Cool White</span>. Personally, I
            prefer Amber in the night and Warm White during the day. Cool White
            is nice for if you need to see something's colour very accurately.
          </p>
        </section>
        <section>
          <h1>Modes</h1>
          <p>
            You have a lot of modes, trust me, I've seen 'em. And you don't even
            need to worry about 'em, ain't that cool?
            <br />
            <span className="text-xs text-gray-400">
              Yeah yeah, that's actually pretty cool.
            </span>
            {"  "}
            <span className="text-gray-300 text-xs">
              Shhhhh! No one asked you Jerry.
            </span>
          </p>
          <p>
            This lamp is actually pretty complex on the inside. It uses a Finite
            State Machine. Sounds like something straight out of a Star Wars
            movie, but it ain't. It's real, I've seen it with my own eyes.
          </p>
          <p>
            Now I hear you say, "Damn, das a lotta modes, Sunny". And you're
            right. Let's break 'em down.
          </p>

          <h2>Wifi Setup (Auto)</h2>
          <p>
            This is your first mode, it gets triggered automatically when you
            power the lamp on.
          </p>
          <p>
            In this mode, the lamp looks for a known network and tries to
            connect in order to sync its internal clock.
          </p>

          <h2>Day (Auto)</h2>
          <p>
            Your lamp is automatically, and by default, in this mode during the
            day. When the lamp appears to be off, and it's day, it's in this
            mode.
          </p>

          <h2>Night (Auto)</h2>
          <p>
            When it's night, and the lamp appears to be off, it's in this mode.
          </p>

          <h2>Day (Manual)</h2>
          <p>
            This mode you trigger by pressing the Power button during the day.
            Simple as that. Oh oh, and it remembers your last chosen temperature
            and brightness.{" "}
            <span className="text-xs text-gray-400">
              Nifty feature I tell you.
            </span>
          </p>

          <h2>Night (Manual)</h2>
          <p>
            This one is the same as the previous, but in the night. It does have
            some special features though, like defaulting to amber and 50%
            brightness so you don't blind{" "}
            <span
              className="line-through text-gray-400"
              title="Editor Mathys says this is not a good use of language"
            >
              your ass
            </span>{" "}
            yourself accidentally in the middle of the night.
          </p>

          <h2>Night Light (Auto)</h2>
          <p>
            Technically, this is the default mode after the sun has set for the
            day, it defaults to amber and 50% brightness. If you tinker with the
            brightness and the temperature, you manually move over to the Manual
            Night mode I mentioned above.
          </p>
          <p>
            Even if you power off the lamp and power it back on, it'll always
            start in this mode in the night.
          </p>
          <p>Let's call it a safety feature.</p>

          <h2>Sunrise (Auto)</h2>
          <p>Ahh the very mode this lamp was named after.</p>
          <p>
            This mode is automatically triggered according to the schedule you
            set on the app.
          </p>
          <p className="text-sm text-gray-400">What does this do, Jerry?</p>
          <p>
            It slowly starts lighting up your room. It starts 30 mins before
            your alarm is set by lighting up at 5% brightness and by the time
            your alarm goes off, it's bright as day.
          </p>
          <p>
            Some smart people did a study and found that this lighting technique
            helped their subjects wake up easier. I mean think about it, it's
            easier to wake up in the Summer than in the Winter right? It's not
            just the cold, it's the darkness too.
          </p>

          <h2>Sundown (Auto)</h2>
          <p>
            This is the equal opposite of the Sunrise mode. This gets triggered
            either at the time you set or it follows the dawn time of Mother
            Earth.
          </p>
          <p className="text-xs text-gray-400">
            We're on Earth? Yes Jerry, we're on Earth. Wait, what the{" "}
            <span
              title="*bleeep*"
              className="inline-block filter-[url(#pixelate)]"
            >
              fuck
            </span>
            ? Are you high again, Jerry?
          </p>
          <p>
            This mode only gets triggered if the lamp was in the manual day mode
            before sundown. The lamp then slowly starts dimming down once the
            sun has set. It then moves to night light mode once the sundown
            sequence has completed.
          </p>

          <h2>Proximity (Auto)</h2>
          <p>
            This is another one of those quirky features. Quite frankly, it
            don't work so great. Mathys has yet to fully figure this one out.
          </p>
          <p>
            But when you have this feature turned on in your settings, you can
            approach this lamp in utter darkness with your hand and it'll light
            up.
          </p>
          <p>
            Let's say, you're looking for your water or your lip-ice in the
            middle of the night, don't fear, Sunrise lamp is here.
          </p>
          <p>
            Just reach out and it'll give off a slight glow as to not disturb
            your sleep, but to help you look for whatever you're looking for.
          </p>

          <h2>Away (Manual)</h2>
          <p>
            This mode will probably not be used, but what the hell, Mathys
            included it anyway.
          </p>
          <p className="text-xs text-gray-400">How thoughtful...</p>
          <p>
            You can trigger this mode by holding the power button for 3 seconds
            or by clicking the Away button on the app.
          </p>
          <p>
            This puts the lamp in a low power sleep mode for when you're not
            home and don't want the cool little features and modes to play
            without you there.
          </p>
          <p>
            You can then untrigger this mode, by holding the power button again
            for 3 seconds or clicking the button in the app.
          </p>
        </section>
        <section>
          <h1>Settings</h1>
          <p>
            Alrighty, we're almost done. Just gonna go through some of the
            important things that need clarification ya know.
          </p>
          <h2>Sunrise- and Sundown Engine</h2>
          <p>
            This is where you can set the time you want your lamp to
            automatically go into the Sunrise or the Sundown modes.
          </p>
          <p>
            You can also turn it off entirely.{" "}
            <span className="text-gray-400 text-xs">
              But who would wanna do that? Exactly, Jerry, exactly.
            </span>
          </p>
          <p>
            Tapping the day once will give it a dashed border, this means that
            day follows the planetary dusk or dawn. Tapping it again will give
            that day a solid infill, this means that day will trigger according
            to the manual target time you set.
          </p>

          <h2>Master Output</h2>
          <p>
            Pretty self-explanatory stuff really. Change the brightness and
            colour temperature from right here on your cellular device.
          </p>
          <p>
            Do note though, that this is depending on the time of day. During
            day time this will always default to the previous colour and
            temperature you selected, but during night it will always default to
            the night light default brightness setting and an amber colour tone.
          </p>

          <h2>Sequencing and Durations</h2>
          <p>
            In here you set how long you want the sunrise and sundown modes to
            actually last. I personally think the default 30 mins is a nice
            sweet spot.
          </p>
          <p className="text-xs text-gray-400">Right, Jerry? Yessir.</p>
          <p>
            It also has a timeout setting, this is for if you leave home before
            the sequence ended, or if you forgot to put on the away mode, this
            automatically pushes the lamp to an auto mode to save some power.
          </p>

          <h2>Location and Timezone</h2>
          <p>
            This here is another pretty cool feature, search for your city and
            watch the coordinates autofill.
          </p>
          <p>
            These digits are used for your precise dusk and dawn calculation. Oh
            and of course, timezone is there too, but unless you emigrate, leave
            it on 2.
          </p>

          <h2>Light Output Limits</h2>
          <p>
            Over here you can set the default night light brightness I was
            talking about earlier. Set this as high or as low as you want, but
            remember, if you trigger the night light in absolute darkness, the
            lamp will use this as its default brightness.
          </p>
          <p>
            Same goes for the proximity glow. Set its brightness here so you can
            go easier on your eyes or singe the{" "}
            <span
              title="*bleeep*"
              className="inline-block filter-[url(#pixelate)]"
            >
              shit
            </span>{" "}
            out of those suckers.
          </p>
          <p>
            Lastly, here are those brightness (intensity) modes I mentioned
            earlier.
          </p>
          <p>
            In variable mode, tapping brightness does nothing, but holding it
            will increase or decrease brightness smoothly (you can reverse the
            dimming direction by holding, letting go and then holding the button
            again in a 5 second window). You'll see a nice animation if you've
            reached the minimum or maximum brightness.
          </p>
          <p>
            In stepped mode, tapping brightness will increase or decrease
            brightness to the next step. You can also do the reverse dimming
            technique here. Holding the brightness button in this mode will
            automatically and sequentially step through the brightness steps
            every 400ms.
          </p>
          <p>
            In hybrid mode you have the best of both worlds where tapping
            increases or decreases the brightness by a step and holding will
            smoothly increase or decrease the brightness like the variable mode.
            And yes, you can also do the dimming directon reversal here.
          </p>
          <p className="text-gray-400 text-xs">
            Woah, lot of info huh. Damn right, Jerry.
          </p>

          <h2>Sensor Calibration & Hardware Diagnostics</h2>
          <p>
            Hopefully you won't ever have to tinker in here, but here you can
            adjust the sensitivity of the buttons, luminance sensor and
            proximity sensor. The defaults are pretty decent though.
          </p>
          <p>
            If you do have to adjust some values, check out the raw values in
            the hardware diagnostics accordion. Here you can see the actual
            values that the buttons and sensors register to more accurately
            modify the sensitivity.
          </p>
          <p>
            Oh and you can turn the proximity sensing off if it bothers you{" "}
            <span className="text-sm text-gray-400">
              (like it did Mathys, during development)
            </span>{" "}
            or if it just don't work.
          </p>
          <hr />
          <h2>The End</h2>
          <p>
            Anyhow, here we are at the end of this little, wordy, adventure. Go
            on and enjoy your lamp for us.
          </p>
          <p className="text-sm mt-6">
            Cheers from Sunny and Jerry <br />
            <span className="text-xs mt-3 text-gray-400">
              Mathys's two <span className="line-through">least</span> favourite
              bugs
            </span>
            <br />
            <span>🐞 & 🐛</span>
          </p>
        </section>
        <section>
          <h1>Stats for Nerds</h1>
          <p>Hours (Research): 21 hours</p>
          <p>Hours (Design): 38 hours</p>
          <p>Hours (Software): 174 hours</p>
          <p>Hours (Hardware & Woodworking): 68 hours</p>
          <p className="font-bold">Grand Total Hours: 301 hours</p>
          <hr />
          <p>Lines of code (App): 1567 lines</p>
          <p>Lines of code (Firmware): 1650 lines</p>
          <p className="font-bold">Grand Total Lines: 3217 lines</p>
          <hr />
          <p>
            Money spent:{" "}
            <span
              title="*priceless*"
              className="inline-block filter-[url(#pixelate)]"
            >
              Priceless
            </span>
          </p>
          <p>Amount of Effort: Worth every second</p>
        </section>
        <section>
          <h1>Future Plans & Additions</h1>
          <p>
            Believe it or not, Mathys wants to keep going, he has a few more
            ideas:
          </p>
          <p className="text-xs text-gray-400">Gimme the notes, Jerry.</p>
          <ol>
            <li>
              Battery (Make it portable and because Eskom is a{" "}
              <span
                title="*bleeep*"
                className="inline-block filter-[url(#pixelate)]"
              >
                bitch
              </span>
              )
            </li>
            <li>
              Another proximity module (To make the proximity detection more
              robust)
            </li>
            <li>
              New Translucent Light Diffusing Dome (To let more light through)
            </li>
            <li>
              Thicker wires (Cause the thin wires might be a fire risk){" "}
              <span className="text-xs text-gray-400">
                Excuse me, come again!?
              </span>
            </li>
          </ol>
        </section>
        <p className="text-xs max-w-1/2 mx-auto text-center mt-20">
          Made with lots of love, time and attention to detail by{" "}
          <span title="Shae's Thysie">Thysie</span> ♥️
        </p>
      </article>
    </div>
  );
}

export default Manual;
