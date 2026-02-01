#include <iostream>
#include <string>

using namespace std;

/*
 * シンプルBFS戦略
 * 各ターンで4方向の到達可能マス数を計算し、最大の方向を選択
 */

typedef enum Direction { NOTHING, UP, DOWN, LEFT, RIGHT } Direction;

struct Tronbike {
  int x0, y0;
  int x1, y1;
};

// 関数プロトタイプ
string direction_to_string(Direction dir);
int count_reachable_cells(int start_x, int start_y, int field[30][20]);
Direction get_best_direction(Tronbike player, int field[30][20]);
void field_update(int field[30][20], Tronbike &player, Tronbike enemy[3]);

int main() {
  // フィールド初期化（1回のみ）
  int field[30][20];
  for (int x = 0; x < 30; x++) {
    for (int y = 0; y < 20; y++) {
      field[x][y] = 0;
    }
  }

  Tronbike player;
  Tronbike enemy[3];

  while (1) {
    int n, p;
    cin >> n >> p;
    cin.ignore();

    int enemy_idx = 0;
    for (int i = 0; i < n; i++) {
      int x0, y0, x1, y1;
      cin >> x0 >> y0 >> x1 >> y1;
      cin.ignore();

      if (i == p) {
        player.x0 = x0;
        player.y0 = y0;
        player.x1 = x1;
        player.y1 = y1;
      } else if (x0 != -1) {
        enemy[enemy_idx].x0 = x0;
        enemy[enemy_idx].y0 = y0;
        enemy[enemy_idx].x1 = x1;
        enemy[enemy_idx].y1 = y1;
        enemy_idx++;
      }
    }

    for (int i = enemy_idx; i < 3; i++) {
      enemy[i].x0 = enemy[i].y0 = enemy[i].x1 = enemy[i].y1 = -1;
    }

    // 現在位置をフィールドに追加（軌跡が蓄積される）
    field_update(field, player, enemy);

    Direction next = get_best_direction(player, field);
    cout << direction_to_string(next) << endl;
  }
}

/*
 * フィールド更新
 * 各ターンで現在位置(x1, y1)のみをマーク
 * フィールドは初期化されないので、軌跡が自然と蓄積される
 */
void field_update(int field[30][20], Tronbike &player, Tronbike enemy[3]) {
  // プレイヤーの現在位置をマーク
  if (player.x1 >= 0 && player.x1 < 30 && player.y1 >= 0 && player.y1 < 20) {
    field[player.x1][player.y1] = 1;
  }

  // 敵機の現在位置をマーク
  for (int i = 0; i < 3; i++) {
    if (enemy[i].x1 >= 0 && enemy[i].x1 < 30 && enemy[i].y1 >= 0 &&
        enemy[i].y1 < 20) {
      field[enemy[i].x1][enemy[i].y1] = i + 2;
    }
  }
}

/*
 * 方向を文字列に変換
 */
string direction_to_string(Direction dir) {
  if (dir == UP)
    return "UP";
  if (dir == DOWN)
    return "DOWN";
  if (dir == LEFT)
    return "LEFT";
  if (dir == RIGHT)
    return "RIGHT";
  return "LEFT";
}

/*
 * 到達可能マス数を計算（BFS）
 */
int count_reachable_cells(int start_x, int start_y, int field[30][20]) {
  if (start_x < 0 || start_x >= 30 || start_y < 0 || start_y >= 20)
    return 0;
  if (field[start_x][start_y] != 0)
    return 0;

  bool visited[30][20];
  for (int x = 0; x < 30; x++) {
    for (int y = 0; y < 20; y++) {
      visited[x][y] = false;
    }
  }

  int queue_x[600], queue_y[600];
  int front = 0, rear = 0;
  int dx[] = {0, 0, -1, 1};
  int dy[] = {-1, 1, 0, 0};

  queue_x[rear] = start_x;
  queue_y[rear] = start_y;
  rear++;
  visited[start_x][start_y] = true;

  while (front < rear) {
    int current_idx = front;
    front++;
    int cx = queue_x[current_idx];
    int cy = queue_y[current_idx];

    for (int i = 0; i < 4; i++) {
      int nx = cx + dx[i];
      int ny = cy + dy[i];
      if (nx < 0 || nx >= 30 || ny < 0 || ny >= 20)
        continue;
      if (visited[nx][ny])
        continue;
      if (field[nx][ny] != 0)
        continue;

      visited[nx][ny] = true;
      queue_x[rear] = nx;
      queue_y[rear] = ny;
      rear++;
    }
  }

  return rear;
}

/*
 * 最良の方向を選択（単純BFS）
 * 4方向それぞれの到達可能マス数を計算し、最大の方向を返す
 */
Direction get_best_direction(Tronbike player, int field[30][20]) {
  int dx[] = {0, 0, -1, 1};
  int dy[] = {-1, 1, 0, 0};
  Direction directions[] = {UP, DOWN, LEFT, RIGHT};

  Direction best_dir = LEFT;
  int max_reachable = -1;

  for (int i = 0; i < 4; i++) {
    int nx = player.x1 + dx[i];
    int ny = player.y1 + dy[i];

    if (nx < 0 || nx >= 30 || ny < 0 || ny >= 20)
      continue;
    if (field[nx][ny] != 0)
      continue;

    // 仮想フィールド作成：現在位置をマーク
    int virtual_field[30][20];
    for (int x = 0; x < 30; x++) {
      for (int y = 0; y < 20; y++) {
        virtual_field[x][y] = field[x][y];
      }
    }
    virtual_field[player.x1][player.y1] = 1;

    // 移動先から到達可能なマス数を計算
    int reachable = count_reachable_cells(nx, ny, virtual_field);

    if (reachable > max_reachable) {
      max_reachable = reachable;
      best_dir = directions[i];
    }
  }

  return best_dir;
}