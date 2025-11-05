#include "gameLogic.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <memory>
#include <functional>
#include <map>
#include <vector>

#ifdef _WIN32
    #include <conio.h>
    #include <windows.h>
#else
    #include <unistd.h>
    #include <termios.h>
    #include <fcntl.h>
    #include <sys/ioctl.h>
    #include <cstring>
#endif

using namespace std;

// ============================================
// Event System for Extensibility
// ============================================

enum class EventType {
    FOOD_EATEN,
    SNAKE_GREW,
    GAME_OVER,
    SCORE_CHANGED,
    HIGH_SCORE_BEATEN
};

class GameEvent {
public:
    EventType type;
    int value;
    string message;
    
    GameEvent(EventType t, int v = 0, string msg = "") 
        : type(t), value(v), message(msg) {}
};

class EventListener {
public:
    virtual ~EventListener() = default;
    virtual void onEvent(const GameEvent& event) = 0;
};

class EventManager {
private:
    map<EventType, vector<EventListener*>> listeners;
    
public:
    void subscribe(EventType type, EventListener* listener) {
        listeners[type].push_back(listener);
    }
    
    void notify(const GameEvent& event) {
        auto it = listeners.find(event.type);
        if (it != listeners.end()) {
            for (auto* listener : it->second) {
                listener->onEvent(event);
            }
        }
    }
};

// ============================================
// Configuration System
// ============================================

class GameConfig {
public:
    // Board settings
    int rows;
    int cols;
    int startingLength;
    
    // Gameplay settings
    int updateDelay;
    int pointsPerFood;
    
    // Display settings
    char snakeHeadChar;
    char snakeBodyChar;
    char foodChar;
    char wallChar;
    char emptyChar;
    
    GameConfig() 
        : rows(20), cols(40), startingLength(3),
          updateDelay(150), pointsPerFood(10),
          snakeHeadChar('O'), snakeBodyChar('o'),
          foodChar('*'), wallChar('#'), emptyChar(' ') {}
};

// ============================================
// High Score Manager
// ============================================

class HighScoreManager : public EventListener {
private:
    const string filename = "game_highest.txt";
    int highScore;
    EventManager* eventManager;
    
public:
    HighScoreManager() : highScore(0), eventManager(nullptr) {
        loadHighScore();
    }
    
    void setEventManager(EventManager* em) {
        eventManager = em;
        if (eventManager) {
            eventManager->subscribe(EventType::SCORE_CHANGED, this);
        }
    }
    
    void onEvent(const GameEvent& event) override {
        if (event.type == EventType::SCORE_CHANGED) {
            checkAndSaveHighScore(event.value);
        }
    }
    
    void loadHighScore() {
        ifstream file(filename);
        if (file.is_open()) {
            file >> highScore;
            file.close();
        } else {
            highScore = 0;
        }
    }
    
    void checkAndSaveHighScore(int score) {
        if (score > highScore) {
            int oldHighScore = highScore;
            highScore = score;
            saveHighScore();
            
            if (eventManager && oldHighScore > 0) {
                eventManager->notify(GameEvent(EventType::HIGH_SCORE_BEATEN, score));
            }
        }
    }
    
    void saveHighScore() {
        ofstream file(filename);
        if (file.is_open()) {
            file << highScore;
            file.close();
        }
    }
    
    int getHighScore() const {
        return highScore;
    }
    
    bool isNewHighScore(int score) const {
        return score > highScore;
    }
};

// ============================================
// Platform-Independent Terminal Control
// ============================================

class TerminalController {
private:
#ifndef _WIN32
    termios originalSettings;
    bool settingsChanged = false;
    int originalFlags = 0;
#endif

public:
    void clearScreen() {
#ifdef _WIN32
        system("cls");
#else
        cout << "\033[H\033[J";
        cout.flush();
        this_thread::sleep_for(chrono::milliseconds(10));
#endif
    }
    
    void setCursorPosition(int row, int col) {
#ifdef _WIN32
        COORD pos = {(SHORT)col, (SHORT)row};
        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
#else
        cout << "\033[" << (row + 1) << ";" << (col + 1) << "H";
        cout.flush();
#endif
    }
    
    void hideCursor() {
#ifdef _WIN32
        HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_CURSOR_INFO info;
        info.dwSize = 100;
        info.bVisible = FALSE;
        SetConsoleCursorInfo(consoleHandle, &info);
#else
        cout << "\033[?25l";
        cout.flush();
#endif
    }
    
    void showCursor() {
#ifdef _WIN32
        HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_CURSOR_INFO info;
        info.dwSize = 100;
        info.bVisible = TRUE;
        SetConsoleCursorInfo(consoleHandle, &info);
#else
        cout << "\033[?25h";
        cout.flush();
#endif
    }
    
    void enableRawMode() {
#ifndef _WIN32
        tcgetattr(STDIN_FILENO, &originalSettings);
        settingsChanged = true;
        
        termios raw = originalSettings;
        raw.c_lflag &= ~(ECHO | ICANON);
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
        
        originalFlags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, originalFlags | O_NONBLOCK);
#endif
    }
    
    void disableRawMode() {
#ifndef _WIN32
        if (settingsChanged) {
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &originalSettings);
            fcntl(STDIN_FILENO, F_SETFL, originalFlags);
            settingsChanged = false;
        }
#endif
    }
    
    bool kbhit() {
#ifdef _WIN32
        return _kbhit() != 0;
#else
        int bytesWaiting;
        ioctl(STDIN_FILENO, FIONREAD, &bytesWaiting);
        return bytesWaiting > 0;
#endif
    }
    
    char getch() {
#ifdef _WIN32
        return _getch();
#else
        char c = 0;
        read(STDIN_FILENO, &c, 1);
        return c;
#endif
    }
    
    ~TerminalController() {
        disableRawMode();
        showCursor();
    }
};

// ============================================
// Game Renderer with Config Support
// ============================================

class GameRenderer {
private:
    TerminalController& terminal;
    HighScoreManager& highScoreManager;
    const GameConfig& config;
    int headerRows;
    int footerRows;
    
public:
    GameRenderer(TerminalController& term, HighScoreManager& hsm, const GameConfig& cfg) 
        : terminal(term), highScoreManager(hsm), config(cfg),
          headerRows(6), footerRows(2) {}
    
    void drawFullScreen(const SnakeGameLogic& game, bool showInstructions = false) {
        auto state = game.getGameState();
        
        ostringstream buffer;
        
        // Title
        buffer << "\n";
        buffer << "  +===============================+\n";
        buffer << "  |       SNAKE GAME              |\n";
        buffer << "  +===============================+\n\n";
        
        // Game board
        buffer << "+";
        for (int i = 0; i < state->cols; i++) buffer << "-";
        buffer << "+\n";
        
        for (int r = 0; r < state->rows; r++) {
            buffer << "|";
            for (int c = 0; c < state->cols; c++) {
                buffer << " ";
            }
            buffer << "|\n";
        }
        
        buffer << "+";
        for (int i = 0; i < state->cols; i++) buffer << "-";
        buffer << "+\n";
        
        // Controls section
        buffer << "\n";
        if (showInstructions) {
            buffer << "  +===================================+\n";
            buffer << "  |  CONTROLS:                        |\n";
            buffer << "  |                                   |\n";
            buffer << "  |  W or UP Arrow    - Move UP       |\n";
            buffer << "  |  S or DOWN Arrow  - Move DOWN     |\n";
            buffer << "  |  A or LEFT Arrow  - Move LEFT     |\n";
            buffer << "  |  D or RIGHT Arrow - Move RIGHT    |\n";
            buffer << "  |  Q                - Quit Game     |\n";
            buffer << "  |                                   |\n";
            buffer << "  |  Press ENTER to start...          |\n";
            buffer << "  +===================================+\n";
        } else {
            buffer << "  Controls: Arrow Keys or WASD  |  Q: Quit\n";
        }
        
        terminal.clearScreen();
        terminal.hideCursor();
        cout << buffer.str();
        cout.flush();
        
        // Output the score after the static board
        terminal.setCursorPosition(4, 0);
        ostringstream scoreBuffer;
        scoreBuffer << "  Score: " << setw(4) << state->score 
                    << "  |  Length: " << setw(3) << state->snakeLength 
                    << "  |  High Score: " << setw(4) << highScoreManager.getHighScore();
        scoreBuffer << "  ";
        
        cout << scoreBuffer.str();
        cout.flush();
    }
    
    void updateGameBoard(const SnakeGameLogic& game) {
        auto state = game.getGameState();
        
        // Build score line
        ostringstream scoreBuffer;
        scoreBuffer << "  Score: " << setw(4) << state->score 
                    << "  |  Length: " << setw(3) << state->snakeLength 
                    << "  |  High Score: " << setw(4) << highScoreManager.getHighScore();
        scoreBuffer << "  ";
        
        // Update score
        terminal.setCursorPosition(4, 0);
        cout << scoreBuffer.str();
        cout.flush();
        
        // Build and render each row
        for (int r = 0; r < state->rows; r++) {
            ostringstream rowBuffer;
            terminal.setCursorPosition(headerRows + r, 1);
            
            for (int c = 0; c < state->cols; c++) {
                int cellType = state->board[r][c];
                
                switch(cellType) {
                    case 0: // EMPTY
                        rowBuffer << config.emptyChar;
                        break;
                    case 1: // SNAKE
                        if (r == state->snake.front().first && 
                            c == state->snake.front().second) {
                            rowBuffer << config.snakeHeadChar;
                        } else {
                            rowBuffer << config.snakeBodyChar;
                        }
                        break;
                    case 2: // FOOD
                        rowBuffer << config.foodChar;
                        break;
                    case 3: // WALL
                        rowBuffer << config.wallChar;
                        break;
                    default:
                        rowBuffer << config.emptyChar;
                }
            }
            
            cout << rowBuffer.str();
        }
        
        cout.flush();
    }
    
    void showGameOver(const SnakeGameLogic& game) {
        auto state = game.getGameState();
        
        ostringstream buffer;
        buffer << "\n";
        buffer << "  +===============================+\n";
        buffer << "  |         GAME OVER!            |\n";
        buffer << "  |   Final Score: " << setw(4) << state->score << "          |\n";
        buffer << "  |   High Score:  " << setw(4) << highScoreManager.getHighScore() << "          |\n";
        
        if (highScoreManager.isNewHighScore(state->score) && state->score > 0) {
            buffer << "  |                               |\n";
            buffer << "  |   *** NEW HIGH SCORE! ***     |\n";
        }
        
        buffer << "  |                               |\n";
        buffer << "  |   Press R to Replay           |\n";
        buffer << "  |   Press Q to Quit             |\n";
        buffer << "  +===============================+\n";
        
        int messageRow = headerRows + state->rows + 3;
        terminal.setCursorPosition(messageRow, 0);
        cout << buffer.str();
        cout.flush();
    }
};

// ============================================
// Input Handler
// ============================================

class InputHandler {
private:
    TerminalController& terminal;
    SnakeGameLogic& game;
    char buffer[3];
    int bufferPos = 0;
    
public:
    InputHandler(TerminalController& term, SnakeGameLogic& g) 
        : terminal(term), game(g) {
        memset(buffer, 0, sizeof(buffer));
    }
    
    char getKey() {
        if (!terminal.kbhit()) return 0;
        return terminal.getch();
    }
    
    char pollInput() {
        char key = getKey();
        if (key == 0) return 0;
        
#ifdef _WIN32
        if (key == -32 || key == 0) {
            key = terminal.getch();
            switch(key) {
                case 72: game.setDirection(SnakeGameLogic::getDirectionUp()); break;
                case 80: game.setDirection(SnakeGameLogic::getDirectionDown()); break;
                case 75: game.setDirection(SnakeGameLogic::getDirectionLeft()); break;
                case 77: game.setDirection(SnakeGameLogic::getDirectionRight()); break;
            }
            return 0;
        }
#else
        if (key == 27) {
            buffer[0] = key;
            bufferPos = 1;
            
            auto startTime = chrono::steady_clock::now();
            while (bufferPos < 3 && chrono::duration_cast<chrono::milliseconds>(
                   chrono::steady_clock::now() - startTime).count() < 20) {
                if (terminal.kbhit()) {
                    buffer[bufferPos++] = terminal.getch();
                }
            }
            
            if (bufferPos >= 3 && buffer[0] == 27 && buffer[1] == '[') {
                switch(buffer[2]) {
                    case 'A': game.setDirection(SnakeGameLogic::getDirectionUp()); break;
                    case 'B': game.setDirection(SnakeGameLogic::getDirectionDown()); break;
                    case 'C': game.setDirection(SnakeGameLogic::getDirectionRight()); break;
                    case 'D': game.setDirection(SnakeGameLogic::getDirectionLeft()); break;
                }
            }
            
            memset(buffer, 0, sizeof(buffer));
            bufferPos = 0;
            return 0;
        }
#endif
        
        switch(key) {
            case 'w': case 'W':
                game.setDirection(SnakeGameLogic::getDirectionUp());
                return 0;
            case 's': case 'S':
                game.setDirection(SnakeGameLogic::getDirectionDown());
                return 0;
            case 'a': case 'A':
                game.setDirection(SnakeGameLogic::getDirectionLeft());
                return 0;
            case 'd': case 'D':
                game.setDirection(SnakeGameLogic::getDirectionRight());
                return 0;
            case 'q': case 'Q':
                return 'Q';
            default:
                return 0;
        }
    }
    
    void clearBuffer() {
        while (terminal.kbhit()) {
            terminal.getch();
        }
        memset(buffer, 0, sizeof(buffer));
        bufferPos = 0;
    }
};

// ============================================
// Game Session Manager
// ============================================

class GameSession {
private:
    SnakeGameLogic game;
    GameConfig config;
    EventManager eventManager;
    TerminalController& terminal;
    HighScoreManager& highScoreManager;
    GameRenderer renderer;
    int currentUpdateDelay;
    int lastScore;
    
public:
    GameSession(TerminalController& term, HighScoreManager& hsm, const GameConfig& cfg)
        : config(cfg), terminal(term), highScoreManager(hsm),
          renderer(term, hsm, config), currentUpdateDelay(cfg.updateDelay),
          lastScore(0) {
        
        // Wire up event system
        highScoreManager.setEventManager(&eventManager);
    }
    
    void initialize() {
        game.initializeBoard(
            config.rows,
            config.cols,
            config.startingLength,
            config.pointsPerFood,
            SnakeGameLogic::getDirectionRight()
        );
    }
    
    bool run() {
        InputHandler input(terminal, game);
        
        // Draw initial screen with instructions
        renderer.drawFullScreen(game, true);
        
        // Wait for ENTER key to start
        bool enterPressed = false;
        while (!enterPressed) {
            if (terminal.kbhit()) {
                char key = terminal.getch();
                if (key == '\n' || key == '\r') {
                    enterPressed = true;
                } else if (key == 'q' || key == 'Q') {
                    return false;
                }
            }
            this_thread::sleep_for(chrono::milliseconds(50));
        }
        
        input.clearBuffer();
        renderer.drawFullScreen(game, false);
        this_thread::sleep_for(chrono::milliseconds(50));
        
        // Game loop
        auto lastUpdate = chrono::steady_clock::now();
        bool gameActive = true;
        
        while (gameActive) {
            auto now = chrono::steady_clock::now();
            auto elapsed = chrono::duration_cast<chrono::milliseconds>(now - lastUpdate).count();
            
            char key = input.pollInput();
            if (key == 'Q') {
                return false;
            }
            
            if (elapsed >= currentUpdateDelay) {
                gameActive = game.update();
                
                // Check for score changes and notify
                auto state = game.getGameState();
                if (state->score != lastScore) {
                    eventManager.notify(GameEvent(EventType::SCORE_CHANGED, state->score));
                    lastScore = state->score;
                }
                
                renderer.updateGameBoard(game);
                lastUpdate = now;
            }
            
            this_thread::sleep_for(chrono::milliseconds(10));
        }
        
        // Game over
        highScoreManager.checkAndSaveHighScore(game.getGameState()->score);
        renderer.showGameOver(game);
        
        // Wait for user input
        while (true) {
            if (terminal.kbhit()) {
                char key = terminal.getch();
                if (key == 'r' || key == 'R') {
                    return true;
                } else if (key == 'q' || key == 'Q') {
                    return false;
                }
            }
            this_thread::sleep_for(chrono::milliseconds(50));
        }
    }
};

// ============================================
// Main Game Application
// ============================================

class SnakeGameApp {
private:
    TerminalController terminal;
    HighScoreManager highScoreManager;
    GameConfig config;
    
public:
    SnakeGameApp() {}
    
    void run() {
        terminal.enableRawMode();
        
        while (true) {
            // Create game session
            GameSession session(terminal, highScoreManager, config);
            session.initialize();
            
            bool replay = session.run();
            if (!replay) {
                break;
            }
        }
        
        terminal.clearScreen();
        terminal.showCursor();
        ostringstream exitBuffer;
        exitBuffer << "\n  Thanks for playing!\n\n";
        cout << exitBuffer.str();
        cout.flush();
    }
};

// ============================================
// Main Entry Point
// ============================================

int main() {
    SnakeGameApp app;
    app.run();
    return 0;
}
