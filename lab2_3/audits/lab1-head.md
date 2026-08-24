# Code Smell Audit — Lab-1 Head (`origin/feat/multiplayer`)

Audit of the codebase at commit `13e38fa` (Lab-1 multiplayer branch head).

## Summary of Findings

| Smell | Category | Location | Confidence | Primary Refactoring Technique |
|---|---|---|---|---|
| **Duplicate Code** | Dispensables | `gameLogic.h:472` | C4 | Extract Method / Parameterize Entity (`refactor-composing-methods`) |
| **Data Clumps** | Bloaters | `gameLogic.h:25` | C4 | Extract Class / Parameterize Object (`refactor-organizing-data`) |
| **Shotgun Surgery** | Change Preventers | `gameLogic.h:420` | C4 | Extract Class / Move Field (`refactor-moving-features`) |
| **Long Method** | Bloaters | `gameLogic.h:455` | C3 | Extract Method (`refactor-composing-methods`) |
| **Long Method** | Bloaters | `main.cpp:568` | C3 | Extract Method (`refactor-composing-methods`) |
| **Large Class** | Bloaters | `main.cpp:284` | C3 | Extract Class (`refactor-moving-features`) |
| **Primitive Obsession** | Bloaters | `gameLogic.h:65` | C3 | Replace Data Value with Object (`refactor-organizing-data`) |
| **Switch Statements** | Object-Orientation Abusers | `main.cpp:383` | C3 | Replace Conditional with Polymorphism (`refactor-simplifying-conditionals`) |
| **Switch Statements** | Object-Orientation Abusers | `main.cpp:480` | C3 | Replace Conditional with Strategy (`refactor-simplifying-conditionals`) |
| **Inappropriate Intimacy** | Couplers | `main.cpp:388` | C3 | Move Method / Hide Delegate (`refactor-moving-features`) |
| **Divergent Change** | Change Preventers | `gameLogic.h:420` | C3 | Extract Class (`refactor-moving-features`) |
| **Feature Envy** | Couplers | `gameLogic.h:324` | C2 | Move Method into `Board` (`refactor-moving-features`) |

---

## Detailed Findings

### 1. Duplicate Code
- **Location:** `gameLogic.h:472` (`SnakeGameLogic::update`)
- **Confidence:** C4 (Certain)
- **Description:** The `update` method in `gameLogic.h:472` contains duplicate logic blocks for player 1 (`gameLogic.h:472-510`) and player 2 (`gameLogic.h:512-550`), duplicating next-position calculation, out-of-bounds verification, wall collision testing, self-collision testing, and food consumption.
- **Prescription:** Parameterize snake entity management over a collection using Extract Method (`refactor-composing-methods`).

### 2. Data Clumps
- **Location:** `gameLogic.h:25` (`GameState`)
- **Confidence:** C4 (Certain)
- **Description:** `GameState` in `gameLogic.h:25` declares duplicated clusters of scalar fields (`score1`, `score2`, `snake1`, `snake2`, `snakeLength1`, `snakeLength2`, `gameOver1`, `gameOver2`) that travel together across functions and threads.
- **Prescription:** Bundle player-specific data into a dedicated `PlayerState` structure via Extract Class (`refactor-organizing-data`).

### 3. Shotgun Surgery
- **Location:** `gameLogic.h:420` (`SnakeGameLogic`)
- **Confidence:** C4 (Certain)
- **Description:** Introducing the second snake in Lab 1 required simultaneous manual field additions and duplicated logic in `GameState` (`gameLogic.h:25`), `SnakeGameLogic` (`gameLogic.h:420`), `StatePublisher` (`gameLogic.h:380`), `InputHandler` (`main.cpp:480`), `GameRenderer` (`main.cpp:360`), and `GameSession` (`main.cpp:550`).
- **Prescription:** Move player fields into a unified player collection via `refactor-moving-features`.

### 4. Long Method (`SnakeGameLogic::update`)
- **Location:** `gameLogic.h:455`
- **Confidence:** C3 (High)
- **Description:** `SnakeGameLogic::update` in `gameLogic.h:455` expanded to 110 lines as a result of duplicating the single-player simulation blocks for both snakes.
- **Prescription:** Decompose the per-snake step into helper methods using Extract Method (`refactor-composing-methods`).

### 5. Long Method (`GameSession::run`)
- **Location:** `main.cpp:568`
- **Confidence:** C3 (High)
- **Description:** `GameSession::run` in `main.cpp:568` handles dual score notifications, timing delays, input polling, and replay loops in a 75-line method.
- **Prescription:** Extract helper methods via Extract Method (`refactor-composing-methods`).

### 6. Large Class (`GameRenderer`)
- **Location:** `main.cpp:284`
- **Confidence:** C3 (High)
- **Description:** `GameRenderer` in `main.cpp:284` spans 165 lines managing banner drawing, dual player score display formatting, winner overlays, and board character rasterization.
- **Prescription:** Split into specialized rendering sub-components using Extract Class (`refactor-moving-features`).

### 7. Primitive Obsession
- **Location:** `gameLogic.h:65` (`CellType`)
- **Confidence:** C3 (High)
- **Description:** Cell types in `gameLogic.h:65` and winner indicator integers in `gameLogic.h:420` use raw integers rather than strongly-typed enums.
- **Prescription:** Replace Data Value with Object (`refactor-organizing-data`).

### 8. Switch Statements
- **Location:** `main.cpp:383` (`updateGameBoard`)
- **Confidence:** C3 (High)
- **Description:** `updateGameBoard` in `main.cpp:383` uses a `switch(cellType)` statement for glyph mapping.
- **Prescription:** Replace Conditional with Polymorphism (`refactor-simplifying-conditionals`).

### 9. Switch Statements (`InputHandler`)
- **Location:** `main.cpp:480`
- **Confidence:** C3 (High)
- **Description:** `InputHandler::pollInput` in `main.cpp:480` contains multiple switch statements routing separate arrow key and WASD keycodes to `setDirection1` and `setDirection2`.
- **Prescription:** Replace Conditional with Strategy (`refactor-simplifying-conditionals`).

### 10. Inappropriate Intimacy
- **Location:** `main.cpp:388` (`GameRenderer`)
- **Confidence:** C3 (High)
- **Description:** `GameRenderer` in `main.cpp:388` directly inspects `state->snake1.front()` and `state->snake2.front()` to distinguish heads from bodies.
- **Prescription:** Encapsulate head evaluation inside player domain logic via `refactor-moving-features`.

### 11. Divergent Change
- **Location:** `gameLogic.h:420` (`SnakeGameLogic`)
- **Confidence:** C3 (High)
- **Description:** `SnakeGameLogic` in `gameLogic.h:420` changes when collision rules, winner evaluation rules, food mechanics, or state publishing change.
- **Prescription:** Extract Class (`refactor-moving-features`).

### 12. Feature Envy
- **Location:** `gameLogic.h:324` (`CollisionDetector`)
- **Confidence:** C2 (Medium)
- **Description:** `CollisionDetector` in `gameLogic.h:324` has methods querying `Board` and `FoodManager` exclusively without maintaining internal state.
- **Prescription:** Move Method into `Board` (`refactor-moving-features`).
