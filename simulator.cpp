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

struct Component {
  int id;
  int size;
  vector<pair<int, int>> cells;
};

struct ComponentAnalysis {
  int num_components;
  int component_id[WIDTH][HEIGHT]; // 各マスが属する成分ID (-1=壁)
  vector<Component> components;
  int player_component[MAX_PLAYERS]; // 各プレイヤーが属する成分ID
};

/*
 * マンハッタン距離を計算
 */
int manhattan_distance(int x1, int y1, int x2, int y2) {
  return abs(x1 - x2) + abs(y1 - y2);
}

/*
 * 連結成分を分析（Flood Fill）
 */
ComponentAnalysis analyze_components(int field[WIDTH][HEIGHT], Player players[],
                                     int num_players) {
  ComponentAnalysis result;
  result.num_components = 0;

  // 初期化
  for (int x = 0; x < WIDTH; x++) {
    for (int y = 0; y < HEIGHT; y++) {
      result.component_id[x][y] = -1;
    }
  }

  for (int i = 0; i < MAX_PLAYERS; i++) {
    result.player_component[i] = -1;
  }

  // Flood Fillで連結成分を発見
  int dx[] = {0, 0, -1, 1};
  int dy[] = {-1, 1, 0, 0};

  // プレイヤーの現在位置を記録（空マスとして扱う）
  bool is_player_pos[WIDTH][HEIGHT] = {{false}};
  for (int i = 0; i < num_players; i++) {
    if (players[i].alive) {
      is_player_pos[players[i].x][players[i].y] = true;
    }
  }

  for (int start_x = 0; start_x < WIDTH; start_x++) {
    for (int start_y = 0; start_y < HEIGHT; start_y++) {
      // 未訪問の空マスから開始
      // プレイヤーの現在位置は空マスとして扱う
      if (field[start_x][start_y] != 0 && !is_player_pos[start_x][start_y])
        continue;
      if (result.component_id[start_x][start_y] >= 0)
        continue;

      // 新しい連結成分を発見
      int comp_id = result.num_components++;
      Component comp;
      comp.id = comp_id;
      comp.size = 0;

      // BFSで成分を探索
      int queue_x[600], queue_y[600];
      int front = 0, rear = 0;

      queue_x[rear] = start_x;
      queue_y[rear] = start_y;
      rear++;
      result.component_id[start_x][start_y] = comp_id;

      while (front < rear) {
        int idx = front++;
        int cx = queue_x[idx];
        int cy = queue_y[idx];
        comp.size++;
        comp.cells.push_back(make_pair(cx, cy));

        for (int i = 0; i < 4; i++) {
          int nx = cx + dx[i];
          int ny = cy + dy[i];

          if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT)
            continue;
          // プレイヤーの現在位置は空マスとして扱う
          if (field[nx][ny] != 0 && !is_player_pos[nx][ny])
            continue;
          if (result.component_id[nx][ny] >= 0)
            continue;

          result.component_id[nx][ny] = comp_id;
          queue_x[rear] = nx;
          queue_y[rear] = ny;
          rear++;
        }
      }

      result.components.push_back(comp);
    }
  }

  // 各プレイヤーが属する成分を特定
  for (int i = 0; i < num_players; i++) {
    if (players[i].alive) {
      result.player_component[i] =
          result.component_id[players[i].x][players[i].y];
    }
  }

  return result;
}

/*
 * 1手進めた仮想状態での連結成分を分析
 */
ComponentAnalysis analyze_after_move(int field[WIDTH][HEIGHT], Player players[],
                                     int num_players, int player_id,
                                     Direction dir) {
  // 仮想フィールドを作成
  int virtual_field[WIDTH][HEIGHT];
  for (int x = 0; x < WIDTH; x++) {
    for (int y = 0; y < HEIGHT; y++) {
      virtual_field[x][y] = field[x][y];
    }
  }

  // プレイヤーの現在位置を壁にする
  virtual_field[players[player_id].x][players[player_id].y] = player_id + 1;

  // 移動先を計算
  int dx[] = {0, 0, -1, 1};
  int dy[] = {-1, 1, 0, 0};
  int dir_idx = (dir == UP) ? 0 : (dir == DOWN) ? 1 : (dir == LEFT) ? 2 : 3;

  Player virtual_players[MAX_PLAYERS];
  for (int i = 0; i < num_players; i++) {
    virtual_players[i] = players[i];
  }

  virtual_players[player_id].x += dx[dir_idx];
  virtual_players[player_id].y += dy[dir_idx];

  return analyze_components(virtual_field, virtual_players, num_players);
}

/*
 * 連結成分を可視化
 */
void visualize_components(ComponentAnalysis &analysis, Player players[],
                          int num_players, const char *title) {
  const char *symbols[] = {"◯", "●", "◎", "◆"};

  cout << "=== " << title << " ===" << endl;
  cout << "Components: " << analysis.num_components << endl;

  // 各成分の情報を表示
  for (size_t i = 0; i < analysis.components.size(); i++) {
    cout << "  Component " << analysis.components[i].id
         << ": size=" << analysis.components[i].size;

    // この成分に属するプレイヤーを表示
    vector<int> players_in_comp;
    for (int p = 0; p < num_players; p++) {
      if (players[p].alive &&
          analysis.player_component[p] == analysis.components[i].id) {
        players_in_comp.push_back(p);
      }
    }

    if (!players_in_comp.empty()) {
      cout << " [Players:";
      for (size_t j = 0; j < players_in_comp.size(); j++) {
        cout << " " << players_in_comp[j];
      }
      cout << "]";
    }
    cout << endl;
  }
  cout << endl;

  // フィールドを成分IDで可視化
  cout << "   ";
  for (int x = 0; x < WIDTH; x++) {
    cout << x % 10;
  }
  cout << endl;
  cout << "   ";
  for (int x = 0; x < WIDTH; x++) {
    cout << "-";
  }
  cout << endl;

  for (int y = 0; y < HEIGHT; y++) {
    cout << setw(2) << y << "|";

    for (int x = 0; x < WIDTH; x++) {
      // プレイヤーの位置かチェック
      bool is_player = false;
      for (int p = 0; p < num_players; p++) {
        if (players[p].alive && x == players[p].x && y == players[p].y) {
          cout << symbols[players[p].id];
          is_player = true;
          break;
        }
      }

      if (!is_player) {
        int comp_id = analysis.component_id[x][y];
        if (comp_id < 0) {
          cout << "#"; // 壁
        } else if (comp_id < 10) {
          cout << comp_id; // 成分ID
        } else {
          cout << "+"; // 10以上
        }
      }
    }
    cout << endl;
  }
  cout << endl;
}

/*
 * 各方向への移動の影響を分析（詳細版）
 */
void analyze_move_options(int field[WIDTH][HEIGHT], Player players[],
                          int num_players, int player_id) {
  const char *dir_names[] = {"UP", "DOWN", "LEFT", "RIGHT"};
  int dx[] = {0, 0, -1, 1};
  int dy[] = {-1, 1, 0, 0};

  cout << "=== Move Analysis for Player " << player_id << " ===" << endl;
  cout << "Current position: (" << players[player_id].x << ", "
       << players[player_id].y << ")" << endl;
  cout << endl;

  // 敵の位置リスト
  vector<pair<int, int>> enemy_positions;
  for (int i = 0; i < num_players; i++) {
    if (i != player_id && players[i].alive) {
      enemy_positions.push_back(make_pair(players[i].x, players[i].y));
    }
  }

  // 仮想フィールド（現在位置を壁にする）
  int virtual_field[WIDTH][HEIGHT];
  for (int x = 0; x < WIDTH; x++) {
    for (int y = 0; y < HEIGHT; y++) {
      virtual_field[x][y] = field[x][y];
    }
  }
  virtual_field[players[player_id].x][players[player_id].y] = player_id + 1;

  for (int i = 0; i < 4; i++) {
    int nx = players[player_id].x + dx[i];
    int ny = players[player_id].y + dy[i];

    // 範囲チェック
    if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT) {
      cout << dir_names[i] << ": OUT OF BOUNDS" << endl;
      continue;
    }

    // 衝突チェック
    bool is_enemy_pos = false;
    for (size_t j = 0; j < enemy_positions.size(); j++) {
      if (nx == enemy_positions[j].first && ny == enemy_positions[j].second) {
        is_enemy_pos = true;
        break;
      }
    }
    if (virtual_field[nx][ny] != 0 && !is_enemy_pos) {
      cout << dir_names[i] << ": COLLISION (wall)" << endl;
      continue;
    }

    // 1. 自分のBFS (距離マップ作成)
    int my_dist[WIDTH][HEIGHT];
    for (int x = 0; x < WIDTH; x++)
      for (int y = 0; y < HEIGHT; y++)
        my_dist[x][y] = 9999;

    int queue_x[1000], queue_y[1000];
    int front = 0, rear = 0;

    queue_x[rear] = nx;
    queue_y[rear] = ny;
    rear++;
    my_dist[nx][ny] = 0;

    bool has_enemy_in_comp = false;
    int reachable = 0;

    while (front < rear) {
      int idx = front++;
      int cx = queue_x[idx];
      int cy = queue_y[idx];
      int cd = my_dist[cx][cy];
      reachable++;

      // 成分内に敵がいるか
      for (size_t j = 0; j < enemy_positions.size(); j++) {
        if (cx == enemy_positions[j].first && cy == enemy_positions[j].second) {
          has_enemy_in_comp = true;
        }
      }

      for (int d = 0; d < 4; d++) {
        int nnx = cx + dx[d];
        int nny = cy + dy[d];
        if (nnx < 0 || nnx >= WIDTH || nny < 0 || nny >= HEIGHT)
          continue;
        if (my_dist[nnx][nny] != 9999)
          continue;

        bool is_ep = false;
        for (size_t j = 0; j < enemy_positions.size(); j++) {
          if (nnx == enemy_positions[j].first &&
              nny == enemy_positions[j].second) {
            is_ep = true;
            break;
          }
        }
        if (virtual_field[nnx][nny] != 0 && !is_ep)
          continue;

        my_dist[nnx][nny] = cd + 1;
        queue_x[rear] = nnx;
        queue_y[rear] = nny;
        rear++;
      }
    }

    bool isolated = false;
    int eval_score = 0;

    if (!has_enemy_in_comp) {
      isolated = true;
      eval_score = reachable;
    } else {
      // 2. 敵のBFS (距離マップ作成)
      int enemy_dist[WIDTH][HEIGHT];
      for (int x = 0; x < WIDTH; x++)
        for (int y = 0; y < HEIGHT; y++)
          enemy_dist[x][y] = 9999;

      int eq_x[1000], eq_y[1000];
      int ef = 0, er = 0;

      for (size_t e = 0; e < enemy_positions.size(); e++) {
        int ex = enemy_positions[e].first;
        int ey = enemy_positions[e].second;
        enemy_dist[ex][ey] = 0;
        eq_x[er] = ex;
        eq_y[er] = ey;
        er++;
      }

      while (ef < er) {
        int idx = ef++;
        int cx = eq_x[idx];
        int cy = eq_y[idx];
        int cd = enemy_dist[cx][cy];

        for (int d = 0; d < 4; d++) {
          int nnx = cx + dx[d];
          int nny = cy + dy[d];
          if (nnx < 0 || nnx >= WIDTH || nny < 0 || nny >= HEIGHT)
            continue;
          if (enemy_dist[nnx][nny] != 9999)
            continue;
          if (nnx == nx && nny == ny)
            continue;

          if (virtual_field[nnx][nny] != 0) {
            bool is_ep = false;
            for (size_t j = 0; j < enemy_positions.size(); j++) {
              if (nnx == enemy_positions[j].first &&
                  nny == enemy_positions[j].second) {
                is_ep = true;
                break;
              }
            }
            if (!is_ep)
              continue;
          }

          enemy_dist[nnx][nny] = cd + 1;
          eq_x[er] = nnx;
          eq_y[er] = nny;
          er++;
        }
      }

      // 3. ボロノイカウント
      int voronoi = 0;
      for (int x = 0; x < WIDTH; x++) {
        for (int y = 0; y < HEIGHT; y++) {
          if (my_dist[x][y] != 9999) {
            if (my_dist[x][y] < enemy_dist[x][y]) {
              voronoi++;
            }
          }
        }
      }
      eval_score = voronoi;
    }

    cout << dir_names[i] << ": Score=" << eval_score
         << " (Isolated=" << (isolated ? "YES" : "NO") << ")";

    if (isolated) {
      cout << " [PRIORITY: Layer3]";
    } else {
      cout << " [Layer1 Voronoi]";
    }
    cout << endl;
  }
  cout << endl;
}

/*
 * BFSで到達可能マス数を計算（従来の方法）
 */
int count_reachable_cells(int start_x, int start_y, int field[WIDTH][HEIGHT]) {
  if (start_x < 0 || start_x >= WIDTH || start_y < 0 || start_y >= HEIGHT)
    return 0;
  if (field[start_x][start_y] != 0)
    return 0;

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

    for (int i = 0; i < 4; i++) {
      int nx = cx + dx[i];
      int ny = cy + dy[i];
      if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT)
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
 * 敵機AI: BFSで最大深度の方向を選択
 */
Direction enemy_ai_move(Player &player, int field[WIDTH][HEIGHT]) {
  int dx[] = {0, 0, -1, 1};
  int dy[] = {-1, 1, 0, 0};
  Direction directions[] = {UP, DOWN, LEFT, RIGHT};

  Direction best_dir = NOTHING;
  int max_reachable = -1;

  // 仮想フィールド作成
  int virtual_field[WIDTH][HEIGHT];
  for (int x = 0; x < WIDTH; x++) {
    for (int y = 0; y < HEIGHT; y++) {
      virtual_field[x][y] = field[x][y];
    }
  }
  virtual_field[player.x][player.y] = 1;

  for (int i = 0; i < 4; i++) {
    int nx = player.x + dx[i];
    int ny = player.y + dy[i];

    if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT)
      continue;
    if (field[nx][ny] != 0)
      continue;

    int reachable = count_reachable_cells(nx, ny, virtual_field);

    if (reachable > max_reachable) {
      max_reachable = reachable;
      best_dir = directions[i];
    }
  }

  return best_dir;
}

/*
 * ★★★ プレイヤー戦略関数 ★★★
 * strategy.cppと同じロジック（第三層BFS）
 */
// ----------------------------------------------------------------
// Forward Declaration
// ----------------------------------------------------------------
Direction player_strategy(Player &player, Player enemies[], int num_enemies,
                          int field[WIDTH][HEIGHT]);

// ----------------------------------------------------------------
// Minimax Strategy (Gold)
// ----------------------------------------------------------------

// 簡易評価関数: 到達可能数 (自分 - 敵)
int evaluate_board(int field[WIDTH][HEIGHT], int my_x, int my_y, int en_x,
                   int en_y) {
  // BFSで自分の到達可能数
  int my_reachable = 0;
  bool visited[WIDTH][HEIGHT] = {{false}};
  int qx[1000], qy[1000];
  int h = 0, t = 0;
  qx[t] = my_x;
  qy[t] = my_y;
  t++;
  visited[my_x][my_y] = true;

  while (h < t) {
    int cx = qx[h];
    int cy = qy[h];
    h++;
    my_reachable++;
    int dx[] = {0, 0, -1, 1};
    int dy[] = {-1, 1, 0, 0};
    for (int d = 0; d < 4; d++) {
      int nx = cx + dx[d];
      int ny = cy + dy[d];
      if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT ||
          field[nx][ny] != 0 || visited[nx][ny])
        continue;
      // 敵位置は壁とみなすが、en_x, en_yは既にfieldで壁になっている前提
      visited[nx][ny] = true;
      qx[t] = nx;
      qy[t] = ny;
      t++;
    }
  }

  // BFSで敵の到達可能数 (簡易的に、敵も壁をすり抜けられないとする)
  // ※厳密にはVoronoiだが、計算量削減のため単純BFS
  int en_reachable = 0;
  bool en_visited[WIDTH][HEIGHT] = {{false}};
  h = 0;
  t = 0;
  qx[t] = en_x;
  qy[t] = en_y;
  t++;
  en_visited[en_x][en_y] = true;

  while (h < t) {
    int cx = qx[h];
    int cy = qy[h];
    h++;
    en_reachable++;
    int dx[] = {0, 0, -1, 1};
    int dy[] = {-1, 1, 0, 0};
    for (int d = 0; d < 4; d++) {
      int nx = cx + dx[d];
      int ny = cy + dy[d];
      if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT ||
          field[nx][ny] != 0 || en_visited[nx][ny])
        continue;
      en_visited[nx][ny] = true;
      qx[t] = nx;
      qy[t] = ny;
      t++;
    }
  }

  return my_reachable - en_reachable;
}

int minimax(int depth, bool is_maximizing, int field[WIDTH][HEIGHT], int my_x,
            int my_y, int en_x, int en_y, int alpha, int beta) {
  if (depth == 0) {
    return evaluate_board(field, my_x, my_y, en_x, en_y);
  }

  int dx[] = {0, 0, -1, 1};
  int dy[] = {-1, 1, 0, 0};

  if (is_maximizing) {
    int max_eval = -999999;
    bool can_move = false;

    for (int d = 0; d < 4; d++) {
      int nx = my_x + dx[d];
      int ny = my_y + dy[d];
      if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT || field[nx][ny] != 0)
        continue;

      can_move = true;
      field[nx][ny] = 1; // Make move
      int eval =
          minimax(depth - 1, false, field, nx, ny, en_x, en_y, alpha, beta);
      field[nx][ny] = 0; // Unmake move

      if (eval > max_eval)
        max_eval = eval;
      if (eval > alpha)
        alpha = eval;
      if (beta <= alpha)
        break;
    }

    if (!can_move)
      return -10000; // 負け
    return max_eval;

  } else {
    int min_eval = 999999;
    bool can_move = false;

    for (int d = 0; d < 4; d++) {
      int nx = en_x + dx[d];
      int ny = en_y + dy[d];
      if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT || field[nx][ny] != 0)
        continue;

      can_move = true;
      field[nx][ny] = 2; // Make move
      int eval =
          minimax(depth - 1, true, field, my_x, my_y, nx, ny, alpha, beta);
      field[nx][ny] = 0; // Unmake move

      if (eval < min_eval)
        min_eval = eval;
      if (eval < beta)
        beta = eval;
      if (beta <= alpha)
        break;
    }

    if (!can_move)
      return 10000; // 勝ち
    return min_eval;
  }
}

Direction run_gold_strategy(Player &player, Player enemies[], int num_enemies,
                            int field[WIDTH][HEIGHT]) {
  // 1vs1のみ対応 (敵が1人の場合)
  if (num_enemies != 1) {
    return player_strategy(player, enemies, num_enemies,
                           field); // Fallback to Silver
  }

  int depth = 6; // 読みの深さ
  int best_score = -999999;
  Direction best_dir = UP;

  int dx[] = {0, 0, -1, 1};
  int dy[] = {-1, 1, 0, 0};
  Direction dirs[] = {UP, DOWN, LEFT, RIGHT};

  // 仮想フィールド
  int v_field[WIDTH][HEIGHT];
  for (int x = 0; x < WIDTH; x++)
    for (int y = 0; y < HEIGHT; y++)
      v_field[x][y] = field[x][y];

  // 現在位置を壁化
  v_field[player.x][player.y] = 1;
  v_field[enemies[0].x][enemies[0].y] = 2; // 敵位置も壁化

  bool valid_move_exists = false;

  for (int i = 0; i < 4; i++) {
    int nx = player.x + dx[i];
    int ny = player.y + dy[i];

    if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT || v_field[nx][ny] != 0)
      continue;

    valid_move_exists = true;
    v_field[nx][ny] = 1; // Make move
    int score = minimax(depth - 1, false, v_field, nx, ny, enemies[0].x,
                        enemies[0].y, -999999, 999999);
    v_field[nx][ny] = 0; // Unmake move

    if (score > best_score) {
      best_score = score;
      best_dir = dirs[i];
    }
  }

  if (!valid_move_exists)
    return UP; // 詰み

  return best_dir;
}

/*
 * Silver Strategy (Simulated by player_strategy)
 */
Direction player_strategy(Player &player, Player enemies[], int num_enemies,
                          int field[WIDTH][HEIGHT]) {
  int dx[] = {0, 0, -1, 1};
  int dy[] = {-1, 1, 0, 0};
  Direction directions[] = {UP, DOWN, LEFT, RIGHT};

  // 敵の位置リスト
  vector<pair<int, int>> enemy_positions;
  for (int i = 0; i < num_enemies; i++) {
    if (enemies[i].alive) {
      enemy_positions.push_back(make_pair(enemies[i].x, enemies[i].y));
    }
  }

  // 仮想フィールド
  int virtual_field[WIDTH][HEIGHT];
  for (int x = 0; x < WIDTH; x++) {
    for (int y = 0; y < HEIGHT; y++) {
      virtual_field[x][y] = field[x][y];
    }
  }
  virtual_field[player.x][player.y] = 1;

  struct Eval {
    Direction dir;
    bool valid;

    // 第二層: 分断勝ち
    bool is_cut_win;  // 敵を分断し、自分が有利
    int my_comp_size; // 自分の成分サイズ

    // 第三層: 隔離
    bool is_isolated; // 敵がいない成分

    // 第一層: ボロノイ
    int voronoi_score; // 支配領域サイズ
  } evals[4];

  for (int i = 0; i < 4; i++) {
    evals[i].dir = directions[i];
    evals[i].valid = false;
    evals[i].is_cut_win = false;
    evals[i].is_isolated = false;
    evals[i].my_comp_size = 0;
    evals[i].voronoi_score = 0;

    int nx = player.x + dx[i];
    int ny = player.y + dy[i];

    // 範囲チェック
    if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT)
      continue;

    // 衝突チェック
    bool is_enemy_pos = false;
    for (size_t j = 0; j < enemy_positions.size(); j++) {
      if (nx == enemy_positions[j].first && ny == enemy_positions[j].second) {
        is_enemy_pos = true;
        break;
      }
    }
    if (virtual_field[nx][ny] != 0 && !is_enemy_pos)
      continue;

    evals[i].valid = true;

    // ----------------------------------------------------------------
    // 連結成分分析 (第二層 & 第三層)
    // ----------------------------------------------------------------
    // 一時的に壁にする
    int original_val = virtual_field[nx][ny];
    virtual_field[nx][ny] = 1;

    // 成分IDマップ
    int comp_map[WIDTH][HEIGHT];
    for (int x = 0; x < WIDTH; ++x)
      for (int y = 0; y < HEIGHT; ++y)
        comp_map[x][y] = -1;

    int comp_sizes[600] = {0}; // 成分IDごとのサイズ
    int comp_count = 0;

    for (int sx = 0; sx < WIDTH; ++sx) {
      for (int sy = 0; sy < HEIGHT; ++sy) {
        bool is_ep = false;
        for (size_t j = 0; j < enemy_positions.size(); j++) {
          if (sx == enemy_positions[j].first &&
              sy == enemy_positions[j].second) {
            is_ep = true;
            break;
          }
        }
        if (!is_ep && virtual_field[sx][sy] != 0)
          continue; // 壁
        if (comp_map[sx][sy] != -1)
          continue; // 訪問済み

        // 新しい成分発見
        int current_id = comp_count++;
        int qx[1000], qy[1000];
        int head = 0, tail = 0;
        qx[tail] = sx;
        qy[tail] = sy;
        tail++;
        comp_map[sx][sy] = current_id;
        int size = 0;

        while (head < tail) {
          int cx = qx[head];
          int cy = qy[head];
          head++;
          size++;

          for (int d = 0; d < 4; d++) {
            int tx = cx + dx[d];
            int ty = cy + dy[d];
            if (tx < 0 || tx >= WIDTH || ty < 0 || ty >= HEIGHT)
              continue;
            if (comp_map[tx][ty] != -1)
              continue;

            bool is_ep2 = false;
            for (size_t j = 0; j < enemy_positions.size(); j++) {
              if (tx == enemy_positions[j].first &&
                  ty == enemy_positions[j].second) {
                is_ep2 = true;
                break;
              }
            }
            if (virtual_field[tx][ty] != 0 && !is_ep2)
              continue;

            comp_map[tx][ty] = current_id;
            qx[tail] = tx;
            qy[tail] = ty;
            tail++;
          }
        }
        comp_sizes[current_id] = size;
      }
    }

    // 自分が移動した後、隣接する最大の成分を探す
    int my_max_comp_size = 0;
    int my_max_comp_id = -1;

    for (int d = 0; d < 4; d++) {
      int tx = nx + dx[d];
      int ty = ny + dy[d];
      if (tx < 0 || tx >= WIDTH || ty < 0 || ty >= HEIGHT)
        continue;
      int cid = comp_map[tx][ty];
      if (cid != -1) {
        if (comp_sizes[cid] > my_max_comp_size) {
          my_max_comp_size = comp_sizes[cid];
          my_max_comp_id = cid;
        }
      }
    }

    // 敵がどの成分にいるか確認
    bool enemy_in_my_comp = false;
    int max_enemy_comp_size = 0;

    for (size_t j = 0; j < enemy_positions.size(); j++) {
      int ex = enemy_positions[j].first;
      int ey = enemy_positions[j].second;
      int eid = comp_map[ex][ey];

      if (eid != -1) {
        if (eid == my_max_comp_id) {
          enemy_in_my_comp = true;
        }
        if (comp_sizes[eid] > max_enemy_comp_size) {
          max_enemy_comp_size = comp_sizes[eid];
        }
      }
    }

    evals[i].my_comp_size = my_max_comp_size;

    if (my_max_comp_id != -1) {
      if (!enemy_in_my_comp) {
        evals[i].is_isolated = true;
        if (my_max_comp_size > max_enemy_comp_size) {
          evals[i].is_cut_win = true;
        }
      }
    } else {
      evals[i].my_comp_size = 0;
    }

    // ----------------------------------------------------------------
    // 第一層: ボロノイ (敵と同じ成分にいる場合のみ計算)
    // ----------------------------------------------------------------
    if (!evals[i].is_cut_win && !evals[i].is_isolated) {
      // my_dist計算 (BFS) ... start from nx, ny
      int my_dist[WIDTH][HEIGHT];
      for (int x = 0; x < WIDTH; ++x)
        for (int y = 0; y < HEIGHT; ++y)
          my_dist[x][y] = 9999;
      int qx[1000], qy[1000];
      int head = 0, tail = 0;
      qx[tail] = nx;
      qy[tail] = ny;
      tail++;
      my_dist[nx][ny] = 0;

      while (head < tail) {
        int cx = qx[head];
        int cy = qy[head];
        head++;
        int cd = my_dist[cx][cy];
        for (int d = 0; d < 4; d++) {
          int tx = cx + dx[d];
          int ty = cy + dy[d];
          if (tx < 0 || tx >= WIDTH || ty < 0 || ty >= HEIGHT)
            continue;
          if (my_dist[tx][ty] != 9999)
            continue;
          // 壁判定 (virtual_field[nx][ny]はすでに1になっているのでOK)
          bool is_ep = false;
          for (size_t j = 0; j < enemy_positions.size(); j++) {
            if (tx == enemy_positions[j].first &&
                ty == enemy_positions[j].second) {
              is_ep = true;
              break;
            }
          }
          if (virtual_field[tx][ty] != 0 && !is_ep)
            continue;

          my_dist[tx][ty] = cd + 1;
          qx[tail] = tx;
          qy[tail] = ty;
          tail++;
        }
      }

      // enemy_dist計算 (BFS)
      int enemy_dist[WIDTH][HEIGHT];
      for (int x = 0; x < WIDTH; ++x)
        for (int y = 0; y < HEIGHT; ++y)
          enemy_dist[x][y] = 9999;
      head = 0;
      tail = 0;
      for (size_t j = 0; j < enemy_positions.size(); j++) {
        int ex = enemy_positions[j].first;
        int ey = enemy_positions[j].second;
        enemy_dist[ex][ey] = 0;
        qx[tail] = ex;
        qy[tail] = ey;
        tail++;
      }

      while (head < tail) {
        int cx = qx[head];
        int cy = qy[head];
        head++;
        int cd = enemy_dist[cx][cy];
        for (int d = 0; d < 4; d++) {
          int tx = cx + dx[d];
          int ty = cy + dy[d];
          if (tx < 0 || tx >= WIDTH || ty < 0 || ty >= HEIGHT)
            continue;
          if (enemy_dist[tx][ty] != 9999)
            continue;
          if (tx == nx && ty == ny)
            continue;

          bool is_ep = false;
          for (size_t j = 0; j < enemy_positions.size(); j++) {
            if (tx == enemy_positions[j].first &&
                ty == enemy_positions[j].second) {
              is_ep = true;
              break;
            }
          }
          if (virtual_field[tx][ty] != 0 && !is_ep)
            continue;

          enemy_dist[tx][ty] = cd + 1;
          qx[tail] = tx;
          qy[tail] = ty;
          tail++;
        }
      }

      // Count
      int score = 0;
      for (int x = 0; x < WIDTH; ++x) {
        for (int y = 0; y < HEIGHT; ++y) {
          if (my_dist[x][y] != 9999) {
            if (my_dist[x][y] < enemy_dist[x][y])
              score++;
          }
        }
      }
      evals[i].voronoi_score = score;
    }

    // 壁を戻す
    virtual_field[nx][ny] = original_val;
  }

  // 選択ロジック
  Direction best_dir = UP;
  int max_val = -1;
  bool found = false;

  // 1. Cut Win
  for (int i = 0; i < 4; i++) {
    if (!evals[i].valid)
      continue;
    if (evals[i].is_cut_win) {
      if (evals[i].my_comp_size > max_val) {
        max_val = evals[i].my_comp_size;
        best_dir = evals[i].dir;
        found = true;
      }
    }
  }
  if (found)
    return best_dir;

  // 2. Isolated
  max_val = -1;
  for (int i = 0; i < 4; i++) {
    if (!evals[i].valid)
      continue;
    if (evals[i].is_isolated) {
      if (evals[i].my_comp_size > max_val) {
        max_val = evals[i].my_comp_size;
        best_dir = evals[i].dir;
        found = true;
      }
    }
  }
  if (found)
    return best_dir;

  // 3. Voronoi
  max_val = -1;
  for (int i = 0; i < 4; i++) {
    if (!evals[i].valid)
      continue;
    found = true;
    if (evals[i].voronoi_score > max_val) {
      max_val = evals[i].voronoi_score;
      best_dir = evals[i].dir;
    }
  }

  if (found)
    return best_dir;

  return UP;
}

/*
 * 方向を適用してプレイヤーを移動
 */
void apply_move(Player &player, Direction dir, int field[WIDTH][HEIGHT]) {
  int dx[] = {0, 0, -1, 1};
  int dy[] = {-1, 1, 0, 0};

  if (dir == NOTHING) {
    player.alive = false;
    return;
  }

  int dir_idx = (dir == UP) ? 0 : (dir == DOWN) ? 1 : (dir == LEFT) ? 2 : 3;
  int nx = player.x + dx[dir_idx];
  int ny = player.y + dy[dir_idx];

  // 範囲チェック
  if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT) {
    player.alive = false;
    return;
  }

  // 衝突チェック
  if (field[nx][ny] != 0) {
    player.alive = false;
    return;
  }

  // 移動実行
  player.x = nx;
  player.y = ny;
  field[nx][ny] = player.id + 1;
}

/*
 * フィールド状態を表示
 */
void print_field(int field[WIDTH][HEIGHT], Player players[], int num_players,
                 int turn) {
  const char *symbols[] = {"◯", "●", "◎", "◆"};

  cout << "=== Turn " << turn << " ===" << endl;

  // 上端の座標表示
  cout << "   ";
  for (int x = 0; x < WIDTH; x++) {
    cout << x % 10;
  }
  cout << endl;
  cout << "   ";
  for (int x = 0; x < WIDTH; x++) {
    cout << "-";
  }
  cout << endl;

  // フィールド描画
  for (int y = 0; y < HEIGHT; y++) {
    cout << setw(2) << y << "|";

    for (int x = 0; x < WIDTH; x++) {
      // プレイヤーの現在位置かチェック
      bool is_player_pos = false;
      for (int p = 0; p < num_players; p++) {
        if (players[p].alive && x == players[p].x && y == players[p].y) {
          cout << symbols[players[p].id];
          is_player_pos = true;
          break;
        }
      }

      if (!is_player_pos) {
        if (field[x][y] == 0) {
          cout << ".";
        } else {
          cout << field[x][y] - 1;
        }
      }
    }
    cout << endl;
  }
  cout << endl;
}

/*
 * ゲームシミュレーション
 */
void simulate_game(int num_players, int max_turns, int verbose_level) {
  // verbose_level: 0=結果のみ, 1=10ターンごと, 2=毎ターン, 3=成分分析付き

  // フィールド初期化
  int field[WIDTH][HEIGHT];
  for (int x = 0; x < WIDTH; x++) {
    for (int y = 0; y < HEIGHT; y++) {
      field[x][y] = 0;
    }
  }

  // プレイヤー初期化
  Player players[MAX_PLAYERS];
  for (int i = 0; i < num_players; i++) {
    players[i].id = i;
    players[i].x = rand() % WIDTH;
    players[i].y = rand() % HEIGHT;
    players[i].alive = true;
    field[players[i].x][players[i].y] = i + 1;
  }

  // 初期状態表示
  if (verbose_level >= 1) {
    cout << "=== Initial Positions ===" << endl;
    for (int i = 0; i < num_players; i++) {
      cout << "Player " << i << ": (" << players[i].x << ", " << players[i].y
           << ")" << endl;
    }
    cout << endl;
    print_field(field, players, num_players, 0);

    if (verbose_level >= 3) {
      ComponentAnalysis init_analysis =
          analyze_components(field, players, num_players);
      visualize_components(init_analysis, players, num_players,
                           "Initial Components");
    }
  }

  // ゲームループ
  int turn = 0;
  while (turn < max_turns) {
    turn++;

    // 生存プレイヤーをカウント
    int alive_count = 0;
    for (int i = 0; i < num_players; i++) {
      if (players[i].alive) {
        alive_count++;
      }
    }

    // 勝敗判定
    if (alive_count <= 1) {
      break;
    }

    // 各プレイヤーの移動
    Direction moves[MAX_PLAYERS];

    for (int i = 0; i < num_players; i++) {
      if (!players[i].alive) {
        moves[i] = NOTHING;
        continue;
      }

      // 全プレイヤー共通: 敵リストを作成
      Player enemies[MAX_PLAYERS - 1];
      int enemy_count = 0;
      for (int j = 0; j < num_players; j++) {
        if (i != j && players[j].alive) {
          enemies[enemy_count++] = players[j];
        }
      }

      if (i == 0) {
        // Player 0: 今はまだSilver戦略 (= player_strategy)
        // 後でここをMinimaxに変える
        moves[i] = player_strategy(players[i], enemies, enemy_count, field);
      } else {
        // Enemy: Silver戦略に強化 (player_strategyを流用)
        moves[i] = player_strategy(players[i], enemies, enemy_count, field);
      }
    }

    // 移動を適用
    for (int i = 0; i < num_players; i++) {
      if (players[i].alive) {
        apply_move(players[i], moves[i], field);
      }
    }

    // ターン終了後の状態表示
    bool should_print = false;
    if (verbose_level == 1 && turn % 10 == 0)
      should_print = true;
    if (verbose_level >= 2)
      should_print = true;

    if (should_print) {
      print_field(field, players, num_players, turn);

      if (verbose_level >= 3) {
        ComponentAnalysis analysis =
            analyze_components(field, players, num_players);
        visualize_components(analysis, players, num_players,
                             "Current Components");

        // プレイヤー0の選択肢を分析
        if (players[0].alive) {
          analyze_move_options(field, players, num_players, 0);
        }
      }
    }
  }

  // 結果表示
  cout << "=== Game Over (Turn " << turn << ") ===" << endl;
  int winner = -1;
  for (int i = 0; i < num_players; i++) {
    if (players[i].alive) {
      winner = i;
      cout << "Player " << i << ": WINNER" << endl;
    } else {
      cout << "Player " << i << ": ELIMINATED" << endl;
    }
  }

  if (winner == 0) {
    cout << "\n★ YOUR STRATEGY WON! ★" << endl;
  } else if (winner >= 0) {
    cout << "\n✗ Enemy AI won." << endl;
  } else {
    cout << "\n- Draw (all eliminated)" << endl;
  }
  cout << endl;

  // 最終状態表示
  if (verbose_level >= 1) {
    print_field(field, players, num_players, turn);

    if (verbose_level >= 3) {
      ComponentAnalysis final_analysis =
          analyze_components(field, players, num_players);
      visualize_components(final_analysis, players, num_players,
                           "Final Components");
    }
  }
}

int main(int argc, char *argv[]) {
  srand(time(NULL));

  // 引数: プレイヤー数, 最大ターン数, 詳細表示レベル
  int num_players = 2;
  int max_turns = 1000;
  int verbose_level = 0;

  if (argc > 1) {
    num_players = atoi(argv[1]);
    if (num_players < 2)
      num_players = 2;
    if (num_players > MAX_PLAYERS)
      num_players = MAX_PLAYERS;
  }

  if (argc > 2) {
    max_turns = atoi(argv[2]);
    if (max_turns < 1)
      max_turns = 1000;
  }

  if (argc > 3) {
    verbose_level = atoi(argv[3]);
    if (verbose_level < 0)
      verbose_level = 0;
  }

  cout << "Tron Battle Simulator with Component Analysis" << endl;
  cout << "Field: " << WIDTH << " x " << HEIGHT << endl;
  cout << "Players: " << num_players
       << " (Player 0 = YOUR STRATEGY, Others = Enemy AI)" << endl;
  cout << "Max Turns: " << max_turns << endl;
  cout << "Verbose Level: " << verbose_level
       << " (0=result only, 1=every 10 turns, 2=every turn, 3=with component "
          "analysis)"
       << endl;
  cout << endl;

  simulate_game(num_players, max_turns, verbose_level);

  return 0;
}
