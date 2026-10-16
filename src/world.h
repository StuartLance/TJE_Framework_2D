/*  World: everything that exists inside a level (map, prisoner, guards, keys, pebble) plus the run progress.
*/
#ifndef WORLD_H
#define WORLD_H

#include <vector>
#include <string>
#include "gamemap.h"
#include "entity.h"

//gameplay tuning read from data/config.txt
struct GameConfig
{
	int lives = 3;
	int pebbles_per_level = 3;
	float player_speed = 38;
	float sprint_speed = 64;
	float stamina_max = 1.5f;
	float stamina_regen = 0.6f;
	float guard_speed = 20;
	float guard_chase_speed = 44;
	float view_range = 44;
	float view_angle = 38; //half angle in degrees
	float noise_radius = 40;
	float pebble_noise_radius = 60;

	bool load(const char* filename);
};

struct KeyItem
{
	Vector2 pos; //pixels
	bool taken = false;
};

struct Pebble
{
	bool active = false;
	Vector2 pos;
	Vector2 vel;
	float time = 0.0f;
};

class World
{
public:
	GameConfig config;
	GameMap map;
	Player player;
	std::vector<Guard> guards;
	std::vector<KeyItem> keys_on_map;
	Pebble pebble;
	std::vector<std::string> levels;

	int level_index = 0;
	int lives = 0;
	int keys = 0;
	int pebbles = 0;
	int times_caught = 0;
	float total_time = 0.0f;
	bool god_mode = false;

	//events raised during update, consumed by PlayStage
	bool player_caught = false;
	bool level_completed = false;

	World();

	void newGame();
	bool loadLevel(int index);
	bool isLastLevel() const { return level_index >= (int)levels.size() - 1; }
	void respawn();

	void update(float dt);
	void render(Image& fb, Vector2 camera);

	bool guardCanSee(const Guard& guard) const;
	void makeNoise(Vector2 pos, float radius);
	void throwPebble();
	bool tryOpenDoor(int tx, int ty);

private:
	LevelSpawns spawns;

	void updatePebble(float dt);
	void renderVisionCones(Image& fb, Vector2 camera);
};

#endif
