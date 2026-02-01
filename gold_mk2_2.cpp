#include <algorithm>
#include <iostream>
#include <queue>
#include <set>
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

struct TerritoryResult {
  int owned_cells[4];
  int degree_sum[4];
  bool shared_component[4][4];
};

TerritoryResult calculate_territory(int field[WIDTH][HEIGHT], int n,
                                    Player players[]) {
  int dist[4][WIDTH][HEIGHT];
  for (int i = 0; i < 4; i++) {
    for (int x = 0; x < WIDTH; x++)
      for (int y = 0; y < HEIGHT; y++)
        dist[i][x][y] = 10000;
  }
  int dx[] = {0, 0, -1, 1}, dy[] = {-1, 1, 0, 0};
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
        int nx = curr.first + dx[dir], ny = curr.second + dy[dir];
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
  TerritoryResult res = {};
  for (int i = 0; i < 4; i++) {
    res.owned_cells[i] = 0;
    res.degree_sum[i] = 0;
    for (int j = 0; j < 4; j++)
      res.shared_component[i][j] = false;
  }
  for (int x = 0; x < WIDTH; x++) {
    for (int y = 0; y < HEIGHT; y++) {
      if (field[x][y] != 0)
        continue;
      int min_d = 10000, closest_p = -1, tie_cnt = 0;
      for (int i = 0; i < n; i++) {
        if (!players[i].alive)
          continue;
        if (dist[i][x][y] < min_d) {
          min_d = dist[i][x][y];
          closest_p = i;
          tie_cnt = 1;
        } else if (dist[i][x][y] == min_d)
          tie_cnt++;
        for (int j = i + 1; j < n; j++) {
          if (players[j].alive && dist[i][x][y] < 10000 &&
              dist[j][x][y] < 10000) {
            res.shared_component[i][j] = res.shared_component[j][i] = true;
          }
        }
      }
      if (closest_p != -1 && tie_cnt == 1) {
        res.owned_cells[closest_p]++;
        for (int dir = 0; dir < 4; dir++) {
          int nx = x + dx[dir], ny = y + dy[dir];
          if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT &&
              field[nx][ny] == 0)
            res.degree_sum[closest_p]++;
        }
      }
    }
  }
  return res;
}

struct StructureStats {
  int articulation_points;
  int max_lost_on_cut;
};

int tin[WIDTH][HEIGHT], low[WIDTH][HEIGHT], timer;
int dfs_size[WIDTH][HEIGHT];

void dfs_struct(int x, int y, int px, int py, int field[WIDTH][HEIGHT],
                int &art_count, int &max_lost) {
  tin[x][y] = low[x][y] = timer++;
  dfs_size[x][y] = 1;
  int children = 0;
  int dx[] = {0, 0, -1, 1}, dy[] = {-1, 1, 0, 0};
  bool is_art = false;

  for (int i = 0; i < 4; i++) {
    int nx = x + dx[i], ny = y + dy[i];
    if (nx == px && ny == py)
      continue;
    if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT && field[nx][ny] == 0) {
      if (tin[nx][ny] != -1) {
        low[x][y] = min(low[x][y], tin[nx][ny]);
      } else {
        dfs_struct(nx, ny, x, y, field, art_count, max_lost);
        dfs_size[x][y] += dfs_size[nx][ny];
        low[x][y] = min(low[x][y], low[nx][ny]);
        if (low[nx][ny] >= tin[x][y] && px != -1) {
          is_art = true;
          max_lost = max(max_lost, dfs_size[nx][ny]);
        }
        children++;
      }
    }
  }
  if (px == -1 && children > 1)
    is_art = true;
  if (is_art)
    art_count++;
}

StructureStats evaluate_structure(int field[WIDTH][HEIGHT], int start_x,
                                  int start_y) {
  for (int x = 0; x < WIDTH; x++)
    for (int y = 0; y < HEIGHT; y++) {
      tin[x][y] = -1;
      low[x][y] = -1;
      dfs_size[x][y] = 0;
    }
  timer = 0;
  int art_count = 0, max_lost = 0;
  if (start_x >= 0 && field[start_x][start_y] == 0)
    dfs_struct(start_x, start_y, -1, -1, field, art_count, max_lost);
  return {art_count, max_lost};
}

int count_reachable(int field[WIDTH][HEIGHT], int x, int y) {
  int dx[] = {0, 0, -1, 1}, dy[] = {-1, 1, 0, 0};
  bool v[WIDTH][HEIGHT];
  for (int x0 = 0; x0 < WIDTH; x0++)
    for (int y0 = 0; y0 < HEIGHT; y0++)
      v[x0][y0] = false;
  queue<pair<int, int>> q;
  q.push({x, y});
  v[x][y] = true;
  int count = 0;
  while (!q.empty()) {
    pair<int, int> curr = q.front();
    q.pop();
    count++;
    for (int d = 0; d < 4; d++) {
      int nx = curr.first + dx[d], ny = curr.second + dy[d];
      if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT &&
          field[nx][ny] == 0 && !v[nx][ny]) {
        v[nx][ny] = true;
        q.push({nx, ny});
      }
    }
  }
  return count;
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

    int dx[] = {0, 0, -1, 1}, dy[] = {-1, 1, 0, 0};
    int bmove = -1;
    long long max_eval = -8000000000000000000LL;

    for (int d = 0; d < 4; d++) {
      int nx = players[p].x1 + dx[d], ny = players[p].y1 + dy[d];
      if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT || field[nx][ny] != 0)
        continue;

      field[nx][ny] = p + 1;
      Player p_after[4];
      for (int i = 0; i < 4; i++)
        p_after[i] = players[i];
      p_after[p].x1 = nx;
      p_after[p].y1 = ny;

      TerritoryResult t = calculate_territory(field, n, p_after);
      bool in_same = false;
      int max_o_size = 0;
      for (int i = 0; i < n; i++) {
        if (i != p && players[i].alive) {
          if (t.shared_component[p][i])
            in_same = true;
          max_o_size = max(
              max_o_size, count_reachable(field, players[i].x1, players[i].y1));
        }
      }

      int my_size = count_reachable(field, nx, ny);
      long long eval = 0;

      if (!in_same) {
        StructureStats ss = evaluate_structure(field, nx, ny);
        eval = (long long)my_size * 1000000 -
               (long long)ss.articulation_points * 2000 -
               (long long)ss.max_lost_on_cut * 5000;
        if (my_size < max_o_size)
          eval -= 2000000000000LL;
      } else {
        int ce = -1;
        int min_d = 10000;
        for (int i = 0; i < n; i++) {
          if (i != p && players[i].alive && t.shared_component[p][i]) {
            int dist = abs(nx - players[i].x1) + abs(ny - players[i].y1);
            if (dist < min_d) {
              min_d = dist;
              ce = i;
            }
          }
        }
        long long worst_case = 4000000000000000LL;
        bool can_e = false;
        if (ce != -1) {
          for (int ed = 0; ed < 4; ed++) {
            int enx = players[ce].x1 + dx[ed], eny = players[ce].y1 + dy[ed];
            if (enx < 0 || enx >= WIDTH || eny < 0 || eny >= HEIGHT ||
                field[enx][eny] != 0)
              continue;
            can_e = true;
            field[enx][eny] = ce + 1;
            Player p_final[4];
            for (int i = 0; i < 4; i++)
              p_final[i] = p_after[i];
            p_final[ce].x1 = enx;
            p_final[ce].y1 = eny;
            TerritoryResult t2 = calculate_territory(field, n, p_final);
            StructureStats ess = evaluate_structure(field, enx, eny);
            int e_mobility = 0;
            for (int i = 0; i < 4; i++) {
              int tx = enx + dx[i], ty = eny + dy[i];
              if (tx >= 0 && tx < WIDTH && ty >= 0 && ty < HEIGHT &&
                  field[tx][ty] == 0)
                e_mobility++;
            }
            long long cv = (long long)t2.owned_cells[p] * 10000 +
                           t2.degree_sum[p] * 10 - t2.owned_cells[ce] * 5000 +
                           (long long)ess.articulation_points * 5000 -
                           (long long)e_mobility * 10000;
            if (cv < worst_case)
              worst_case = cv;
            field[enx][eny] = 0;
          }
        }
        eval = can_e ? worst_case : (long long)my_size * 10000 + 10000000;
      }

      if (my_size < 30)
        eval -= (long long)(30 - my_size) * (30 - my_size) * 100000000LL;

      if (eval > max_eval) {
        max_eval = eval;
        bmove = d;
      } else if (eval == max_eval) {
        auto get_w = [&](int x, int y) {
          int c = 0;
          for (int i = 0; i < 4; i++) {
            int tx = x + dx[i], ty = y + dy[i];
            if (tx < 0 || tx >= WIDTH || ty < 0 || ty >= HEIGHT ||
                field[tx][ty] != 0)
              c++;
          }
          return c;
        };
        if (bmove == -1 || get_w(nx, ny) > get_w(players[p].x1 + dx[bmove],
                                                 players[p].y1 + dy[bmove]))
          bmove = d;
      }
      field[nx][ny] = 0;
    }
    if (bmove != -1)
      cout << direction_to_string(bmove) << endl;
    else
      cout << "UP" << endl;
  }
  return 0;
}
