---
name: project-knowledge
description: >-
  Consult or record project knowledge documents in docs/knowledge/.
  Use this skill when researching project features, architecture, past design decisions,
  or when the user asks to record session knowledge for future agents.
---

# Project Knowledge Base Skill

This skill governs how agents interact with the project knowledge repository located in `docs/knowledge/`.
It defines two core workflows:
1. **Consulting Knowledge (Reading)**: Finding and reading only the necessary knowledge doc(s) without wasting context.
2. **Recording Knowledge (Writing)**: Capturing the current session's learnings into cleanly separated, dated topic documents and indexing them.

---

## 1. Consulting Knowledge (Reading Workflow)

When researching a topic or seeking context on past decisions:

### Step 1: Read ONLY the Index File
- Open and read [`docs/knowledge/index.md`](file:///c:/Development/Kubi/docs/knowledge/index.md).
- **CRITICAL**: Do **NOT** open or read all knowledge documents into context. Reading all docs wastes tokens and pollutes the context window.

### Step 2: Identify Candidate Document(s)
- Inspect the table of documents, creation dates, and descriptions in `index.md`.
- Match the user request or current problem to the specific document(s) listed.

### Step 3: Open Only the Relevant Document(s)
- Use `view_file` to inspect only the specific document(s) relevant to your task.

---

## 2. Recording Session Knowledge (Writing Workflow)

When the user asks you to record knowledge, persist learnings, or document session work:

### Step 1: Synthesize Session Learnings
- Review the session's technical achievements, bug investigations, edge cases, or hardware constraints discovered.
- If multiple distinct topics were touched (e.g., audio chimes vs. sensor debounce vs. state machine logic), **split them into separate documents** to keep topics modular and cleanly separated.

### Step 2: Naming Convention
- File location: `docs/knowledge/<YYYY-MM-DD>-<descriptive-topic-slug>.md`
  - Must include the creation date in `YYYY-MM-DD` format.
  - Slug must clearly state the topic (e.g., `2026-09-05-pomodoro-idle-start-and-persistence.md`).

### Step 3: Document Structure
Each knowledge document should follow this standardized template:
```markdown
# YYYY-MM-DD: <Descriptive Topic Title>

## 1. Context & Motivation
- Why was this work done? What was the previous behavior or problem?

## 2. Key Architecture & Design Decisions
- What technical solution was chosen and why?
- What state machine, timing, or protocol rules were implemented?

## 3. Code Touchpoints & Files
- List all relevant files and symbols with clickable markdown links.

## 4. Gotchas & Verification
- What subtle bugs or pitfalls should future developers avoid?
- How to test or verify this behavior (simulator, hardware, CLI commands).
```

### Step 4: Update the Index
- Open [`docs/knowledge/index.md`](file:///c:/Development/Kubi/docs/knowledge/index.md).
- Add an entry to the table under `## Knowledge Catalog`:
  - Document link: `[`YYYY-MM-DD-<slug>.md`](./YYYY-MM-DD-<slug>.md)`
  - Date: `YYYY-MM-DD`
  - Topics / Description: 1–2 sentence summary of what is covered in the doc.

### Step 5: Commit
- Verify all changes and git commit the new knowledge docs and updated index.
