---
trigger: always_on
---

	The Goal
In this game your are a program driving the legendary tron light cycle and fighting against other programs on the game grid.

The light cycle moves in straight lines and only turn in 90° angles while leaving a solid light ribbon in its wake. Each cycle and associated ribbon features a different color.
Should a light cycle stop, hit a light ribbon or goes off the game grid it will be instantly deactivated.

The last cycle in play wins the game. Your goal is to be the best program: once sent to the arena, programs will compete against each-others in battles gathering 2 to 4 cycles. The more battles you win, the better your rank will be.
 	Rules
Each battle is fought with 2 players. Each player plays in turn during a battle. When your turn comes, the following happens:
Information about the location of players on the grid is sent on the standard input of your program. So your AI must read information on the standard input at the beginning of a turn.
Once the inputs have been read for the current game turn, your AI must provide its next move information on the standard ouput. The output for a game turn must be a single line stating the next direction of the light cycle: either UP, DOWN, LEFT or RIGHT.
Your light cycle will move in the direction your AI provided.
At this point your AI should wait for your next game turn information and so on and so forth. In the mean time, the AI of the other players will receive information the same way you did.
If your AI does not provide output fast enough when your turn comes, or if you provide an invalid output or if your output would make the light cycle move into an obstacle, then your program loses.

If another AI loses before yours, its light ribbon disappears and the game continues until there is only one player left.

The game grid has a 30 by 20 cells width and height. Each player starts at a random location on the grid.
 
Victory Conditions
Be the last remaining player
 	Game Input
Input for one game turn
Line 1: Two integers N and P. Where N is the total number of players and P is your player number for this game.

The N following lines: One line per player. First line is for player 0, next line for player 1, etc. Each line contains four values X0, Y0, X1 and Y1. (X0, Y0) are the coordinates of the initial position of the light ribbon (tail) and (X1, Y1) are the coordinates of the current position of the light ribbon (head) of the player. Once a player loses, his/her X0 Y0 X1 Y1 coordinates are all equal to -1 (no more light ribbon on the grid for this player).

Output for one game turn
A single line with UP, DOWN, LEFT or RIGHT
Constraints
2 ≤ N ≤ 2
0 ≤ P < N
0 ≤ X0, X1 < 30
0 ≤ Y0, Y1 < 20
Your AI must answer in less than 100ms for each game turn.
Example
For a battle with 2 players. Your player starts at location (9,5). Other player starts at location (10,7). You play first.
Input for turn 1 (your turn)
2 0         (N P)
9 5 9 5	    (X0 Y0 X1 Y1)
10 7 10 7   (X0 Y0 X1 Y1)
Output for turn 1
RIGHT
Moving from position (9,5) to (10,5)
Input for turn 2 (what the other player gets)
2 1         (N P)
9 5 10 5    (X0 Y0 X1 Y1)
10 7 10 7   (X0 Y0 X1 Y1)
Output for turn 2
UP
Moving from position (10,7) to (10,6)
Input for turn 3 (your turn)
2 0         (N P)
9 5 10 5    (X0 Y0 X1 Y1)
10 7 10 6   (X0 Y0 X1 Y1)
Output for turn 3
UP
Moving from position (10,5) to (10,4)
Input for turn 4 (what the other player gets)
2 1         (N P)
9 5 10 4    (X0 Y0 X1 Y1)
10 7 10 6   (X0 Y0 X1 Y1)
Output for turn 4
UP
Moving from position (10,6) to (10,5)

The other player loses as his/her light cycle collides with the light ribbon of your light cycle. You win the battle!