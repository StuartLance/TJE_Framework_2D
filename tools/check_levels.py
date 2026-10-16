"""Sanity-checks level files: equal row widths, closed borders, solvable with the keys available."""
import sys
from collections import deque

def flood(grid, start, passable):
    seen, q = {start}, deque([start])
    while q:
        x, y = q.popleft()
        for nx, ny in ((x+1,y),(x-1,y),(x,y+1),(x,y-1)):
            if (nx, ny) not in seen and passable(grid[ny][nx]):
                seen.add((nx, ny)); q.append((nx, ny))
    return seen

for path in sys.argv[1:]:
    g = [l.rstrip('\n') for l in open(path) if l.strip()]
    w = len(g[0]); assert all(len(r) == w for r in g), f"{path}: ragged rows {[len(r) for r in g]}"
    assert set(g[0]) == {'#'} and set(g[-1]) == {'#'} and all(r[0] == r[-1] == '#' for r in g), f"{path}: open border"
    find = lambda c: [(x, y) for y, r in enumerate(g) for x, ch in enumerate(r) if ch == c]
    p, e = find('P')[0], find('E')[0]
    no_doors = flood(g, p, lambda c: c not in '#D')
    all_doors = flood(g, p, lambda c: c != '#')
    keys = [k for k in find('K') if k in no_doors]
    assert e not in no_doors, f"{path}: exit reachable without doors"
    assert e in all_doors, f"{path}: exit unreachable"
    assert len(keys) >= 1, f"{path}: no reachable key"
    print(f"{path}: OK {w}x{len(g)} guards={len(find('H'))+len(find('V'))} keys={len(find('K'))} doors={len(find('D'))}")
