#include "../gameLogic.h"

#include <cassert>
#include <iostream>
#include <random>

using namespace std;

void test_food_never_on_snake() {
    Board board;
    board.initialize(10, 10);

    Snake snake;
    snake.initialize({5, 5}, 3, RIGHT, board);

    mt19937 rng(12345);
    FoodManager food(rng);

    for (int i = 0; i < 100; i++) {
        food.placeRandom(board);

        assert(food.isPresent());

        auto foodPosition = food.getPosition();

        for (const auto& segment : snake.getBody()) {
            assert(foodPosition != segment);
        }

        food.remove(board);
    }
}

void test_direction_cannot_reverse() {
    DirectionController controller;

    controller.initialize(RIGHT);

    assert(!controller.isValidChange(LEFT));
    assert(controller.isValidChange(UP));
    assert(controller.isValidChange(DOWN));
}

void test_game_ends_when_snake_leaves_board() {
    SnakeGameLogic game;

    game.initializeBoard(
        3,      // rows
        5,      // cols
        1,      // snake length
        10,     // points per food
        RIGHT
    );

    // Snake starts at (1,2).
    // First update -> (1,3), still inside.
    assert(game.update());

    // Second update -> (1,4), still inside.
    assert(game.update());

    // Third update -> (1,5), outside the board.
    assert(!game.update());

    assert(game.isGameOver());
}
class FixedFoodPositionProvider : public FoodPositionProvider {
public:
    pair<int,int> choosePosition(
        const vector<pair<int,int>>& emptyCells) override {
        return {1, 3};
    }
};

void test_eating_food_increases_score() {
    FixedFoodPositionProvider provider;
    SnakeGameLogic game(provider);

    game.initializeBoard(3, 5, 1, 10, RIGHT);

    assert(game.update());
    assert(game.getScore() == 10);
}
int main() {
    test_food_never_on_snake();
    test_direction_cannot_reverse();
    test_game_ends_when_snake_leaves_board();
    test_eating_food_increases_score();

    cout << "All Part B tests passed.\n";
    return 0;
}