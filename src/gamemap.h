/*  Tile grid of a level: loading from text files, rendering, collisions, line of sight and pathfinding.
*/
#ifndef GAMEMAP_H
#define GAMEMAP_H

#include <vector>
#include <string>
#include "image.h"

#define TILE_SIZE 8

enum eTile : unsigned char
{
	TILE_FLOOR = 0,
	TILE_WALL = 1,
	TILE_DOOR = 2,
	TILE_DOOR_OPEN = 3,
	TILE_EXIT = 4,
	TILE_FLOOR_ALT = 5,
	TILE_WALL_TOP = 6,
};

//indices of non-map icons inside the tileset
enum eIcon
{
	ICON_KEY = 8,
	ICON_PEBBLE = 9,
	ICON_HEART = 10,
	ICON_HEART_EMPTY = 11,
	ICON_ALERT = 12,
	ICON_QUESTION = 13,
	ICON_DUST = 14,
};

struct GuardSpawn
{
	int x, y;
	bool vertical;
};

//entities found while parsing a level file
struct LevelSpawns
{
	int player_x = 1, player_y = 1;
	std::vector<GuardSpawn> guards;
	std::vector<Vector2> keys; //tile coordinates
};

class GameMap
{
public:
	int width = 0;
	int height = 0;
	std::vector<unsigned char> tiles;

	bool load(const char* filename, LevelSpawns& spawns);

	eTile getTile(int x, int y) const;
	void setTile(int x, int y, eTile t);

	bool isSolid(int tx, int ty) const; //blocks movement
	bool isSolidAtPixel(float px, float py) const;
	bool hasLineOfSight(Vector2 from, Vector2 to) const;

	//breadth-first search over the grid, returns the first tile to step into to go from (fx,fy) to (tx,ty)
	bool findNextStep(int fx, int fy, int tx, int ty, int& out_x, int& out_y) const;

	void render(Image& fb, const Image& tileset, Vector2 camera) const;
};

void drawTile(Image& fb, const Image& tileset, int index, int x, int y);

#endif
