#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

/**
 * Auto-generated code below aims at helping you parse
 * the standard input according to the problem statement.
 **/
typedef enum Direction { NOTHING, UP, DOWN, LEFT, RIGHT } Direction;
typedef enum Front { BACK, FRONT, WEST, EAST } Front;

typedef struct Tronbike {
  int x0;
  int y0;
  int x1;
  int y1;
  Direction direction;
} Tronbike;

Direction get_safety_path(Tronbike player, Tronbike enemy[3],
                          int field[30][20]);
Direction get_clockwise_path(Tronbike player, Tronbike enemy[3],
                             int field[30][20]);
Direction get_direction(Tronbike player, Tronbike enemy[3], int field[30][20]);
Direction get_direction_from_front(Direction current_dir, Front front);
string direction_to_char(Direction direction);
Front get_front(Tronbike bike, Direction direction);
void field_update(int field[30][20], Tronbike *player, Tronbike *enemy);
void tron_init(int field[30][20], Tronbike *player, Tronbike *enemy);
int count_reachable_cells(int start_x, int start_y, int field[30][20]);
void calculate_enemy_reach_time(Tronbike enemy[3], int field[30][20],
                                int enemy_reach[30][20]);
int calculate_attack_score(int my_x, int my_y, Tronbike enemy[3],
                           int field[30][20]);
bool is_separated_from_enemies(Tronbike player, Tronbike enemy[3],
                               int field[30][20]);
Direction get_fill_path(Tronbike player, int field[30][20]);

int main() {
  // player
  Tronbike player;
  Tronbike enemy[3];
  int field[30][20];

  // fieldを初期化（全て0=移動可能）
  for (int x = 0; x < 30; x++) {
    for (int y = 0; y < 20; y++) {
      field[x][y] = 0;
    }
  }

  // game loop
  while (1) {
    int n; // total number of players (2 to 4).
    int p; // your player number (0 to 3).
    cin >> n >> p;
    cin.ignore();

    // 入力データを読み取り、playerとenemyに格納
    int enemy_idx = 0;
    for (int i = 0; i < n; i++) {
      int x0; // starting X coordinate of lightcycle (or -1)
      int y0; // starting Y coordinate of lightcycle (or -1)
      int x1; // current X coordinate of lightcycle
      int y1; // current Y coordinate of lightcycle

      cin >> x0 >> y0 >> x1 >> y1;
      cin.ignore();

      // プレイヤー番号iが自分の番号pと一致すればplayer、そうでなければenemy
      if (i == p) {
        player.x0 = x0;
        player.y0 = y0;
        player.x1 = x1;
        player.y1 = y1;
      } else if (x0 != -1) { // 負けていない敵機のみ記録
        enemy[enemy_idx].x0 = x0;
        enemy[enemy_idx].y0 = y0;
        enemy[enemy_idx].x1 = x1;
        enemy[enemy_idx].y1 = y1;
        enemy_idx++;
      }
    }

    // 全データ読み取り後にfieldを更新
    field_update(field, &player, enemy);
    /*
    戦略メモ
    敵機と自分の位置を保存しておくDBを作っておかなくてはいけない。
    というよりも、フィールドの状態を記録しておく２次元テーブル作っておけばいいだけ。
    その後、じゃあ次はどのマスに向かっていけばいいかっていう探索プログラムを書くんだ

    オブジェクトメモ
    ・移動済みフィールドを記録するテーブル。field
    ・プレイヤーおよび敵機の現在地を記録するテーブル player, enemy
    ・移動済みフィールドおよび画面端を避けて方向を返す処理 get_direction
    */

    // Write an action using cout. DON'T FORGET THE "<< endl"
    // To debug: cerr << "Debug messages..." << endl;

    Direction next = get_direction(player, enemy, field);
    cout << direction_to_char(next)
         << endl; // A single line with UP, DOWN, LEFT or RIGHT
  }
}

void tron_init(int field[30][20], Tronbike *player, Tronbike *enemy) {
  player->x1 = player->x0;
  player->y1 = player->y0;
  for (int i = 0; i < 3; i++) {
    enemy[i].x1 = enemy[i].x0;
    enemy[i].y1 = enemy[i].y0;
  }
  field_update(field, player, enemy);
}

void field_update(int field[30][20], Tronbike *player, Tronbike *enemy) {
  // プレイヤーの現在位置を記録（1 = プレイヤーの軌跡）
  if (player->x1 >= 0 && player->x1 < 30 && player->y1 >= 0 &&
      player->y1 < 20) {
    field[player->x1][player->y1] = 1;
  }

  // 敵機の現在位置を記録（2, 3, 4 = 敵機の軌跡）
  for (int i = 0; i < 3; i++) {
    // 有効な座標のみ記録（-1でないことを確認）
    if (enemy[i].x1 >= 0 && enemy[i].x1 < 30 && enemy[i].y1 >= 0 &&
        enemy[i].y1 < 20) {
      field[enemy[i].x1][enemy[i].y1] = i + 2;
    }
  }
}

/*
playerが次の移動先を取得するための関数
BFSを行い、深さ５まで移動可能な経路を進路として取得する。なお、深さ５まで探索（4の5乗-探索済みマス）
するが、探索自体は１マス進むごとに繰り返す（再計算を行い常に安全なルートを選択する）
->子関数　get_safety_path
* get_safety_pathは、深さ５まで移動可能な経路がない場合、 NOTHINGを返す
* BFSなのは委員だけど、Frontの優先順位は

安全なルート（深さ５まで移動可能なルート）がない場合、”最終盤面”フラグを立て、単純なBFSで「時計回りの渦」を
描くような動きをするように経路を設定する。左、前、右の順で探索すれば自ずと時計回りしてくれるはず。
->子関数　get_clockwise_path

引数：各tronbikeの現在地、フィールドの状態
戻り値：進む方向
*/
Direction get_direction(Tronbike player, Tronbike enemy[3], int field[30][20]) {
  static bool is_last_round = false;

  // 袋小路検出：敵機と完全に分離されている場合
  if (is_separated_from_enemies(player, enemy, field)) {
    return get_fill_path(player, field); // 一筆書き戦略
  }

  // 通常の総合評価（防御・攻撃・危険度のバランス）
  Direction direction;
  if (is_last_round)
    return get_clockwise_path(player, enemy, field);
  direction = get_safety_path(player, enemy, field);
  if (direction == NOTHING) {
    is_last_round = true;
    return get_clockwise_path(player, enemy, field);
  }
  return direction;
}

/*
BFSで深さ6まで移動可能な経路を進路として取得する。
複数の方向が選択可能な場合、到達可能マス数が最大の方向を優先する。
さらに、敵機との経路競合を考慮して危険度の低い方向を選択する。
*/
Direction get_safety_path(Tronbike player, Tronbike enemy[3],
                          int field[30][20]) {
  // 敵機の各マスへの最短到達時間を計算
  int enemy_reach[30][20];
  calculate_enemy_reach_time(enemy, field, enemy_reach);

  // 最外周マスを除外した仮想フィールドを作成
  int virtual_field[30][20];
  for (int x = 0; x < 30; x++) {
    for (int y = 0; y < 20; y++) {
      virtual_field[x][y] = field[x][y];
      // 最外周マスを壁としてマーク
      if (x == 0 || x == 29 || y == 0 || y == 19) {
        if (field[x][y] == 0) {
          virtual_field[x][y] = -1; // 一時的に移動不可能としてマーク
        }
      }
    }
  }

  // 各方向の評価情報を格納する構造体
  struct DirectionScore {
    Direction dir;
    bool movable;         // その方向に移動可能か
    bool safe_to_depth_6; // 深さ6まで到達可能か
    int reachable_cells;  // 到達可能なマス数
    float danger_score;   // 危険度スコア（0.0〜1.0、高いほど危険）
    int attack_score;     // 攻撃スコア：敵削減マス数
    bool is_edge_cell;    // 最外周マスか
  };

  // 4方向の移動（UP, DOWN, LEFT, RIGHT）
  int dx[] = {0, 0, -1, 1};
  int dy[] = {-1, 1, 0, 0};
  Direction directions[] = {UP, DOWN, LEFT, RIGHT};

  // 各方向のスコアを初期化
  DirectionScore scores[4];
  for (int i = 0; i < 4; i++) {
    scores[i].dir = directions[i];
    scores[i].movable = false;
    scores[i].safe_to_depth_6 = false;
    scores[i].reachable_cells = 0;
    scores[i].danger_score = 0.0;
    scores[i].attack_score = 0;
    scores[i].is_edge_cell = false;
  }

  // 各方向を評価
  for (int i = 0; i < 4; i++) {
    int nx = player.x1 + dx[i];
    int ny = player.y1 + dy[i];

    // 範囲チェック
    if (nx < 0 || nx >= 30 || ny < 0 || ny >= 20) {
      continue;
    }

    // 移動可能かチェック（仮想フィールドを使用）
    if (virtual_field[nx][ny] != 0) {
      continue;
    }

    scores[i].movable = true;

    // 最外周マスかチェック
    if (nx == 0 || nx == 29 || ny == 0 || ny == 19) {
      scores[i].is_edge_cell = true;
    }

    // この方向から深さ6まで到達可能かチェック
    // BFS用のノード構造体
    struct Node {
      int x, y;
      int depth;
    };

    vector<Node> queue;
    bool visited[30][20] = {false};

    Node start = {nx, ny, 1};
    queue.push_back(start);
    visited[nx][ny] = true;
    visited[player.x1][player.y1] = true; // 現在地も訪問済みに

    bool reached_depth_6 = false;
    int queue_idx = 0;
    int conflict_count = 0; // 敵機と競合するノード数

    while (queue_idx < queue.size()) {
      Node current = queue[queue_idx++];

      if (current.depth >= 6) {
        reached_depth_6 = true;
        // 深さ6到達後も探索を続けて、到達可能マス数をカウント
      }

      // 敵機との競合をチェック
      // 自分がこのノードに到達する手数: current.depth
      // 敵機がこのノードに到達する手数: enemy_reach[current.x][current.y]
      if (enemy_reach[current.x][current.y] != 999) {
        // 敵機の方が早く到達、または同じタイミングで到達する場合
        if (enemy_reach[current.x][current.y] <= current.depth) {
          conflict_count++;
        }
      }

      // 4方向を探索
      for (int j = 0; j < 4; j++) {
        int nnx = current.x + dx[j];
        int nny = current.y + dy[j];

        // 範囲チェック
        if (nnx < 0 || nnx >= 30 || nny < 0 || nny >= 20) {
          continue;
        }

        // 移動可能かチェック（仮想フィールドを使用）
        if (virtual_field[nnx][nny] != 0) {
          continue;
        }

        // 訪問済みチェック
        if (visited[nnx][nny]) {
          continue;
        }

        Node next = {nnx, nny, current.depth + 1};
        queue.push_back(next);
        visited[nnx][nny] = true;
      }
    }

    scores[i].safe_to_depth_6 = reached_depth_6;
    scores[i].reachable_cells =
        queue.size(); // 訪問したノード数 = 到達可能マス数

    // 危険度スコアを計算（0.0〜1.0）
    // 到達可能なノードのうち、敵機と競合するノードの割合
    if (queue.size() > 0) {
      scores[i].danger_score = (float)conflict_count / (float)queue.size();
    } else {
      scores[i].danger_score = 1.0; // 移動不可能な方向は最大危険度
    }

    // 攻撃スコアを計算：この方向に進んだ場合の敵機の削減マス数
    scores[i].attack_score = calculate_attack_score(nx, ny, enemy, field);
  }

  // 最適な方向を選択
  // 総合スコア = (到達可能マス × 防御重み) + (攻撃スコア × 攻撃重み) - (危険度
  // × 危険重み)
  Direction best_direction = NOTHING;
  float best_score = -9999.0;
  bool found_safe = false;

  const float DEFENSE_WEIGHT = 1.0; // 自分の領域重視
  const float ATTACK_WEIGHT = 1.2;  // 攻撃スコア重視
  const float DANGER_WEIGHT = 50.0; // 危険度の重み（調整可能）

  for (int i = 0; i < 4; i++) {
    if (!scores[i].movable) {
      continue;
    }

    // 総合スコアを計算
    // 攻撃スコアは条件付きで適用：敵を削った後も敵の方が有利な場合のみ
    float attack_bonus = 0.0;

    // 現在の敵機の到達可能マス数を計算
    int current_enemy_cells = 0;
    for (int e = 0; e < 3; e++) {
      if (enemy[e].x1 >= 0 && enemy[e].x1 < 30 && enemy[e].y1 >= 0 &&
          enemy[e].y1 < 20) {
        current_enemy_cells +=
            count_reachable_cells(enemy[e].x1, enemy[e].y1, field);
      }
    }

    // 攻撃後の敵の領域 = 現在の敵の領域 - 攻撃スコア
    int enemy_cells_after_attack = current_enemy_cells - scores[i].attack_score;

    // 「削減後の敵の領域 > 自分の領域」の場合のみ攻撃スコアを適用
    if (enemy_cells_after_attack > scores[i].reachable_cells) {
      attack_bonus = (float)scores[i].attack_score * ATTACK_WEIGHT;
    }

    float total_score = (float)scores[i].reachable_cells * DEFENSE_WEIGHT +
                        attack_bonus - (scores[i].danger_score * DANGER_WEIGHT);

    // 優先順位1: 深さ6に到達可能な方向
    if (scores[i].safe_to_depth_6) {
      if (!found_safe || total_score > best_score) {
        best_direction = scores[i].dir;
        best_score = total_score;
        found_safe = true;
      }
    }
    // 優先順位2: 深さ6に到達できないが、総合スコアが最も高い方向
    else if (!found_safe) {
      if (total_score > best_score) {
        best_direction = scores[i].dir;
        best_score = total_score;
      }
    }
  }

  // 全方向が移動不可能な場合、最外周マスも含めて再評価
  if (best_direction == NOTHING) {
    // 元のfieldを使って再評価（最外周マスも含む）
    for (int i = 0; i < 4; i++) {
      int nx = player.x1 + dx[i];
      int ny = player.y1 + dy[i];

      // 範囲チェック
      if (nx < 0 || nx >= 30 || ny < 0 || ny >= 20) {
        continue;
      }

      // 元のfieldで移動可能かチェック
      if (field[nx][ny] != 0) {
        continue;
      }

      // 最外周マスのみを対象（仮想フィールドでは移動不可能だった場所）
      if (scores[i].movable) {
        continue; // 既に評価済みの方向はスキップ
      }

      // 到達可能マス数を計算（簡易版）
      int reachable = count_reachable_cells(nx, ny, field);

      if (reachable > best_score || best_direction == NOTHING) {
        best_direction = directions[i];
        best_score = (float)reachable;
      }
    }
  }

  return best_direction;
}

Direction get_clockwise_path(Tronbike player, Tronbike enemy[3],
                             int field[30][20]) {

  // 方向から座標の変化量を取得
  auto get_delta = [](Direction dir, int &dx, int &dy) {
    dx = 0;
    dy = 0;
    if (dir == UP)
      dy = -1;
    else if (dir == DOWN)
      dy = 1;
    else if (dir == LEFT)
      dx = -1;
    else if (dir == RIGHT)
      dx = 1;
  };

  // 移動可能かチェックする関数
  auto is_movable = [&](Direction dir) -> bool {
    int dx, dy;
    get_delta(dir, dx, dy);
    int nx = player.x1 + dx;
    int ny = player.y1 + dy;

    // 範囲チェック
    if (nx < 0 || nx >= 30 || ny < 0 || ny >= 20) {
      return false;
    }

    // 移動可能かチェック（field[nx][ny] == 0 が移動可能）
    if (field[nx][ny] != 0) {
      return false;
    }

    return true;
  };

  // WEST, FRONT, EAST の順で優先して評価
  Front priority[] = {WEST, FRONT, EAST};

  for (int i = 0; i < 3; i++) {
    Direction dir = get_direction_from_front(player.direction, priority[i]);
    if (dir != NOTHING && is_movable(dir)) {
      return dir;
    }
  }

  // どれも移動できない場合、BACK（後ろ）も試す
  Direction back_dir = get_direction_from_front(player.direction, BACK);
  if (back_dir != NOTHING && is_movable(back_dir)) {
    return back_dir;
  }

  // どこにも移動できない
  return NOTHING;
}

/*
bikeの前回の移動方向を基準にした時、bikeにとってそのDirectionがどちらの方向なのかを返す
引数：各tronbikeの現在地、フィールドの状態
戻り値：進む方向
*/
Front get_front(Tronbike bike, Direction direction) {
  if (bike.direction == UP) {
    if (direction == UP)
      return FRONT;
    if (direction == DOWN)
      return BACK;
    if (direction == LEFT)
      return WEST;
    if (direction == RIGHT)
      return EAST;
  }
  if (bike.direction == DOWN) {
    if (direction == UP)
      return BACK;
    if (direction == DOWN)
      return FRONT;
    if (direction == LEFT)
      return EAST;
    if (direction == RIGHT)
      return WEST;
  }
  if (bike.direction == LEFT) {
    if (direction == UP)
      return WEST;
    if (direction == DOWN)
      return EAST;
    if (direction == LEFT)
      return FRONT;
    if (direction == RIGHT)
      return BACK;
  }
  if (bike.direction == RIGHT) {
    if (direction == UP)
      return EAST;
    if (direction == DOWN)
      return WEST;
    if (direction == LEFT)
      return BACK;
    if (direction == RIGHT)
      return FRONT;
  }
  // デフォルト値（ここには到達しないはず）
  return FRONT;
}

/*
playerの現在の方向を基準に、Front列挙子に対応するDirectionを取得する関数
引数：current_dir - 現在の進行方向, front - 相対的な方向
戻り値：絶対的な方向（Direction）
*/
Direction get_direction_from_front(Direction current_dir, Front front) {
  if (current_dir == UP) {
    if (front == WEST)
      return LEFT;
    if (front == FRONT)
      return UP;
    if (front == EAST)
      return RIGHT;
    if (front == BACK)
      return DOWN;
  } else if (current_dir == DOWN) {
    if (front == WEST)
      return RIGHT;
    if (front == FRONT)
      return DOWN;
    if (front == EAST)
      return LEFT;
    if (front == BACK)
      return UP;
  } else if (current_dir == LEFT) {
    if (front == WEST)
      return DOWN;
    if (front == FRONT)
      return LEFT;
    if (front == EAST)
      return UP;
    if (front == BACK)
      return RIGHT;
  } else if (current_dir == RIGHT) {
    if (front == WEST)
      return UP;
    if (front == FRONT)
      return RIGHT;
    if (front == EAST)
      return DOWN;
    if (front == BACK)
      return LEFT;
  }
  return NOTHING;
}

string direction_to_char(Direction direction) {
  switch (direction) {
  case UP:
    return "UP";
  case DOWN:
    return "DOWN";
  case LEFT:
    return "LEFT";
  case RIGHT:
    return "RIGHT";
  default:
    return "NOTHING";
  }
}

/*
指定された開始位置から到達可能なマスの総数を計算する
引数：start_x, start_y - 開始位置, field - フィールド状態
戻り値：到達可能なマスの数
*/
int count_reachable_cells(int start_x, int start_y, int field[30][20]) {
  // 範囲チェック
  if (start_x < 0 || start_x >= 30 || start_y < 0 || start_y >= 20) {
    return 0;
  }

  // 開始位置が移動不可の場合
  if (field[start_x][start_y] != 0) {
    return 0;
  }

  struct Node {
    int x, y;
  };

  vector<Node> queue;
  bool visited[30][20] = {false};

  // 4方向の移動
  int dx[] = {0, 0, -1, 1};
  int dy[] = {-1, 1, 0, 0};

  Node start = {start_x, start_y};
  queue.push_back(start);
  visited[start_x][start_y] = true;

  int queue_idx = 0;
  while (queue_idx < queue.size()) {
    Node current = queue[queue_idx++];

    // 4方向を探索
    for (int i = 0; i < 4; i++) {
      int nx = current.x + dx[i];
      int ny = current.y + dy[i];

      // 範囲チェック
      if (nx < 0 || nx >= 30 || ny < 0 || ny >= 20) {
        continue;
      }

      // 移動可能かチェック
      if (field[nx][ny] != 0) {
        continue;
      }

      // 訪問済みチェック
      if (visited[nx][ny]) {
        continue;
      }

      Node next = {nx, ny};
      queue.push_back(next);
      visited[nx][ny] = true;
    }
  }

  return queue.size();
}

/*
敵機の各マスへの最短到達時間を計算する
引数：enemy - 敵機配列, field - フィールド状態, enemy_reach - 出力用配列
enemy_reach[x][y] = その座標に敵機が到達する最短手数（到達不可能なら999）
*/
void calculate_enemy_reach_time(Tronbike enemy[3], int field[30][20],
                                int enemy_reach[30][20]) {
  // 初期化：全てのマスを到達不可能（999）に設定
  for (int x = 0; x < 30; x++) {
    for (int y = 0; y < 20; y++) {
      enemy_reach[x][y] = 999;
    }
  }

  // 各敵機についてBFSを実行
  for (int e = 0; e < 3; e++) {
    // 無効な敵機（負けている）はスキップ
    if (enemy[e].x1 < 0 || enemy[e].x1 >= 30 || enemy[e].y1 < 0 ||
        enemy[e].y1 >= 20) {
      continue;
    }

    struct Node {
      int x, y;
      int steps; // この敵機からの手数
    };

    vector<Node> queue;
    bool visited[30][20] = {false};

    // 4方向の移動
    int dx[] = {0, 0, -1, 1};
    int dy[] = {-1, 1, 0, 0};

    // 敵機の現在位置から開始
    Node start = {enemy[e].x1, enemy[e].y1, 0};
    queue.push_back(start);
    visited[enemy[e].x1][enemy[e].y1] = true;

    // この敵機の現在位置は0手で到達
    if (enemy_reach[enemy[e].x1][enemy[e].y1] > 0) {
      enemy_reach[enemy[e].x1][enemy[e].y1] = 0;
    }

    int queue_idx = 0;
    while (queue_idx < queue.size()) {
      Node current = queue[queue_idx++];

      // 4方向を探索
      for (int i = 0; i < 4; i++) {
        int nx = current.x + dx[i];
        int ny = current.y + dy[i];

        // 範囲チェック
        if (nx < 0 || nx >= 30 || ny < 0 || ny >= 20) {
          continue;
        }

        // 移動可能かチェック（現在のfieldで移動不可能な場所はスキップ）
        if (field[nx][ny] != 0) {
          continue;
        }

        // 訪問済みチェック
        if (visited[nx][ny]) {
          continue;
        }

        Node next = {nx, ny, current.steps + 1};
        queue.push_back(next);
        visited[nx][ny] = true;

        // より早い到達時間で更新
        if (enemy_reach[nx][ny] > next.steps) {
          enemy_reach[nx][ny] = next.steps;
        }
      }
    }
  }
}

/*
攻撃スコアを計算：指定位置に移動した場合、敵機の到達可能マス数がどれだけ減るか
引数：my_x, my_y - 移動先座標, enemy - 敵機配列, field - フィールド状態
戻り値：削減されるマス数（正の値ほど攻撃的に有利）
*/
int calculate_attack_score(int my_x, int my_y, Tronbike enemy[3],
                           int field[30][20]) {
  // 現在の敵機の到達可能マス数を計算
  int current_enemy_cells = 0;
  for (int e = 0; e < 3; e++) {
    if (enemy[e].x1 >= 0 && enemy[e].x1 < 30 && enemy[e].y1 >= 0 &&
        enemy[e].y1 < 20) {
      current_enemy_cells +=
          count_reachable_cells(enemy[e].x1, enemy[e].y1, field);
    }
  }

  // 仮想フィールドを作成（指定位置に移動した状態）
  int virtual_field[30][20];
  for (int x = 0; x < 30; x++) {
    for (int y = 0; y < 20; y++) {
      virtual_field[x][y] = field[x][y];
    }
  }
  // 移動先を占有済みにマーク
  if (my_x >= 0 && my_x < 30 && my_y >= 0 && my_y < 20) {
    virtual_field[my_x][my_y] = 1;
  }

  // 移動後の敵機の到達可能マス数を計算
  int future_enemy_cells = 0;
  for (int e = 0; e < 3; e++) {
    if (enemy[e].x1 >= 0 && enemy[e].x1 < 30 && enemy[e].y1 >= 0 &&
        enemy[e].y1 < 20) {
      future_enemy_cells +=
          count_reachable_cells(enemy[e].x1, enemy[e].y1, virtual_field);
    }
  }

  // 削減量を返す（正の値 = 敵の選択肢を減らせた）
  return current_enemy_cells - future_enemy_cells;
}

/*
自分と敵機が完全に分離されているかチェック
引数：player - 自機, enemy - 敵機配列, field - フィールド状態
戻り値：分離されていればtrue
*/
bool is_separated_from_enemies(Tronbike player, Tronbike enemy[3],
                               int field[30][20]) {
  // 自分の到達可能エリアをBFSで探索
  struct Node {
    int x, y;
  };

  vector<Node> queue;
  bool visited[30][20] = {false};

  int dx[] = {0, 0, -1, 1};
  int dy[] = {-1, 1, 0, 0};

  Node start = {player.x1, player.y1};
  queue.push_back(start);
  visited[player.x1][player.y1] = true;

  int queue_idx = 0;
  while (queue_idx < queue.size()) {
    Node current = queue[queue_idx++];

    // このマスに敵機がいるかチェック
    for (int e = 0; e < 3; e++) {
      if (enemy[e].x1 == current.x && enemy[e].y1 == current.y) {
        // 敵機と同じエリアにいる = 分離されていない
        return false;
      }
    }

    // 4方向を探索
    for (int i = 0; i < 4; i++) {
      int nx = current.x + dx[i];
      int ny = current.y + dy[i];

      if (nx < 0 || nx >= 30 || ny < 0 || ny >= 20)
        continue;
      if (field[nx][ny] != 0)
        continue;
      if (visited[nx][ny])
        continue;

      Node next = {nx, ny};
      queue.push_back(next);
      visited[nx][ny] = true;
    }
  }

  // 自分の到達可能エリアに敵機がいない = 分離されている
  return true;
}

/*
袋小路での一筆書き戦略：敵を気にせず効率的に全マスを埋める
引数：player - 自機, field - フィールド状態
戻り値：最適な移動方向
*/
Direction get_fill_path(Tronbike player, int field[30][20]) {
  // シンプルなBFSで最も深い経路を選択
  struct Node {
    int x, y;
    int depth;
    Direction first_direction;
  };

  int dx[] = {0, 0, -1, 1};
  int dy[] = {-1, 1, 0, 0};
  Direction directions[] = {UP, DOWN, LEFT, RIGHT};

  vector<Node> queue;
  bool visited[30][20] = {false};

  Node start = {player.x1, player.y1, 0, NOTHING};
  queue.push_back(start);
  visited[player.x1][player.y1] = true;

  Direction best_direction = NOTHING;
  int max_depth = 0;

  int queue_idx = 0;
  while (queue_idx < queue.size()) {
    Node current = queue[queue_idx++];

    for (int i = 0; i < 4; i++) {
      int nx = current.x + dx[i];
      int ny = current.y + dy[i];

      if (nx < 0 || nx >= 30 || ny < 0 || ny >= 20)
        continue;
      if (field[nx][ny] != 0)
        continue;
      if (visited[nx][ny])
        continue;

      Direction first_dir =
          (current.depth == 0) ? directions[i] : current.first_direction;

      Node next = {nx, ny, current.depth + 1, first_dir};
      queue.push_back(next);
      visited[nx][ny] = true;

      if (next.depth > max_depth) {
        max_depth = next.depth;
        best_direction = first_dir;
      }
    }
  }

  return best_direction;
}
