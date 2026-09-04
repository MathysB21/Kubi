import { type ReactNode, useState } from "react";
import { Minus, Plus } from "lucide-react";
import { AnimatePresence, motion, type Variants } from "motion/react";
import { cn } from "../lib/utils";

type AccordionProps = {
  items: AccordionItem[];
  className?: string;
  isLoading?: boolean;
  defaultOpen?: boolean;
};

type AccordionItem = {
  headerContent?: ReactNode;
  bodyContent?: ReactNode;
  defaultOpen?: boolean;
};

const accordionContainerVariant: Variants = {
  initial: {},
  animate: {
    transition: {
      staggerChildren: 0.08,
    },
  },
  exit: {
    transition: {
      staggerChildren: 0.06,
      staggerDirection: -1,
    },
  },
};

const accordionVariant: Variants = {
  initial: {
    x: -20,
    opacity: 0,
    filter: "blur(6px)",
  },
  animate: {
    x: 0,
    opacity: 1,
    filter: "blur(0px)",
  },
  exit: {
    x: 20,
    opacity: 0,
    filter: "blur(6px)",
  },
};

export const Accordion = ({
  items,
  className,
  isLoading = false,
  defaultOpen = false,
}: AccordionProps) => {
  return (
    <div className={cn("relative w-full", className)}>
      <AnimatePresence initial={false} mode="wait">
        {isLoading ? (
          <motion.div
            animate="animate"
            className="relative z-10 space-y-3 overflow-x-clip"
            exit="exit"
            initial="initial"
            key="loading"
            variants={accordionContainerVariant}
          >
            {Array.from({ length: 3 }).map((_, index) => (
              <motion.div
                className="h-[69px] w-full animate-pulse rounded-2xl bg-zinc-800"
                key={index}
                variants={accordionVariant}
              />
            ))}
          </motion.div>
        ) : (
          <motion.div
            animate="animate"
            className="relative z-10 space-y-3 overflow-x-clip"
            exit="exit"
            initial="initial"
            key="loaded"
            variants={accordionContainerVariant}
          >
            {items.map((item, index) => (
              <AccordionContent
                bodyContent={item.bodyContent}
                defaultOpen={item.defaultOpen ?? defaultOpen}
                headerContent={item.headerContent}
                key={index}
              />
            ))}
          </motion.div>
        )}
      </AnimatePresence>
    </div>
  );
};

const AccordionContent = ({
  headerContent,
  bodyContent,
  defaultOpen,
}: AccordionItem & { defaultOpen: boolean }) => {
  const [isOpen, setIsOpen] = useState(defaultOpen);

  return (
    <motion.div
      className="overflow-hidden rounded-2xl border border-zinc-800 bg-zinc-900 transition-colors hover:border-amber-500/50 hover:bg-zinc-800/50"
      variants={accordionVariant}
    >
      <motion.header
        className="flex cursor-pointer list-none items-center justify-between gap-4 p-5"
        data-state={isOpen ? "open" : "closed"}
        onClick={() => setIsOpen(!isOpen)}
        whileHover="hover"
      >
        {headerContent}
        <motion.div
          animate={{ rotate: isOpen ? 180 : 0 }}
          transition={{ duration: 0.2, ease: "easeOut" }}
        >
          <motion.div
            variants={{ hover: { scale: 1.2, transition: { duration: 0.2 } } }}
          >
            {isOpen ? (
              <Minus
                className="size-5 shrink-0 lg:size-6 text-amber-500"
                strokeWidth={3}
              />
            ) : (
              <Plus
                className="size-5 shrink-0 lg:size-6 text-amber-500"
                strokeWidth={3}
              />
            )}
          </motion.div>
        </motion.div>
      </motion.header>

      <AnimatePresence initial={false}>
        {isOpen && (
          <motion.section
            animate={{
              height: "auto",
              opacity: 1,
              transition: {
                height: { duration: 0.2, ease: "easeOut" },
                opacity: { duration: 0.25, delay: 0.15 },
              },
            }}
            exit={{
              height: 0,
              opacity: 0,
              transition: {
                height: { duration: 0.2, ease: "easeOut" },
                opacity: { duration: 0.25 },
              },
            }}
            initial={{ height: 0, opacity: 0 }}
            key="content"
            style={{ overflow: "hidden" }}
          >
            <div className="px-5 pt-1 pb-5 text-sm leading-relaxed lg:text-base text-zinc-400">
              {bodyContent}
            </div>
          </motion.section>
        )}
      </AnimatePresence>
    </motion.div>
  );
};
