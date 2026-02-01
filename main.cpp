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

// 戦略レベル
typedef enum StrategyLevel {
  STRATEGY_ATTACK,  // 攻撃：敵妨害
  STRATEGY_DEFENSE, // 防御：自分の経路確保
  STRATEGY_SURVIVE  // 生存：最大深度確保
} StrategyLevel;

// 小戦略
typedef enum SubStrategy {
  SUB_BLOCK_ENEMY_MAX,    // 敵の最大深度妨害
  SUB_PROTECT_MY_MAX,     // 自分の最大深度防御
  SUB_MAX_DEPTH_NO_EDGE,  // 最外周除外の最大深度
  SUB_MAX_DEPTH_WITH_EDGE // 全マス考慮の最大深度
} SubStrategy;

typedef struct Tronbike {
  int x0;
  int y0;
  int x1;
  int y1;
  Direction direction;
} Tronbike;

// マンハッタン距離フィールド
struct ManhattanField {
  int player_dist[30][20];   // 自分からの距離
  int enemy_dist[3][30][20]; // 各敵機からの距離

  void initialize(Tronbike player, Tronbike enemy[3], int field[30][20]);
  void update_distances();
  int get_max_depth_pos(int player_id, int &out_x, int &out_y,
                        int field[30][20]);
};

// 敵機の脅威度評価
struct ThreatScore {
  int enemy_id;
  int max_depth;      // 最大深度
  int distance_to_me; // 自分からの距離
  int area_overlap;   // 自分との領域重複度
  bool is_behind;     // 後方にいるか
  float total_threat; // 総合脅威度
};

// 暫定所有権情報
struct TerritoryInfo {
  int player_provisional[30][20]; // 各ノードの暫定所有者ID (-1=player,
                                  // 0-2=enemy, 99=neutral)
  int player_distance[30][20];    // 自分の初期位置からの距離
  int enemy_distance[3][30][20];  // 各敵の初期位置からの距離
  int player_count;               // 自分の暫定所有ノード数
  int enemy_count;                // 敵の暫定所有ノード数（合計）
};

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
Direction block_enemy_max_depth(Tronbike player, Tronbike enemy[3],
                                int field[30][20], ManhattanField &mfield);
void calculate_threat_scores(Tronbike player, Tronbike enemy[3],
                             int field[30][20], ManhattanField &mfield,
                             ThreatScore threats[3]);
Direction avoid_pincer_attack(Tronbike player, Tronbike enemy[3],
                              int field[30][20], ManhattanField &mfield);
void calculate_provisional_territory(Tronbike player, Tronbike enemy[3],
                                     int field[30][20],
                                     TerritoryInfo &territory);
Direction select_territory_move(Tronbike player, Tronbike enemy[3],
                                int field[30][20], TerritoryInfo &territory);
Direction block_nearby_enemy(Tronbike player, Tronbike enemy[3],
                             int field[30][20]);
Direction block_enemy_escape(Tronbike player, Tronbike enemy[3],
                             int field[30][20], ManhattanField &mfield);
int calculate_enemy_exits(int enemy_x, int enemy_y, int field[30][20]);
Direction select_optimal_block(Tronbike player, Tronbike enemy[3],
                               int field[30][20]);
Direction escape_from_edge(Tronbike player, Tronbike enemy[3],
                           int field[30][20], TerritoryInfo &territory);
bool validate_direction(Direction dir, Tronbike player, int field[30][20]);
Direction get_max_depth_direction(Tronbike player, int field[30][20]);

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

  // 暫定所有権情報を計算
  TerritoryInfo territory;
  calculate_provisional_territory(player, enemy, field, territory);

  // ManhattanFieldを初期化（出口封鎖で使用）
  ManhattanField mfield;
  mfield.initialize(player, enemy, field);

  // 勝ち確定判定：自分の暫定所有ノード数が敵より大きい場合
  if (territory.player_count > territory.enemy_count) {
    // 勝ち確定状態なら一筆書き戦略
    Direction dir = get_fill_path(player, field);
    if (validate_direction(dir, player, field)) {
      return dir;
    }
  }

  // 外周退避：外周マスにいて敵が近い場合は退避ルートを確保
  Direction escape_dir = escape_from_edge(player, enemy, field, territory);
  if (validate_direction(escape_dir, player, field)) {
    return escape_dir;
  }

  // 動的出口封鎖：敵の出口を段階的に減らす
  Direction dynamic_block_dir = select_optimal_block(player, enemy, field);
  if (validate_direction(dynamic_block_dir, player, field)) {
    return dynamic_block_dir;
  }

  // 敵機出口封鎖：敵の最大深度経路を先回りして封じる
  Direction escape_block_dir = block_enemy_escape(player, enemy, field, mfield);
  if (validate_direction(escape_block_dir, player, field)) {
    return escape_block_dir;
  }

  // 近距離敵機ブロック：敵が近い場合は進路を塞ぐ
  Direction block_dir = block_nearby_enemy(player, enemy, field);
  if (validate_direction(block_dir, player, field)) {
    return block_dir;
  }

  // 暫定所有権戦略：暫定所有ノードを守る/増やす
  Direction territory_dir =
      select_territory_move(player, enemy, field, territory);
  if (validate_direction(territory_dir, player, field)) {
    return territory_dir;
  }

  // 袋小路検出：敵機と完全に分離されている場合
  if (is_separated_from_enemies(player, enemy, field)) {
    Direction dir = get_fill_path(player, field);
    if (validate_direction(dir, player, field)) {
      return dir;
    }
  }

  // 通常の総合評価（フォールバック）
  Direction direction;
  if (is_last_round) {
    direction = get_clockwise_path(player, enemy, field);
    if (validate_direction(direction, player, field)) {
      return direction;
    }
  }

  direction = get_safety_path(player, enemy, field);
  if (validate_direction(direction, player, field)) {
    return direction;
  }

  // 全ての戦略が失敗した場合、最大深度経路を選択（最終フォールバック）
  return get_max_depth_direction(player, field);
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

/*
ManhattanField::initialize
各マスから各プレイヤーへのマンハッタン距離を初期化
*/
void ManhattanField::initialize(Tronbike player, Tronbike enemy[3],
                                int field[30][20]) {
  // 自分からの距離を計算
  for (int x = 0; x < 30; x++) {
    for (int y = 0; y < 20; y++) {
      player_dist[x][y] = abs(x - player.x1) + abs(y - player.y1);
    }
  }

  // 各敵機からの距離を計算
  for (int e = 0; e < 3; e++) {
    for (int x = 0; x < 30; x++) {
      for (int y = 0; y < 20; y++) {
        // 無効な敵機の場合は999（到達不可能）
        if (enemy[e].x1 < 0 || enemy[e].x1 >= 30 || enemy[e].y1 < 0 ||
            enemy[e].y1 >= 20) {
          enemy_dist[e][x][y] = 999;
        } else {
          enemy_dist[e][x][y] = abs(x - enemy[e].x1) + abs(y - enemy[e].y1);
        }
      }
    }
  }
}

/*
ManhattanField::get_max_depth_pos
指定プレイヤーの最大深度の位置を取得
player_id: 0=自分, 1-3=敵機
戻り値: 最大深度
*/
int ManhattanField::get_max_depth_pos(int player_id, int &out_x, int &out_y,
                                      int field[30][20]) {
  int max_dist = -1;
  out_x = -1;
  out_y = -1;

  if (player_id == 0) {
    // 自分の最大深度
    for (int x = 0; x < 30; x++) {
      for (int y = 0; y < 20; y++) {
        if (field[x][y] == 0 && player_dist[x][y] > max_dist) {
          max_dist = player_dist[x][y];
          out_x = x;
          out_y = y;
        }
      }
    }
  } else {
    // 敵機の最大深度
    int e = player_id - 1;
    for (int x = 0; x < 30; x++) {
      for (int y = 0; y < 20; y++) {
        if (field[x][y] == 0 && enemy_dist[e][x][y] > max_dist &&
            enemy_dist[e][x][y] < 999) {
          max_dist = enemy_dist[e][x][y];
          out_x = x;
          out_y = y;
        }
      }
    }
  }

  return max_dist;
}

/*
ManhattanField::update_distances (簡易版)
現在は毎ターン initialize を呼び出すため、この関数は使用しない
*/
void ManhattanField::update_distances() {
  // 実装は省略（毎ターンinitializeで十分）
}

/*
block_enemy_max_depth
敵機の最大深度経路を妨害する攻撃戦略
敵機からマンハッタン距離7のマスを優先的に目指す
*/
Direction block_enemy_max_depth(Tronbike player, Tronbike enemy[3],
                                int field[30][20], ManhattanField &mfield) {
  int dx[] = {0, 0, -1, 1};
  int dy[] = {-1, 1, 0, 0};
  Direction directions[] = {UP, DOWN, LEFT, RIGHT};

  // 全敵機の脅威度を計算
  ThreatScore threats[3];
  calculate_threat_scores(player, enemy, field, mfield, threats);

  // 最も脅威的な敵機を対象にする
  int threat_enemy_id = threats[0].enemy_id;

  // 脅威となる敵機がいない場合（脅威度が0）は何もしない
  if (threats[0].total_threat <= 0.0) {
    return NOTHING;
  }

  // 敵機からマンハッタン距離7のマスを探す
  struct Target {
    int x, y;
    int my_dist;
    int enemy_dist;
  };

  vector<Target> targets;

  for (int x = 0; x < 30; x++) {
    for (int y = 0; y < 20; y++) {
      // 空いているマスで、敵機からの距離が7の場所
      if (field[x][y] == 0 && mfield.enemy_dist[threat_enemy_id][x][y] == 7) {
        Target t;
        t.x = x;
        t.y = y;
        t.my_dist = mfield.player_dist[x][y];
        t.enemy_dist = 7;
        targets.push_back(t);
      }
    }
  }

  // ターゲットがない場合は何もしない
  if (targets.empty()) {
    return NOTHING;
  }

  // 自分が敵より早く到達できるターゲットを優先
  // さらに自分から近い順にソート
  sort(targets.begin(), targets.end(), [](const Target &a, const Target &b) {
    if (a.my_dist != b.my_dist) {
      return a.my_dist < b.my_dist;
    }
    return false;
  });

  // 最優先ターゲットに向かう方向を選択
  Target best_target = targets[0];

  // 自分が敵より遅い場合は妨害できないのでNOTHING
  if (best_target.my_dist >= best_target.enemy_dist) {
    return NOTHING;
  }

  // 現在の自分の最大深度を取得
  int current_my_x, current_my_y;
  int current_my_max_depth =
      mfield.get_max_depth_pos(0, current_my_x, current_my_y, field);

  // ターゲットに最も近づく方向を選択
  Direction best_dir = NOTHING;
  int min_dist_after_move = 9999;

  for (int i = 0; i < 4; i++) {
    int nx = player.x1 + dx[i];
    int ny = player.y1 + dy[i];

    // 範囲チェック
    if (nx < 0 || nx >= 30 || ny < 0 || ny >= 20) {
      continue;
    }

    // 移動可能かチェック
    if (field[nx][ny] != 0) {
      continue;
    }

    // 自分の最大深度への影響をチェック
    // 仮想的にこの位置に移動した場合の自分の最大深度を計算
    int virtual_field[30][20];
    for (int x = 0; x < 30; x++) {
      for (int y = 0; y < 20; y++) {
        virtual_field[x][y] = field[x][y];
      }
    }
    virtual_field[nx][ny] = 1; // 仮想的に占有

    int virtual_my_x, virtual_my_y;
    int virtual_my_max_depth = count_reachable_cells(nx, ny, virtual_field);

    // 自分の最大深度が大幅に減る場合はこの方向を避ける
    // 例：現在の80%以下になる場合は避ける
    if (virtual_my_max_depth < current_my_max_depth * 0.8) {
      continue;
    }

    // ターゲットへの距離を計算
    int dist_to_target = abs(nx - best_target.x) + abs(ny - best_target.y);

    if (dist_to_target < min_dist_after_move) {
      min_dist_after_move = dist_to_target;
      best_dir = directions[i];
    }
  }

  return best_dir;
}

/*
calculate_threat_scores
全敵機の脅威度を計算
*/
void calculate_threat_scores(Tronbike player, Tronbike enemy[3],
                             int field[30][20], ManhattanField &mfield,
                             ThreatScore threats[3]) {
  for (int e = 0; e < 3; e++) {
    threats[e].enemy_id = e;
    threats[e].max_depth = 0;
    threats[e].distance_to_me = 9999;
    threats[e].area_overlap = 0;
    threats[e].is_behind = false;
    threats[e].total_threat = 0.0;

    // 無効な敵機はスキップ
    if (enemy[e].x1 < 0 || enemy[e].x1 >= 30 || enemy[e].y1 < 0 ||
        enemy[e].y1 >= 20) {
      continue;
    }

    // 最大深度を取得
    int enemy_target_x, enemy_target_y;
    threats[e].max_depth =
        mfield.get_max_depth_pos(e + 1, enemy_target_x, enemy_target_y, field);

    // 自分からの距離（マンハッタン距離）
    threats[e].distance_to_me =
        abs(player.x1 - enemy[e].x1) + abs(player.y1 - enemy[e].y1);

    // 領域重複度を計算（自分と敵の距離が近いマスの数）
    int overlap_count = 0;
    for (int x = 0; x < 30; x++) {
      for (int y = 0; y < 20; y++) {
        if (field[x][y] == 0) {
          int my_dist = mfield.player_dist[x][y];
          int enemy_dist = mfield.enemy_dist[e][x][y];
          // 両者の距離が近い（差が3以下）場合は重複
          if (abs(my_dist - enemy_dist) <= 3) {
            overlap_count++;
          }
        }
      }
    }
    threats[e].area_overlap = overlap_count;

    // 後方位置判定（自分の移動方向の反対側にいるか）
    // 簡易版：自分より後ろにいるか
    int dx = enemy[e].x1 - player.x0;
    int dy = enemy[e].y1 - player.y0;
    if (dx * (player.x1 - player.x0) < 0 || dy * (player.y1 - player.y0) < 0) {
      threats[e].is_behind = true;
    }

    // 総合脅威度を計算
    float max_depth_score = (float)threats[e].max_depth * 1.0;
    float distance_score =
        100.0 / (float)(threats[e].distance_to_me + 1); // 近いほど脅威
    float overlap_score = (float)threats[e].area_overlap * 0.1;
    float behind_penalty = threats[e].is_behind ? 20.0 : 0.0;

    threats[e].total_threat =
        max_depth_score + distance_score + overlap_score + behind_penalty;
  }

  // 脅威度でソート（降順）
  for (int i = 0; i < 2; i++) {
    for (int j = i + 1; j < 3; j++) {
      if (threats[j].total_threat > threats[i].total_threat) {
        ThreatScore temp = threats[i];
        threats[i] = threats[j];
        threats[j] = temp;
      }
    }
  }
}

/*
avoid_pincer_attack
複数敵機による挟み撃ちを回避
最悪ケース（各敵機が最適に動く）を想定
*/
Direction avoid_pincer_attack(Tronbike player, Tronbike enemy[3],
                              int field[30][20], ManhattanField &mfield) {
  int dx[] = {0, 0, -1, 1};
  int dy[] = {-1, 1, 0, 0};
  Direction directions[] = {UP, DOWN, LEFT, RIGHT};

  // 有効な敵機の数をカウント
  int active_enemy_count = 0;
  for (int e = 0; e < 3; e++) {
    if (enemy[e].x1 >= 0 && enemy[e].x1 < 30 && enemy[e].y1 >= 0 &&
        enemy[e].y1 < 20) {
      active_enemy_count++;
    }
  }

  // 敵機が1機以下なら挟み撃ちの心配なし
  if (active_enemy_count <= 1) {
    return NOTHING;
  }

  // 各方向の包囲度を評価
  struct DirectionEval {
    Direction dir;
    bool movable;
    float encirclement_score;  // 包囲度（低いほど安全）
    int min_distance_to_enemy; // 最も近い敵機までの距離
  };

  DirectionEval evals[4];

  for (int i = 0; i < 4; i++) {
    evals[i].dir = directions[i];
    evals[i].movable = false;
    evals[i].encirclement_score = 9999.0;
    evals[i].min_distance_to_enemy = 9999;

    int nx = player.x1 + dx[i];
    int ny = player.y1 + dy[i];

    // 範囲チェック
    if (nx < 0 || nx >= 30 || ny < 0 || ny >= 20) {
      continue;
    }

    // 移動可能かチェック
    if (field[nx][ny] != 0) {
      continue;
    }

    evals[i].movable = true;

    // 包囲度を計算
    // 各敵機からの最短距離を合計
    float total_inverse_distance = 0.0;
    int min_dist = 9999;

    for (int e = 0; e < 3; e++) {
      if (enemy[e].x1 < 0 || enemy[e].x1 >= 30 || enemy[e].y1 < 0 ||
          enemy[e].y1 >= 20) {
        continue;
      }

      // この位置から敵機までのマンハッタン距離
      int dist = abs(nx - enemy[e].x1) + abs(ny - enemy[e].y1);

      if (dist < min_dist) {
        min_dist = dist;
      }

      // 距離の逆数を加算（近い敵ほど包囲度が高い）
      total_inverse_distance += 100.0 / (float)(dist + 1);
    }

    evals[i].encirclement_score = total_inverse_distance;
    evals[i].min_distance_to_enemy = min_dist;
  }

  // 包囲度が最も低い方向を選択
  Direction best_dir = NOTHING;
  float min_encirclement = 9999.0;
  int max_min_distance = -1;

  for (int i = 0; i < 4; i++) {
    if (!evals[i].movable) {
      continue;
    }

    // 包囲度が低い方向を優先
    // 同じ包囲度なら、最も近い敵機との距離が遠い方を優先
    if (evals[i].encirclement_score < min_encirclement ||
        (evals[i].encirclement_score == min_encirclement &&
         evals[i].min_distance_to_enemy > max_min_distance)) {
      min_encirclement = evals[i].encirclement_score;
      max_min_distance = evals[i].min_distance_to_enemy;
      best_dir = evals[i].dir;
    }
  }

  // 挟み撃ち判定：包囲度が一定以上高い場合のみ回避行動
  // threshold: 敵機2機で平均距離3程度の場合に発動
  float threshold = 66.0; // 100/(3+1) * 2 ≈ 50, 余裕を持って66

  if (min_encirclement > threshold) {
    return best_dir; // 挟み撃ちされそうなので回避
  }

  return NOTHING; // 挟み撃ちの危険なし
}

/*
calculate_provisional_territory
初期位置からのマンハッタン距離で暫定所有権を計算
*/
void calculate_provisional_territory(Tronbike player, Tronbike enemy[3],
                                     int field[30][20],
                                     TerritoryInfo &territory) {
  // 初期化
  territory.player_count = 0;
  territory.enemy_count = 0;

  // 各プレイヤーの初期位置からのマンハッタン距離を計算
  for (int x = 0; x < 30; x++) {
    for (int y = 0; y < 20; y++) {
      // 壁や既に占有されているマスはスキップ
      if (field[x][y] != 0) {
        territory.player_provisional[x][y] = -99; // 占有済み
        continue;
      }

      // 自分の初期位置からの距離
      territory.player_distance[x][y] = abs(x - player.x0) + abs(y - player.y0);

      // 各敵機の初期位置からの距離
      int min_enemy_dist = 9999;
      int closest_enemy_id = -1;

      for (int e = 0; e < 3; e++) {
        if (enemy[e].x0 < 0 || enemy[e].x0 >= 30 || enemy[e].y0 < 0 ||
            enemy[e].y0 >= 20) {
          territory.enemy_distance[e][x][y] = 9999;
          continue;
        }

        territory.enemy_distance[e][x][y] =
            abs(x - enemy[e].x0) + abs(y - enemy[e].y0);

        if (territory.enemy_distance[e][x][y] < min_enemy_dist) {
          min_enemy_dist = territory.enemy_distance[e][x][y];
          closest_enemy_id = e;
        }
      }

      // 暫定所有者を決定
      if (territory.player_distance[x][y] < min_enemy_dist) {
        // 自分が最も近い → 自分の暫定所有
        territory.player_provisional[x][y] = -1;
        territory.player_count++;
      } else if (territory.player_distance[x][y] > min_enemy_dist) {
        // 敵が最も近い → 敵の暫定所有
        territory.player_provisional[x][y] = closest_enemy_id;
        territory.enemy_count++;
      } else {
        // 同距離 → 中立
        territory.player_provisional[x][y] = 99;
      }
    }
  }
}

/*
select_territory_move
暫定所有ノードを最大化する方向を選択
*/
Direction select_territory_move(Tronbike player, Tronbike enemy[3],
                                int field[30][20], TerritoryInfo &territory) {
  int dx[] = {0, 0, -1, 1};
  int dy[] = {-1, 1, 0, 0};
  Direction directions[] = {UP, DOWN, LEFT, RIGHT};

  Direction best_dir = NOTHING;
  float best_score = -99999.0;

  for (int i = 0; i < 4; i++) {
    int nx = player.x1 + dx[i];
    int ny = player.y1 + dy[i];

    // 範囲チェック
    if (nx < 0 || nx >= 30 || ny < 0 || ny >= 20) {
      continue;
    }

    // 移動可能かチェック
    if (field[nx][ny] != 0) {
      continue;
    }

    // 【改善1】袋小路回避：この方向に進んだ場合の到達可能セル数を計算
    int virtual_field[30][20];
    for (int x = 0; x < 30; x++) {
      for (int y = 0; y < 20; y++) {
        virtual_field[x][y] = field[x][y];
      }
    }
    virtual_field[nx][ny] = 1; // 次の位置を占有
    int reachable_cells = count_reachable_cells(nx, ny, virtual_field);

    // 到達可能セルが少なすぎる場合はスキップ（袋小路）
    if (reachable_cells < 20) {
      continue;
    }

    // 【追加】敵機との深度比較：移動後も敵機より深度を確保できるか
    bool depth_safe = true;
    for (int e = 0; e < 3; e++) {
      if (enemy[e].x1 < 0 || enemy[e].x1 >= 30 || enemy[e].y1 < 0 ||
          enemy[e].y1 >= 20) {
        continue;
      }

      // 敵機の到達可能セル数を計算
      int enemy_reachable =
          count_reachable_cells(enemy[e].x1, enemy[e].y1, field);

      // 自分の移動後の深度が敵より大幅に少ない場合は危険
      // 80%以上を確保できない場合はスキップ
      if (reachable_cells < enemy_reachable * 0.8) {
        depth_safe = false;
        break;
      }
    }

    if (!depth_safe) {
      continue; // この方向は深度的に危険なのでスキップ
    }

    // 【改善3】外周保存：外周セル（x=0,29 または y=0,19）の使用を避ける
    bool is_outer_edge = (nx == 0 || nx == 29 || ny == 0 || ny == 19);

    // この方向に移動した場合のスコアを計算
    // 現在位置からの新しいマンハッタン距離を計算
    int new_player_dist[30][20];
    for (int x = 0; x < 30; x++) {
      for (int y = 0; y < 20; y++) {
        new_player_dist[x][y] = abs(x - nx) + abs(y - ny);
      }
    }

    // 変化を評価
    int protected_nodes = 0; // 守れる暫定所有ノード
    int captured_nodes = 0;  // 奪える敵の暫定所有ノード
    int neutral_nodes = 0;   // 確保できる中立ノード
    int lost_nodes = 0;      // 失う暫定所有ノード
    int boundary_nodes = 0;  // 境界線上のノード（敵と距離が近い）

    for (int x = 0; x < 30; x++) {
      for (int y = 0; y < 20; y++) {
        if (field[x][y] != 0)
          continue; // 占有済みはスキップ

        // 最も近い敵機までの距離
        int min_enemy_dist = 9999;
        for (int e = 0; e < 3; e++) {
          if (territory.enemy_distance[e][x][y] < min_enemy_dist) {
            min_enemy_dist = territory.enemy_distance[e][x][y];
          }
        }

        // 現在の暫定所有状態
        int current_owner = territory.player_provisional[x][y];

        // 移動後の暫定所有者を判定
        int new_owner;
        if (new_player_dist[x][y] < min_enemy_dist) {
          new_owner = -1; // 自分
        } else if (new_player_dist[x][y] > min_enemy_dist) {
          new_owner = territory.player_provisional[x][y]; // 敵のまま
        } else {
          new_owner = 99; // 中立
        }

        // 【改善2】境界線判定：敵と自分の距離差が小さい（3以内）
        int distance_diff =
            abs(territory.player_distance[x][y] - min_enemy_dist);
        bool is_boundary = (distance_diff <= 3);

        // 変化を集計
        if (current_owner == -1) {
          // 元々自分の暫定所有ノード
          if (new_owner == -1) {
            protected_nodes++; // 守れた
            if (is_boundary) {
              boundary_nodes++; // 境界線上のノードを守れた
            }
          } else {
            lost_nodes++; // 失った
          }
        } else if (current_owner >= 0 && current_owner < 3) {
          // 元々敵の暫定所有ノード
          if (new_owner == -1) {
            captured_nodes++; // 奪えた
            if (is_boundary) {
              boundary_nodes += 2; // 境界線上のノードを奪えたのでボーナス
            }
          }
        } else if (current_owner == 99) {
          // 元々中立
          if (new_owner == -1) {
            neutral_nodes++;  // 確保できた
            boundary_nodes++; // 中立ノードは境界線上
          }
        }
      }
    }

    // スコア計算
    float score = (float)boundary_nodes * 15.0 + // 境界線上のノードを最優先
                  (float)protected_nodes * 10.0 + (float)captured_nodes * 8.0 +
                  (float)neutral_nodes * 5.0 - (float)lost_nodes * 15.0 +
                  (float)reachable_cells * 2.0; // 到達可能セル数も評価

    // 外周ペナルティ
    if (is_outer_edge) {
      score -= 50.0; // 外周を使う場合は大幅減点
    }

    if (score > best_score) {
      best_score = score;
      best_dir = directions[i];
    }
  }

  return best_dir;
}

/*
block_nearby_enemy
近距離の敵機の進路を塞ぐ
*/
Direction block_nearby_enemy(Tronbike player, Tronbike enemy[3],
                             int field[30][20]) {
  int dx[] = {0, 0, -1, 1};
  int dy[] = {-1, 1, 0, 0};
  Direction directions[] = {UP, DOWN, LEFT, RIGHT};

  // 距離閾値：この距離以下の敵を対象にする
  const int DISTANCE_THRESHOLD = 10;

  // 最も近い敵機を探す
  int closest_enemy_id = -1;
  int min_distance = 9999;

  for (int e = 0; e < 3; e++) {
    if (enemy[e].x1 < 0 || enemy[e].x1 >= 30 || enemy[e].y1 < 0 ||
        enemy[e].y1 >= 20) {
      continue;
    }

    int dist = abs(player.x1 - enemy[e].x1) + abs(player.y1 - enemy[e].y1);
    if (dist < min_distance) {
      min_distance = dist;
      closest_enemy_id = e;
    }
  }

  // 近い敵機がいない
  if (closest_enemy_id == -1 || min_distance > DISTANCE_THRESHOLD) {
    return NOTHING;
  }

  // 敵機の移動方向を予測（現在の進行方向）
  int enemy_dx = enemy[closest_enemy_id].x1 - enemy[closest_enemy_id].x0;
  int enemy_dy = enemy[closest_enemy_id].y1 - enemy[closest_enemy_id].y0;

  // 敵機の次の予想位置
  int enemy_next_x = enemy[closest_enemy_id].x1 + enemy_dx;
  int enemy_next_y = enemy[closest_enemy_id].y1 + enemy_dy;

  // 敵機の進路上のマスを探す
  struct BlockCandidate {
    int x, y;
    int distance_to_block; // ブロック位置までの距離
    Direction dir;
  };

  BlockCandidate candidates[4];
  int candidate_count = 0;

  // 各方向について評価
  for (int i = 0; i < 4; i++) {
    int nx = player.x1 + dx[i];
    int ny = player.y1 + dy[i];

    // 範囲チェック
    if (nx < 0 || nx >= 30 || ny < 0 || ny >= 20) {
      continue;
    }

    // 移動可能かチェック
    if (field[nx][ny] != 0) {
      continue;
    }

    // 深度チェック：この方向に進んだ場合の到達可能セル数
    int virtual_field[30][20];
    for (int x = 0; x < 30; x++) {
      for (int y = 0; y < 20; y++) {
        virtual_field[x][y] = field[x][y];
      }
    }
    virtual_field[nx][ny] = 1;
    int my_reachable = count_reachable_cells(nx, ny, virtual_field);

    // 敵機の深度と比較
    int enemy_reachable = count_reachable_cells(
        enemy[closest_enemy_id].x1, enemy[closest_enemy_id].y1, field);

    // 移動後の深度が敵より大幅に少ない場合はスキップ
    if (my_reachable < enemy_reachable * 0.8) {
      continue;
    }

    // この位置が敵機の進路上にあるか、または敵機に近づくか
    int dist_to_enemy = abs(nx - enemy[closest_enemy_id].x1) +
                        abs(ny - enemy[closest_enemy_id].y1);
    int dist_to_enemy_next = abs(nx - enemy_next_x) + abs(ny - enemy_next_y);

    // 敵機の進路上にある場合は高い優先度
    if (dist_to_enemy_next <= 2) {
      candidates[candidate_count].x = nx;
      candidates[candidate_count].y = ny;
      candidates[candidate_count].distance_to_block = dist_to_enemy_next;
      candidates[candidate_count].dir = directions[i];
      candidate_count++;
    }
    // または敵機に近づく場合
    else if (dist_to_enemy < min_distance) {
      candidates[candidate_count].x = nx;
      candidates[candidate_count].y = ny;
      candidates[candidate_count].distance_to_block = dist_to_enemy;
      candidates[candidate_count].dir = directions[i];
      candidate_count++;
    }
  }

  // 候補がない
  if (candidate_count == 0) {
    return NOTHING;
  }

  // 最も敵機に近づける、または進路を塞げる方向を選択
  Direction best_dir = NOTHING;
  int best_distance = 9999;

  for (int i = 0; i < candidate_count; i++) {
    if (candidates[i].distance_to_block < best_distance) {
      best_distance = candidates[i].distance_to_block;
      best_dir = candidates[i].dir;
    }
  }

  return best_dir;
}

/*
block_enemy_escape
敵機の最大深度経路（出口）を封じる
*/
Direction block_enemy_escape(Tronbike player, Tronbike enemy[3],
                             int field[30][20], ManhattanField &mfield) {
  int dx[] = {0, 0, -1, 1};
  int dy[] = {-1, 1, 0, 0};
  Direction directions[] = {UP, DOWN, LEFT, RIGHT};

  struct BlockTarget {
    int x, y;
    int distance_from_player; // 自分からの距離
    int distance_to_exit;     // 出口からの距離
    float score;
    Direction dir;
  };

  BlockTarget best_target;
  best_target.score = -99999.0;
  best_target.dir = NOTHING;

  // 各敵機について評価
  for (int e = 0; e < 3; e++) {
    if (enemy[e].x1 < 0 || enemy[e].x1 >= 30 || enemy[e].y1 < 0 ||
        enemy[e].y1 >= 20) {
      continue;
    }

    // 敵機の最大深度位置を取得
    int exit_x, exit_y;
    int enemy_max_depth =
        mfield.get_max_depth_pos(e + 1, exit_x, exit_y, field);

    if (enemy_max_depth <= 0)
      continue;

    // 敵機現在位置から最大深度位置への経路を逆BFSで追跡
    // 最大深度位置から敵機への逆BFS
    bool visited[30][20] = {{false}};
    int parent_x[30][20];
    int parent_y[30][20];
    int dist_from_exit[30][20];

    for (int x = 0; x < 30; x++) {
      for (int y = 0; y < 20; y++) {
        parent_x[x][y] = -1;
        parent_y[x][y] = -1;
        dist_from_exit[x][y] = -1;
      }
    }

    // BFSキュー
    int queue_x[600];
    int queue_y[600];
    int queue_front = 0;
    int queue_rear = 0;

    // 最大深度位置から開始
    queue_x[queue_rear] = exit_x;
    queue_y[queue_rear] = exit_y;
    queue_rear++;
    visited[exit_x][exit_y] = true;
    dist_from_exit[exit_x][exit_y] = 0;

    while (queue_front < queue_rear) {
      int cx = queue_x[queue_front];
      int cy = queue_y[queue_front];
      queue_front++;

      // 敵機位置に到達したら終了
      if (cx == enemy[e].x1 && cy == enemy[e].y1) {
        break;
      }

      // 4方向探索
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
        parent_x[nx][ny] = cx;
        parent_y[nx][ny] = cy;
        dist_from_exit[nx][ny] = dist_from_exit[cx][cy] + 1;
        queue_x[queue_rear] = nx;
        queue_y[queue_rear] = ny;
        queue_rear++;
      }
    }

    // 経路上の各位置を評価
    for (int x = 0; x < 30; x++) {
      for (int y = 0; y < 20; y++) {
        if (!visited[x][y])
          continue;
        if (field[x][y] != 0)
          continue;
        if (dist_from_exit[x][y] <= 0)
          continue; // 出口自体は除外

        // 自分からこの位置までの距離
        int my_dist = abs(player.x1 - x) + abs(player.y1 - y);

        // 敵機からこの位置までの距離
        int enemy_dist = dist_from_exit[x][y];

        // 自分が先に到達できる場合のみ
        if (my_dist >= enemy_dist)
          continue;

        // スコア計算：出口に近いほど高得点、自分の到達時間が短いほど高得点
        float score = (float)(enemy_max_depth - dist_from_exit[x][y]) * 10.0 -
                      (float)my_dist * 0.5;

        if (score > best_target.score) {
          best_target.x = x;
          best_target.y = y;
          best_target.distance_from_player = my_dist;
          best_target.distance_to_exit = dist_from_exit[x][y];
          best_target.score = score;
        }
      }
    }
  }

  // 最適な封鎖位置が見つからなかった
  if (best_target.score <= -99999.0) {
    return NOTHING;
  }

  // 封鎖位置への最初の一歩を返す
  for (int i = 0; i < 4; i++) {
    int nx = player.x1 + dx[i];
    int ny = player.y1 + dy[i];

    if (nx < 0 || nx >= 30 || ny < 0 || ny >= 20)
      continue;
    if (field[nx][ny] != 0)
      continue;

    // この方向がターゲットに近づくか
    int current_dist =
        abs(player.x1 - best_target.x) + abs(player.y1 - best_target.y);
    int next_dist = abs(nx - best_target.x) + abs(ny - best_target.y);

    if (next_dist < current_dist) {
      // 深度チェック
      int virtual_field[30][20];
      for (int x = 0; x < 30; x++) {
        for (int y = 0; y < 20; y++) {
          virtual_field[x][y] = field[x][y];
        }
      }
      virtual_field[nx][ny] = 1;
      int my_reachable = count_reachable_cells(nx, ny, virtual_field);

      // 深度が十分確保できる場合のみ
      if (my_reachable >= 10) {
        return directions[i];
      }
    }
  }

  return NOTHING;
}

/*
calculate_enemy_exits
敵の出口数を計算（最大深度への経路の多様性）
*/
int calculate_enemy_exits(int enemy_x, int enemy_y, int field[30][20]) {
  int dx[] = {0, 0, -1, 1};
  int dy[] = {-1, 1, 0, 0};

  // BFSで最大深度を見つける
  bool visited[30][20] = {{false}};
  int dist[30][20];

  for (int x = 0; x < 30; x++) {
    for (int y = 0; y < 20; y++) {
      dist[x][y] = -1;
    }
  }

  int queue_x[600];
  int queue_y[600];
  int queue_front = 0;
  int queue_rear = 0;

  queue_x[queue_rear] = enemy_x;
  queue_y[queue_rear] = enemy_y;
  queue_rear++;
  visited[enemy_x][enemy_y] = true;
  dist[enemy_x][enemy_y] = 0;

  int max_dist = 0;

  while (queue_front < queue_rear) {
    int cx = queue_x[queue_front];
    int cy = queue_y[queue_front];
    queue_front++;

    if (dist[cx][cy] > max_dist) {
      max_dist = dist[cx][cy];
    }

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
      dist[nx][ny] = dist[cx][cy] + 1;
      queue_x[queue_rear] = nx;
      queue_y[queue_rear] = ny;
      queue_rear++;
    }
  }

  // 最大深度の位置の数をカウント（出口数の指標）
  int exit_count = 0;
  for (int x = 0; x < 30; x++) {
    for (int y = 0; y < 20; y++) {
      if (dist[x][y] == max_dist) {
        exit_count++;
      }
    }
  }

  return exit_count;
}

/*
select_optimal_block
動的出口封鎖：敵の出口を最も減らせる位置を選択
*/
Direction select_optimal_block(Tronbike player, Tronbike enemy[3],
                               int field[30][20]) {
  int dx[] = {0, 0, -1, 1};
  int dy[] = {-1, 1, 0, 0};
  Direction directions[] = {UP, DOWN, LEFT, RIGHT};

  // 最も脅威的な敵を特定（最も近い敵）
  int target_enemy = -1;
  int min_dist = 9999;

  for (int e = 0; e < 3; e++) {
    if (enemy[e].x1 < 0 || enemy[e].x1 >= 30 || enemy[e].y1 < 0 ||
        enemy[e].y1 >= 20) {
      continue;
    }

    int dist = abs(player.x1 - enemy[e].x1) + abs(player.y1 - enemy[e].y1);
    if (dist < min_dist) {
      min_dist = dist;
      target_enemy = e;
    }
  }

  if (target_enemy == -1 || min_dist > 15) {
    return NOTHING; // 敵が遠い場合はスキップ
  }

  // 現在の敵の出口数を計算
  int original_exits = calculate_enemy_exits(enemy[target_enemy].x1,
                                             enemy[target_enemy].y1, field);

  // 各方向への移動を評価
  Direction best_dir = NOTHING;
  float best_score = -99999.0;

  for (int i = 0; i < 4; i++) {
    int nx = player.x1 + dx[i];
    int ny = player.y1 + dy[i];

    if (nx < 0 || nx >= 30 || ny < 0 || ny >= 20)
      continue;
    if (field[nx][ny] != 0)
      continue;

    // 仮想フィールドでこの位置をブロック
    int test_field[30][20];
    for (int x = 0; x < 30; x++) {
      for (int y = 0; y < 20; y++) {
        test_field[x][y] = field[x][y];
      }
    }
    test_field[nx][ny] = 1;

    // 敵の各方向への移動をシミュレート
    int min_enemy_exits = 9999;

    for (int j = 0; j < 4; j++) {
      int enemy_nx = enemy[target_enemy].x1 + dx[j];
      int enemy_ny = enemy[target_enemy].y1 + dy[j];

      if (enemy_nx < 0 || enemy_nx >= 30 || enemy_ny < 0 || enemy_ny >= 20)
        continue;
      if (test_field[enemy_nx][enemy_ny] != 0)
        continue;

      // この方向に敵が進んだ場合の出口数
      int virtual_field[30][20];
      for (int x = 0; x < 30; x++) {
        for (int y = 0; y < 20; y++) {
          virtual_field[x][y] = test_field[x][y];
        }
      }
      virtual_field[enemy_nx][enemy_ny] = 2; // 敵機が占有

      int exits = calculate_enemy_exits(enemy_nx, enemy_ny, virtual_field);
      if (exits < min_enemy_exits) {
        min_enemy_exits = exits;
      }
    }

    // この位置をブロックすることで減らせる出口数
    int exit_reduction = original_exits - min_enemy_exits;

    // 自分の安全性チェック
    int my_reachable = count_reachable_cells(nx, ny, test_field);

    if (my_reachable < 10)
      continue; // 袋小路チェック

    // スコア計算
    float score = (float)exit_reduction * 25.0 - // 出口減少を最優先
                  (float)min_dist * 0.3;         // 距離ペナルティ

    // 3個以上出口を減らせる場合は大幅ボーナス
    if (exit_reduction >= 3) {
      score += 100.0;
    }

    if (score > best_score) {
      best_score = score;
      best_dir = directions[i];
    }
  }

  // 出口を2個以上減らせる場合のみ実行
  if (best_score > 40.0) {
    return best_dir;
  }

  return NOTHING;
}

/*
escape_from_edge
外周マスにいる時に敵機が近い場合、退避ルートを確保するために離れる
*/
Direction escape_from_edge(Tronbike player, Tronbike enemy[3],
                           int field[30][20], TerritoryInfo &territory) {
  // 外周マスにいるかチェック
  bool on_edge =
      (player.x1 == 0 || player.x1 == 29 || player.y1 == 0 || player.y1 == 19);

  if (!on_edge) {
    return NOTHING; // 外周にいない場合はスキップ
  }

  // 最も近い敵機を探す
  int closest_enemy = -1;
  int min_dist = 9999;

  for (int e = 0; e < 3; e++) {
    if (enemy[e].x1 < 0 || enemy[e].x1 >= 30 || enemy[e].y1 < 0 ||
        enemy[e].y1 >= 20) {
      continue;
    }

    int dist = abs(player.x1 - enemy[e].x1) + abs(player.y1 - enemy[e].y1);
    if (dist < min_dist) {
      min_dist = dist;
      closest_enemy = e;
    }
  }

  // 敵が遠い（10マス以上）場合はスキップ
  if (closest_enemy == -1 || min_dist >= 10) {
    return NOTHING;
  }

  // 勝ち確定かチェック
  if (territory.player_count > territory.enemy_count) {
    return NOTHING; // 勝ち確定なら外周にいても問題な��
  }

  // 敵機から離れる方向を選択
  int dx[] = {0, 0, -1, 1};
  int dy[] = {-1, 1, 0, 0};
  Direction directions[] = {UP, DOWN, LEFT, RIGHT};

  Direction best_dir = NOTHING;
  int max_dist = -1;
  int max_reachable = -1;

  for (int i = 0; i < 4; i++) {
    int nx = player.x1 + dx[i];
    int ny = player.y1 + dy[i];

    if (nx < 0 || nx >= 30 || ny < 0 || ny >= 20)
      continue;
    if (field[nx][ny] != 0)
      continue;

    // この方向に進んだ場合の敵機からの距離
    int new_dist =
        abs(nx - enemy[closest_enemy].x1) + abs(ny - enemy[closest_enemy].y1);

    // 到達可能セル数もチェック
    int virtual_field[30][20];
    for (int x = 0; x < 30; x++) {
      for (int y = 0; y < 20; y++) {
        virtual_field[x][y] = field[x][y];
      }
    }
    virtual_field[nx][ny] = 1;
    int reachable = count_reachable_cells(nx, ny, virtual_field);

    // 袋小路は避ける
    if (reachable < 10)
      continue;

    // フィールド内側に向かう方向を優先
    bool towards_center = false;
    if (player.x1 == 0 && nx > player.x1)
      towards_center = true;
    if (player.x1 == 29 && nx < player.x1)
      towards_center = true;
    if (player.y1 == 0 && ny > player.y1)
      towards_center = true;
    if (player.y1 == 19 && ny < player.y1)
      towards_center = true;

    // スコア計算：敵から離れる + 内側に向かう + 到達可能数
    int score = new_dist * 10 + (towards_center ? 50 : 0) + reachable / 10;

    if (new_dist > max_dist ||
        (new_dist == max_dist && reachable > max_reachable)) {
      max_dist = new_dist;
      max_reachable = reachable;
      best_dir = directions[i];
    }
  }

  return best_dir;
}

/*
validate_direction
進路の最終バリデーション：袋小路でない、既知のノードでない
*/
bool validate_direction(Direction dir, Tronbike player, int field[30][20]) {
  if (dir == NOTHING) {
    return false;
  }

  int dx[] = {0, 0, -1, 1};
  int dy[] = {-1, 1, 0, 0};
  Direction directions[] = {UP, DOWN, LEFT, RIGHT};

  int nx = player.x1;
  int ny = player.y1;

  // 方向から次の位置を計算
  for (int i = 0; i < 4; i++) {
    if (directions[i] == dir) {
      nx = player.x1 + dx[i];
      ny = player.y1 + dy[i];
      break;
    }
  }

  // 範囲チェック
  if (nx < 0 || nx >= 30 || ny < 0 || ny >= 20) {
    return false;
  }

  // 既知のノードチェック（占有済み）
  if (field[nx][ny] != 0) {
    return false;
  }

  // 袋小路チェック
  int virtual_field[30][20];
  for (int x = 0; x < 30; x++) {
    for (int y = 0; y < 20; y++) {
      virtual_field[x][y] = field[x][y];
    }
  }
  virtual_field[nx][ny] = 1;
  int reachable = count_reachable_cells(nx, ny, virtual_field);

  // 到達可能セルが10未満なら袋小路
  if (reachable < 10) {
    return false;
  }

  return true;
}

/*
get_max_depth_direction
BFSで最大深度経路を選択（フォールバック）
*/
Direction get_max_depth_direction(Tronbike player, int field[30][20]) {
  int dx[] = {0, 0, -1, 1};
  int dy[] = {-1, 1, 0, 0};
  Direction directions[] = {UP, DOWN, LEFT, RIGHT};

  // 各方向の最大深度を計算
  Direction best_dir = NOTHING;
  int max_depth = -1;

  for (int i = 0; i < 4; i++) {
    int nx = player.x1 + dx[i];
    int ny = player.y1 + dy[i];

    if (nx < 0 || nx >= 30 || ny < 0 || ny >= 20)
      continue;
    if (field[nx][ny] != 0)
      continue;

    // この方向に進んだ場合の最大深度を計算
    int virtual_field[30][20];
    for (int x = 0; x < 30; x++) {
      for (int y = 0; y < 20; y++) {
        virtual_field[x][y] = field[x][y];
      }
    }
    virtual_field[nx][ny] = 1;

    // BFS で最大深度を計算
    bool visited[30][20] = {{false}};
    int dist[30][20];
    for (int x = 0; x < 30; x++) {
      for (int y = 0; y < 20; y++) {
        dist[x][y] = -1;
      }
    }

    int queue_x[600];
    int queue_y[600];
    int queue_front = 0;
    int queue_rear = 0;

    queue_x[queue_rear] = nx;
    queue_y[queue_rear] = ny;
    queue_rear++;
    visited[nx][ny] = true;
    dist[nx][ny] = 0;

    int depth = 0;

    while (queue_front < queue_rear) {
      int cx = queue_x[queue_front];
      int cy = queue_y[queue_front];
      queue_front++;

      if (dist[cx][cy] > depth) {
        depth = dist[cx][cy];
      }

      for (int j = 0; j < 4; j++) {
        int nx2 = cx + dx[j];
        int ny2 = cy + dy[j];

        if (nx2 < 0 || nx2 >= 30 || ny2 < 0 || ny2 >= 20)
          continue;
        if (visited[nx2][ny2])
          continue;
        if (virtual_field[nx2][ny2] != 0)
          continue;

        visited[nx2][ny2] = true;
        dist[nx2][ny2] = dist[cx][cy] + 1;
        queue_x[queue_rear] = nx2;
        queue_y[queue_rear] = ny2;
        queue_rear++;
      }
    }

    // 外周ペナルティ
    bool is_outer_edge = (nx == 0 || nx == 29 || ny == 0 || ny == 19);
    int edge_penalty = is_outer_edge ? 50 : 0; // 外周なら大幅減点

    // スコア = 深度 - 外周ペナルティ
    int score = depth - edge_penalty;

    if (score > max_depth) {
      max_depth = score;
      best_dir = directions[i];
    }
  }

  return best_dir;
}
