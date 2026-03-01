"""
Assignment 1: Multi-Robot Path Planning on a Grid
Author: [Your Name]

STATE SPACE DESIGN:
    Each state is a tuple: (x, y, checkpoint_index, time)
    - (x, y): current position of the robot on the grid
    - checkpoint_index: how many checkpoints have been visited so far.
                        When checkpoint_index == total_checkpoints, the robot
                        is free to head to the goal.
    - time: the current time step. This is essential for collision checking
            against reservations made by higher-priority robots.

ALGORITHM USED:
    Breadth-First Search (BFS)
    - BFS explores states level by level (each level = one time step forward).
    - Since every move costs exactly 1 energy, BFS naturally finds the
      shortest (minimum energy) path first.
    - The first time BFS reaches the goal state, it is guaranteed to be
      via the fewest moves, which helps stay within the energy limit.

HEURISTIC:
    None. BFS is an uninformed search — it does not use a heuristic.
    This is intentional to stay within Chapters 1-3 material.

COLLISION AVOIDANCE:
    Higher-priority robots are planned first. Their (x, y, t) positions are
    stored in a "reservations" set. When planning a lower-priority robot,
    any state where (x, y, t) is already reserved is treated as blocked.
"""

from collections import deque  # deque is used for the BFS queue (efficient O(1) popleft)


# ─────────────────────────────────────────────────────────────────────────────
# SECTION 1: SAMPLE INPUT DATA
# ─────────────────────────────────────────────────────────────────────────────

# The grid is defined as a list of lists (rows from TOP to BOTTOM).
# Each inner list contains the characters for that row.
# This matches the input file format: row 0 here = y = N-1 (top row).
#
# Cell types:
#   '.'  = free cell
#   'X'  = obstacle (impassable)
#   '^'  = one-way: must move up
#   'v'  = one-way: must move down
#   '<'  = one-way: must move left
#   '>'  = one-way: must move right
#
# This sample is the exact example from the assignment spec:
#   5x5 grid, 2 robots, each with 1 checkpoint at (2,2)

GRID_ROWS = [
    ['.', '.', '.', '.', '.'],   # y = 4 (top row)
    ['.', 'X', '.', 'X', '.'],   # y = 3
    ['.', '.', '.', '.', '.'],   # y = 2
    ['.', 'X', '.', 'X', '.'],   # y = 1
    ['.', '.', '.', '.', '.'],   # y = 0 (bottom row)
]

# Each robot is defined as a list:
# [ID, Priority, EnergyLimit, startX, startY, goalX, goalY, [[cp1X, cp1Y], [cp2X, cp2Y], ...]]
#
# Robot 1: higher priority (2), starts bottom-left, goal top-right, checkpoint center
# Robot 2: lower priority (1),  starts bottom-right, goal top-left, checkpoint center
ROBOTS_DATA = [
    [1, 2, 30, 0, 0, 4, 4, [[2, 2]]],
    [2, 1, 25, 4, 0, 0, 4, [[2, 2]]],
]


def load_from_sample():
    """
    Builds the grid and robots list from the hardcoded sample data above.

    The GRID_ROWS list is stored top-to-bottom (matching the input file format),
    but our coordinate system has y=0 at the BOTTOM. So we flip it:
        grid[y][x] = GRID_ROWS[N - 1 - y][x]

    Returns:
        grid   : 2D list grid[y][x] with y=0 at bottom
        N      : number of rows
        M      : number of columns
        robots : list of robot dicts
    """
    N = len(GRID_ROWS)
    M = len(GRID_ROWS[0])

    # Flip GRID_ROWS so that grid[0] = bottom row, grid[N-1] = top row
    grid = []
    for y in range(N):
        grid.append(GRID_ROWS[N - 1 - y])  # grid[y][x] now matches (x,y) coords

    # Build robot dicts from the flat list format
    robots = []
    for entry in ROBOTS_DATA:
        robot_id  = entry[0]
        priority  = entry[1]
        energy    = entry[2]
        start     = (entry[3], entry[4])   # (startX, startY)
        goal      = (entry[5], entry[6])   # (goalX, goalY)
        checkpoints = [tuple(cp) for cp in entry[7]]  # list of (cpX, cpY) tuples

        robots.append({
            "id":          robot_id,
            "priority":    priority,
            "energy":      energy,
            "start":       start,
            "goal":        goal,
            "checkpoints": checkpoints
        })

    return grid, N, M, robots


# ─────────────────────────────────────────────────────────────────────────────
# SECTION 2: MOVEMENT / GRID UTILITIES
# ─────────────────────────────────────────────────────────────────────────────

def get_neighbors(x, y, grid, N, M):
    """
    Returns all valid (nx, ny) positions a robot can move TO from (x, y).

    Rules:
    1. If the current cell is a one-way cell (^, v, <, >), the robot MUST
       move in that direction — no other moves (including wait) are allowed
       from a one-way cell.
    2. Otherwise, the robot can move in any of the 4 cardinal directions,
       OR stay in place (wait).
    3. The destination cell must be in bounds and not an obstacle (X).

    Note: "wait" (staying in place) is included as a valid action here,
    but only for free cells. You cannot wait on a one-way cell.
    """
    cell = grid[y][x]  # character at current position

    # ── One-way cells force a specific direction ──
    one_way_map = {
        '^': (x,   y+1),   # must go up
        'v': (x,   y-1),   # must go down
        '<': (x-1, y),     # must go left
        '>': (x+1, y),     # must go right
    }

    if cell in one_way_map:
        # Forced move: only one candidate destination
        nx, ny = one_way_map[cell]
        # Check bounds and not obstacle
        if 0 <= nx < M and 0 <= ny < N and grid[ny][nx] != 'X':
            return [(nx, ny)]
        else:
            return []  # trapped on one-way cell with no valid exit

    # ── Free cell: 4 directions + wait ──
    candidates = [
        (x,   y+1),  # up
        (x,   y-1),  # down
        (x-1, y),    # left
        (x+1, y),    # right
        (x,   y),    # wait (stay in place) — same cell, next time step
    ]

    valid = []
    for nx, ny in candidates:
        if 0 <= nx < M and 0 <= ny < N:  # within bounds
            if grid[ny][nx] != 'X':       # not an obstacle
                valid.append((nx, ny))
    return valid


# ─────────────────────────────────────────────────────────────────────────────
# SECTION 3: BFS PLANNER
# ─────────────────────────────────────────────────────────────────────────────

def bfs(robot, grid, N, M, reservations):
    """
    Plans a path for a single robot using Breadth-First Search (BFS).

    Parameters:
        robot        : dict with 'start', 'goal', 'checkpoints', 'energy'
        grid         : 2D grid[y][x]
        N, M         : grid dimensions
        reservations : set of (x, y, t) tuples that are blocked by
                       higher-priority robots at specific time steps

    Returns:
        A list of (x, y) tuples representing the path (including start),
        or None if no valid path exists within the energy limit.

    BFS STATE:
        (x, y, cp_index, time)
        - x, y       : current position
        - cp_index   : number of checkpoints visited so far.
                       When cp_index == len(checkpoints), the robot can go to goal.
        - time       : current time step (starts at 0)

    BFS STRUCTURE:
        - Queue holds: (state, path_so_far)
        - We explore states in order of increasing time (= BFS levels).
        - A state is only visited once (tracked by 'visited' set).
        - We stop when we reach the goal with all checkpoints done,
          within the energy limit.
    """

    start       = robot["start"]
    goal        = robot["goal"]
    checkpoints = robot["checkpoints"]
    energy_limit = robot["energy"]
    num_cps     = len(checkpoints)

    # ── Initial state ──
    # At time 0, robot is at start, no checkpoints visited yet.
    start_state = (start[0], start[1], 0, 0)
    #               x          y       cp  t

    # ── BFS queue ──
    # Each entry: (state, path)
    # path is a list of (x, y) positions from start up to current state.
    queue = deque()
    queue.append((start_state, [start]))  # start position is always in path

    # ── Visited set ──
    # Prevents re-exploring the same (x, y, cp_index, time) state.
    # Without this, BFS would loop forever (especially with "wait" actions).
    visited = set()
    visited.add(start_state)

    # ── BFS main loop ──
    while queue:
        state, path = queue.popleft()
        x, y, cp_index, t = state

        # ── Check if we've reached the goal with all checkpoints done ──
        if (x, y) == goal and cp_index == num_cps:
            # t == len(path) - 1 == total moves taken
            return path  # success!

        # ── Prune: if we've used all energy, don't expand further ──
        if t >= energy_limit:
            continue

        # ── Expand: try all valid next positions ──
        for nx, ny in get_neighbors(x, y, grid, N, M):
            nt = t + 1  # moving (or waiting) costs 1 time step and 1 energy

            # ── Collision check: is (nx, ny) reserved at time nt? ──
            if (nx, ny, nt) in reservations:
                continue  # blocked by a higher-priority robot

            # ── Checkpoint progression ──
            # If we land on the NEXT checkpoint, advance cp_index.
            new_cp_index = cp_index
            if cp_index < num_cps and (nx, ny) == checkpoints[cp_index]:
                new_cp_index = cp_index + 1  # checkpoint visited!

            # ── Build new state ──
            new_state = (nx, ny, new_cp_index, nt)

            # ── Skip if already visited ──
            if new_state in visited:
                continue

            visited.add(new_state)

            # ── Add to queue with extended path ──
            queue.append((new_state, path + [(nx, ny)]))

    # If queue is exhausted with no solution found:
    return None


# ─────────────────────────────────────────────────────────────────────────────
# SECTION 4: MULTI-ROBOT COORDINATOR
# ─────────────────────────────────────────────────────────────────────────────

def plan_all_robots(grid, N, M, robots):
    """
    Plans paths for ALL robots using priority-based sequential planning.

    Steps:
    1. Sort robots by priority (descending — highest priority goes first).
    2. Plan the highest-priority robot first, with no reservations.
    3. Record its path as (x, y, t) reservations.
    4. Plan each subsequent robot avoiding existing reservations.
    5. Return results in order of Robot ID.

    Returns:
        results: list of dicts (in robot ID order), each with:
                 'id', 'path' (list of (x,y) or None), 'error' (bool)
    """

    # ── Step 1: Sort by descending priority ──
    sorted_robots = sorted(robots, key=lambda r: r["priority"], reverse=True)

    # ── Reservations: set of (x, y, t) blocked cells ──
    reservations = set()

    # ── Plan each robot in priority order ──
    robot_paths = {}  # robot_id → path (list of (x,y)) or None

    for robot in sorted_robots:
        path = bfs(robot, grid, N, M, reservations)
        robot_paths[robot["id"]] = path

        if path is not None:
            # Record this robot's positions at each time step as reservations
            # so lower-priority robots avoid them.
            for t, (x, y) in enumerate(path):
                reservations.add((x, y, t))
            # After the robot reaches its goal, it stays there forever.
            # We add reservations for future time steps too, because a
            # lower-priority robot might try to pass through the goal cell later.
            # We extend reservations up to a reasonable bound (energy limits sum).
            max_time = sum(r["energy"] for r in robots)
            goal_x, goal_y = path[-1]
            final_t = len(path) - 1
            for future_t in range(final_t + 1, max_time + 1):
                reservations.add((goal_x, goal_y, future_t))

    # ── Collect results in order of Robot ID ──
    results = []
    for robot in sorted(robots, key=lambda r: r["id"]):
        path = robot_paths[robot["id"]]
        if path is None:
            results.append({"id": robot["id"], "path": None, "error": True})
        else:
            results.append({"id": robot["id"], "path": path, "error": False})

    return results


# ─────────────────────────────────────────────────────────────────────────────
# SECTION 5: OUTPUT WRITER
# ─────────────────────────────────────────────────────────────────────────────

def write_output(results, filename="output.txt"):
    """
    Writes the planning results to output.txt.

    Format for a successful path:
        Robot <ID>:
        Path: (x0,y0) -> (x1,y1) -> ... -> (xT,yT)
        Total Time: T
        Total Energy: E

    Format for failure:
        Error: No valid path found for Robot <ID>
    """
    with open(filename, "w") as f:
        for i, result in enumerate(results):
            robot_id = result["id"]

            if result["error"]:
                # No path found for this robot
                f.write(f"Error: No valid path found for Robot {robot_id}\n")
            else:
                path = result["path"]
                T = len(path) - 1          # total moves = path length - 1
                E = T                      # each move costs 1 energy

                # Build path string: "(x0,y0) -> (x1,y1) -> ..."
                path_str = " -> ".join(f"({x},{y})" for x, y in path)

                f.write(f"Robot {robot_id}:\n")
                f.write(f"Path: {path_str}\n")
                f.write(f"Total Time: {T}\n")
                f.write(f"Total Energy: {E}\n")

            # Blank line between robots (except after the last one)
            if i < len(results) - 1:
                f.write("\n")


# ─────────────────────────────────────────────────────────────────────────────
# SECTION 6: MAIN ENTRY POINT
# ─────────────────────────────────────────────────────────────────────────────

def main():
    """
    Main function — orchestrates the full pipeline:
    1. Load the hardcoded sample grid and robots (GRID_ROWS / ROBOTS_DATA)
    2. Plan paths for all robots (BFS + priority ordering)
    3. Print results to the console
    """
    print("Loading sample data ...")
    grid, N, M, robots = load_from_sample()

    print(f"Grid: {N} rows x {M} cols | Robots: {len(robots)}")
    for r in robots:
        print(f"  Robot {r['id']}: priority={r['priority']}, energy={r['energy']}, "
              f"start={r['start']}, goal={r['goal']}, checkpoints={r['checkpoints']}")

    print("\nPlanning paths ...")
    results = plan_all_robots(grid, N, M, robots)

    # ── Print results in the required output format ──
    print("\n" + "="*50)
    for i, res in enumerate(results):
        robot_id = res["id"]
        if res["error"]:
            print(f"Error: No valid path found for Robot {robot_id}")
        else:
            path = res["path"]
            T = len(path) - 1
            path_str = " -> ".join(f"({x},{y})" for x, y in path)
            print(f"Robot {robot_id}:")
            print(f"Path: {path_str}")
            print(f"Total Time: {T}")
            print(f"Total Energy: {T}")
        if i < len(results) - 1:
            print()


# Run main when script is executed directly
if __name__ == "__main__":
    main()