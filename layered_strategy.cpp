#include <iostream>
#include <string>
#include <vector>

using namespace std;

/**
 * Auto-generated code below aims at helping you parse
 * the standard input according to the problem statement.
 **/

// ===== Phase 1: 基本データ構造 =====
typedef enum Direction { NOTHING, UP, DOWN, LEFT, RIGHT } Direction;

struct Tronbike {
  int x0, y0; // 初期位置
  int x1, y1; // 現在位置
};

// 戦略状態管理
struct StrategyState {
  bool layer2_active;
  bool layer3_active;
  vector<Direction> layer2_path;
  vector<Direction> layer3_path;
  int layer2_target_x, layer2_target_y;

  StrategyState()
      : layer2_active(false), layer3_active(false), layer2_target_x(-1),
        layer2_target_y(-1) {}
};

// 隣接空ノード数管理
struct AdjacentEmptyCount {
  int count[30][20];

  void initialize(int field[30][20]);
  void update(int x, int y, int field[30][20]);
  bool is_narrow_path(int x, int y);
};

// ===== ユーティリティ関数 =====
string direction_to_string(Direction dir);
bool is_player_outside(Tronbike player, Tronbike enemy);
int count_reachable_cells(int start_x, int start_y, int field[30][20]);

// ===== 3層戦略関数 =====
Direction layer1_close_combat(Tronbike player, Tronbike enemy[3],
                              int field[30][20]);
Direction layer2_territory_control(Tronbike player, Tronbike enemy[3],
                                   int field[30][20], StrategyState &state,
                                   AdjacentEmptyCount &adj_count);
Direction layer3_max_depth(Tronbike player, int field[30][20],
                           StrategyState &state);
Direction get_direction(Tronbike player, Tronbike enemy[3], int field[30][20],
                        StrategyState &state, AdjacentEmptyCount &adj_count);
void field_update(int field[30][20], Tronbike &player, Tronbike enemy[3]);

int main() {
  // フィールド初期化
  int field[30][20];
  for (int x = 0; x < 30; x++) {
    for (int y = 0; y < 20; y++) {
      field[x][y] = 0;
    }
  }

  // 戦略状態初期化
  StrategyState state;
  AdjacentEmptyCount adj_count;
  Tronbike player;
  Tronbike enemy[3];

  // game loop
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

    // 無効な敵機をマーク
    for (int i = enemy_idx; i < 3; i++) {
      enemy[i].x0 = enemy[i].y0 = enemy[i].x1 = enemy[i].y1 = -1;
    }

    // フィールド更新
    field_update(field, player, enemy);

    // 隣接空ノード数を初期化
    adj_count.initialize(field);

    // 戦略選択
    Direction next = get_direction(player, enemy, field, state, adj_count);

    cout << direction_to_string(next) << endl;
  }
}
// ===== 関数実装 =====

/*
 * Direction enumを文字列に変換
 * 引数: dir - 変換する方向
 * 戻り値: "UP", "DOWN", "LEFT", "RIGHT"のいずれか
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

void AdjacentEmptyCount::initialize(int field[30][20]) {
  int dx[] = {0, 0, -1, 1};
  int dy[] = {-1, 1, 0, 0};
  for (int x = 0; x < 30; x++) {
    for (int y = 0; y < 20; y++) {
      count[x][y] = 0;
      if (field[x][y] != 0)
        continue;
      for (int i = 0; i < 4; i++) {
        int nx = x + dx[i];
        int ny = y + dy[i];
        if (nx >= 0 && nx < 30 && ny >= 0 && ny < 20 && field[nx][ny] == 0) {
          count[x][y]++;
        }
      }
    }
  }
}

/*
 * 隣接空ノード数を更新
 * 引数: x, y - 更新されたノードの座標
 *       field - フィールド状態
 * 機能: 簡易実装として全体を再計算
 */
void AdjacentEmptyCount::update(int x, int y, int field[30][20]) {
  initialize(field);
}

/*
 * 指定されたノードが隘路かどうかを判定
 * 引数: x, y - 判定するノードの座標
 * 戻り値: 隣接空ノード数が2の場合true（隘路）
 */
bool AdjacentEmptyCount::is_narrow_path(int x, int y) {
  return count[x][y] == 2;
}

/*
 * 自分が敵より外側にいるかを判定
 * 引数: player - 自分の情報
 *       enemy - 敵機の情報
 * 戻り値: 自分がフィールド中心から遠い場合true
 * 機能: 中心(14.5, 9.5)からの距離を比較
 */
bool is_player_outside(Tronbike player, Tronbike enemy) {
  float cx = 14.5, cy = 9.5;
  float pd =
      (player.x1 - cx) * (player.x1 - cx) + (player.y1 - cy) * (player.y1 - cy);
  float ed =
      (enemy.x1 - cx) * (enemy.x1 - cx) + (enemy.y1 - cy) * (enemy.y1 - cy);
  return pd > ed;
}

/*
 * 指定位置から到達可能なマス数を計算
 * 引数: start_x, start_y - 開始位置
 *       field - フィールド状態
 * 戻り値: BFSで到達可能なマスの総数
 */
int count_reachable_cells(int start_x, int start_y, int field[30][20]) {
  if (start_x < 0 || start_x >= 30 || start_y < 0 || start_y >= 20)
    return 0;
  if (field[start_x][start_y] != 0)
    return 0;

  bool visited[30][20] = {{false}};
  int queue_x[600], queue_y[600];
  int front = 0, rear = 0;
  int dx[] = {0, 0, -1, 1};
  int dy[] = {-1, 1, 0, 0};

  queue_x[rear] = start_x;
  queue_y[rear] = start_y;
  rear++;
  visited[start_x][start_y] = true;

  while (front < rear) {
    // 重要: front++する前にインデックスを保存
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
 * 第一層：近距離戦
 * 引数: player - 自機
 *       enemy - 敵機配列
 *       field - フィールド状態
 * 戻り値: 選択された移動方向（該当なしの場合NOTHING）
 * 機能: マンハッタン距離3以内の敵機（仮想敵）に対して、
 *       自分が外側なら距離を離し、内側なら接近して敵を端に追いやる
 */
Direction layer1_close_combat(Tronbike player, Tronbike enemy[3],
                              int field[30][20]) {
  const int CLOSE_DISTANCE = 3;
  int dx[] = {0, 0, -1, 1};
  int dy[] = {-1, 1, 0, 0};
  Direction directions[] = {UP, DOWN, LEFT, RIGHT};

  // 仮想敵を特定（距離3以内の最も近い敵）
  int virtual_enemy_id = -1;
  int min_dist = 9999;

  for (int e = 0; e < 3; e++) {
    if (enemy[e].x1 < 0 || enemy[e].x1 >= 30 || enemy[e].y1 < 0 ||
        enemy[e].y1 >= 20) {
      continue;
    }

    int dist = abs(player.x1 - enemy[e].x1) + abs(player.y1 - enemy[e].y1);
    if (dist <= CLOSE_DISTANCE && dist < min_dist) {
      min_dist = dist;
      virtual_enemy_id = e;
    }
  }

  // 仮想敵がいない場合
  if (virtual_enemy_id == -1) {
    return NOTHING;
  }

  Tronbike virtual_enemy = enemy[virtual_enemy_id];

  // 自分が外側か内側かを判定
  // bool i_am_outside = is_player_outside(player, virtual_enemy);

  Direction best_dir = NOTHING;
  int best_score = -9999;

  for (int i = 0; i < 4; i++) {
    int nx = player.x1 + dx[i];
    int ny = player.y1 + dy[i];

    if (nx < 0 || nx >= 30 || ny < 0 || ny >= 20)
      continue;
    if (field[nx][ny] != 0)
      continue;

    // 移動後の敵との距離
    int new_dist = abs(nx - virtual_enemy.x1) + abs(ny - virtual_enemy.y1);

    // スコア計算
    int score = -new_dist;
    /*
    int score;
    if (i_am_outside) {
      // 外側なら距離を離す
      score = new_dist;
    } else {
      // 内側なら接近
      score = -new_dist;
    }
    */

    // 安全性チェック：袋小路にならないか
    int virtual_field[30][20];
    for (int x = 0; x < 30; x++) {
      for (int y = 0; y < 20; y++) {
        virtual_field[x][y] = field[x][y];
      }
    }
    virtual_field[nx][ny] = 1;
    int reachable = count_reachable_cells(nx, ny, virtual_field);

    if (reachable < 10)
      continue; // 袋小路は避ける

    if (score > best_score) {
      best_score = score;
      best_dir = directions[i];
    }
  }

  return best_dir;
}

/*
 * 第二層：能動的陣取り戦略（未実装）
 * 機能: 敵の最大深度経路上の隘路を封鎖
 */
Direction layer2_territory_control(Tronbike player, Tronbike enemy[3],
                                   int field[30][20], StrategyState &state,
                                   AdjacentEmptyCount &adj_count) {
  return NOTHING; // Phase 3で実装予定
}

/*
 * 第三層：最大深度経路
 * 機能: BFSで到達可能マス数が最大の方向を選択
 * 注意: フラグなし、常に再計算
 */
Direction layer3_max_depth(Tronbike player, int field[30][20],
                           StrategyState &state) {
  int dx[] = {0, 0, -1, 1};
  int dy[] = {-1, 1, 0, 0};
  Direction directions[] = {UP, DOWN, LEFT, RIGHT};

  Direction best_dir = LEFT;
  int max_depth = -1;

  for (int i = 0; i < 4; i++) {
    int nx = player.x1 + dx[i];
    int ny = player.y1 + dy[i];

    if (nx < 0 || nx >= 30 || ny < 0 || ny >= 20)
      continue;
    if (field[nx][ny] != 0)
      continue;

    // 仮想フィールドを作成：現在位置をマーク
    int virtual_field[30][20];
    for (int x = 0; x < 30; x++) {
      for (int y = 0; y < 20; y++) {
        virtual_field[x][y] = field[x][y];
      }
    }
    // 現在位置を占有済みとしてマーク
    virtual_field[player.x1][player.y1] = 1;

    // 移動先(nx, ny)から到達可能なマス数を計算
    int depth = count_reachable_cells(nx, ny, virtual_field);

    if (depth > max_depth) {
      max_depth = depth;
      best_dir = directions[i];
    }
  }

  return best_dir;
}

/*
 * 統合：3層戦略の優先度制御
 * 機能: 第一層→第二層→第三層の順で評価し、最適な方向を返す
 */
Direction get_direction(Tronbike player, Tronbike enemy[3], int field[30][20],
                        StrategyState &state, AdjacentEmptyCount &adj_count) {
  Direction dir;

  // 第一層：近距離戦
  dir = layer1_close_combat(player, enemy, field);
  if (dir != NOTHING)
    return dir;

  // 第二層：能動的陣取り
  dir = layer2_territory_control(player, enemy, field, state, adj_count);
  if (dir != NOTHING)
    return dir;

  // 第三層：最大深度経路
  return layer3_max_depth(player, field, state);
}

/*
 * フィールド更新
 * 機能: プレイヤーと敵機の現在位置をフィールドに記録
 */
void field_update(int field[30][20], Tronbike &player, Tronbike enemy[3]) {
  if (player.x1 >= 0 && player.x1 < 30 && player.y1 >= 0 && player.y1 < 20) {
    field[player.x1][player.y1] = 1;
  }
  for (int i = 0; i < 3; i++) {
    if (enemy[i].x1 >= 0 && enemy[i].x1 < 30 && enemy[i].y1 >= 0 &&
        enemy[i].y1 < 20) {
      field[enemy[i].x1][enemy[i].y1] = i + 2;
    }
  }
}
