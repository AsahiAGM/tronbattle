#ifndef TRON_COMMON_H
#define TRON_COMMON_H

const int WIDTH = 30;
const int HEIGHT = 20;
const int MAX_PLAYERS = 4;

typedef enum Direction { NOTHING, UP, DOWN, LEFT, RIGHT } Direction;

struct Player {
  int id;
  int x;
  int y;
  bool alive;
};

#endif // TRON_COMMON_H
