import { useEffect, useState } from "react";
import { Sun, Moon } from "lucide-react";

export const SkyArc = ({ data }: { data: any }) => {
    const [timeLeft, setTimeLeft] = useState("");
    const [isDay, setIsDay] = useState(true);

    useEffect(() => {
        // Tick the countdown every second
        const timer = setInterval(() => {
            const now = new Date();
            const currentMins = now.getHours() * 60 + now.getMinutes();

            // Earth Planet Times for Day/Night Icon toggle
            const dawnMins = data.planetDawnH * 60 + data.planetDawnM;
            const duskMins = data.planetDuskH * 60 + data.planetDuskM;
            setIsDay(currentMins >= dawnMins && currentMins < duskMins);

            // Feature Target Times
            const nextSunrise = data.alarmHour * 60 + data.alarmMinute;
            const nextSundown =
                data.cfgSundownHour * 60 + data.cfgSundownMinute;

            let targetMins;
            let label;

            if (currentMins < nextSunrise) {
                targetMins = nextSunrise;
                label = "Sunrise";
            } else if (currentMins < nextSundown) {
                targetMins = nextSundown;
                label = "Sundown";
            } else {
                targetMins = nextSunrise + 1440; // Tomorrow's sunrise
                label = "Sunrise";
            }

            const diff = targetMins - currentMins;
            const h = Math.floor(diff / 60);
            const m = diff % 60;
            setTimeLeft(
                `T-${String(h).padStart(2, "0")}:${String(m).padStart(2, "0")} to ${label}`,
            );
        }, 1000);

        return () => clearInterval(timer);
    }, [data]);

    const pad = (num: number) => String(num).padStart(2, "0");

    return (
        <div className="relative w-full flex flex-col items-center mt-0 mb-2">
            {/* THE ARC SVG */}
            <div className="relative w-full h-auto">
                <svg
                    viewBox="0 0 200 100"
                    className="w-full h-full overflow-visible"
                >
                    <defs>
                        {/* Base Arc: Dark Zinc -> Light Zinc -> Dark Zinc */}
                        <linearGradient
                            id="baseArc"
                            x1="0%"
                            y1="100%"
                            x2="100%"
                            y2="100%"
                        >
                            <stop offset="0%" stopColor="#18181b" />{" "}
                            {/* bg-zinc-900 */}
                            <stop offset="50%" stopColor="#52525b" />{" "}
                            {/* bg-zinc-500 */}
                            <stop offset="100%" stopColor="#18181b" />{" "}
                            {/* bg-zinc-900 */}
                        </linearGradient>

                        {/* Progress Arc: Light Zinc -> Pure White */}
                        <linearGradient
                            id="progArc"
                            x1="0%"
                            y1="100%"
                            x2="100%"
                            y2="0%"
                        >
                            <stop offset="0%" stopColor="#52525b" />{" "}
                            {/* bg-zinc-500 */}
                            <stop offset="100%" stopColor="#ffffff" />{" "}
                            {/* white */}
                        </linearGradient>
                    </defs>

                    {/* Full Background Path */}
                    {/* <path
                        d="M 20 90 A 80 70 0 0 1 180 90"
                        fill="none"
                        stroke="url(#baseArc)"
                        strokeWidth="2"
                        strokeLinecap="round"
                    /> */}
                    {/* Full Background Path (Flatter Radius) */}
                    <path
                        d="M 10 90 A 121 121 0 0 1 190 90"
                        fill="none"
                        stroke="url(#baseArc)"
                        strokeWidth="2"
                        strokeLinecap="round"
                    />

                    {/* Left Progress Overlay Path */}
                    {/* <path
                        d="M 20 90 A 80 70 0 0 1 100 20"
                        fill="none"
                        stroke="url(#progArc)"
                        strokeWidth="3"
                        strokeLinecap="round"
                    /> */}
                    <path
                        d="M 10 90 A 121 121 0 0 1 190 90"
                        fill="none"
                        stroke="url(#progArc)"
                        strokeWidth="3"
                        strokeLinecap="round"
                        pathLength="100"
                        strokeDasharray="50 100"
                    />
                </svg>

                {/* ABSOLUTE CENTERED ICON */}
                {/* top-[20%] perfectly aligns with the Y-coordinate (20) of the SVG apex */}
                {/* <div className="absolute top-[20%] left-1/2 -translate-x-1/2 -translate-y-1/2 bg-zinc-950 p-1.5 rounded-full">
                    {isDay ? (
                        <Sun
                            size={20}
                            className="text-amber-500 drop-shadow-[0_0_8px_rgba(245,158,11,0.5)]"
                        />
                    ) : (
                        <Moon
                            size={20}
                            className="text-sky-500 drop-shadow-[0_0_8px_rgba(14,165,233,0.5)]"
                        />
                    )}
                </div> */}
                {/* top-[50%] aligns perfectly with the new flattened apex (Y=50) */}
                <div className="absolute top-[50%] left-1/2 -translate-x-1/2 -translate-y-1/2 bg-zinc-950 p-1.5 rounded-full">
                    {isDay ? (
                        <Sun
                            size={24}
                            className="text-amber-500 drop-shadow-[0_0_8px_rgba(245,158,11,0.5)]"
                        />
                    ) : (
                        <Moon
                            size={24}
                            className="text-sky-500 drop-shadow-[0_0_8px_rgba(14,165,233,0.5)]"
                        />
                    )}
                </div>
            </div>

            {/* TYPOGRAPHY OVERLAY */}
            <div className="flex flex-col items-center -mt-6 z-10 space-y-1.5">
                <p className="text-zinc-500 text-[12px] font-mono tracking-widest uppercase">
                    {pad(data.planetDawnH)}:{pad(data.planetDawnM)}{" "}
                    <span className="text-zinc-700 mx-2">|</span>{" "}
                    {pad(data.planetDuskH)}:{pad(data.planetDuskM)}
                </p>
                <p className="text-zinc-300 text-xs font-semibold tracking-widest uppercase bg-zinc-950/80 px-3 py-1 rounded-full border border-zinc-800/50">
                    {timeLeft || "Calculating..."}
                </p>
            </div>
        </div>
    );
};
