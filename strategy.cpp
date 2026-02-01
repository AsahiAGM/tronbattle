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

void field_update(int field[WIDTH][HEIGHT], Tronbike &player, Tronbike enemy[],
                  int num_enemies) {
  field[player.x1][player.y1] = 1;
  for (int i = 0; i < num_enemies; i++) {
    if (enemy[i].x1 >= 0) {
      field[enemy[i].x1][enemy[i].y1] = 2;
    }
  }
}

/*
 * ★★★ 階層的戦略 ★★★
 * 第二層：分断戦略（Cut Strategy）- 分断して勝てる手を最優先
 * 第三層：生存戦略（Isolation Strategy）- 敵がいない成分で最長生存
 * 第一層：領域戦略（Voronoi Strategy）- 敵と同じ成分なら支配領域最大化
 */
Direction get_best_move(int field[WIDTH][HEIGHT], Tronbike &player,
                        Tronbike enemy[], int num_enemies) {
  int dx[] = {0, 0, -1, 1};
  int dy[] = {-1, 1, 0, 0};
  Direction directions[] = {UP, DOWN, LEFT, RIGHT};

  // 敵の位置リスト
  vector<pair<int, int>> enemy_positions;
  for (int i = 0; i < num_enemies; i++) {
    if (enemy[i].x1 >= 0) {
      enemy_positions.push_back(make_pair(enemy[i].x1, enemy[i].y1));
    }
  }

  // 仮想フィールド
  int virtual_field[WIDTH][HEIGHT];
  for (int x = 0; x < WIDTH; x++) {
    for (int y = 0; y < HEIGHT; y++) {
      virtual_field[x][y] = field[x][y];
    }
  }
  virtual_field[player.x1][player.y1] = 1;

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

    int nx = player.x1 + dx[i];
    int ny = player.y1 + dy[i];

    // 範囲・衝突チェック
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

    evals[i].valid = true;

    // ----------------------------------------------------------------
    // 連結成分分析 (第二層 & 第三層)
    // ----------------------------------------------------------------
    // この一手を打った後の盤面でFlood Fillを行う

    // 一時的に壁にする
    int original_val = virtual_field[nx][ny];
    virtual_field[nx][ny] = 1;

    // 成分IDマップ
    int comp_map[WIDTH][HEIGHT];
    for (int x = 0; x < WIDTH; ++x)
      for (int y = 0; y < HEIGHT; ++y)
        comp_map[x][y] = -1;

    int my_comp_id = -1;
    int comp_sizes[600] = {0}; // 成分IDごとのサイズ
    int comp_count = 0;

    // 自分が移動した後の nx, ny は壁扱い。
    // 隣接4方向のうち、空いているマスを始点として成分分解する

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

    // 自分が移動した後、隣接する最大の成分を探す (それが自分の生存可能領域)
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
        // 敵がいない成分なら、第三層
        // もしここが自分の成分サイズ > 敵の成分サイズ なら 勝ち確 (Cut Win)
        if (my_max_comp_size > max_enemy_comp_size) {
          evals[i].is_cut_win = true;
        }
      }
    } else {
      // 自分が動くと周りが全部壁（詰み）
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
          // 自分の位置nx,nyも壁扱い
          if (tx == nx && ty == ny)
            continue;

          bool is_ep = false; // 敵位置はスルー
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
  // 1. Cut Win (MySize > EnemySize) 最大
  Direction best_dir = UP;
  int max_val = -1;
  bool found = false;

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

  // 2. Isolated (敵なし) MySize 最大
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

  // 3. Voronoi Score 最大
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
        enemy[enemy_count].y1 = y1; // 重複?
        enemy_count++;
      }
    }

    field_update(field, player, enemy, enemy_count);

    Direction move = get_best_move(field, player, enemy, enemy_count);

    cout << direction_to_string(move) << endl;
  }

  return 0;
}
