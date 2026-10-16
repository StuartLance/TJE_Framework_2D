#include "world.h"
#include "game.h"
#include <fstream>
#include <sstream>
#include <cmath>

bool GameConfig::load(const char* filename)
{
	std::ifstream file(filename);
	if (!file.is_open())
	{
		std::cerr << "Could not open config: " << filename << ", using defaults" << std::endl;
		return false;
	}

	std::string line;
	while (std::getline(file, line))
	{
		if (line.empty() || line[0] == '#')
			continue;
		std::istringstream ss(line);
		std::string key;
		float value;
		if (!(ss >> key >> value))
			continue;

		if (key == "lives") lives = (int)value;
		else if (key == "pebbles_per_level") pebbles_per_level = (int)value;
		else if (key == "player_speed") player_speed = value;
		else if (key == "sprint_speed") sprint_speed = value;
		else if (key == "stamina_max") stamina_max = value;
		else if (key == "stamina_regen") stamina_regen = value;
		else if (key == "guard_speed") guard_speed = value;
		else if (key == "guard_chase_speed") guard_chase_speed = value;
		else if (key == "view_range") view_range = value;
		else if (key == "view_angle") view_angle = value;
		else if (key == "noise_radius") noise_radius = value;
		else if (key == "pebble_noise_radius") pebble_noise_radius = value;
	}
	return true;
}

World::World()
{
	config.load("data/config.txt");

	std::ifstream file("data/levels/levels.txt");
	std::string line;
	while (std::getline(file, line))
	{
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		if (!line.empty())
			levels.push_back("data/levels/" + line);
	}
	if (levels.empty())
		std::cerr << "No levels found in data/levels/levels.txt" << std::endl;
}

void World::newGame()
{
	lives = config.lives;
	times_caught = 0;
	total_time = 0.0f;
	loadLevel(0);
}

bool World::loadLevel(int index)
{
	if (index < 0 || index >= (int)levels.size())
		return false;
	level_index = index;
	if (!map.load(levels[index].c_str(), spawns))
		return false;

	keys = 0;
	pebbles = config.pebbles_per_level;
	keys_on_map.clear();
	for (const Vector2& k : spawns.keys)
	{
		KeyItem item;
		item.pos = Vector2(k.x * TILE_SIZE, k.y * TILE_SIZE);
		keys_on_map.push_back(item);
	}
	respawn();
	return true;
}

void World::respawn()
{
	player = Player();
	player.pos = Vector2((float)spawns.player_x * TILE_SIZE, (float)spawns.player_y * TILE_SIZE);
	player.stamina = config.stamina_max;

	guards.clear();
	for (const GuardSpawn& s : spawns.guards)
	{
		Guard g;
		g.spawn(s.x, s.y, s.vertical);
		guards.push_back(g);
	}

	pebble.active = false;
	player_caught = false;
	level_completed = false;
}

void World::update(float dt)
{
	total_time += dt;
	player.update(dt, *this);
	updatePebble(dt);

	for (Guard& g : guards)
	{
		g.update(dt, *this);
		Vector2 d = g.center() - player.center();
		if (!god_mode && d.length() < 6.0)
			player_caught = true;
	}

	//pick up keys
	for (KeyItem& k : keys_on_map)
	{
		if (k.taken)
			continue;
		Vector2 d = Vector2(k.pos.x + 4, k.pos.y + 4) - player.center();
		if (d.length() < 6.0)
		{
			k.taken = true;
			keys++;
			Game::instance->playSound("key");
		}
	}

	if (map.getTile(player.tileX(), player.tileY()) == TILE_EXIT)
		level_completed = true;
}

bool World::tryOpenDoor(int tx, int ty)
{
	if (map.getTile(tx, ty) != TILE_DOOR || keys <= 0)
		return false;
	keys--;
	map.setTile(tx, ty, TILE_DOOR_OPEN);
	Game::instance->playSound("door");
	return true;
}

bool World::guardCanSee(const Guard& guard) const
{
	Vector2 to = player.center() - guard.center();
	float dist = (float)to.length();
	float range = config.view_range * (guard.state == Guard::CHASE ? 1.4f : 1.0f);
	if (dist > range)
		return false;

	//outside the vision cone only very close prisoners are noticed (or any while chasing)
	if (dist > 10.0f && guard.state != Guard::CHASE)
	{
		Vector2 facing = guard.facing;
		float cos_angle = (to.x * facing.x + to.y * facing.y) / dist;
		if (cos_angle < cos(config.view_angle * DEG2RAD))
			return false;
	}
	return map.hasLineOfSight(guard.center(), player.center());
}

void World::makeNoise(Vector2 pos, float radius)
{
	int tx = (int)floor(pos.x / TILE_SIZE);
	int ty = (int)floor(pos.y / TILE_SIZE);
	for (Guard& g : guards)
	{
		Vector2 d = g.center() - pos;
		if (d.length() <= radius)
			g.hearNoise(tx, ty);
	}
}

void World::throwPebble()
{
	if (pebbles <= 0 || pebble.active)
		return;
	const Vector2 dirs[4] = { Vector2(0, 1), Vector2(0, -1), Vector2(-1, 0), Vector2(1, 0) };
	pebbles--;
	pebble.active = true;
	pebble.time = 0.0f;
	pebble.pos = player.center();
	pebble.vel = dirs[player.dir] * 110.0f;
	Game::instance->playSound("throw");
}

void World::updatePebble(float dt)
{
	if (!pebble.active)
		return;
	pebble.time += dt;
	Vector2 next = pebble.pos + pebble.vel * dt;
	bool hit_wall = map.isSolidAtPixel(next.x, next.y);
	if (!hit_wall)
		pebble.pos = next;
	if (hit_wall || pebble.time > 0.5f)
	{
		pebble.active = false;
		makeNoise(pebble.pos, config.pebble_noise_radius);
		Game::instance->playSound("land");
	}
}

void World::renderVisionCones(Image& fb, Vector2 camera)
{
	//accumulate in a mask first so overlapping rays don't darken the tint
	std::vector<unsigned char> mask(fb.width * fb.height, 0);
	const int rays = 48;
	float half = (float)(config.view_angle * DEG2RAD);

	for (const Guard& g : guards)
	{
		unsigned char level = g.isAlerted() ? 2 : 1;
		Vector2 c = g.center();
		float base = atan2(g.facing.y, g.facing.x);
		for (int i = 0; i < rays; ++i)
		{
			float a = base - half + 2 * half * i / (rays - 1);
			float ca = cos(a), sa = sin(a);
			for (float r = 3; r < config.view_range; r += 1.0f)
			{
				float px = c.x + ca * r, py = c.y + sa * r;
				if (map.isSolidAtPixel(px, py))
					break;
				int sx = (int)(px - camera.x), sy = (int)(py - camera.y);
				if (sx < 0 || sy < 0 || sx >= (int)fb.width || sy >= (int)fb.height)
					continue;
				unsigned char& m = mask[sy * fb.width + sx];
				if (level > m)
					m = level;
			}
		}
	}

	const Color calm(255, 230, 120, 40);
	const Color alert(255, 50, 50, 70);
	for (unsigned int y = 0; y < fb.height; ++y)
		for (unsigned int x = 0; x < fb.width; ++x)
		{
			unsigned char m = mask[y * fb.width + x];
			if (m)
				fb.blendPixel(x, y, m == 2 ? alert : calm);
		}
}

void World::render(Image& fb, Vector2 camera)
{
	Game* game = Game::instance;
	map.render(fb, game->tileset, camera);

	int bob = (int)(sin(game->time * 5.0f) * 1.5f);
	for (const KeyItem& k : keys_on_map)
		if (!k.taken)
			drawTile(fb, game->tileset, ICON_KEY, (int)(k.pos.x - camera.x), (int)(k.pos.y - camera.y) + bob);

	renderVisionCones(fb, camera);

	if (pebble.active)
		drawTile(fb, game->tileset, ICON_PEBBLE, (int)(pebble.pos.x - 4 - camera.x), (int)(pebble.pos.y - 4 - camera.y - sin(pebble.time / 0.5f * PI) * 6));

	for (const Guard& g : guards)
	{
		g.render(fb, game->guard_sheet, camera);
		int bubble = -1;
		if (g.isAlerted()) bubble = ICON_ALERT;
		else if (g.state == Guard::INVESTIGATE || g.state == Guard::SEARCH) bubble = ICON_QUESTION;
		if (bubble != -1)
			drawTile(fb, game->tileset, bubble, (int)(g.pos.x - camera.x), (int)(g.pos.y - camera.y) - 9 + bob);
	}

	if (player.sprinting && player.moving)
		drawTile(fb, game->tileset, ICON_DUST, (int)(player.pos.x - camera.x), (int)(player.pos.y - camera.y) + 2);
	player.render(fb, game->player_sheet, camera);
}
