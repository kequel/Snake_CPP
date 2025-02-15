# Snake Game in C++

This is a classic Snake game written in C++ using the SDL2 library, developed as part of the Basics of Computer Programming course at Gdańsk University of Technology. 

## Author
- Karolina Glaza [GitHub](https://github.com/kequel)

### Version
- Current version: **1.0**

## Description
Snake is an arcade game where the player controls a snake that moves around the screen, eating food to grow longer. 
The goal is to survive as long as possible while eating food and avoiding collisions with the walls or itself. The game includes mechanics for increasing speed, power-ups, and a ranking system.

## Gameplay Mechanics
#### Snake Behavior

- The snake moves in four directions: up, down, left, and right.

- It does not wrap around the board – hitting a wall results in game over.

- When the snake collides with itself, the game ends.

- Speed increases over time, making the game progressively harder.

- The snake's body segments alternate in size for a visual effect.

#### Blue Dots (Food)

- Spawns randomly on the board.

- Eating a blue dot increases the snake's length.

- Consuming a blue dot grants points to the player.

#### Red Dots (Bonus)

- Occasionally spawns and remains on the board for a limited time.

- A progress bar indicates how long it will stay visible.

- Can apply one of two effects:

  - Slows down the snake for a short duration.

  - Shortens the snake's length by a few segments.

- If not collected in time, it disappears.

#### Scoring System

- Players earn points by consuming blue and red dots.

- A high score list keeps track of the top three best runs.

- If the player achieves a new high score, they can enter their name.


## Features
1. Game parameters are defined in the beginning of the main.cpp file, which allows easy customization.
2. Smooth snake movement and increasing difficulty, depending on world time, not the computer speed.
3. Customed animated graphics with pulsating food items.

## Cuts from the game:

![cut1](img/1.png)
![cut2](img/2.png)
![cut3](img/3.png)

## Installation 

To compile and run the game, ensure SDL2 is installed. 


