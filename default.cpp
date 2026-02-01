#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

/**
 * Auto-generated code below aims at helping you parse
 * the standard input according to the problem statement.
 **/

int main() {

  // game loop
  while (1) {
    int n; // total number of players (2 to 4).
    int p; // your player number (0 to 3).
    cin >> n >> p;
    cin.ignore();
    for (int i = 0; i < n; i++) {
      int x0; // starting X coordinate of lightcycle (or -1)
      int y0; // starting Y coordinate of lightcycle (or -1)
      int x1; // starting X coordinate of lightcycle (can be the same as X0 if
              // you play before this player)
      int y1; // starting Y coordinate of lightcycle (can be the same as Y0 if
              // you play before this player)
      cin >> x0 >> y0 >> x1 >> y1;
      cin.ignore();
    }

    // Write an action using cout. DON'T FORGET THE "<< endl"
    // To debug: cerr << "Debug messages..." << endl;

    cout << "LEFT" << endl; // A single line with UP, DOWN, LEFT or RIGHT
  }
}