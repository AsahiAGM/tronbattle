#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <vector>

using namespace std;

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

/*
 * 第三層BFS戦略（strategy.cppと同じ）
 */
Direction layer3_bfs_strategy(Player &player, Player enemies[], int num_enemies,
                              int field[WIDTH][HEIGHT]) {
  int dx[] = {0, 0, -1, 1};
  int dy[] = {-1, 1, 0, 0};
  Direction directions[] = {UP, DOWN, LEFT, RIGHT};

  vector<pair<int, int>> enemy_positions;
  for (int i = 0; i < num_enemies; i++) {
    if (enemies[i].alive) {
      enemy_positions.push_back(make_pair(enemies[i].x, enemies[i].y));
    }
  }

  int virtual_field[WIDTH][HEIGHT];
  for (int x = 0; x < WIDTH; x++) {
    for (int y = 0; y < HEIGHT; y++) {
      virtual_field[x][y] = field[x][y];
    }
  }
  virtual_field[player.x][player.y] = 1;

  Direction best_no_enemy = UP;
  Direction best_with_enemy = UP;
  int max_size_no_enemy = -1;
  int max_size_with_enemy = -1;

  for (int i = 0; i < 4; i++) {
    int nx = player.x + dx[i];
    int ny = player.y + dy[i];

    if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT)
      continue;

    bool is_enemy_pos = false;
    for (size_t j = 0; j < enemy_positions.size(); j++) {
      if (nx == enemy_positions[j].first && ny == enemy_positions[j].second) {
        is_enemy_pos = true;
        break;
      }
    }
    if (virtual_field[nx][ny] != 0 && !is_enemy_pos)
      continue;

    bool visited[WIDTH][HEIGHT] = {{false}};
    int queue_x[600], queue_y[600];
    int front = 0, rear = 0;

    queue_x[rear] = nx;
    queue_y[rear] = ny;
    rear++;
    visited[nx][ny] = true;

    int comp_size = 0;
    bool has_enemy = false;

    while (front < rear) {
      int idx = front++;
      int cx = queue_x[idx];
      int cy = queue_y[idx];
      comp_size++;

      for (size_t j = 0; j < enemy_positions.size(); j++) {
        if (cx == enemy_positions[j].first && cy == enemy_positions[j].second) {
          has_enemy = true;
        }
      }

      for (int d = 0; d < 4; d++) {
        int nnx = cx + dx[d];
        int nny = cy + dy[d];

        if (nnx < 0 || nnx >= WIDTH || nny < 0 || nny >= HEIGHT)
          continue;
        if (visited[nnx][nny])
          continue;

        bool is_enemy_pos2 = false;
        for (size_t j = 0; j < enemy_positions.size(); j++) {
          if (nnx == enemy_positions[j].first &&
              nny == enemy_positions[j].second) {
            is_enemy_pos2 = true;
            break;
          }
        }

        if (virtual_field[nnx][nny] != 0 && !is_enemy_pos2)
          continue;

        visited[nnx][nny] = true;
        queue_x[rear] = nnx;
        queue_y[rear] = nny;
        rear++;
      }
    }

    if (!has_enemy) {
      if (comp_size > max_size_no_enemy) {
        max_size_no_enemy = comp_size;
        best_no_enemy = directions[i];
      }
    } else {
      if (comp_size > max_size_with_enemy) {
        max_size_with_enemy = comp_size;
        best_with_enemy = directions[i];
      }
    }
  }

  cout << "[Layer3] NoEnemy:" << max_size_no_enemy
       << " WithEnemy:" << max_size_with_enemy << " -> ";

  if (max_size_no_enemy > 0) {
    cout << "選択: 敵なし成分 (size=" << max_size_no_enemy << ")" << endl;
    return best_no_enemy;
  }

  if (max_size_with_enemy > 0) {
    cout << "選択: 敵あり成分 (size=" << max_size_with_enemy << ")" << endl;
    return best_with_enemy;
  }

  cout << "選択: デフォルト UP" << endl;
  return UP;
}

int main() {
  int field[WIDTH][HEIGHT];

  for (int x = 0; x < WIDTH; x++) {
    for (int y = 0; y < HEIGHT; y++) {
      field[x][y] = 0;
    }
  }

  cout << "=== 第三層BFS戦略テスト ===" << endl << endl;

  // テストケース1: グラフが分断されている状況
  cout << "【テストケース1】グラフ分断（左側:プレイヤー、右側:敵）" << endl;

  // 中央に垂直の壁を作成（x=14, y=0-19）
  for (int y = 0; y < HEIGHT; y++) {
    field[14][y] = 1;
  }

  // プレイヤー（左側）
  Player player;
  player.id = 0;
  player.x = 7;
  player.y = 10;
  player.alive = true;

  // 敵（右側）
  Player enemies[1];
  enemies[0].id = 1;
  enemies[0].x = 22;
  enemies[0].y = 10;
  enemies[0].alive = true;

  cout << "プレイヤー位置: (" << player.x << ", " << player.y << ")" << endl;
  cout << "敵位置: (" << enemies[0].x << ", " << enemies[0].y << ")" << endl;
  cout << "中央に壁 (x=14)" << endl << endl;

  Direction move1 = layer3_bfs_strategy(player, enemies, 1, field);
  cout << "決定: "
       << (move1 == UP     ? "UP"
           : move1 == DOWN ? "DOWN"
           : move1 == LEFT ? "LEFT"
                           : "RIGHT")
       << endl;
  cout << endl;

  // テストケース2: 同じ成分にいる状況
  cout << "【テストケース2】同じ成分（壁なし）" << endl;

  // フィールドをクリア
  for (int x = 0; x < WIDTH; x++) {
    for (int y = 0; y < HEIGHT; y++) {
      field[x][y] = 0;
    }
  }

  player.x = 10;
  player.y = 10;
  enemies[0].x = 15;
  enemies[0].y = 10;

  cout << "プレイヤー位置: (" << player.x << ", " << player.y << ")" << endl;
  cout << "敵位置: (" << enemies[0].x << ", " << enemies[0].y << ")" << endl;
  cout << "壁なし" << endl << endl;

  Direction move2 = layer3_bfs_strategy(player, enemies, 1, field);
  cout << "決定: "
       << (move2 == UP     ? "UP"
           : move2 == DOWN ? "DOWN"
           : move2 == LEFT ? "LEFT"
                           : "RIGHT")
       << endl;
  cout << endl;

  // テストケース3: 複数の成分、サイズが異なる
  cout << "【テストケース3】複数成分、サイズ差あり" << endl;

  // フィールドをクリア
  for (int x = 0; x < WIDTH; x++) {
    for (int y = 0; y < HEIGHT; y++) {
      field[x][y] = 0;
    }
  }

  // 複雑な壁を作成（小部屋と大部屋）
  // 小部屋（左上、5x5）
  for (int x = 0; x <= 5; x++) {
    field[x][5] = 1;
  }
  for (int y = 0; y <= 5; y++) {
    field[5][y] = 1;
  }

  // プレイヤー（小部屋）
  player.x = 2;
  player.y = 2;

  // 敵（大部屋）
  enemies[0].x = 10;
  enemies[0].y = 10;

  cout << "プレイヤー位置: (" << player.x << ", " << player.y << ") - 小部屋"
       << endl;
  cout << "敵位置: (" << enemies[0].x << ", " << enemies[0].y << ") - 大部屋"
       << endl
       << endl;

  Direction move3 = layer3_bfs_strategy(player, enemies, 1, field);
  cout << "決定: "
       << (move3 == UP     ? "UP"
           : move3 == DOWN ? "DOWN"
           : move3 == LEFT ? "LEFT"
                           : "RIGHT")
       << endl;
  cout << endl;

  cout << "=== テスト完了 ===" << endl;

  return 0;
}
