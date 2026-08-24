# Code Smell Audit — `main` Branch

Audit of the codebase at commit `8a9aa60` (`origin/main`) prior to feature modifications.

## Summary of Findings

| Smell | Category | Location | Confidence | Primary Refactoring Technique |
|---|---|---|---|---|
| **Shotgun Surgery** | Change Preventers | `gameLogic.h:25` | C4 | Move Field / Extract Class (`refactor-moving-features`) |
| **Divergent Change** | Change Preventers | `gameLogic.h:500` | C3 | Extract Class (`refactor-moving-features`) |
| **Long Method** | Bloaters | `gameLogic.h:555` | C3 | Extract Method (`refactor-composing-methods`) |
| **Long Method** | Bloaters | `main.cpp:568` | C3 | Extract Method (`refactor-composing-methods`) |
| **Large Class** | Bloaters | `main.cpp:284` | C3 | Extract Class (`refactor-moving-features`) |
| **Primitive Obsession** | Bloaters | `gameLogic.h:65` | C3 | Replace Data Value with Object (`refactor-organizing-data`) |
| **Data Clumps** | Bloaters | `gameLogic.h:527` | C3 | Introduce Parameter Object (`refactor-simplifying-method-calls`) |
| **Switch Statements** | Object-Orientation Abusers | `main.cpp:383` | C3 | Replace Conditional with Polymorphism (`refactor-simplifying-conditionals`) |
| **Switch Statements** | Object-Orientation Abusers | `main.cpp:468` | C3 | Replace Conditional with State/Strategy (`refactor-simplifying-conditionals`) |
| **Inappropriate Intimacy** | Couplers | `main.cpp:388` | C3 | Move Method / Hide Delegate (`refactor-moving-features`) |
| **Feature Envy** | Couplers | `gameLogic.h:324` | C2 | Move Method into `Board` (`refactor-moving-features`) |
| **Temporary Field** | Object-Orientation Abusers | `main.cpp:447` | C2 | Extract Class (`refactor-organizing-data`) |

---

## Detailed Findings

### 1. Shotgun Surgery
- **Location:** `gameLogic.h:25` (`GameState`), `gameLogic.h:502` (`SnakeGameLogic`)
- **Confidence:** C4 (Certain)
- **Description:** Player count and snake instance state are hardcoded as scalar members. Introducing additional players or modifying entity tracking requires simultaneous modifications across `GameState` (`gameLogic.h:25`), `SnakeGameLogic` (`gameLogic.h:502`), `StatePublisher` (`gameLogic.h:463`), `GameRenderer` (`main.cpp:360`), `InputHandler` (`main.cpp:443`), and `GameSession` (`main.cpp:538`).
- **Prescription:** Encapsulate player state into a dedicated collection and parameterize entity management via `refactor-moving-features`.

### 2. Divergent Change
- **Location:** `gameLogic.h:500` (`SnakeGameLogic`)
- **Confidence:** C3 (High)
- **Description:** `SnakeGameLogic` in `gameLogic.h:500` has multiple distinct reasons to change: collision handling rules, score rules, input processing, and state publication mechanics.
- **Prescription:** Extract specialized sub-controllers for scoring and game-loop orchestration using Extract Class (`refactor-moving-features`).

### 3. Long Method (`SnakeGameLogic::update`)
- **Location:** `gameLogic.h:555`
- **Confidence:** C3 (High)
- **Description:** The `update` method in `gameLogic.h:555` spans 56 lines handling direction input resolution, next-position calculation, wall and self collision checks, food consumption, growth triggering, respawning food, win condition checking, and state snapshot publishing in a single sequential block.
- **Prescription:** Extract private helper methods (`handleMovement`, `checkCollisions`, `handleFoodConsumption`) via Extract Method (`refactor-composing-methods`).

### 4. Long Method (`GameSession::run`)
- **Location:** `main.cpp:568`
- **Confidence:** C3 (High)
- **Description:** The `run` method in `main.cpp:568` spans 71 lines combining initial full-screen rendering, start-key blocking wait, input polling, tick timing calculations, state progression checks, event notifications, and game-over input handling.
- **Prescription:** Extract method stages (`waitForStartKey`, `processGameTick`, `handleGameOverLoop`) via Extract Method (`refactor-composing-methods`).

### 5. Large Class (`GameRenderer`)
- **Location:** `main.cpp:284`
- **Confidence:** C3 (High)
- **Description:** `GameRenderer` in `main.cpp:284` spans 154 lines and encompasses title banner generation, instructions panel rendering, board boundary construction, per-cell character formatting, score bar construction, and game over screen rendering.
- **Prescription:** Extract sub-renderers (e.g. `HudRenderer`, `BoardRenderer`) via Extract Class (`refactor-moving-features`).

### 6. Primitive Obsession
- **Location:** `gameLogic.h:65` (`CellType`), `gameLogic.h:85` (`grid`)
- **Confidence:** C3 (High)
- **Description:** Board cells in `gameLogic.h:65` and `gameLogic.h:85` are represented as raw `int` values rather than strongly-typed structures or enum classes, leading to scattered magic integers across `gameLogic.h:117` and `main.cpp:383`.
- **Prescription:** Replace integer type codes with dedicated types via Replace Data Value with Object (`refactor-organizing-data`).

### 7. Data Clumps
- **Location:** `gameLogic.h:527` (`initializeBoard`)
- **Confidence:** C3 (High)
- **Description:** `initializeBoard` in `gameLogic.h:527` passes `(int rows, int cols, int startingLength, int pointsPerFood, Direction initialDirection)` together across initialization boundaries.
- **Prescription:** Bundle related parameters into a parameter object via Introduce Parameter Object (`refactor-simplifying-method-calls`).

### 8. Switch Statements
- **Location:** `main.cpp:383` (`updateGameBoard`)
- **Confidence:** C3 (High)
- **Description:** `updateGameBoard` in `main.cpp:383` relies on a `switch(cellType)` statement to map cell integer codes to configured glyph characters.
- **Prescription:** Replace Conditional with Polymorphism / Lookup Map (`refactor-simplifying-conditionals`).

### 9. Switch Statements (`InputHandler`)
- **Location:** `main.cpp:468`, `main.cpp:490`, `main.cpp:504`
- **Confidence:** C3 (High)
- **Description:** `InputHandler` contains multiple switch statements in `main.cpp:468`, `main.cpp:490`, and `main.cpp:504` mapping platform key codes directly to game direction setter invocations.
- **Prescription:** Replace Conditional with Strategy / Command mapping table (`refactor-simplifying-conditionals`).

### 10. Inappropriate Intimacy
- **Location:** `main.cpp:388` (`GameRenderer`)
- **Confidence:** C3 (High)
- **Description:** In `main.cpp:388`, `GameRenderer` accesses the head position of the snake (`state->snake.front()`) directly from the published state deque to decide head vs. body glyphs.
- **Prescription:** Move glyph resolution into the domain model or hide delegate via `refactor-moving-features`.

### 11. Feature Envy
- **Location:** `gameLogic.h:324` (`CollisionDetector`)
- **Confidence:** C2 (Medium)
- **Description:** `CollisionDetector` in `gameLogic.h:324` operates entirely on external `Board` and `FoodManager` instances without maintaining state of its own.
- **Prescription:** Move boundary and wall checking directly into `Board` (`refactor-moving-features`).

### 12. Temporary Field
- **Location:** `main.cpp:447` (`InputHandler`)
- **Confidence:** C2 (Medium)
- **Description:** In `main.cpp:447`, `buffer[3]` and `bufferPos` in `InputHandler` are only populated and valid during POSIX escape sequence decoding.
- **Prescription:** Extract POSIX escape parser into a dedicated helper class via Extract Class (`refactor-organizing-data`).
