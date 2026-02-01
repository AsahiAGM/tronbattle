#include <iostream>
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
    bool is_cut_win;      // 敵を分断し、自分が有利
    bool is_near_cut_win; // 次の手でCut Winできる
    int my_comp_size;     // 自分の成分サイズ
    bool is_isolated;     // 敵がいない成分
    int voronoi_score;    // 支配領域サイズ
  } evals[4];

  for (int i = 0; i < 4; i++) {
    evals[i].dir = directions[i];
    evals[i].valid = false;
    evals[i].is_cut_win = false;
    evals[i].is_near_cut_win = false;
    evals[i].is_isolated = false;
    evals[i].my_comp_size = 0;
    evals[i].voronoi_score = 0;

    int nx = player.x1 + dx[i];
    int ny = player.y1 + dy[i];

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
    // 連結成分分析
    // ----------------------------------------------------------------
    int original_val = virtual_field[nx][ny];
    virtual_field[nx][ny] = 1; // 一時的に壁にする

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

    // 敵がどの成分にいるか確認（修正：敵位置は壁なので、敵の「隣接マス」の成分を見る）
    bool comp_has_enemy[600] = {false};
    int max_enemy_comp_size = 0;

    for (size_t j = 0; j < enemy_positions.size(); j++) {
      int ex = enemy_positions[j].first;
      int ey = enemy_positions[j].second;

      // 敵の周囲4方向をチェック
      for (int d = 0; d < 4; d++) {
        int tx = ex + dx[d];
        int ty = ey + dy[d];
        if (tx < 0 || tx >= WIDTH || ty < 0 || ty >= HEIGHT)
          continue;

        int eid = comp_map[tx][ty];
        if (eid != -1) {
          comp_has_enemy[eid] = true;
          if (comp_sizes[eid] > max_enemy_comp_size) {
            max_enemy_comp_size = comp_sizes[eid];
          }
        }
      }
    }

    bool enemy_in_my_comp = false;
    if (my_max_comp_id != -1 && comp_has_enemy[my_max_comp_id]) {
      enemy_in_my_comp = true;
    }

    evals[i].my_comp_size = my_max_comp_size;
    if (my_max_comp_id != -1) {
      if (!enemy_in_my_comp) {
        evals[i].is_isolated = true;
        // 敵がいない成分なら第三層
        // ただし、Cut Win判定は「自分が敵より有利なサイズを持っているか」
        // 自分がIsolatedで、かつ自分のサイズが敵の最大サイズより大きければ勝ち確
        if (my_max_comp_size > max_enemy_comp_size) {
          evals[i].is_cut_win = true;
        }
      } else {
        // 敵と同じ成分にいる -> 分断も隔離もできていない
        // 第一層(Voronoi)へ
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
            is_near_cut_win = true;
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

    virtual_field[nx][ny] = original_val;
  }

  // Selection
  Direction best_dir = UP;
  int max_score = -1;
  bool found = false;
  for (int i = 0; i < 4; i++) {
    if (!evals[i].valid)
      continue;
    found = true;
    int current_score = 0;
    if (evals[i].is_cut_win) {
      // 勝ち確：圧倒的スコア
      current_score = 100000 + evals[i].my_comp_size;
    } else if (evals[i].is_near_cut_win) {
      // リーチ：準圧倒的スコア
      current_score = 50000 + evals[i].my_comp_size;
    } else if (evals[i].is_isolated) {
      // 隔離エリア：確定生存数
      current_score = evals[i].my_comp_size;
    } else {
      // 敵ありエリア：ボロノイ領域（推定生存数）
      current_score = evals[i].voronoi_score;
    }
    if (current_score > max_score) {
      max_score = current_score;
      best_dir = evals[i].dir;
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
