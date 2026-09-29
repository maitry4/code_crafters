# Lab 4 — The Tests You Cannot Write

## Section 1 — Five Rules

1. The snake dies when its head enters its own body.
2. Eating food increases the score by exactly the configured number of points.
3. Food never appears on a cell occupied by the snake.
4. The snake cannot immediately reverse its direction.
5. The snake dies when it moves outside the board.

## Section 2 — Part B

| Rule                                                                       | Test written? | Blocking dependency                                                                                                                                                                                         |
| -------------------------------------------------------------------------- | ------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Snake dies when its head enters its own body                               | No            | `gameLogic.h:579` — `SnakeGameLogic::update()` checks `snake.checkSelfCollision(newHead)`, but there is no way to place the snake into a controlled self-collision state through the public game interface. |
| Eating food increases the score by exactly the configured number of points | No            | `gameLogic.h:588` — the score is changed when food is eaten, but the food position is selected internally by `FoodManager`, so the test cannot reliably place food directly in front of the snake.          |
| Food never appears on a cell occupied by the snake                         | Yes           | —                                                                                                                                                                                                           |
| The snake cannot immediately reverse its direction                         | Yes           | —                                                                                                                                                                                                           |
| The snake dies when it moves outside the board                             | Yes           | —                                                                                                                                                                                                           |

### Rules testable without changing source

3 out of 5 rules were directly testable without changing production source.

## Section 3 — Coverage

Branch coverage: **85.71%**

Coverage shows which branches were executed, but it does not prove that the resulting behaviour was checked by an assertion.

The food-scoring behaviour was not directly verified by the original Part B tests because the tests could not deterministically place food in the snake's path. Therefore, execution of food-related code alone would not establish that eating food increases the score by exactly the configured amount.

## Section 4 — Making the Hardest Rule Testable

### Rule selected

Eating food increases the score by exactly the configured number of points.

### Why it was difficult to test

`FoodManager` selected a random empty cell internally. The test therefore had no deterministic way to arrange food directly in front of the snake.

### Seam

A **dependency-injection seam** was introduced around food-position selection.

`FoodManager` now depends on a `FoodPositionProvider`. The normal production implementation, `RandomFoodPositionProvider`, preserves the existing random behaviour. Tests can provide a deterministic implementation instead.

### Enabling point

The enabling point is the food-position selection performed by `FoodManager::placeRandom()`.

### What production code gave up

`FoodManager` no longer directly performs the random position selection itself. It delegates that decision to the injected `FoodPositionProvider`.

### Commit 1

`cfc9820` — `Add seam for deterministic food placement`

This commit contains the production seam only.

### Commit 2

`4d9a0f9` — `Test score increase when food is eaten`

This commit contains the test-only change using a deterministic food-position provider.

### Result

After introducing the seam, the test could deterministically place food in front of the snake and assert:

```cpp
assert(game.getScore() == 10);
```

The scoring rule was therefore made testable.

## Section 5 — Test Double

The test uses a **stub**.

The `FixedFoodPositionProvider` supplies a predetermined food position to the system. It provides controlled data to the code under test rather than being used to verify that the system called a collaborator.

## Section 6 — Test Smells

| Location              | Smell                  | One-line fix                                                                                                           |
| --------------------- | ---------------------- | ---------------------------------------------------------------------------------------------------------------------- |
| `lab4/test.cpp:22,27` | **Assertion roulette** | Add assertion messages so a failure identifies whether food presence or a particular snake segment caused the failure. |
| `lab4/test.cpp:39-41` | **Eager test**         | Split the reverse-direction assertion from the valid-direction assertions into separate focused tests.                 |
