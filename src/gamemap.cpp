#include "gamemap.h"
#include <fstream>
#include <queue>
#include <algorithm>
#include <cmath>

void drawTile(Image& fb, const Image& tileset, int index, int x, int y)
{
	int columns = tileset.width / TILE_SIZE;
	fb.drawImage(tileset, x, y, Area((float)(index % columns) * TILE_SIZE, (float)(index / columns) * TILE_SIZE, TILE_SIZE, TILE_SIZE));
}

bool GameMap::load(const char* filename, LevelSpawns& spawns)
{
	std::ifstream file(filename);
	if (!file.is_open())
	{
		std::cerr << "Could not open level: " << filename << std::endl;
		return false;
	}

	std::vector<std::string> rows;
	std::string line;
	while (std::getline(file, line))
	{
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		if (!line.empty())
			rows.push_back(line);
	}

	height = (int)rows.size();
	width = 0;
	for (const std::string& row : rows)
		width = std::max(width, (int)row.size());
	tiles.assign(width * height, TILE_WALL);
	spawns = LevelSpawns();

	for (int y = 0; y < height; ++y)
		for (int x = 0; x < (int)rows[y].size(); ++x)
		{
			eTile tile = TILE_FLOOR;
			switch (rows[y][x])
			{
				case '#': tile = TILE_WALL; break;
				case 'D': tile = TILE_DOOR; break;
				case 'E': tile = TILE_EXIT; break;
				case ',': tile = TILE_FLOOR_ALT; break;
				case 'P': spawns.player_x = x; spawns.player_y = y; break;
				case 'K': spawns.keys.push_back(Vector2((float)x, (float)y)); break;
				case 'H': spawns.guards.push_back({ x, y, false }); break;
				case 'V': spawns.guards.push_back({ x, y, true }); break;
			}
			setTile(x, y, tile);
		}

	std::cout << " + Level loaded: " << filename << " (" << width << "x" << height << ")" << std::endl;
	return true;
}

eTile GameMap::getTile(int x, int y) const
{
	if (x < 0 || y < 0 || x >= width || y >= height)
		return TILE_WALL;
	return (eTile)tiles[y * width + x];
}

void GameMap::setTile(int x, int y, eTile t)
{
	if (x < 0 || y < 0 || x >= width || y >= height)
		return;
	tiles[y * width + x] = t;
}

bool GameMap::isSolid(int tx, int ty) const
{
	eTile t = getTile(tx, ty);
	return t == TILE_WALL || t == TILE_DOOR;
}

bool GameMap::isSolidAtPixel(float px, float py) const
{
	return isSolid((int)floor(px / TILE_SIZE), (int)floor(py / TILE_SIZE));
}

bool GameMap::hasLineOfSight(Vector2 from, Vector2 to) const
{
	Vector2 delta = to - from;
	float dist = (float)delta.length();
	int steps = (int)(dist / 2.0f) + 1;
	for (int i = 1; i < steps; ++i)
	{
		float t = i / (float)steps;
		if (isSolidAtPixel(from.x + delta.x * t, from.y + delta.y * t))
			return false;
	}
	return true;
}

bool GameMap::findNextStep(int fx, int fy, int tx, int ty, int& out_x, int& out_y) const
{
	if (fx == tx && fy == ty)
	{
		out_x = fx; out_y = fy;
		return true;
	}
	if (isSolid(tx, ty))
		return false;

	//search backwards from the target so the first visited neighbour of the origin is the next step
	std::vector<int> dist(width * height, -1);
	std::queue<int> open;
	dist[ty * width + tx] = 0;
	open.push(ty * width + tx);
	const int dx[4] = { 1, -1, 0, 0 };
	const int dy[4] = { 0, 0, 1, -1 };

	while (!open.empty())
	{
		int current = open.front(); open.pop();
		int cx = current % width, cy = current / width;
		for (int i = 0; i < 4; ++i)
		{
			int nx = cx + dx[i], ny = cy + dy[i];
			if (nx == fx && ny == fy)
			{
				out_x = cx; out_y = cy;
				return true;
			}
			if (isSolid(nx, ny) || dist[ny * width + nx] != -1)
				continue;
			dist[ny * width + nx] = dist[current] + 1;
			open.push(ny * width + nx);
		}
	}
	return false;
}

void GameMap::render(Image& fb, const Image& tileset, Vector2 camera) const
{
	int start_x = (int)floor(camera.x / TILE_SIZE);
	int start_y = (int)floor(camera.y / TILE_SIZE);
	int end_x = start_x + fb.width / TILE_SIZE + 2;
	int end_y = start_y + fb.height / TILE_SIZE + 2;

	for (int y = start_y; y < end_y; ++y)
		for (int x = start_x; x < end_x; ++x)
		{
			if (x < 0 || y < 0 || x >= width || y >= height)
				continue;
			int sx = x * TILE_SIZE - (int)camera.x;
			int sy = y * TILE_SIZE - (int)camera.y;
			eTile t = getTile(x, y);

			//walls show their brick face only when the tile below is walkable (fake top-down depth)
			if (t == TILE_WALL)
			{
				drawTile(fb, tileset, getTile(x, y + 1) == TILE_WALL ? TILE_WALL_TOP : TILE_WALL, sx, sy);
				continue;
			}

			//every walkable tile gets a floor below it, with some variation
			bool alt = t == TILE_FLOOR_ALT || ((x * 7 + y * 13) % 11 == 0);
			drawTile(fb, tileset, alt ? TILE_FLOOR_ALT : TILE_FLOOR, sx, sy);
			if (t == TILE_DOOR || t == TILE_DOOR_OPEN || t == TILE_EXIT)
				drawTile(fb, tileset, t, sx, sy);
		}
}
