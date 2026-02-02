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
// 修正版mk2_3: 連結成分判定を追加
struct TerritoryResult {
  int owned_cells[4];
  int total_reachable[4];
  int degree_sum[4]; // 領域の「太さ・構造」の評価指標
  bool shared[4][4]; // プレイヤーiとjが同じ領域を共有しているか
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

  TerritoryResult res = {{0}, {0}, {0}, {{false}}};

  // 連結成分判定: 共通の到達可能マスがあるかチェック
  for (int i = 0; i < n; i++) {
    for (int j = i + 1; j < n; j++) {
      if (!players[i].alive || !players[j].alive)
        continue;
      bool is_shared = false;
      // 高速化のため、全走査を一回やるか、ここでbreakを入れる
      for (int x = 0; x < WIDTH; x++) {
        for (int y = 0; y < HEIGHT; y++) {
          if (dist[i][x][y] < 10000 && dist[j][x][y] < 10000) {
            is_shared = true;
            goto found_shared;
          }
        }
      }
    found_shared:
      res.shared[i][j] = res.shared[j][i] = is_shared;
    }
  }

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

// 独立時の生存用BFSスコア（mk2_2から移植）
// 隣接点ごとにBFSを行い、最大の連結成分サイズとその深さを返す
pair<int, int> get_survival_score(int field[WIDTH][HEIGHT], int x, int y) {
  int dx[] = {0, 0, -1, 1}, dy[] = {-1, 1, 0, 0};

  // Check valid neighbors
  vector<pair<int, int>> neighbors;
  for (int d = 0; d < 4; d++) {
    int nx = x + dx[d], ny = y + dy[d];
    if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT && field[nx][ny] == 0) {
      neighbors.push_back({nx, ny});
    }
  }

  if (neighbors.empty())
    return {0, 0};

  pair<int, int> best_res = {0, 0};
  long long best_val = -1;

  for (auto start : neighbors) {
    int dist[WIDTH][HEIGHT];
    for (int i = 0; i < WIDTH; i++)
      for (int j = 0; j < HEIGHT; j++)
        dist[i][j] = -1;

    queue<pair<int, int>> q;
    q.push(start);
    dist[start.first][start.second] = 0;

    int count = 0;
    int max_d = 0;

    while (!q.empty()) {
      pair<int, int> curr = q.front();
      q.pop();
      count++;
      int d = dist[curr.first][curr.second];
      if (d > max_d)
        max_d = d;

      for (int dir = 0; dir < 4; dir++) {
        int nx2 = curr.first + dx[dir], ny2 = curr.second + dy[dir];
        if (nx2 >= 0 && nx2 < WIDTH && ny2 >= 0 && ny2 < HEIGHT &&
            field[nx2][ny2] == 0 && dist[nx2][ny2] == -1) {
          dist[nx2][ny2] = d + 1;
          q.push({nx2, ny2});
        }
      }
    }

    long long val = (long long)count * 1000000 + max_d;
    if (val > best_val) {
      best_val = val;
      best_res = {count, max_d};
    }
  }
  return best_res;
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
    long long max_eval = -9000000000000000000LL; // 最小値をより低く設定

    cerr << "--- Gold MK2.3 Strategy ---" << endl;

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

      // テリトリー計算 (Voronoi & Connectivity)
      TerritoryResult t = calculate_territory(field, n, temp_players);

      bool isolated = true;
      for (int i = 0; i < n; i++) {
        if (i != p && players[i].alive) {
          if (t.shared[p][i]) {
            isolated = false;
            break;
          }
        }
      }

      long long eval = 0;

      if (isolated) {
        // 孤立している場合、BFS生存戦略を使用
        pair<int, int> score = get_survival_score(field, nx, ny);
        // Voronoiのスコアよりも優先されるように非常に大きな値を加算する
        // score.first = count (max 600), score.second = max_dist
        // max val approx 600 * 1M = 600M.
        // Voronoi eval is around 600 * 10000 = 6M.
        // Isolation is safe, so prioritize safety.
        eval = (long long)score.first * 10000000 + score.second * 1000;
        eval += 1000000000000LL; // Isolation bonus (Ensure it's picked over
                                 // connected risky moves?) Actually, if
                                 // isolated, we assume we are safe from
                                 // enemies, so we just want to maximize space.
      } else {
        // 敵と共存している場合 (Standard Voronoi)
        long long my_territory = t.owned_cells[p];
        long long my_reach = t.total_reachable[p];
        long long my_structure = t.degree_sum[p];

        // 連結成分による評価を追加 (分断時の最大領域確保)
        pair<int, int> suv_score = get_survival_score(field, nx, ny);
        long long max_comp_size = suv_score.first;

        long long max_enemy_territory = 0;
        for (int i = 0; i < n; i++) {
          if (i != p && players[i].alive) {
            // 敵の領土評価: 確定領土を重視
            max_enemy_territory =
                max(max_enemy_territory, (long long)t.owned_cells[i]);
          }
        }

        eval = my_territory * 10000 + my_structure * 10 -
               max_enemy_territory * 5000;

        // 分断が発生している場合（合計到達可能数 > 最大成分サイズ）、
        // または単純に生存領域を最大化するために max_comp_size を加算
        eval += max_comp_size * 100;

        // 詰みチェック
        if (my_territory == 0) {
          if (max_comp_size == 0)
            eval -= 10000000000LL; // 完全に詰み
          else if (max_comp_size < 10)
            eval -= 1000000000LL; // かなり狭い
        }
      }

      cerr << "Dir: " << direction_to_string(d) << " Iso:" << isolated
           << " Eval:" << eval << endl;

      if (eval > max_eval) {
        max_eval = eval;
        best_move = d;
      } else if (eval == max_eval) {
        // 同点なら壁沿い（Hug Wall）を優先 (スペース節約)
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

    if (best_move != -1) {
      cout << direction_to_string(best_move) << endl;
    } else {
      // フォールバック: 生存重視のBFSで最善の手を探す
      // (メインループの評価で全て負の値などになった場合のエマージェンシー)
      int best_d = -1;
      long long best_s = -1;
      for (int d = 0; d < 4; d++) {
        int nx = players[p].x1 + dx[d], ny = players[p].y1 + dy[d];
        if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT ||
            field[nx][ny] != 0)
          continue;

        field[nx][ny] = p + 1;
        pair<int, int> score = get_survival_score(field, nx, ny);
        field[nx][ny] = 0;

        long long s = (long long)score.first * 1000000 + score.second;
        if (s > best_s) {
          best_s = s;
          best_d = d;
        }
      }
      if (best_d != -1)
        cout << direction_to_string(best_d) << endl;
      else
        cout << "UP" << endl;
    }
  }
  return 0;
}
