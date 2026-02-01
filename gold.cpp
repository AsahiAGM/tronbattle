#include <iostream>
#include <set>
#include <string>
#include <vector>

using namespace std;

/**
 * Auto-generated code below aims at helping you parse
 * the standard input according to the problem statement.
 **/

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

void field_update(int field[WIDTH][HEIGHT], int n, Tronbike players[]) {
  // 生きているプレイヤーの現在位置を壁にする
  for (int i = 0; i < n; i++) {
    if (players[i].x1 >= 0) { // Alive
      // IDに対応する値 (10 + i) をセット
      field[players[i].x1][players[i].y1] = 10 + i;

      // 初期位置も念のためセット (Turn 0対策)
      field[players[i].x0][players[i].y0] = 10 + i;
    }
  }
}

void clear_dead_players(int field[WIDTH][HEIGHT], int n, Tronbike players[]) {
  // 死んだプレイヤーの壁を消去
  for (int i = 0; i < n; i++) {
    if (players[i].x1 == -1) { // Dead
      int val = 10 + i;
      for (int x = 0; x < WIDTH; x++) {
        for (int y = 0; y < HEIGHT; y++) {
          if (field[x][y] == val) {
            field[x][y] = 0;
          }
        }
      }
    }
  }
}

/*
 * ★★★ 階層的戦略 ★★★
 * 第二層：分断戦略（Cut Strategy）- 分断して勝てる手を最優先
 * 第三層：生存戦略（Isolation Strategy）- 敵がいない成分で最長生存
 * 第一層：領域戦略（Voronoi Strategy）- 敵と同じ成分なら支配領域最大化
 */
// get_best_move のシグネチャ変更に注意 (enemy配列 -> players配列)
// ★最長経路（延命手）の近似シミュレーション★
// 指定された位置からWarnsdorff's
// Heuristicを用いてシミュレーションし、生き残れる手数を返す。
int count_survival_steps(int field[WIDTH][HEIGHT], int start_x, int start_y) {
  int f[WIDTH][HEIGHT];
  for (int x = 0; x < WIDTH; x++) {
    for (int y = 0; y < HEIGHT; y++) {
      f[x][y] = field[x][y];
    }
  }

  int cx = start_x;
  int cy = start_y;
  f[cx][cy] = 1; // 自分の壁
  int steps = 0;
  int dx[] = {0, 0, -1, 1};
  int dy[] = {-1, 1, 0, 0};

  while (true) {
    int best_d = -1;
    int min_degree = 5;

    for (int d = 0; d < 4; d++) {
      int nx = cx + dx[d];
      int ny = cy + dy[d];
      if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT || f[nx][ny] != 0)
        continue;

      // 次数を計算
      int degree = 0;
      for (int d2 = 0; d2 < 4; d2++) {
        int nnx = nx + dx[d2];
        int nny = ny + dy[d2];
        if (nnx < 0 || nnx >= WIDTH || nny < 0 || nny >= HEIGHT ||
            f[nnx][nny] != 0)
          continue;
        degree++;
      }

      if (degree < min_degree) {
        min_degree = degree;
        best_d = d;
      } else if (degree == min_degree && best_d != -1) {
        // タイブレーカー: 壁際を優先
        auto get_walls = [&](int x, int y) {
          int w = 0;
          for (int i = 0; i < 4; i++) {
            int tx = x + dx[i], ty = y + dy[i];
            if (tx < 0 || tx >= WIDTH || ty < 0 || ty >= HEIGHT ||
                f[tx][ty] != 0)
              w++;
          }
          return w;
        };
        if (get_walls(nx, ny) > get_walls(cx + dx[best_d], cy + dy[best_d])) {
          best_d = d;
        }
      }
    }

    if (best_d == -1)
      break;
    cx += dx[best_d];
    cy += dy[best_d];
    f[cx][cy] = 1;
    steps++;
  }
  return steps;
}

Direction get_best_move(int field[WIDTH][HEIGHT], int my_id, int n,
                        Tronbike players[]) {
  int dx[] = {0, 0, -1, 1};
  int dy[] = {-1, 1, 0, 0};
  Direction directions[] = {UP, DOWN, LEFT, RIGHT};

  Tronbike &player = players[my_id];

  // 敵の位置リスト
  vector<pair<int, int>> enemy_positions;
  for (int i = 0; i < n; i++) {
    if (i != my_id && players[i].x1 >= 0) {
      enemy_positions.push_back(make_pair(players[i].x1, players[i].y1));
    }
  }

  // 仮想フィールド
  int virtual_field[WIDTH][HEIGHT];
  for (int x = 0; x < WIDTH; x++) {
    for (int y = 0; y < HEIGHT; y++) {
      virtual_field[x][y] = field[x][y];
    }
  }
  virtual_field[player.x1][player.y1] = 1; // 自分は壁

  struct Eval {
    Direction dir;
    bool valid;
    bool is_cut_win;
    bool is_near_cut_win;
    int my_comp_size;
    bool is_isolated;
    bool is_defeat; // 分断負け（敵より小さい領域に隔離された）
    int voronoi_score;
    bool is_danger;     // 敵の隣接マス（衝突リスクあり）
    int distance_score; // 敵との距離スコア (New for N>=3)
    int final_score;    // 最終評価スコア
    int survival_steps; // シミュレーションされた生存手数
    int enemy_count;    // 領域内の敵の数
  } evals[4];

  for (int i = 0; i < 4; i++) {
    evals[i].dir = directions[i];
    evals[i].valid = false;
    evals[i].is_cut_win = false;
    evals[i].is_near_cut_win = false;
    evals[i].is_isolated = false;
    evals[i].is_defeat = false;
    evals[i].is_danger = false;
    evals[i].my_comp_size = 0;
    evals[i].voronoi_score = 0;
    evals[i].final_score = -999999;
    evals[i].distance_score = 0;
    evals[i].enemy_count = 0;

    int nx = player.x1 + dx[i];
    int ny = player.y1 + dy[i];

    if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT)
      continue;

    // 衝突チェック
    // 敵の隣接マスチェック (マンハッタン距離1)
    bool is_enemy_adjacent = false;
    for (size_t j = 0; j < enemy_positions.size(); j++) {
      int ex = enemy_positions[j].first;
      int ey = enemy_positions[j].second;
      if (abs(nx - ex) + abs(ny - ey) == 1) {
        is_enemy_adjacent = true;
      }
    }

    // 壁なら即座に不可
    if (virtual_field[nx][ny] != 0)
      continue;

    evals[i].valid = true;
    if (is_enemy_adjacent)
      evals[i].is_danger = true;

    int original_val = virtual_field[nx][ny];
    // ----------------------------------------------------------------
    // 連結成分分析
    // ----------------------------------------------------------------
    // int original_val = virtual_field[nx][ny]; // This line was duplicated,
    // removed the original one.
    virtual_field[nx][ny] = 1; // 一時的に壁にする

    // ★生存チェック: その先に行き場があるか？★
    // 1手進んだ後、さらに1手進める場所がなければ即死（行き止まり）
    bool has_future_move = false;
    for (int d2 = 0; d2 < 4; d2++) {
      int nnx = nx + dx[d2];
      int nny = ny + dy[d2];
      if (nnx < 0 || nnx >= WIDTH || nny < 0 || nny >= HEIGHT)
        continue;

      // 敵の予測位置チェックまですると重いので、まずは静的な壁チェック
      // virtual_fieldには既に自分の現在位置も壁として入っている

      bool is_ep_next = false;
      for (size_t j = 0; j < enemy_positions.size(); j++) {
        if (nnx == enemy_positions[j].first &&
            nny == enemy_positions[j].second) {
          is_ep_next = true;
          break;
        }
      }

      if (virtual_field[nnx][nny] == 0 && !is_ep_next) {
        has_future_move = true;
        break;
      }
    }

    if (!has_future_move) {
      // 行き止まり確定なので、この手は無効
      evals[i].valid = false;
      virtual_field[nx][ny] = original_val; // 元に戻す
      continue;
    }

    int comp_map[WIDTH][HEIGHT];
    for (int x = 0; x < WIDTH; ++x)
      for (int y = 0; y < HEIGHT; ++y)
        comp_map[x][y] = -1;

    int comp_sizes[600] = {0};
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
          continue;
        if (comp_map[sx][sy] != -1)
          continue;

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

    // 領域内の敵の数をカウント
    int comp_enemy_count[600] = {0};
    int max_enemy_comp_size = 0;

    for (size_t j = 0; j < enemy_positions.size(); j++) {
      int ex = enemy_positions[j].first;
      int ey = enemy_positions[j].second;

      // 1人の敵が複数の成分に接している可能性を考慮。
      // ただし、「その敵が次にどの成分に入りうるか」が重要。
      set<int> touching_comps;
      for (int d = 0; d < 4; d++) {
        int tx = ex + dx[d];
        int ty = ey + dy[d];
        if (tx < 0 || tx >= WIDTH || ty < 0 || ty >= HEIGHT)
          continue;
        int eid = comp_map[tx][ty];
        if (eid != -1) {
          touching_comps.insert(eid);
        }
      }
      for (int cid : touching_comps) {
        comp_enemy_count[cid]++;
        if (comp_sizes[cid] > max_enemy_comp_size) {
          max_enemy_comp_size = comp_sizes[cid];
        }
      }
    }

    if (my_max_comp_id != -1) {
      int enemies = comp_enemy_count[my_max_comp_id];
      evals[i].enemy_count = enemies;

      // ★実効サイズ（Effective Size）★
      // N>=3なら混戦を避けるため人数で割る。N=2なら単純サイズ。
      if (n >= 3) {
        evals[i].my_comp_size = comp_sizes[my_max_comp_id] / (enemies + 1);
      } else {
        evals[i].my_comp_size = comp_sizes[my_max_comp_id];
      }

      evals[i].is_isolated = (enemies == 0);

      if (evals[i].is_isolated) {
        // 孤立成分
        // Cut Win判定（自分が敵の最大領域より大きいか）
        if (evals[i].my_comp_size >= max_enemy_comp_size) {
          if (n >= 3) {
            if (evals[i].my_comp_size > 200 || max_enemy_comp_size < 30) {
              evals[i].is_cut_win = true;
            }
          } else {
            evals[i].is_cut_win = true;
          }
        } else {
          evals[i].is_defeat = true;
        }
      } else {
        // 敵と同じ成分にいる -> 混戦領域
        // 実効サイズが敵の最大サイズより圧倒的に小さければ、いずれ敗北する
        // (is_defeat)
        if (n >= 3 && evals[i].my_comp_size < max_enemy_comp_size * 0.4) {
          evals[i].is_defeat = true;
        }
      }
    } else {
      evals[i].my_comp_size = 0;
    }

    // ----------------------------------------------------------------
    // 2手読み: 次の手でCut Winできるか？ (Near Cut Win)
    // ----------------------------------------------------------------
    bool is_near_cut_win = false;

    if (!evals[i].is_cut_win && !evals[i].is_isolated) {
      // 現在の1手目 (nx, ny) はすでに virtual_field で壁になっている
      // ここからさらに1手進めてみる
      for (int d2 = 0; d2 < 4; d2++) {
        int nnx = nx + dx[d2];
        int nny = ny + dy[d2];

        if (nnx < 0 || nnx >= WIDTH || nny < 0 || nny >= HEIGHT)
          continue;

        // 衝突チェック (2手目)
        bool is_ep2 = false;
        for (size_t j = 0; j < enemy_positions.size(); j++) {
          if (nnx == enemy_positions[j].first &&
              nny == enemy_positions[j].second) {
            is_ep2 = true;
            break;
          }
        }
        if (virtual_field[nnx][nny] != 0 && !is_ep2)
          continue;

        // 2手目を仮定
        int original_val2 = virtual_field[nnx][nny];
        virtual_field[nnx][nny] = 1;

        // 成分分析 (軽量版: 成分分解してMySize > EnemyMax だけ見る)
        {
          int comp_map2[WIDTH][HEIGHT];
          for (int x = 0; x < WIDTH; ++x)
            for (int y = 0; y < HEIGHT; ++y)
              comp_map2[x][y] = -1;
          int comp_sizes2[600] = {0};
          int comp_count2 = 0;

          // 全マス走査
          for (int sx = 0; sx < WIDTH; ++sx) {
            for (int sy = 0; sy < HEIGHT; ++sy) {
              bool is_ep = false;
              for (size_t j = 0; j < enemy_positions.size(); j++) {
                if (sx == enemy_positions[j].first &&
                    sy == enemy_positions[j].second)
                  is_ep = true;
                break;
              }
              if (!is_ep && virtual_field[sx][sy] != 0)
                continue;
              if (comp_map2[sx][sy] != -1)
                continue;

              int cid = comp_count2++;
              int qx[1000], qy[1000];
              int head = 0, tail = 0;
              qx[tail] = sx;
              qy[tail] = sy;
              tail++;
              comp_map2[sx][sy] = cid;
              int size = 0;
              while (head < tail) {
                int cx = qx[head];
                int cy = qy[head];
                head++;
                size++;
                for (int d3 = 0; d3 < 4; d3++) {
                  int tx = cx + dx[d3];
                  int ty = cy + dy[d3];
                  if (tx < 0 || tx >= WIDTH || ty < 0 || ty >= HEIGHT)
                    continue;
                  if (comp_map2[tx][ty] != -1)
                    continue;
                  bool is_ep3 = false;
                  for (size_t j = 0; j < enemy_positions.size(); j++) {
                    if (tx == enemy_positions[j].first &&
                        ty == enemy_positions[j].second)
                      is_ep3 = true;
                    break;
                  }
                  if (virtual_field[tx][ty] != 0 && !is_ep3)
                    continue;
                  comp_map2[tx][ty] = cid;
                  qx[tail] = tx;
                  qy[tail] = ty;
                  tail++;
                }
              }
              comp_sizes2[cid] = size;
            }
          }

          // Check
          int my_max2 = 0;
          int my_id2 = -1;
          for (int d3 = 0; d3 < 4; d3++) {
            int tx = nnx + dx[d3];
            int ty = nny + dy[d3];
            if (tx < 0 || tx >= WIDTH || ty < 0 || ty >= HEIGHT)
              continue;
            int cid = comp_map2[tx][ty];
            if (cid != -1 && comp_sizes2[cid] > my_max2) {
              my_max2 = comp_sizes2[cid];
              my_id2 = cid;
            }
          }

          // Enemy Check
          int enemy_max2 = 0;
          bool enemy_in_my2 = false;
          // 敵位置隣接チェック
          bool comp_has_enemy2[600] = {false};
          for (size_t j = 0; j < enemy_positions.size(); j++) {
            int ex = enemy_positions[j].first;
            int ey = enemy_positions[j].second;
            for (int d4 = 0; d4 < 4; d4++) {
              int tx = ex + dx[d4];
              int ty = ey + dy[d4];
              if (tx < 0 || tx >= WIDTH || ty < 0 || ty >= HEIGHT)
                continue;
              int cid = comp_map2[tx][ty];
              if (cid != -1) {
                comp_has_enemy2[cid] = true;
                if (comp_sizes2[cid] > enemy_max2)
                  enemy_max2 = comp_sizes2[cid];
              }
            }
          }
          if (my_id2 != -1 && comp_has_enemy2[my_id2])
            enemy_in_my2 = true;

          if (my_id2 != -1 && !enemy_in_my2 && my_max2 > enemy_max2) {
            if (n >= 3) {
              if (my_max2 > 200 || enemy_max2 < 30)
                is_near_cut_win = true;
            } else {
              is_near_cut_win = true;
            }
          }
        }

        // 戻す
        virtual_field[nnx][nny] = original_val2;

        if (is_near_cut_win)
          break; // 1つでもあればOK
      }
    }
    evals[i].is_near_cut_win = is_near_cut_win;

    // Voronoi
    if (!evals[i].is_cut_win && !evals[i].is_isolated) {
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

      // Count Voronoi & Distance
      int score = 0;
      for (int x = 0; x < WIDTH; x++) {
        for (int y = 0; y < HEIGHT; y++) {
          if (my_dist[x][y] == 9999)
            continue;

          if (n >= 3) {
            // 多人数戦: 安全圏（確実な領土）を重視
            // 敵との距離差(margin)が大きいほど価値が高い
            int margin = enemy_dist[x][y] -
                         my_dist[x][y]; // Corrected en_dist to enemy_dist
            if (margin > 0) {
              // 差が2以上なら確実性が高いので高評価
              // 差が1（競合）なら低評価
              if (margin >= 2)
                score += 20;
              else
                score += 5;
            }
          } else {
            // 1vs1: 「安全圏（Security）」の拡大戦略
            // 敵に近い場所（前線）ほど「奪い合い」の価値が高く、そこを「安全圏」に変える動きを最優先する。
            if (my_dist[x][y] < enemy_dist[x][y]) {
              int dist_to_enemy = enemy_dist[x][y];
              int weight = (60 - dist_to_enemy); // 近接地点(60)〜遠隔地点(10)

              score += weight * 10; // 支配領域（基本価値）

              // 安全圏ボーナス (敵より2手以上近い)
              // 係争地(margin 1)を安全圏(margin 2)に変えることで、
              // 自然と「敵の背後（以前は係争地だった場所）」を安全圏として取り込む。
              if (enemy_dist[x][y] - my_dist[x][y] >= 2) {
                score += weight * 20;
              }
            }
          }
        }
      }

      evals[i].voronoi_score = score;

      // 敵との距離も評価
      // n >= 3: 逃げるために使う (Distance大 -> Score大)
      // n == 2: 攻めるために使う (Distance小 -> Score大, 後段で処理)
      int min_dist = 9999;
      for (size_t j = 0; j < enemy_positions.size(); j++) {
        int d = abs(nx - enemy_positions[j].first) +
                abs(ny - enemy_positions[j].second);
        if (d < min_dist)
          min_dist = d;
      }
      evals[i].distance_score = min_dist;
    } // if (!...is_isolated) end

    virtual_field[nx][ny] = original_val;
  } // for loop end

  // Selection
  Direction best_dir = UP;
  int max_score = -2000000000; // 十分に小さい値で初期化
  bool found = false;

  // ★Safety Filter: 生存領域サイズの最大値を把握★
  int global_max_comp_size = 0;
  for (int i = 0; i < 4; i++) {
    if (evals[i].valid && evals[i].my_comp_size > global_max_comp_size) {
      global_max_comp_size = evals[i].my_comp_size;
    }
  }

  for (int i = 0; i < 4; i++) {
    if (!evals[i].valid)
      continue;
    found = true;
    int current_score = 0;

    // ★Safety Filter: 自滅（袋小路への進入）を絶対回避する★
    if (global_max_comp_size > 0) {
      // 1on1では厳密(80%)、多人数では柔軟(50%)に評価
      double threshold = (n == 2) ? 0.8 : 0.5;
      if (evals[i].my_comp_size < global_max_comp_size * threshold) {
        current_score -= 10000000; // 超特大減点
      }
    }

    if (evals[i].is_defeat) {
      current_score -= 5000000; // 敗北確定は絶対回避
    }

    // ★スコア評価の基本方針: 広さ(生存空間)を第一の評価軸にする★
    // 生存可能な実効マス数 my_comp_size をベーススコアとし、
    // 1マスの減少がボロノイスコアのいかなる優位性よりも重くなるように設定(重み10000)。
    current_score += (long long)evals[i].my_comp_size * 10000;

    // 戦略ボーナス
    if (evals[i].is_cut_win) {
      current_score += 2000000; // 勝ち確最優先
    } else if (evals[i].is_near_cut_win) {
      current_score += 1000000; // リーチ
    } else if (evals[i].is_isolated) {
      // 隔離エリア: シミュレーションによる「正確な手数」を追加評価
      int cur_nx = player.x1 + dx[i];
      int cur_ny = player.y1 + dy[i];
      evals[i].survival_steps =
          count_survival_steps(virtual_field, cur_nx, cur_ny);
      current_score += (long long)evals[i].survival_steps * 10000;
    } else {
      // Voronoi
      current_score += evals[i].voronoi_score;

      // ★多人数戦のアグレッシブ化★
      // 距離を取って逃げる(Distance Score)のをやめ、領域を奪い合う。

      // Hug Wall (修正版: アグレッシブに使うため距離制限撤廃)
      // ただし袋小路(3方壁)は避ける
      int wall_neighbors = 0;
      for (int d = 0; d < 4; d++) {
        int tx = (player.x1 + dx[i]) + dx[d];
        int ty = (player.y1 + dy[i]) + dy[d];
        if (tx < 0 || tx >= WIDTH || ty < 0 || ty >= HEIGHT ||
            field[tx][ty] != 0)
          wall_neighbors++;
      }
      if (wall_neighbors >= 3) {
        current_score -= 50000;
      } else {
        current_score += wall_neighbors * 20; // 強化: 5->20
      }

      if (n >= 3) {
        current_score += evals[i].distance_score * 5;
      }

      // 多人数戦では事故防止のため避ける。
      // 1on1(n=2)では、相手が避けることを前提に強気に攻める（でないと押し負ける）。
      // Danger減点は全廃止（同時着なし、早い者勝ちルールのため）
    }

    evals[i].final_score = current_score; // Save for debug

    if (current_score > max_score) {
      max_score = current_score;
      best_dir = evals[i].dir;
    }
  }

  // ★Debug Output★
  std::cerr << "--- Turn Debug N:" << n << " ---" << std::endl;
  for (int i = 0; i < 4; i++) {
    std::string dstr = direction_to_string(evals[i].dir);
    std::cerr << "Dir: " << dstr << " Valid: " << evals[i].valid
              << " Score: " << (evals[i].valid ? evals[i].final_score : -1)
              << " Voronoi: " << evals[i].voronoi_score
              << " MyCompEffe: " << evals[i].my_comp_size
              << " EnCount: " << evals[i].enemy_count
              << " CutWin: " << evals[i].is_cut_win
              << " Danger: " << evals[i].is_danger << " Surv: "
              << (evals[i].is_isolated ? evals[i].survival_steps : 0)
              << std::endl;
  }
  std::cerr << "Selected: " << direction_to_string(best_dir)
            << " MaxScore: " << max_score << std::endl;

  // 延命戦略: 全ての手がリスクあり(マイナス評価)の場合
  if (max_score < 0) {
    int max_survival = -1;
    Direction backup_dir = UP;
    bool found_safe = false;

    // 1. Dangerでない手の中で最大生存手数を探す
    for (int i = 0; i < 4; i++) {
      if (!evals[i].valid)
        continue;
      if (evals[i].is_danger)
        continue;

      found_safe = true;
      int cur_nx = player.x1 + dx[i];
      int cur_ny = player.y1 + dy[i];
      int survival = count_survival_steps(virtual_field, cur_nx, cur_ny);
      int score = survival * 1000 + evals[i].my_comp_size;

      if (score > max_survival) {
        max_survival = score;
        backup_dir = evals[i].dir;
      }
    }

    // 2. 全部Dangerなら、Dangerの中で最大生存手数を探す
    if (!found_safe) {
      for (int i = 0; i < 4; i++) {
        if (!evals[i].valid)
          continue;

        int cur_nx = player.x1 + dx[i];
        int cur_ny = player.y1 + dy[i];
        int survival = count_survival_steps(virtual_field, cur_nx, cur_ny);
        int score = survival * 1000 + evals[i].my_comp_size;

        if (score > max_survival) {
          max_survival = score;
          backup_dir = evals[i].dir;
        }
      }
    }

    if (max_survival != -1) {
      return backup_dir;
    }
  }

  if (found)
    return best_dir;
  return UP;
}

int main() {
  int field[WIDTH][HEIGHT];
  for (int x = 0; x < WIDTH; x++)
    for (int y = 0; y < HEIGHT; ++y)
      field[x][y] = 0;

  while (1) {
    int n, p;
    cin >> n >> p;
    cin.ignore();

    // 全プレイヤー情報を格納固定 (最大4人)
    Tronbike players[4];

    for (int i = 0; i < n; i++) {
      int x0, y0, x1, y1;
      cin >> x0 >> y0 >> x1 >> y1;
      cin.ignore();

      players[i].x0 = x0;
      players[i].y0 = y0;
      players[i].x1 = x1;
      players[i].y1 = y1;
    }

    // 死んだプレイヤーの壁を除去
    clear_dead_players(field, n, players);

    // 最新の位置を壁に追加
    field_update(field, n, players);

    Direction move = get_best_move(field, p, n, players);
    cout << direction_to_string(move) << endl;
  }
  return 0;
}
