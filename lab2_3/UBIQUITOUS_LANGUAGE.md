# Ubiquitous Language

## Game Entities

| Term | Definition | Aliases to avoid | In code |
|---|---|---|---|
| **Snake** | The player-controlled entity consisting of a head and ordered body segments that advances across the board | Player, worm, avatar | `Snake` — `gameLogic.h:166` |
| **Food** | A consumable item placed on an empty board cell that awards points and extends snake length when collected | Fruit, apple, pellet, prize | `FoodManager` — `gameLogic.h:258` |
| **Board** | The fixed two-dimensional discrete grid of cells bounded by outer walls where the game takes place | Grid, map, arena, canvas | `Board` — `gameLogic.h:83` |
| **Cell** | An individual coordinate unit on the board holding a distinct state (empty, snake segment, food, or wall) | Tile, square, pixel, block | `CellType` — `gameLogic.h:65` |

## Gameplay & Lifecycle

| Term | Definition | Aliases to avoid | In code |
|---|---|---|---|
| **Tick** | A single discrete simulation step advancing snake movement and collision resolution | Frame, turn, cycle, interval | `update()` — `gameLogic.h:555` |
| **Direction** | The current or buffered cardinal heading of movement (Up, Down, Left, Right) | Heading, orientation, vector | `Direction` — `gameLogic.h:54` |
| **Growth** | The increment in snake body segments deferred across subsequent movement ticks after eating food | Elongation, expansion, stretch | `grow()` — `gameLogic.h:227` |
| **Collision** | An intersection event between a snake's head and a wall, its own body, or another snake causing death | Hit, crash, impact | `CollisionDetector` — `gameLogic.h:316` |
| **Score** | The tally of points accumulated by a player within the active game session | Points, tally, count | `score` — `gameLogic.h:29` |
| **High Score** | The maximum score achieved across all recorded sessions, persisted to disk | Record, best score, top score | `HighScoreManager` — `main.cpp:106` |
| **Session** | A single continuous run of the game from launch/restart to game over | Match, round, playthrough | `GameSession` — `main.cpp:537` |

## Relationships

- A **Board** contains a discrete grid of **Cells**.
- Exactly one **Food** exists on an empty **Cell** at any given time.
- A **Snake** occupies one or more **Cells** on the **Board**.
- Each **Tick**, a **Snake** advances one **Cell** in its active **Direction**.
- Consuming **Food** triggers **Growth** and increments the **Score**.
- A **Collision** with a wall or snake body terminates the **Session** as Game Over.
- When a **Session** ends, the **Score** is compared against the **High Score**.

## Example dialogue

> **Dev:** "When a **Snake** consumes **Food**, does it extend immediately on that same **Tick**?"
> **Domain expert:** "No — the head advances onto the food cell, but the tail remains in place during that tick, realizing the **Growth**."
> **Dev:** "And does **Collision** detection occur before or after moving into the new **Cell**?"
> **Domain expert:** "Before committing the move. If the prospective head position hits a wall or body segment, a **Collision** occurs immediately and ends the **Session**."
> **Dev:** "If the player reverses **Direction** 180 degrees in a single frame, is that allowed?"
> **Domain expert:** "No, 180-degree reversals are invalid and ignored so the **Snake** cannot immediately collide with its own neck."

## Flagged ambiguities

- **Food vs. Fruit**: Casual discussions and generic problem descriptions refer to "Fruit" or "Apple", while the codebase exclusively names the item `Food` (`FOOD`, `FoodManager`, `isFood`). Canonical term is **Food**.
- **Wall vs. Out of Bounds**: The boundary is checked both by checking grid boundaries (`isOutOfBounds`) and checking boundary walls (`isWall`), though `Board::getCellType` returns `WALL` for out-of-bounds. Canonical domain concept is a boundary **Wall**.
- **Tick vs. Update Delay**: `GameConfig::updateDelay` specifies milliseconds between ticks, but the simulation itself operates in discrete logical **Ticks**.

## Code drift

| Canonical term | Called in code | Location | Note |
|---|---|---|---|
| **Tick** | `update` / `updateDelay` | `gameLogic.h:555`, `main.cpp:85` | The discrete logical step is called update or delay rather than tick |
| **Player** | *(absent)* | `gameLogic.h:500` | In single-player code, player identity is implicitly conflated with `Snake` |
| **Growth** | `growthPending` | `gameLogic.h:170` | Termed pending growth rather than active growth state |
