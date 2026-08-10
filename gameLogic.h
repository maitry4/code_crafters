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
    int score1;                      ///< Player 1 score
    int score2;                      ///< Player 2 score
    bool gameOver1;                  ///< Player 1 game over flag
    bool gameOver2;                  ///< Player 2 game over flag
    int winner;                      ///< 0 = none/tie, 1 = Player 1, 2 = Player 2
    pair<int, int> food;             ///< Current Food position
    bool foodExists;                 ///< Whether Food is present on the board
    deque<pair<int, int>> snake1;    ///< Snake 1 body segments
    deque<pair<int, int>> snake2;    ///< Snake 2 body segments
    int snake1Length;                ///< Current length of snake 1
    int snake2Length;                ///< Current length of snake 2
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
    SNAKE1 = 1, 
    SNAKE2 = 2,
    FOOD = 3, 
    WALL = 4 
};

// ============================================================================
// BOARD MANAGEMENT
// ============================================================================

/**
 * @brief Manages the game board state and cell operations.
 */
class Board {
private:
    vector<vector<int>> grid;
    int rows;
    int cols;

public:
    void initialize(int rows, int cols) {
        this->rows = rows;
        this->cols = cols;
        grid = vector<vector<int>>(rows, vector<int>(cols, EMPTY));
    }

    bool isInBounds(int r, int c) const {
        return r >= 0 && r < rows && c >= 0 && c < cols;
    }

    int getCellType(int r, int c) const {
        if (!isInBounds(r, c)) return WALL;
        return grid[r][c];
    }

    void setCellType(int r, int c, int cellType) {
        if (isInBounds(r, c)) {
            grid[r][c] = cellType;
        }
    }

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
 */
class Snake {
private:
    deque<pair<int, int>> body;
    int growthPending;
    int myCellType;

public:
    Snake() : growthPending(0), myCellType(SNAKE1) {}
    
    void setCellType(int type) {
        myCellType = type;
    }

    void initialize(pair<int, int> startPos, int length, Direction direction, Board& board) {
        body.clear();
        growthPending = 0;
        
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
            board.setCellType(r, c, myCellType);
        }
    }

    void move(pair<int, int> newHead, Board& board) {
        body.push_front(newHead);
        board.setCellType(newHead.first, newHead.second, myCellType);
        
        if (growthPending > 0) {
            growthPending--;
        } else {
            pair<int, int> tail = body.back();
            body.pop_back();
            board.setCellType(tail.first, tail.second, EMPTY);
        }
    }
    
    // Specifically remove snake from board upon death
    void clearFromBoard(Board& board) {
        for (auto& segment : body) {
            if (board.getCellType(segment.first, segment.second) == myCellType) {
                board.setCellType(segment.first, segment.second, EMPTY);
            }
        }
        body.clear();
    }

    void grow(int amount = 1) {
        growthPending += amount;
    }

    bool checkSelfCollision(pair<int, int> pos) const {
        for (size_t i = 1; i < body.size(); i++) {
            if (body[i] == pos) return true;
        }
        return false;
    }
    
    bool checkOtherCollision(pair<int, int> pos) const {
        for (size_t i = 0; i < body.size(); i++) {
            if (body[i] == pos) return true;
        }
        return false;
    }

    pair<int, int> getHead() const { return body.empty() ? pair<int,int>{-1,-1} : body.front(); }
    const deque<pair<int, int>>& getBody() const { return body; }
    size_t getLength() const { return body.size(); }
    bool hasPendingGrowth() const { return growthPending > 0; }
};

// ============================================================================
// FOOD MANAGEMENT
// ============================================================================

class FoodManager {
private:
    pair<int, int> position;
    bool exists;
    mt19937& rng;

public:
    FoodManager(mt19937& rng) : exists(false), rng(rng) {}

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

class CollisionDetector {
public:
    static bool isOutOfBounds(pair<int, int> pos, const Board& board) {
        return !board.isInBounds(pos.first, pos.second);
    }

    static bool isWall(pair<int, int> pos, const Board& board) {
        return board.getCellType(pos.first, pos.second) == WALL;
    }

    static bool isFood(pair<int, int> pos, const FoodManager& foodManager) {
        return foodManager.isPresent() && pos == foodManager.getPosition();
    }
};

// ============================================================================
// DIRECTION CONTROL
// ============================================================================

class DirectionController {
private:
    Direction current;
    Direction next;
    atomic<int> atomicInput;

public:
    DirectionController() : current(NONE), next(NONE) {
        atomicInput.store(static_cast<int>(NONE), memory_order_relaxed);
    }

    bool isValidChange(Direction newDir) const {
        if (current == UP && newDir == DOWN) return false;
        if (current == DOWN && newDir == UP) return false;
        if (current == LEFT && newDir == RIGHT) return false;
        if (current == RIGHT && newDir == LEFT) return false;
        return true;
    }

    void setInput(Direction dir) {
        atomicInput.store(static_cast<int>(dir), memory_order_release);
    }

    void processInput() {
        int dirValue = atomicInput.exchange(static_cast<int>(NONE), memory_order_acquire);
        Direction inputDir = static_cast<Direction>(dirValue);
        
        if (inputDir != NONE && isValidChange(inputDir)) {
            next = inputDir;
        }
        
        current = next;
    }

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

class StatePublisher {
private:
    shared_ptr<const GameState> currentState;
    shared_ptr<GameState> writeBuffer;
    shared_ptr<GameState> readBuffer;

public:
    StatePublisher() {
        writeBuffer = make_shared<GameState>();
        readBuffer = make_shared<GameState>();
        atomic_store(&currentState, static_pointer_cast<const GameState>(writeBuffer));
    }

    void publish(const Board& board, const Snake& snake1, const Snake& snake2, const FoodManager& foodManager, 
                 int score1, int score2, bool gameOver1, bool gameOver2, int winner) {
        writeBuffer->rows = board.getRows();
        writeBuffer->cols = board.getCols();
        writeBuffer->score1 = score1;
        writeBuffer->score2 = score2;
        writeBuffer->gameOver1 = gameOver1;
        writeBuffer->gameOver2 = gameOver2;
        writeBuffer->winner = winner;
        writeBuffer->food = foodManager.getPosition();
        writeBuffer->foodExists = foodManager.isPresent();
        writeBuffer->snake1 = snake1.getBody();
        writeBuffer->snake2 = snake2.getBody();
        writeBuffer->snake1Length = snake1.getLength();
        writeBuffer->snake2Length = snake2.getLength();
        writeBuffer->board = board.getGrid();
        
        atomic_store(&currentState, static_pointer_cast<const GameState>(writeBuffer));
        swap(writeBuffer, readBuffer);
    }

    shared_ptr<const GameState> getState() const {
        return atomic_load(&currentState);
    }
};

// ============================================================================
// MAIN GAME LOGIC
// ============================================================================

class SnakeGameLogic {
private:
    Board board;
    Snake snake1;
    Snake snake2;
    FoodManager foodManager;
    DirectionController dirController1;
    DirectionController dirController2;
    StatePublisher statePublisher;
    
    mt19937 rng;
    int score1;
    int score2;
    int pointsPerFood;
    bool gameOver1;
    bool gameOver2;
    int winner;

    void evaluateWinner() {
        if (score1 > score2) winner = 1;
        else if (score2 > score1) winner = 2;
        else winner = 0; // tie
    }

public:
    SnakeGameLogic() : foodManager(rng), score1(0), score2(0), pointsPerFood(10), gameOver1(false), gameOver2(false), winner(0) {
        auto seed = chrono::high_resolution_clock::now().time_since_epoch().count();
        rng.seed(static_cast<unsigned int>(seed));
        snake1.setCellType(SNAKE1);
        snake2.setCellType(SNAKE2);
    }

    void initializeBoard(int rows, int cols, int startingLength, 
                        int pointsPerFood, Direction initialDirection) {
        this->pointsPerFood = pointsPerFood;
        score1 = 0;
        score2 = 0;
        gameOver1 = false;
        gameOver2 = false;
        winner = 0;
        
        board.initialize(rows, cols);
        dirController1.initialize(RIGHT);
        dirController2.initialize(LEFT);
        
        pair<int, int> startPos1 = {rows / 4, cols / 4};
        pair<int, int> startPos2 = {rows * 3 / 4, cols * 3 / 4};
        
        snake1.initialize(startPos1, startingLength, RIGHT, board);
        snake2.initialize(startPos2, startingLength, LEFT, board);
        
        foodManager.placeRandom(board);
        statePublisher.publish(board, snake1, snake2, foodManager, score1, score2, gameOver1, gameOver2, winner);
    }

    void setDirection1(Direction newDir) { dirController1.setInput(newDir); }
    void setDirection2(Direction newDir) { dirController2.setInput(newDir); }

    bool update() {
        if (gameOver1 && gameOver2) {
            return false;
        }
        
        if (!gameOver1) dirController1.processInput();
        if (!gameOver2) dirController2.processInput();
        
        pair<int, int> newHead1 = {-1, -1};
        pair<int, int> newHead2 = {-1, -1};
        
        if (!gameOver1) newHead1 = dirController1.getNextPosition(snake1.getHead());
        if (!gameOver2) newHead2 = dirController2.getNextPosition(snake2.getHead());
        
        bool justDied1 = false;
        bool justDied2 = false;
        
        // 1. Check out of bounds and walls and self collision
        if (!gameOver1) {
            if (CollisionDetector::isOutOfBounds(newHead1, board) || 
                CollisionDetector::isWall(newHead1, board) || 
                snake1.checkSelfCollision(newHead1)) {
                justDied1 = true;
                if (!gameOver2 && !justDied2) {
                    winner = 2; // default to P2
                }
            }
        }
        
        if (!gameOver2) {
            if (CollisionDetector::isOutOfBounds(newHead2, board) || 
                CollisionDetector::isWall(newHead2, board) || 
                snake2.checkSelfCollision(newHead2)) {
                justDied2 = true;
                if (!gameOver1 && !justDied1) {
                    winner = 1; // default to P1
                }
            }
        }
        
        // 2. Check head-to-head collision
        if (!gameOver1 && !gameOver2 && !justDied1 && !justDied2) {
            if (newHead1 == newHead2) {
                justDied1 = true;
                justDied2 = true;
                evaluateWinner();
            }
        }
        
        // 3. Check cross collisions (head to body)
        if (!gameOver1 && !justDied1 && !gameOver2 && !justDied2) {
            bool s1HitsS2 = snake2.checkOtherCollision(newHead1);
            bool s2HitsS1 = snake1.checkOtherCollision(newHead2);
            
            if (s1HitsS2 || s2HitsS1) {
                justDied1 = true;
                justDied2 = true;
                evaluateWinner();
            }
        }
        
        if (justDied1) {
            gameOver1 = true;
            snake1.clearFromBoard(board);
        }
        if (justDied2) {
            gameOver2 = true;
            snake2.clearFromBoard(board);
        }
        
        // Evaluate winner properly if both died simultaneously and we haven't set winner yet
        if (gameOver1 && gameOver2 && winner == 0) {
            evaluateWinner();
        }

        // Handle food for survivors
        if (!gameOver1 && !justDied1) {
            if (CollisionDetector::isFood(newHead1, foodManager)) {
                snake1.grow();
                score1 += pointsPerFood;
                foodManager.remove(board);
            }
            snake1.move(newHead1, board);
        }
        
        if (!gameOver2 && !justDied2) {
            if (CollisionDetector::isFood(newHead2, foodManager)) {
                snake2.grow();
                score2 += pointsPerFood;
                foodManager.remove(board);
            }
            snake2.move(newHead2, board);
        }
        
        if (!foodManager.isPresent() && (!gameOver1 || !gameOver2)) {
            foodManager.placeRandom(board);
        }
        
        if (!foodManager.isPresent() && !snake1.hasPendingGrowth() && !snake2.hasPendingGrowth()) {
            gameOver1 = true;
            gameOver2 = true;
            evaluateWinner();
            statePublisher.publish(board, snake1, snake2, foodManager, score1, score2, gameOver1, gameOver2, winner);
            return false;
        }
        
        statePublisher.publish(board, snake1, snake2, foodManager, score1, score2, gameOver1, gameOver2, winner);
        return !(gameOver1 && gameOver2);
    }

    shared_ptr<const GameState> getGameState() const { return statePublisher.getState(); }
    int getRows() const { return statePublisher.getState()->rows; }
    int getCols() const { return statePublisher.getState()->cols; }
    
    static Direction getDirectionUp() { return UP; }
    static Direction getDirectionDown() { return DOWN; }
    static Direction getDirectionLeft() { return LEFT; }
    static Direction getDirectionRight() { return RIGHT; }
};

#endif // GAMELOGIC_H
