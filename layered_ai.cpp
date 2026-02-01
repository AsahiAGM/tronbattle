#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

const int WIDTH = 30;
const int HEIGHT = 20;

typedef enum Direction {
  NOTHING = 0,
  UP = 1,
  DOWN = 2,
  LEFT = 3,
  RIGHT = 4
} Direction;

struct Tronbike {
  int x0, y0;
  int x1, y1;
};

struct Component {
  int size;
  bool has_player;
  bool has_enemy;
};

/*
 * 方向を文字列に変換
 */
string direction_to_string(Direction dir) {
  switch (dir) {
  case UP:
    return "UP";
  case DOWN:
    return "DOWN";
  case LEFT:
    return "LEFT";
  case RIGHT:
    return "RIGHT";
  default:
    return "UP";
  }
}

/*
 * フィールド状態を更新
 */
void field_update(int field[WIDTH][HEIGHT], Tronbike &player, Tronbike enemy[],
                  int num_enemies) {
  // プレイヤーの現在位置をマーク
  field[player.x1][player.y1] = 1;

  // 敵の現在位置をマーク
  for (int i = 0; i < num_enemies; i++) {
    if (enemy[i].x1 >= 0) {
      field[enemy[i].x1][enemy[i].y1] = 2;
    }
  }
}

/*
 * 連結成分を分析（Flood Fill）
 * プレイヤーの現在位置は空マスとして扱う
 */
Component analyze_component(int field[WIDTH][HEIGHT], int start_x, int start_y,
                            int player_x, int player_y,
                            vector<pair<int, int>> enemy_positions) {
  Component comp;
  comp.size = 0;
  comp.has_player = false;
  comp.has_enemy = false;

  bool visited[WIDTH][HEIGHT] = {{false}};
  int queue_x[600], queue_y[600];
  int front = 0, rear = 0;
  int dx[] = {0, 0, -1, 1};
  int dy[] = {-1, 1, 0, 0};

  queue_x[rear] = start_x;
  queue_y[rear] = start_y;
  rear++;
  visited[start_x][start_y] = true;

  while (front < rear) {
    int idx = front++;
    int cx = queue_x[idx];
    int cy = queue_y[idx];
    comp.size++;

    // プレイヤーがこの成分にいるか
    if (cx == player_x && cy == player_y) {
      comp.has_player = true;
    }

    // 敵がこの成分にいるか
    for (size_t i = 0; i < enemy_positions.size(); i++) {
      if (cx == enemy_positions[i].first && cy == enemy_positions[i].second) {
        comp.has_enemy = true;
      }
    }

    for (int i = 0; i < 4; i++) {
      int nx = cx + dx[i];
      int ny = cy + dy[i];

      if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT)
        continue;
      if (visited[nx][ny])
        continue;

      // プレイヤーと敵の現在位置は空マスとして扱う
      bool is_current_player = (nx == player_x && ny == player_y);
      bool is_current_enemy = false;
      for (size_t j = 0; j < enemy_positions.size(); j++) {
        if (nx == enemy_positions[j].first && ny == enemy_positions[j].second) {
          is_current_enemy = true;
          break;
        }
      }

      if (field[nx][ny] != 0 && !is_current_player && !is_current_enemy)
        continue;

      visited[nx][ny] = true;
      queue_x[rear] = nx;
      queue_y[rear] = ny;
      rear++;
    }
  }

  return comp;
}

/*
 * 1手進めた仮想フィールドでの連結成分を分析
 */
Component analyze_component_after_move(int field[WIDTH][HEIGHT], int player_x,
                                       int player_y, Direction dir,
                                       vector<pair<int, int>> enemy_positions) {
  // 仮想フィールドを作成
  int virtual_field[WIDTH][HEIGHT];
  for (int x = 0; x < WIDTH; x++) {
    for (int y = 0; y < HEIGHT; y++) {
      virtual_field[x][y] = field[x][y];
    }
  }

  // プレイヤーの現在位置を壁にする
  virtual_field[player_x][player_y] = 1;

  // 移動先を計算
  int dx[] = {0, 0, -1, 1};
  int dy[] = {-1, 1, 0, 0};
  int dir_idx = dir - 1; // UP=1, DOWN=2, LEFT=3, RIGHT=4

  int new_x = player_x + dx[dir_idx];
  int new_y = player_y + dy[dir_idx];

  // 移動先が有効かチェック
  if (new_x < 0 || new_x >= WIDTH || new_y < 0 || new_y >= HEIGHT) {
    Component invalid;
    invalid.size = 0;
    invalid.has_player = false;
    invalid.has_enemy = false;
    return invalid;
  }

  // 移動先が壁かチェック（敵の現在位置は除く）
  bool is_enemy_pos = false;
  for (size_t i = 0; i < enemy_positions.size(); i++) {
    if (new_x == enemy_positions[i].first &&
        new_y == enemy_positions[i].second) {
      is_enemy_pos = true;
      break;
    }
  }

  if (virtual_field[new_x][new_y] != 0 && !is_enemy_pos) {
    Component invalid;
    invalid.size = 0;
    invalid.has_player = false;
    invalid.has_enemy = false;
    return invalid;
  }

  // 移動先から連結成分を分析
  return analyze_component(virtual_field, new_x, new_y, new_x, new_y,
                           enemy_positions);
}

/*
 * ★★★ 第三層：BFS戦略 ★★★
 * 連結成分内に自分だけがいる場合の最長生存戦略
 */
Direction layer3_bfs_strategy(int field[WIDTH][HEIGHT], Tronbike &player,
                              vector<pair<int, int>> enemy_positions) {
  Direction directions[] = {UP, DOWN, LEFT, RIGHT};
  Direction best_dir = UP;
  int max_size = -1;

  // 各方向への移動をシミュレート
  for (int i = 0; i < 4; i++) {
    Component comp = analyze_component_after_move(
        field, player.x1, player.y1, directions[i], enemy_positions);

    // 移動不可能な場合はスキップ
    if (comp.size == 0)
      continue;

    // 連結成分内に敵がいない場合のみ評価
    if (!comp.has_enemy) {
      if (comp.size > max_size) {
        max_size = comp.size;
        best_dir = directions[i];
      }
    }
  }

  // 敵がいない成分が見つからなかった場合は、最大サイズの成分を選択
  if (max_size == -1) {
    max_size = -1;
    for (int i = 0; i < 4; i++) {
      Component comp = analyze_component_after_move(
          field, player.x1, player.y1, directions[i], enemy_positions);
      if (comp.size > max_size) {
        max_size = comp.size;
        best_dir = directions[i];
      }
    }
  }

  return best_dir;
}

int main() {
  int field[WIDTH][HEIGHT];

  // フィールド初期化
  for (int x = 0; x < WIDTH; x++) {
    for (int y = 0; y < HEIGHT; y++) {
      field[x][y] = 0;
    }
  }

  // ゲームループ
  while (1) {
    int n, p;
    cin >> n >> p;
    cin.ignore();

    Tronbike player;
    Tronbike enemy[3];
    int enemy_count = 0;

    for (int i = 0; i < n; i++) {
      int x0, y0, x1, y1;
      cin >> x0 >> y0 >> x1 >> y1;
      cin.ignore();

      if (i == p) {
        player.x0 = x0;
        player.y0 = y0;
        player.x1 = x1;
        player.y1 = y1;
      } else if (x0 >= 0) {
        enemy[enemy_count].x0 = x0;
        enemy[enemy_count].y0 = y0;
        enemy[enemy_count].x1 = x1;
        enemy[enemy_count].y1 = y1;
        enemy_count++;
      }
    }

    // フィールド更新
    field_update(field, player, enemy, enemy_count);

    // 敵の位置リストを作成
    vector<pair<int, int>> enemy_positions;
    for (int i = 0; i < enemy_count; i++) {
      if (enemy[i].x1 >= 0) {
        enemy_positions.push_back(make_pair(enemy[i].x1, enemy[i].y1));
      }
    }

    // 第三層：BFS戦略
    Direction move = layer3_bfs_strategy(field, player, enemy_positions);

    cout << direction_to_string(move) << endl;
  }

  return 0;
}
