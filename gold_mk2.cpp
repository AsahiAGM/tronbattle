#include <algorithm>
#include <iostream>
#include <queue>
#include <string>
#include <vector>

using namespace std;

const int WIDTH = 30;
const int HEIGHT = 20;

struct Player {
  int x0, y0, x1, y1;
  bool alive;
};

string direction_to_string(int dir) {
  if (dir == 0)
    return "UP";
  if (dir == 1)
    return "DOWN";
  if (dir == 2)
    return "LEFT";
  if (dir == 3)
    return "RIGHT";
  return "UP";
}

// 敵対的領土（Voronoi）計算
// 各プレイヤーから全マスへの最短手数を計算し、誰が最も早く到達できるかを判定する
struct TerritoryResult {
  int owned_cells[4];
  int total_reachable[4];
  int degree_sum[4]; // 領域の「太さ・構造」の評価指標
};

TerritoryResult calculate_territory(int field[WIDTH][HEIGHT], int n,
                                    Player players[]) {
  int dist[4][WIDTH][HEIGHT];
  for (int i = 0; i < 4; i++) {
    for (int x = 0; x < WIDTH; x++) {
      for (int y = 0; y < HEIGHT; y++) {
        dist[i][x][y] = 10000;
      }
    }
  }

  int dx[] = {0, 0, -1, 1};
  int dy[] = {-1, 1, 0, 0};

  for (int i = 0; i < n; i++) {
    if (!players[i].alive)
      continue;
    queue<pair<int, int>> q;
    q.push({players[i].x1, players[i].y1});
    dist[i][players[i].x1][players[i].y1] = 0;

    while (!q.empty()) {
      pair<int, int> curr = q.front();
      q.pop();
      int d = dist[i][curr.first][curr.second];

      for (int dir = 0; dir < 4; dir++) {
        int nx = curr.first + dx[dir];
        int ny = curr.second + dy[dir];
        if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT &&
            field[nx][ny] == 0) {
          if (dist[i][nx][ny] > d + 1) {
            dist[i][nx][ny] = d + 1;
            q.push({nx, ny});
          }
        }
      }
    }
  }

  TerritoryResult res = {{0}, {0}, {0}};
  for (int x = 0; x < WIDTH; x++) {
    for (int y = 0; y < HEIGHT; y++) {
      if (field[x][y] != 0)
        continue;

      int min_d = 10000;
      int closest_p = -1;
      int tie_count = 0;

      for (int i = 0; i < n; i++) {
        if (!players[i].alive)
          continue;
        if (dist[i][x][y] < min_d) {
          min_d = dist[i][x][y];
          closest_p = i;
          tie_count = 1;
        } else if (dist[i][x][y] == min_d) {
          tie_count++;
        }
      }

      if (closest_p != -1) {
        // 誰かが到達可能なマス
        for (int i = 0; i < n; i++) {
          if (dist[i][x][y] < 10000)
            res.total_reachable[i]++;
        }

        // 単独で最も近いプレイヤーがそのタイルを支配（テリトリー）
        if (tie_count == 1) {
          res.owned_cells[closest_p]++;
          // 周囲の空きマス数（degree）を構造の強さとして加算
          for (int dir = 0; dir < 4; dir++) {
            int nx = x + dx[dir], ny = y + dy[dir];
            if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT &&
                field[nx][ny] == 0) {
              res.degree_sum[closest_p]++;
            }
          }
        }
      }
    }
  }
  return res;
}

int main() {
  int field[WIDTH][HEIGHT];
  for (int x = 0; x < WIDTH; x++)
    for (int y = 0; y < HEIGHT; y++)
      field[x][y] = 0;
  Player players[4];
  for (int i = 0; i < 4; i++)
    players[i].alive = false;

  while (1) {
    int n, p;
    if (!(cin >> n >> p))
      break;

    for (int i = 0; i < n; i++) {
      int x0, y0, x1, y1;
      cin >> x0 >> y0 >> x1 >> y1;
      if (x0 == -1) {
        if (players[i].alive) {
          for (int x = 0; x < WIDTH; x++)
            for (int y = 0; y < HEIGHT; y++)
              if (field[x][y] == i + 1)
                field[x][y] = 0;
          players[i].alive = false;
        }
      } else {
        players[i].alive = true;
        field[x0][y0] = i + 1;
        field[x1][y1] = i + 1;
        players[i].x0 = x0;
        players[i].y0 = y0;
        players[i].x1 = x1;
        players[i].y1 = y1;
      }
    }

    int dx[] = {0, 0, -1, 1};
    int dy[] = {-1, 1, 0, 0};

    int best_move = -1;
    long long max_eval = -2000000000000000LL;

    cerr << "--- Adversarial Thinking (Voronoi & Structure) ---" << endl;

    for (int d = 0; d < 4; d++) {
      int nx = players[p].x1 + dx[d];
      int ny = players[p].y1 + dy[d];

      if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT || field[nx][ny] != 0)
        continue;

      // 1手シミュレーション：自分の移動
      field[nx][ny] = p + 1;
      Player temp_players[4];
      for (int i = 0; i < 4; i++)
        temp_players[i] = players[i];
      temp_players[p].x1 = nx;
      temp_players[p].y1 = ny;

      // テリトリー計算
      TerritoryResult t = calculate_territory(field, n, temp_players);

      long long my_territory = t.owned_cells[p];
      long long my_reach = t.total_reachable[p];
      long long my_structure = t.degree_sum[p];

      long long max_enemy_territory = 0;
      for (int i = 0; i < n; i++) {
        if (i != p && players[i].alive) {
          max_enemy_territory =
              max(max_enemy_territory, (long long)t.owned_cells[i]);
        }
      }

      // 評価関数：
      // 1. 領土数（Voronoi）: 最優先。敵が到達できない「確定領土」。
      // 2. 構造の強さ（Degree）: 一本道や袋小路を避け、選択肢が多い形を好む。
      // 3. 到達可能性（Reach）:
      // 自分だけが使える場所だけでなく、競争の余地がある場所。
      // 4. 敵領土の抑止: ライバルの最大領土を削る。

      long long eval =
          my_territory * 10000 + my_structure * 10 - max_enemy_territory * 5000;

      // 詰み（領土なし）は致命的
      if (my_territory == 0) {
        if (my_reach == 0)
          eval -= 10000000000LL; // 完全に詰み
        else
          eval -= 1000000000LL; // かなり狭い
      }

      cerr << "Dir: " << direction_to_string(d) << " Terr:" << my_territory
           << " Struct:" << my_structure << " EnTerr:" << max_enemy_territory
           << " Eval:" << eval << endl;

      if (eval > max_eval) {
        max_eval = eval;
        best_move = d;
      } else if (eval == max_eval) {
        // 同点なら壁沿い（Hug Wall）を優先
        auto get_walls = [&](int x, int y) {
          int count = 0;
          for (int i = 0; i < 4; i++) {
            int tx = x + dx[i], ty = y + dy[i];
            if (tx < 0 || tx >= WIDTH || ty < 0 || ty >= HEIGHT ||
                field[tx][ty] != 0)
              count++;
          }
          return count;
        };
        if (best_move == -1 ||
            get_walls(nx, ny) > get_walls(players[p].x1 + dx[best_move],
                                          players[p].y1 + dy[best_move])) {
          best_move = d;
        }
      }

      field[nx][ny] = 0; // 戻す
    }

    if (best_move != -1)
      cout << direction_to_string(best_move) << endl;
    else
      cout << "UP" << endl;
  }
  return 0;
}
