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

// ============================================================================
// ENUMERATIONS
// ============================================================================

enum Direction { 
    UP = 0, 
    DOWN = 1, 
    LEFT = 2, 
    RIGHT = 3, 
    NONE = 4 
};

enum CellType { 
    EMPTY = 0, 
    SNAKE = 1, 
    FOOD = 2, 
    WALL = 3 
};

// ============================================================================
// GAME STATE SNAPSHOT
// ============================================================================

struct PlayerState {
    deque<pair<int, int>> body;
    int length = 0;
    int score = 0;
    bool isAlive = true;
};

struct GameState {
    vector<vector<int>> board;
    int rows;
    int cols;
    vector<PlayerState> players;
    int score;
    int snakeLength;
    bool gameOver;
    int winner;                      ///< 0 = tie/none, 1 = Player 1, 2 = Player 2
    pair<int, int> food;
    bool foodExists;
};

// ============================================================================
// BOARD MANAGEMENT
// ============================================================================

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

class Snake {
private:
    deque<pair<int, int>> body;
    int growthPending;

public:
    Snake() : growthPending(0) {}

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
            board.setCellType(r, c, SNAKE);
        }
    }

    void move(pair<int, int> newHead, Board& board) {
        body.push_front(newHead);
        board.setCellType(newHead.first, newHead.second, SNAKE);
        
        if (growthPending > 0) {
            growthPending--;
        } else {
            pair<int, int> tail = body.back();
            body.pop_back();
            board.setCellType(tail.first, tail.second, EMPTY);
        }
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

    bool checkCollisionWith(pair<int, int> pos) const {
        for (const auto& segment : body) {
            if (segment == pos) return true;
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
        
        uniform_int_distribution<int> dist(0, static_cast<int>(emptyCells.size()) - 1);
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
    DirectionController() : current(NONE), next(NONE), atomicInput(static_cast<int>(NONE)) {}

    DirectionController(const DirectionController& other)
        : current(other.current), next(other.next), atomicInput(other.atomicInput.load()) {}

    DirectionController& operator=(const DirectionController& other) {
        if (this != &other) {
            current = other.current;
            next = other.next;
            atomicInput.store(other.atomicInput.load());
        }
        return *this;
    }

    DirectionController(DirectionController&& other) noexcept
        : current(other.current), next(other.next), atomicInput(other.atomicInput.load()) {}

    DirectionController& operator=(DirectionController&& other) noexcept {
        if (this != &other) {
            current = other.current;
            next = other.next;
            atomicInput.store(other.atomicInput.load());
        }
        return *this;
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
// SNAKE PLAYER ENTITY
// ============================================================================

struct SnakePlayer {
    Snake snake;
    DirectionController directionController;
    int score = 0;
    bool isAlive = true;

    SnakePlayer() = default;
    SnakePlayer(const SnakePlayer&) = default;
    SnakePlayer& operator=(const SnakePlayer&) = default;
    SnakePlayer(SnakePlayer&&) noexcept = default;
    SnakePlayer& operator=(SnakePlayer&&) noexcept = default;
};

// ============================================================================
// STATE PUBLISHER
// ============================================================================

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

    void publish(const Board& board, const vector<SnakePlayer>& players, 
                 const FoodManager& foodManager, bool gameOver, int winner) {
        writeBuffer->rows = board.getRows();
        writeBuffer->cols = board.getCols();
        writeBuffer->gameOver = gameOver;
        writeBuffer->winner = winner;
        writeBuffer->food = foodManager.getPosition();
        writeBuffer->foodExists = foodManager.isPresent();
        writeBuffer->board = board.getGrid();
        
        writeBuffer->players.clear();
        for (const auto& p : players) {
            PlayerState ps;
            ps.body = p.snake.getBody();
            ps.length = static_cast<int>(p.snake.getLength());
            ps.score = p.score;
            ps.isAlive = p.isAlive;
            writeBuffer->players.push_back(ps);
        }
        
        if (!writeBuffer->players.empty()) {
            writeBuffer->score = writeBuffer->players[0].score;
            writeBuffer->snakeLength = writeBuffer->players[0].length;
        } else {
            writeBuffer->score = 0;
            writeBuffer->snakeLength = 0;
        }
        
        currentState.store(writeBuffer, memory_order_release);
        swap(writeBuffer, readBuffer);
    }

    shared_ptr<const GameState> getState() const {
        return currentState.load(memory_order_acquire);
    }
};

// ============================================================================
// MAIN GAME LOGIC
// ============================================================================

class SnakeGameLogic {
private:
    static constexpr size_t NUM_PLAYERS = 2; // Two player multiplayer

    Board board;
    vector<SnakePlayer> players;
    FoodManager foodManager;
    StatePublisher statePublisher;
    
    mt19937 rng;
    int pointsPerFood;
    bool gameOver;
    int winner;

public:
    SnakeGameLogic() : foodManager(rng), pointsPerFood(10), gameOver(false), winner(0) {
        auto seed = chrono::high_resolution_clock::now().time_since_epoch().count();
        rng.seed(static_cast<unsigned int>(seed));
    }

    void initializeBoard(int rows, int cols, int startingLength, 
                        int pointsPerFood, Direction initialDirection) {
        this->pointsPerFood = pointsPerFood;
        gameOver = false;
        winner = 0;
        
        board.initialize(rows, cols);
        players.clear();
        players.resize(NUM_PLAYERS);
        
        // Player 1
        players[0].score = 0;
        players[0].isAlive = true;
        players[0].directionController.initialize(RIGHT);
        pair<int, int> startPos1 = {rows / 3, cols / 2};
        players[0].snake.initialize(startPos1, startingLength, RIGHT, board);
        
        // Player 2
        players[1].score = 0;
        players[1].isAlive = true;
        players[1].directionController.initialize(LEFT);
        pair<int, int> startPos2 = {2 * rows / 3, cols / 2};
        players[1].snake.initialize(startPos2, startingLength, LEFT, board);
        
        foodManager.placeRandom(board);
        statePublisher.publish(board, players, foodManager, gameOver, winner);
    }

    void setDirection(Direction newDir) {
        setDirection(0, newDir);
    }

    void setDirection(size_t playerIndex, Direction newDir) {
        if (playerIndex < players.size()) {
            players[playerIndex].directionController.setInput(newDir);
        }
    }

    void setDirection1(Direction newDir) { setDirection(0, newDir); }
    void setDirection2(Direction newDir) { setDirection(1, newDir); }

    bool update() {
        if (gameOver) {
            return false;
        }
        
        vector<pair<int, int>> nextPositions(players.size());
        vector<bool> diedThisTick(players.size(), false);
        bool mutualCollision = false;

        // 1. Calculate next positions
        for (size_t i = 0; i < players.size(); i++) {
            if (!players[i].isAlive) continue;
            players[i].directionController.processInput();
            nextPositions[i] = players[i].directionController.getNextPosition(players[i].snake.getHead());
        }

        // 2. Check wall and self collisions for each player
        for (size_t i = 0; i < players.size(); i++) {
            if (!players[i].isAlive) continue;
            pair<int, int> newHead = nextPositions[i];

            if (CollisionDetector::isOutOfBounds(newHead, board) || 
                CollisionDetector::isWall(newHead, board) ||
                players[i].snake.checkSelfCollision(newHead)) {
                diedThisTick[i] = true;
            }
        }

        // 3. Check inter-snake collisions (hits other snake body or head-on collision)
        if (players.size() >= 2 && players[0].isAlive && players[1].isAlive) {
            if (nextPositions[0] == nextPositions[1] ||
                players[1].snake.checkCollisionWith(nextPositions[0]) ||
                players[0].snake.checkCollisionWith(nextPositions[1])) {
                mutualCollision = true;
            }
        }

        // Handle mutual collision -> game over for both, highest score wins
        if (mutualCollision) {
            gameOver = true;
            for (auto& p : players) p.isAlive = false;
            if (players[0].score > players[1].score) winner = 1;
            else if (players[1].score > players[0].score) winner = 2;
            else winner = 0; // Tie
        } else {
            // Apply single player deaths (other player declared default winner)
            for (size_t i = 0; i < players.size(); i++) {
                if (diedThisTick[i]) {
                    players[i].isAlive = false;
                    size_t other = (i == 0) ? 1 : 0;
                    if (players[other].isAlive && winner == 0) {
                        winner = static_cast<int>(other + 1);
                    }
                }
            }

            // Move surviving players & handle food
            for (size_t i = 0; i < players.size(); i++) {
                if (!players[i].isAlive) continue;
                pair<int, int> newHead = nextPositions[i];

                if (CollisionDetector::isFood(newHead, foodManager)) {
                    players[i].snake.grow();
                    players[i].score += pointsPerFood;
                    foodManager.remove(board);
                }

                players[i].snake.move(newHead, board);
            }

            // Check if all players dead
            size_t aliveCount = 0;
            for (const auto& p : players) {
                if (p.isAlive) aliveCount++;
            }
            if (aliveCount == 0) {
                gameOver = true;
                if (winner == 0) {
                    if (players[0].score > players[1].score) winner = 1;
                    else if (players[1].score > players[0].score) winner = 2;
                    else winner = 0;
                }
            }
        }

        // Respawn food if eaten
        if (!foodManager.isPresent() && !gameOver) {
            foodManager.placeRandom(board);
        }

        statePublisher.publish(board, players, foodManager, gameOver, winner);
        return !gameOver;
    }

    shared_ptr<const GameState> getGameState() const {
        return statePublisher.getState();
    }

    int getRows() const { return statePublisher.getState()->rows; }
    int getCols() const { return statePublisher.getState()->cols; }
    int getScore() const { return statePublisher.getState()->score; }
    bool isGameOver() const { return statePublisher.getState()->gameOver; }

    static Direction getDirectionUp() { return UP; }
    static Direction getDirectionDown() { return DOWN; }
    static Direction getDirectionLeft() { return LEFT; }
    static Direction getDirectionRight() { return RIGHT; }
};

#endif // GAMELOGIC_H
