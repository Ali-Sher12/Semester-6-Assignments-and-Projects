from collections import deque

grid_data = [
    ['.', '.', '.', '.', '.'],
    ['.', 'X', '.', 'X', '.'],
    ['.', '.', '.', '.', '.'],
    ['.', 'X', '.', 'X', '.'],
    ['.', '.', '.', '.', '.'],
]

# Robots: [ID, Priority, Energy, startX, startY, goalX, goalY, checkpoints]
robots_data = [
    [1, 2, 30, 0, 0, 4, 4, [[2, 2]]],
    [2, 1, 25, 4, 0, 0, 4, [[2, 2]]],
]


def get_input():
    rows = len(grid_data)
    cols = len(grid_data[0])

    new_grid = []
    for i in range(rows):
        new_grid.append(grid_data[rows - 1 - i])

    bot_list = []
    for r in robots_data:
        bot = {}
        bot["id"] = r[0]
        bot["prio"] = r[1]
        bot["power"] = r[2]
        bot["start"] = (r[3], r[4])
        bot["end"] = (r[5], r[6])
        bot["cps"] = [tuple(x) for x in r[7]]
        bot_list.append(bot)

    return new_grid, rows, cols, bot_list


def moves_from(x, y, g, n, m):
    ch = g[y][x]

    forced = {
        '^': (x, y+1),
        'v': (x, y-1),
        '<': (x-1, y),
        '>': (x+1, y)
    }

    if ch in forced:
        nx, ny = forced[ch]
        if 0 <= nx < m and 0 <= ny < n and g[ny][nx] != 'X':
            return [(nx, ny)]
        return []

    poss = [
        (x, y+1),
        (x, y-1),
        (x-1, y),
        (x+1, y),
        (x, y)
    ]

    ok = []
    for nx, ny in poss:
        if 0 <= nx < m and 0 <= ny < n:
            if g[ny][nx] != 'X':
                ok.append((nx, ny))

    return ok


def search_path(bot, g, n, m, booked):
    sx, sy = bot["start"]
    gx, gy = bot["end"]
    cp_list = bot["cps"]
    max_steps = bot["power"]

    start_node = (sx, sy, 0, 0)

    q = deque()
    q.append((start_node, [(sx, sy)]))

    seen = set()
    seen.add(start_node)

    while q:
        (x, y, cp_i, time_now), route = q.popleft()

        if (x, y) == (gx, gy) and cp_i == len(cp_list):
            return route

        if time_now >= max_steps:
            continue

        next_cells = moves_from(x, y, g, n, m)

        for nx, ny in next_cells:
            new_time = time_now + 1

            if (nx, ny, new_time) in booked:
                continue

            new_cp = cp_i
            if cp_i < len(cp_list):
                if (nx, ny) == cp_list[cp_i]:
                    new_cp += 1

            new_node = (nx, ny, new_cp, new_time)

            if new_node in seen:
                continue

            seen.add(new_node)
            q.append((new_node, route + [(nx, ny)]))

    return None


def run_all(g, n, m, bots):
    bots_sorted = sorted(bots, key=lambda b: b["prio"], reverse=True)

    reserved = set()
    results = {}

    biggest_power = max(b["power"] for b in bots)

    for b in bots_sorted:
        path = search_path(b, g, n, m, reserved)

        if path is None:
            results[b["id"]] = None
            return results, False

        results[b["id"]] = path

        for t, (xx, yy) in enumerate(path):
            reserved.add((xx, yy, t))

        fx, fy = path[-1]
        finish_t = len(path) - 1

        for extra_t in range(finish_t + 1, finish_t + biggest_power + 1):
            reserved.add((fx, fy, extra_t))

    return results, True


def main():
    grid, n, m, bots = get_input()

    answers, ok = run_all(grid, n, m, bots)

    print("=" * 50)

    for b in sorted(bots, key=lambda x: x["id"]):
        pid = b["id"]
        p = answers.get(pid)

        if p is None:
            print(f"Error: No valid path found for Robot {pid}")
        else:
            total_time = len(p) - 1
            txt = " -> ".join(f"({a},{b})" for a, b in p)

            print(f"Robot {pid}:")
            print(f"Path: {txt}")
            print(f"Total Time: {total_time}")
            print(f"Total Energy: {total_time}")

        print()


if __name__ == "__main__":
    main()