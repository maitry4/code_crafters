// gameLogic.h
#ifndef GAMELOGIC_H
#define GAMELOGIC_H

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <iostream>
#include <deque>
#include <vector>
#include <random>
#include <atomic>
#include <memory>
#include <chrono>

using namespace std;

/**
 * @brief Immutable snapshot of the game state at a specific point in time.
 * 
 * This structure is designed to be thread-safe when accessed through
 * shared_ptr with proper memory ordering. All fields are copied from
 * the game logic state during publishing.
 */
struct GameState {
    vector<vector<int>> board;      ///< 2D grid representing the game board
    int rows;                        ///< Number of rows in the board
    int cols;                        ///< Number of columns in the board
    int score;                       ///< Current game score (Player 1)
    int score2;                      ///< Current game score (Player 2)
    bool gameOver;                   ///< Game over flag
    int losingPlayer;                ///< 0: none, 1: P1 lost, 2: P2 lost, 3: Both lost
    pair<int, int> food;            ///< Current food position
    bool foodExists;                 ///< Whether food is present on the board
    deque<pair<int, int>> snake;    ///< Player 1 snake body segments
    int snakeLength;                 ///< Current length of Player 1 snake
    deque<pair<int, int>> snake2;   ///< Player 2 snake body segments
    int snakeLength2;                ///< Current length of Player 2 snake
};

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

class Board;
class Snake;
class FoodManager;
class CollisionDetector;
class DirectionController;

// ============================================================================
// ENUMERATIONS
// ============================================================================

/**
 * @brief Represents movement directions for the snake.
 */
enum Direction { 
    UP = 0, 
    DOWN = 1, 
    LEFT = 2, 
    RIGHT = 3, 
    NONE = 4 
};

/**
 * @brief Represents different types of cells on the game board.
 */
enum CellType { 
    EMPTY = 0, 
    SNAKE = 1, 
    FOOD = 2, 
    WALL = 3,
    SNAKE2 = 4
};

// ============================================================================
// BOARD MANAGEMENT
// ============================================================================

/**
 * @brief Manages the game board state and cell operations.
 * 
 * Provides a clean interface for board manipulation including cell access,
 * modification, and initialization. This class encapsulates all board-related
 * logic and provides boundary checking.
 */
class Board {
private:
    vector<vector<int>> grid;
    int rows;
    int cols;

public:
    /**
     * @brief Initializes the board with specified dimensions.
     * @param rows Number of rows
     * @param cols Number of columns
     */
    void initialize(int rows, int cols) {
        this->rows = rows;
        this->cols = cols;
        grid = vector<vector<int>>(rows, vector<int>(cols, EMPTY));
    }

    /**
     * @brief Checks if a position is within board boundaries.
     * @param r Row index
     * @param c Column index
     * @return True if position is valid, false otherwise
     */
    bool isInBounds(int r, int c) const {
        return r >= 0 && r < rows && c >= 0 && c < cols;
    }

    /**
     * @brief Gets the cell type at specified position.
     * @param r Row index
     * @param c Column index
     * @return CellType at the position
     */
    int getCellType(int r, int c) const {
        if (!isInBounds(r, c)) return WALL;
        return grid[r][c];
    }

    /**
     * @brief Sets the cell type at specified position.
     * @param r Row index
     * @param c Column index
     * @param cellType Type to set
     */
    void setCellType(int r, int c, int cellType) {
        if (isInBounds(r, c)) {
            grid[r][c] = cellType;
        }
    }

    /**
     * @brief Gets all empty cell positions on the board.
     * @return Vector of empty cell coordinates
     */
    vector<pair<int, int>> getEmptyCells() const {
        vector<pair<int, int>> emptyCells;
        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                if (grid[r][c] == EMPTY) {
                    emptyCells.push_back({r, c});
                }
            }
        }
        return emptyCells;
    }

    int getRows() const { return rows; }
    int getCols() const { return cols; }
    const vector<vector<int>>& getGrid() const { return grid; }
};

// ============================================================================
// SNAKE MANAGEMENT
// ============================================================================

/**
 * @brief Manages the snake entity including movement, growth, and collision.
 * 
 * Encapsulates all snake-related behavior including body segment tracking,
 * movement mechanics, and growth logic. Provides a clean interface for
 * snake operations.
 */
class Snake {
private:
    deque<pair<int, int>> body;
    int growthPending;
    int cellType;

public:
    Snake() : growthPending(0), cellType(SNAKE) {}

    /**
     * @brief Initializes the snake at a starting position.
     * @param startPos Initial head position
     * @param length Starting length of the snake
     * @param direction Initial movement direction
     * @param board Reference to the game board
     * @param cellType Board cell type to set for this snake
     */
    void initialize(pair<int, int> startPos, int length, Direction direction, Board& board, int cellType = SNAKE) {
        body.clear();
        growthPending = 0;
        this->cellType = cellType;
        
        int startRow = startPos.first;
        int startCol = startPos.second;
        
        for (int i = 0; i < length; i++) {
            int r = startRow;
            int c = startCol;
            
            switch (direction) {
                case RIGHT: c -= i; break;
                case LEFT:  c += i; break;
                case UP:    r += i; break;
                case DOWN:  r -= i; break;
                case NONE:  break;
            }
            
            body.push_back({r, c});
            board.setCellType(r, c, cellType);
        }
    }

    /**
     * @brief Moves the snake to a new head position.
     * @param newHead New head position
     * @param board Reference to the game board
     */
    void move(pair<int, int> newHead, Board& board) {
        body.push_front(newHead);
        board.setCellType(newHead.first, newHead.second, cellType);
        
        if (growthPending > 0) {
            growthPending--;
        } else {
            pair<int, int> tail = body.back();
            body.pop_back();
            board.setCellType(tail.first, tail.second, EMPTY);
        }
    }

    /**
     * @brief Adds growth to the snake.
     * @param amount Amount of segments to grow
     */
    void grow(int amount = 1) {
        growthPending += amount;
    }

    /**
     * @brief Checks if a position collides with the snake body (excluding head).
     * @param pos Position to check
     * @return True if collision detected, false otherwise
     */
    bool checkSelfCollision(pair<int, int> pos) const {
        for (size_t i = 1; i < body.size(); i++) {
            if (body[i] == pos) return true;
        }
        return false;
    }

    /**
     * @brief Checks if a position collides with any segment of the snake body (including head).
     * @param pos Position to check
     * @return True if collision detected, false otherwise
     */
    bool checkBodyCollision(pair<int, int> pos) const {
        for (size_t i = 0; i < body.size(); i++) {
            if (body[i] == pos) return true;
        }
        return false;
    }

    pair<int, int> getHead() const { return body.front(); }
    const deque<pair<int, int>>& getBody() const { return body; }
    size_t getLength() const { return body.size(); }
    bool hasPendingGrowth() const { return growthPending > 0; }
};

// ============================================================================
// FOOD MANAGEMENT
// ============================================================================

/**
 * @brief Manages food placement and state on the game board.
 * 
 * Handles random food placement ensuring food appears only on empty cells.
 * Uses a random number generator for unpredictable food positions.
 */
class FoodManager {
private:
    pair<int, int> position;
    bool exists;
    mt19937& rng;

public:
    /**
     * @brief Constructs a food manager with a random number generator.
     * @param rng Reference to random number generator
     */
    FoodManager(mt19937& rng) : exists(false), rng(rng) {}

    /**
     * @brief Places food at a random empty location on the board.
     * @param board Reference to the game board
     */
    void placeRandom(Board& board) {
        vector<pair<int, int>> emptyCells = board.getEmptyCells();
        
        if (emptyCells.empty()) {
            exists = false;
            return;
        }
        
        uniform_int_distribution<int> dist(0, emptyCells.size() - 1);
        int idx = dist(rng);
        position = emptyCells[idx];
        board.setCellType(position.first, position.second, FOOD);
        exists = true;
    }

    /**
     * @brief Removes the current food from the board.
     * @param board Reference to the game board
     */
    void remove(Board& board) {
        if (exists) {
            board.setCellType(position.first, position.second, EMPTY);
            exists = false;
        }
    }

    pair<int, int> getPosition() const { return position; }
    bool isPresent() const { return exists; }
};

// ============================================================================
// COLLISION DETECTION
// ============================================================================

/**
 * @brief Handles all collision detection logic for the game.
 * 
 * Centralizes collision detection including boundary checks, wall collisions,
 * self-collisions, and food pickup detection.
 */
class CollisionDetector {
public:
    /**
     * @brief Checks if position is out of bounds.
     * @param pos Position to check
     * @param board Reference to the game board
     * @return True if out of bounds, false otherwise
     */
    static bool isOutOfBounds(pair<int, int> pos, const Board& board) {
        return !board.isInBounds(pos.first, pos.second);
    }

    /**
     * @brief Checks if position contains a wall.
     * @param pos Position to check
     * @param board Reference to the game board
     * @return True if wall detected, false otherwise
     */
    static bool isWall(pair<int, int> pos, const Board& board) {
        return board.getCellType(pos.first, pos.second) == WALL;
    }

    /**
     * @brief Checks if position matches food location.
     * @param pos Position to check
     * @param foodManager Reference to food manager
     * @return True if food is at position, false otherwise
     */
    static bool isFood(pair<int, int> pos, const FoodManager& foodManager) {
        return foodManager.isPresent() && pos == foodManager.getPosition();
    }
};

// ============================================================================
// DIRECTION CONTROL
// ============================================================================

/**
 * @brief Manages direction changes with validation.
 * 
 * Ensures direction changes follow game rules (e.g., cannot reverse 180 degrees).
 * Provides thread-safe direction updates through atomic operations.
 */
class DirectionController {
private:
    Direction current;
    Direction next;
    atomic<int> atomicInput;

public:
    DirectionController() : current(NONE), next(NONE) {
        atomicInput.store(static_cast<int>(NONE), memory_order_relaxed);
    }

    /**
     * @brief Validates if a direction change is allowed.
     * @param newDir Proposed new direction
     * @return True if change is valid, false otherwise
     */
    bool isValidChange(Direction newDir) const {
        if (current == UP && newDir == DOWN) return false;
        if (current == DOWN && newDir == UP) return false;
        if (current == LEFT && newDir == RIGHT) return false;
        if (current == RIGHT && newDir == LEFT) return false;
        return true;
    }

    /**
     * @brief Sets the next direction (thread-safe).
     * @param dir Direction to set
     */
    void setInput(Direction dir) {
        atomicInput.store(static_cast<int>(dir), memory_order_release);
    }

    /**
     * @brief Processes input and updates current direction.
     */
    void processInput() {
        int dirValue = atomicInput.exchange(static_cast<int>(NONE), memory_order_acquire);
        Direction inputDir = static_cast<Direction>(dirValue);
        
        if (inputDir != NONE && isValidChange(inputDir)) {
            next = inputDir;
        }
        
        current = next;
    }

    /**
     * @brief Calculates next position based on current direction.
     * @param currentPos Current position
     * @return Next position after move
     */
    pair<int, int> getNextPosition(pair<int, int> currentPos) const {
        int newRow = currentPos.first;
        int newCol = currentPos.second;
        
        switch (current) {
            case UP:    newRow--; break;
            case DOWN:  newRow++; break;
            case LEFT:  newCol--; break;
            case RIGHT: newCol++; break;
            case NONE:  break;
        }
        return {newRow, newCol};
    }

    void initialize(Direction initialDir) {
        current = initialDir;
        next = initialDir;
    }

    Direction getCurrent() const { return current; }
};

// ============================================================================
// STATE PUBLISHER
// ============================================================================

/**
 * @brief Manages thread-safe publishing of game state snapshots.
 * 
 * Uses double buffering and atomic operations to provide lock-free
 * state updates between game logic and rendering threads.
 */
class StatePublisher {
private:
    atomic<shared_ptr<const GameState>> currentState;
    shared_ptr<GameState> writeBuffer;
    shared_ptr<GameState> readBuffer;

public:
    StatePublisher() {
        writeBuffer = make_shared<GameState>();
        readBuffer = make_shared<GameState>();
        currentState.store(writeBuffer, memory_order_relaxed);
    }

    /**
     * @brief Publishes a new game state snapshot.
     */
    void publish(const Board& board, const Snake& snake1, const Snake& snake2,
                 const FoodManager& foodManager, int score1, int score2,
                 bool gameOver, int losingPlayer) {
        writeBuffer->rows = board.getRows();
        writeBuffer->cols = board.getCols();
        writeBuffer->score = score1;
        writeBuffer->score2 = score2;
        writeBuffer->gameOver = gameOver;
        writeBuffer->losingPlayer = losingPlayer;
        writeBuffer->food = foodManager.getPosition();
        writeBuffer->foodExists = foodManager.isPresent();
        writeBuffer->snake = snake1.getBody();
        writeBuffer->snakeLength = snake1.getLength();
        writeBuffer->snake2 = snake2.getBody();
        writeBuffer->snakeLength2 = snake2.getLength();
        writeBuffer->board = board.getGrid();
        
        // Atomic swap with memory_order_release ensures visibility
        currentState.store(writeBuffer, memory_order_release);
        swap(writeBuffer, readBuffer);
    }

    /**
     * @brief Gets the current game state (thread-safe).
     * @return Shared pointer to immutable game state
     */
    shared_ptr<const GameState> getState() const {
        return currentState.load(memory_order_acquire);
    }
};

// ============================================================================
// MAIN GAME LOGIC
// ============================================================================

/**
 * @brief Main game logic controller coordinating all game systems.
 * 
 * Orchestrates the interaction between Board, Snake entities, FoodManager, and other
 * components. Manages game loop updates, scoring, and state publishing.
 * Designed for thread-safe operation with separate game and render threads.
 */
class SnakeGameLogic {
private:
    Board board;
    Snake snake;
    Snake snake2;
    FoodManager foodManager;
    DirectionController directionController;
    DirectionController directionController2;
    StatePublisher statePublisher;
    
    mt19937 rng;
    int score;
    int score2;
    int pointsPerFood;
    bool gameOver;
    int losingPlayer;

public:
    SnakeGameLogic() : foodManager(rng), score(0), score2(0), pointsPerFood(10), gameOver(false), losingPlayer(0) {
        auto seed = chrono::high_resolution_clock::now().time_since_epoch().count();
        rng.seed(static_cast<unsigned int>(seed));
    }

    /**
     * @brief Initializes the game with specified parameters.
     * @param rows Number of board rows
     * @param cols Number of board columns
     * @param startingLength Initial snake length
     * @param pointsPerFood Points awarded per food
     * @param initialDirection Starting movement direction
     */
    void initializeBoard(int rows, int cols, int startingLength, 
                        int pointsPerFood, Direction initialDirection) {
        this->pointsPerFood = pointsPerFood;
        score = 0;
        score2 = 0;
        gameOver = false;
        losingPlayer = 0;
        
        board.initialize(rows, cols);
        directionController.initialize(initialDirection);
        directionController2.initialize(LEFT);

        pair<int, int> startPos1 = {rows / 3, cols / 2};
        snake.initialize(startPos1, startingLength, initialDirection, board, SNAKE);
        
        pair<int, int> startPos2 = {2 * rows / 3, cols / 2};
        snake2.initialize(startPos2, startingLength, LEFT, board, SNAKE2);
        
        foodManager.placeRandom(board);
        statePublisher.publish(board, snake, snake2, foodManager, score, score2, gameOver, losingPlayer);
    }

    /**
     * @brief Sets the snake direction (thread-safe input).
     * @param newDir Direction to move
     * @param player Player ID (1 or 2)
     */
    void setDirection(Direction newDir, int player = 1) {
        if (player == 1) {
            directionController.setInput(newDir);
        } else if (player == 2) {
            directionController2.setInput(newDir);
        }
    }

    /**
     * @brief Updates the game state by one tick.
     * @return True if game continues, false if game over
     */
    bool update() {
        if (gameOver) {
            return false;
        }
        
        // Process direction inputs
        directionController.processInput();
        directionController2.processInput();
        
        // Calculate next positions
        pair<int, int> newHead1 = directionController.getNextPosition(snake.getHead());
        pair<int, int> newHead2 = directionController2.getNextPosition(snake2.getHead());
        
        bool p1Lost = false;
        bool p2Lost = false;

        // Head-on collision check
        if (newHead1 == newHead2) {
            p1Lost = true;
            p2Lost = true;
        }

        // P1 collision checks
        if (!p1Lost) {
            if (CollisionDetector::isOutOfBounds(newHead1, board) ||
                CollisionDetector::isWall(newHead1, board) ||
                snake.checkSelfCollision(newHead1) ||
                snake2.checkBodyCollision(newHead1)) {
                p1Lost = true;
            }
        }

        // P2 collision checks
        if (!p2Lost) {
            if (CollisionDetector::isOutOfBounds(newHead2, board) || 
                CollisionDetector::isWall(newHead2, board) || 
                snake2.checkSelfCollision(newHead2) || 
                snake.checkBodyCollision(newHead2)) {
                p2Lost = true;
            }
        }

        if (p1Lost || p2Lost) {
            gameOver = true;
            if (p1Lost && p2Lost) losingPlayer = 3;
            else if (p1Lost) losingPlayer = 1;
            else losingPlayer = 2;
            statePublisher.publish(board, snake, snake2, foodManager, score, score2, gameOver, losingPlayer);
            return false;
        }

        // Handle food collision
        bool p1Eats = CollisionDetector::isFood(newHead1, foodManager);
        bool p2Eats = CollisionDetector::isFood(newHead2, foodManager);

        if (p1Eats) {
            snake.grow();
            score += pointsPerFood;
            foodManager.remove(board);
        } else if (p2Eats) {
            snake2.grow();
            score2 += pointsPerFood;
            foodManager.remove(board);
        }

        // Move snakes
        snake.move(newHead1, board);
        snake2.move(newHead2, board);

        // Place new food if needed
        if (!foodManager.isPresent()) {
            foodManager.placeRandom(board);
        }

        // Check win condition (board full)
        if (!foodManager.isPresent() && !snake.hasPendingGrowth() && !snake2.hasPendingGrowth()) {
            gameOver = true;
            statePublisher.publish(board, snake, snake2, foodManager, score, score2, gameOver, losingPlayer);
            return false;
        }

        // Publish updated state
        statePublisher.publish(board, snake, snake2, foodManager, score, score2, gameOver, losingPlayer);
        return true;
    }

    // ========================================================================
    // THREAD-SAFE ACCESSORS (for render thread)
    // ========================================================================

    shared_ptr<const GameState> getGameState() const {
        return statePublisher.getState();
    }

    int getRows() const {
        auto state = statePublisher.getState();
        return state->rows;
    }

    int getCols() const {
        auto state = statePublisher.getState();
        return state->cols;
    }

    int getScore() const {
        auto state = statePublisher.getState();
        return state->score;
    }

    int getScore2() const {
        auto state = statePublisher.getState();
        return state->score2;
    }

    bool isGameOver() const {
        auto state = statePublisher.getState();
        return state->gameOver;
    }

    int getCellType(int r, int c) const {
        auto state = statePublisher.getState();
        if (r >= 0 && r < state->rows && c >= 0 && c < state->cols) {
            return state->board[r][c];
        }
        return WALL;
    }

    /**
     * @brief Converts cell type to display character.
     * @param cellType Type of cell
     * @return Character representing the cell
     */
    char renderSymbol(int cellType) const {
        switch(cellType) {
            case EMPTY: return ' ';
            case SNAKE: return 'O';
            case FOOD:  return '*';
            case WALL:  return '#';
            case SNAKE2: return 'X';
        }
        return ' ';
    }

    // Static direction accessors
    static Direction getDirectionUp() { return UP; }
    static Direction getDirectionDown() { return DOWN; }
    static Direction getDirectionLeft() { return LEFT; }
    static Direction getDirectionRight() { return RIGHT; }
};

#endif // GAMELOGIC_H
