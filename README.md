# Snake Game

A terminal-based Snake game built with C++ featuring cross-platform compatibility, thread-safe game logic, and persistent high score tracking.

---

## For Players

### Getting Started

Follow the install steps below for your OS, then run the compiled binary from a terminal in this folder. A `game_highest.txt` file will be created beside the executable to persist your best score.

### Game Screenshots

**Initial Board Display**
The game board with instructions showing all control options before gameplay begins.
![Game Initial Board Display](https://github.com/maitry4/code_crafters/blob/main/Game%20Screenshots/game_initial_board_display.png)

**Gameplay in Action**
Real-time action showing the snake (displayed as `O` for head and `o` for body segments), food (`*`), and current score/length tracking at the top.

![Game Being Played](https://github.com/maitry4/code_crafters/blob/main/Game%20Screenshots/game_being_played.png)

**Game Over Screen**
Final score display with high score tracking and options to replay or quit. New high scores are highlighted.

![Game Over](https://github.com/maitry4/code_crafters/blob/main/Game%20Screenshots/game_over.png)

**Quit Confirmation**
Clean exit message displayed when you quit the game.

![Game Quit Message](https://github.com/maitry4/code_crafters/blob/main/Game%20Screenshots/game_quit_message.png)

#### Installation

**Windows:**
1. Install MinGW-w64 (or use MSYS2) with `g++` supporting C++20
2. Compile:
   - `g++ -std=c++20 main.cpp -o main.exe`
3. Run:
   - `main.exe` (or directly run this on command prompt (might get older version))

**Linux/macOS:**
1. Ensure you have `g++` installed
2. Compile:
   - `g++ -std=c++20 main.cpp -o snake_game`  
3. Run:
   - `./snake_game`

### How to Play

Navigate your snake around the board, eat the food (marked with `*`), and grow longer. Avoid hitting walls or yourself!

**Goal:** Eat as much food as possible to increase your score and beat your personal high score.

**Controls:**
- **Move Up:** `W` or `↑ Arrow Key`
- **Move Down:** `S` or `↓ Arrow Key`
- **Move Left:** `A` or `← Arrow Key`
- **Move Right:** `D` or `→ Arrow Key`
- **Quit:** `Q`

### Gameplay Rules

- Each food eaten grants **10 points**
- The snake grows by one segment for each food consumed
- The game ends if the snake:
  - Hits the boundary wall
  - Collides with itself
- After game over, press `R` to replay or `Q` to quit

### Features

- **High Score Tracking:** Your best score is automatically saved to `game_highest.txt`
- **Real-Time Score Display:** Monitor your current score, snake length, and high score at the top of the screen
- **Smooth Controls:** Responsive arrow key and WASD input handling
- **Cross-Platform:** Works seamlessly on Windows, Linux, and macOS

---

## For Contributors

### Project Overview

This is a terminal-based Snake game implementing sophisticated game design patterns: **component-based architecture**, **observer pattern (event system)**, **lock-free thread-safe state publishing**, **double-buffering for rendering**, **configuration system**, and **platform abstraction layers**.

### Architecture

The codebase is organized into several modular components:

#### 1. **Game Logic (`gameLogic.h`)**
The core game engine using **component-based architecture** with **immutable state snapshots** and **atomic operations** for thread safety.

**Component Classes:**

- **`Board`**: Manages the game board state with boundary checking and cell operations (`getCellType()`, `setCellType()`, `getEmptyCells()`)
- **`Snake`**: Encapsulates snake behavior including movement, growth, and self-collision detection (`move()`, `grow()`, `checkSelfCollision()`)
- **`FoodManager`**: Handles random food placement on empty cells using seeded RNG (`placeRandom()`, `remove()`)
- **`CollisionDetector`**: Static utility class for collision detection (`isOutOfBounds()`, `isWall()`, `isFood()`)
- **`DirectionController`**: Manages direction changes with validation to prevent 180° reversals; uses atomic operations for thread-safe input (`setInput()`, `processInput()`, `getNextPosition()`)
- **`StatePublisher`**: Thread-safe state publishing using double buffering and atomic operations (`publish()`, `getState()`)
- **`SnakeGameLogic`**: Main orchestrator coordinating all components; manages game loop and state updates

**Key Concepts:**
- **GameState Struct:** Immutable snapshot of the game at any moment (board state, snake position, score, etc.)
- **Lock-Free Threading:** Uses `std::atomic<shared_ptr<const GameState>>` to safely publish state from the game thread to the render thread without locks
- **Double Buffering:** Maintains `writeBuffer` and `readBuffer` to prevent torn reads during state updates
- **Component Separation:** Each game entity (Board, Snake, Food) is self-contained with clear responsibilities

**Critical Methods:**
- `initializeBoard()`: Sets up the game with specified dimensions, starting length, points per food, and initial direction
- `update()`: Game loop tick—processes input, moves snake, checks collisions, handles food, publishes state
- `getGameState()`: Lock-free read of current game state (safe for render thread)
- `setDirection()`: Thread-safe direction input (validated by DirectionController)

Additional details:
- State is published via atomic store with `memory_order_release` and read with `memory_order_acquire` to ensure visibility without locks.
- All components are designed for single-threaded game logic with thread-safe state publishing for rendering.

#### 2. **Application Layer (`main.cpp`)**
Handles game lifecycle, user interface, and platform abstraction.

**Event System:**
- **`EventManager`**: Observer pattern implementation for loose coupling (`subscribe()`, `notify()`)
- **`GameEvent`**: Event data structure with type, value, and message
- **`EventListener`**: Interface for event subscribers (e.g., `HighScoreManager`)
- **Event Types:** `FOOD_EATEN`, `SNAKE_GREW`, `GAME_OVER`, `SCORE_CHANGED`, `HIGH_SCORE_BEATEN`

**Configuration System:**
- **`GameConfig`**: Centralized configuration for board size, game speed, display characters, and scoring
- Supports customization of snake head/body characters, food character, wall character, update delay, and points per food

**High Score Management:**
- **`HighScoreManager`**: Persists high scores to `game_highest.txt`; subscribes to score change events
- Automatically saves new high scores and notifies via events

**Platform Abstraction (`TerminalController`):**
Handles all platform-specific terminal operations with unified API.

**Windows Support:**
- Uses `<conio.h>` for keyboard input (`_kbhit()`, `_getch()`)
- Uses Windows Console API for cursor positioning, screen clearing, and cursor visibility

**Linux/macOS Support:**
- Uses `termios` for raw input mode configuration
- Uses ANSI escape sequences for cursor control, screen manipulation, and cursor visibility
- Uses `fcntl` and `ioctl` for non-blocking I/O

**Key Methods:**
- `clearScreen()`: Cross-platform screen clearing
- `setCursorPosition()`: Atomic cursor positioning
- `hideCursor()` / `showCursor()`: Cursor visibility control
- `enableRawMode()` / `disableRawMode()`: Terminal configuration for Linux
- `kbhit()` / `getch()`: Cross-platform non-blocking keyboard input

**Rendering (`GameRenderer`):**
- Uses `GameConfig` for customizable display characters
- `drawFullScreen()`: Initial board layout with instructions
- `updateGameBoard()`: Efficient per-frame updates (only redraws game cells)
- `showGameOver()`: Game-over screen with score display and new high score highlighting

**Input Handling (`InputHandler`):**
- Handles arrow key sequences (different on Windows vs. Linux)
- Supports both arrow keys and WASD input
- `pollInput()`: Non-blocking input polling with buffer management

**Game Session Management:**
- **`GameSession`**: Manages a single game session from initialization to game over
- Handles game loop timing, input processing, event notifications, and replay logic
- Integrates EventManager, HighScoreManager, and GameRenderer

**Application Lifecycle (`SnakeGameApp`):**
- Main application class managing the overall game lifecycle
- Handles session creation, replay loop, and graceful shutdown
- Manages TerminalController and HighScoreManager instances

Notes:
- Arrow keys on Windows use the `_getch()` extended key prefix; on POSIX, a short ESC sequence timeout is used for reliability.
- Event system enables easy extension (e.g., sound effects, achievements) without modifying core game logic.

### Tech Stack & Design Choices

| Technology | Rationale |
|---|---|
| **C++20** | Modern standard for atomic operations, smart pointers, and concurrency primitives |
| **std::atomic** | Lock-free thread safety without mutex overhead; minimal latency for input/render synchronization |
| **std::shared_ptr** | Automatic memory management for game state snapshots; safe concurrent access |
| **Component-Based Architecture** | Separation of concerns: Board, Snake, FoodManager, CollisionDetector are independent, testable modules |
| **Observer Pattern (Event System)** | Loose coupling between game logic and UI/score systems; enables easy extension without modifying core |
| **Configuration System** | Centralized `GameConfig` allows easy customization of game parameters without code changes |
| **Double Buffering** | Prevents visual glitches (tearing) when updating display while game thread modifies state |
| **ANSI Escape Sequences** | Portable cursor control across Linux terminals; more reliable than system calls |
| **Terminal Raw Mode** | Non-blocking, responsive input without OS event loop overhead |

### Cross-Platform Compatibility

**Supported Platforms:**
- ✅ **Windows 7+** 
- ✅ **Linux** 

### Build & Run (Developer)

All source lives in the repo root for now:

```
.
├─ main.cpp          # Application layer: event system, config, UI, session management, platform abstraction
└─ gameLogic.h       # Game logic: component-based architecture (Board, Snake, FoodManager, etc.) + thread-safe state publishing
```

Commands:
- Windows (MinGW-w64):
  - `g++ -std=c++20 main.cpp -o main.exe`
  - Run with `main.exe`
- Linux/macOS:
  - `g++ -std=c++20 main.cpp -o snake_game`  
  - Run with `./snake_game`

Binary creates/reads `game_highest.txt` in the working directory for persistent high score.

### Contribution Guidelines

1. Fork and create a feature branch from `main`.
2. Keep edits focused; prefer small, cohesive changes.
3. Follow the code style and layout below.
4. Test on at least one platform (Windows or Linux/macOS). If you change terminal I/O, please sanity-test both.
5. Open a PR with a clear description, screenshots if UI/behavior changes, and build/run notes.

### Coding Style

- C++20, no exceptions unless necessary; avoid unnecessary try/catch.
- Use descriptive names; avoid cryptic abbreviations.
- Keep comments minimal and meaningful (non-obvious rationale, edge cases).
- Preserve existing indentation and formatting.

### Adding Features Safely

**Configuration Changes:**
- New gameplay options (speed, board size, display characters): extend `GameConfig` class in `main.cpp` and pass to `GameSession` constructor
- Modify default values in `GameConfig::GameConfig()` constructor or create custom config instances

**Game Logic Extensions:**
- New cell types: extend `CellType` enum in `gameLogic.h`, update `CollisionDetector` methods, and render mapping in `GameRenderer::updateGameBoard()`
- New game entities: create new component classes following the pattern (e.g., `ObstacleManager`, `PowerUpManager`) and integrate in `SnakeGameLogic`
- New collision types: add static methods to `CollisionDetector` class

**Event System Integration:**
- New event types: add to `EventType` enum and notify via `EventManager::notify()` in appropriate game logic locations
- New event subscribers: implement `EventListener` interface and subscribe via `EventManager::subscribe()`
- Example: Sound effects listener can subscribe to `FOOD_EATEN` and `GAME_OVER` events

**UI/Input Changes:**
- Input changes: update `InputHandler::pollInput()` on both code paths (Windows and POSIX)
- Rendering changes: prefer buffering lines (as done) and a single flush per frame to avoid flicker
- New UI screens: extend `GameRenderer` with new methods or create specialized renderer classes

**Session Management:**
- Game loop modifications: extend `GameSession::run()` method
- New game modes: create specialized session classes or add mode flags to `GameConfig`
