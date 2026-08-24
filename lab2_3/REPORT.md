# Lab 2_3 — Group A16

---

## 1. Tool and install route — [3]

| | |
|---|---|
| Agent used for run 2 | Antigravity IDE Agent (Gemini 3.7 Flash) |
| `ubiquitous-language` install route | Installed into agent customization roots (`~/.gemini/config/skills` and `.agents/skills`) |
| `refactoring/` pack install route | Installed into agent customization roots (`~/.gemini/config/skills/refactoring` and `.agents/skills/refactoring`) |

Skills were installed directly into Antigravity customization roots and executed via tool-assisted session exploration and structured prompts.

---

## 2. What I changed in the glossary — [4]

The generated file is at `lab2_3/UBIQUITOUS_LANGUAGE.md`. We refined the glossary by:
1. Canonicalizing `Food` over casual terms like `Fruit` or `Apple` to align domain discourse directly with codebase identifiers (`FOOD`, `FoodManager`).
2. Purging implementation-specific infrastructure terms (`StatePublisher`, `TerminalController`, `GameConfig`) that represent architectural plumbing rather than core domain concepts.
3. Defining `Tick` explicitly as the discrete simulation cycle to resolve confusion with configuration-level `updateDelay`.
4. Documenting `Player` under code drift since the baseline single-player codebase conflated player identity with the `Snake` entity.

---

## 3. Smell delta — [6]

Reports: `lab2_3/audits/main.md` (the code as you received it) and `lab2_3/audits/lab1-head.md` (after your Lab-1 PR).

| | count | representative site (`file:line`) |
|---|---|---|
| Smells my Lab-1 PR **introduced** | 2 | `gameLogic.h:472` |
| Smells my Lab-1 PR **left untouched** | 10 | `main.cpp:284` |
| Smells my Lab-1 PR **removed** | 0 | `gameLogic.h:25` |

The third row will probably be zero. Leave it in.

---

## 4. Rejected candidates — [6]

At least three things the agent reported that are *not* real findings on this codebase.

| smell reported | `file:line` | why it does not hold |
|---|---|---|
| Lazy Class | `main.cpp:443` | `InputHandler` is not a lazy class because it encapsulates critical platform-specific keyboard abstractions (Windows conio vs. POSIX raw terminal escape sequences) rather than pointless indirection. |
| Dead Code | `gameLogic.h:664` | The static direction getters (`getDirectionUp`, etc.) provide an intentional, stable encapsulation layer for external input mapping without exposing internal enum storage details. |
| Large Class | `gameLogic.h:500` | `SnakeGameLogic` acts as the orchestrator/facade in a component-based architecture; its sub-concerns are already delegated to `Board`, `Snake`, `FoodManager`, and `StatePublisher`. |

---

## 5. Commit map — [7]

Run `lab2_3/check-lab2_3.sh` and paste the table it prints.

| # | sha | subject | what it is |
|---|---|---|---|
| 1 | 3234dee | docs: establish domain ubiquitous language glossary | glossary |
| 2 | 39b7793 | audit: document baseline code smells at main branch | smell report |
| 3 | 65ab132 | refactor: parameterize snake entity count to single source of truth | the refactor alone |
| 4 | d8e6236 | feat: add second player multiplayer mechanics | the feature alone |

---

## 6. Two-run measurement — [4]

Run 1 is your Lab-1 branch — the numbers you already reported. Run 2 is commit 4 alone.

| | Run 1 (Lab 1) | Run 2 (commit 4) |
|---|---|---|
| Smells introduced | 2 | 0 |
| Lines changed, `git diff --shortstat -w` | 4 files changed, 330 insertions(+), 354 deletions(-) | 2 files changed, 160 insertions(+), 161 deletions(-) |
| Lines changed, **raw** (no `-w`) | 4 files changed, 349 insertions(+), 373 deletions(-) | 2 files changed, 187 insertions(+), 188 deletions(-) |
| Functions reached | 6 | 4 |
| Prompts to working code | 2 | 1 |
| Wall-clock time | 15 mins | 5 mins |

Commit 3 (the refactor) on its own: 2 files changed, 193 insertions(+), 267 deletions(-) lines `-w`, 2 files changed, 199 insertions(+), 273 deletions(-) raw.

`check-lab2_3.sh` prints the four line-count numbers for run 2. Use them — they are measured the same way for every group, which is what makes the class comparison mean anything.

---

## 7. Analysis Q1–Q2 — [5]

**Q1. Which smell did commit 3 actually fix?**
Commit 3 fixed **Shotgun Surgery** (`gameLogic.h:25`) and pre-empted **Duplicate Code** (`gameLogic.h:472`). In the original codebase, snake entities were hardcoded as scalar members. Adding or modifying player entities required synchronized changes across `GameState`, `SnakeGameLogic`, `StatePublisher`, `GameRenderer`, `InputHandler`, and `GameSession`. Commit 3 parameterized player entities into a `vector<PlayerState>` / `vector<SnakePlayer>` collection governed by a single constant (`NUM_PLAYERS`). Now, entity capacity and lifecycle modifications require editing only the central collection structure and loop logic rather than scattering duplicated fields across the codebase.

**Q2. Compare commit 4 to your Lab-1 diff.**
In Lab 1, adding multiplayer required copy-pasting movement, wall collision, self-collision, and food logic for two snakes across 684 lines (`-w`), introducing severe Data Clumps and code duplication. In Commit 4, because entity management was already generalized in Commit 3, the feature diff dropped to 321 lines (`-w`)—a 53% reduction in code churn. The feature commit was strictly limited to genuine feature logic: Player 2 key mappings in `InputHandler`, dual start positions in `initializeBoard`, mutual snake collision checks in `update()`, and winner banner rendering in `GameRenderer`.

---

## 8. Analysis Q3–Q4 — [5]

**Q3. Go back through your Lab-1 `LLM-LOG.md`. Did the assistant ever suggest restructuring before adding the feature?**
No. In Lab 1, the assistant immediately implemented the feature by duplicating scalar fields (`score1`/`score2`, `snake1`/`snake2`, `gameOver1`/`gameOver2`) and copy-pasting execution logic. To have prompted restructuring first, our prompt would have needed an explicit architectural instruction such as: *"Refactor the existing single-player assumptions so that snake count is parameterized in a single collection before implementing any multiplayer behavior."*

**Q4. How do you know commit 3 did not change behaviour?**
Honestly, we cannot guarantee zero behavioral change without comprehensive automated regression tests. We verified Commit 3 through manual testing: validating single-player movement, boundary/self collisions, food consumption, score increments, and high-score saving. However, manual testing cannot detect subtle race conditions or edge-case timing anomalies. To truly know, we would have needed deterministic unit test suites asserting identical state transitions before and after refactoring.

---

## If you did not finish

N/A — all deliverables completed.
