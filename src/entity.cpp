#include "entity.h"
#include "world.h"
#include "controls.h"
#include "game.h"
#include <cmath>
#include <algorithm>

// ---------- Character ----------

void Character::faceTowards(Vector2 v)
{
	if (v.x == 0 && v.y == 0)
		return;
	if (fabs(v.x) > fabs(v.y))
		dir = v.x < 0 ? DIR_LEFT : DIR_RIGHT;
	else
		dir = v.y < 0 ? DIR_UP : DIR_DOWN;
}

void Character::render(Image& fb, const Image& sheet, Vector2 camera) const
{
	int frame = moving ? ((int)(anim_time * 8.0f) % 2) : 0;
	fb.drawImage(sheet, (int)floor(pos.x - camera.x), (int)floor(pos.y - camera.y), Area((float)frame * 8, (float)dir * 8, 8, 8));
}

// ---------- Player ----------

//collision box inside the 8x8 sprite (feet area) so the prisoner fits 1-tile corridors
static const float BOX_X0 = 1, BOX_X1 = 6, BOX_Y0 = 2, BOX_Y1 = 7;

static bool tryMove(Player& p, World& world, Vector2 delta)
{
	Vector2 next = p.pos + delta;
	const float cx[2] = { next.x + BOX_X0, next.x + BOX_X1 };
	const float cy[2] = { next.y + BOX_Y0, next.y + BOX_Y1 };
	bool blocked = false;
	for (int i = 0; i < 2; ++i)
		for (int j = 0; j < 2; ++j)
		{
			int tx = (int)floor(cx[i] / TILE_SIZE), ty = (int)floor(cy[j] / TILE_SIZE);
			if (world.map.isSolid(tx, ty) && !world.tryOpenDoor(tx, ty))
				blocked = true;
		}
	if (!blocked)
		p.pos = next;
	return !blocked;
}

void Player::update(float dt, World& world)
{
	const GameConfig& cfg = world.config;
	Vector2 input = Controls::direction();
	moving = input.x != 0 || input.y != 0;

	sprinting = moving && Controls::buttonA() && stamina > 0;
	if (sprinting)
		stamina = std::max(0.0f, stamina - dt);
	else
		stamina = std::min(cfg.stamina_max, stamina + dt * cfg.stamina_regen);

	if (moving)
	{
		faceTowards(input);
		anim_time += dt * (sprinting ? 1.6f : 1.0f);
		float speed = sprinting ? cfg.sprint_speed : cfg.player_speed;
		float step = speed * dt;

		//when blocked, slide towards the tile center on the other axis to take corners smoothly
		if (input.x != 0 && !tryMove(*this, world, Vector2(input.x * step, 0)) && input.y == 0)
		{
			float offset = round(pos.y / TILE_SIZE) * TILE_SIZE - pos.y;
			tryMove(*this, world, Vector2(0, clamp(offset, -step, step)));
		}
		if (input.y != 0 && !tryMove(*this, world, Vector2(0, input.y * step)) && input.x == 0)
		{
			float offset = round(pos.x / TILE_SIZE) * TILE_SIZE - pos.x;
			tryMove(*this, world, Vector2(clamp(offset, -step, step), 0));
		}
	}

	//running is loud: nearby guards come to check
	noise_timer -= dt;
	if (sprinting && noise_timer <= 0)
	{
		world.makeNoise(center(), cfg.noise_radius);
		noise_timer = 0.3f;
	}
}

// ---------- Guard ----------

void Guard::spawn(int tx, int ty, bool vertical)
{
	home_x = next_x = tx;
	home_y = next_y = ty;
	pos = Vector2((float)tx * TILE_SIZE, (float)ty * TILE_SIZE);
	patrol_dx = vertical ? 0 : 1;
	patrol_dy = vertical ? 1 : 0;
	facing = Vector2((float)patrol_dx, (float)patrol_dy);
	faceTowards(facing);
	state = PATROL;
	state_timer = 0.0f;
}

void Guard::setState(eState s, float timer)
{
	state = s;
	state_timer = timer;
}

void Guard::hearNoise(int tx, int ty)
{
	if (isAlerted())
		return;
	setState(INVESTIGATE);
	target_x = tx;
	target_y = ty;
}

void Guard::chooseNextTile(World& world)
{
	int cx = next_x, cy = next_y;

	if (state == PATROL)
	{
		if (world.map.isSolid(cx + patrol_dx, cy + patrol_dy))
		{
			//turn around and stand for a moment
			patrol_dx = -patrol_dx;
			patrol_dy = -patrol_dy;
			state_timer = 0.8f;
			facing = Vector2((float)patrol_dx, (float)patrol_dy);
			faceTowards(facing);
			return;
		}
		next_x = cx + patrol_dx;
		next_y = cy + patrol_dy;
		return;
	}

	if (cx == target_x && cy == target_y)
	{
		if (state == RETURN)
		{
			setState(PATROL);
			facing = Vector2((float)patrol_dx, (float)patrol_dy);
		}
		else
			setState(SEARCH, 2.0f);
		return;
	}

	int nx, ny;
	if (!world.map.findNextStep(cx, cy, target_x, target_y, nx, ny))
	{
		setState(SEARCH, 1.5f);
		return;
	}
	next_x = nx;
	next_y = ny;
}

void Guard::update(float dt, World& world)
{
	const GameConfig& cfg = world.config;
	state_timer -= dt;
	Vector2 to_player = world.player.center() - center();
	bool sees = world.guardCanSee(*this);

	switch (state)
	{
		case PATROL: case INVESTIGATE: case SEARCH: case RETURN:
			if (sees)
			{
				setState(ALERT, 0.5f);
				Game::instance->playSound("alert");
			}
			break;
		case CHASE:
			if (sees)
			{
				target_x = world.player.tileX();
				target_y = world.player.tileY();
			}
			break;
		case ALERT:
			break;
	}

	if (state == ALERT)
	{
		moving = false;
		facing = to_player / (float)to_player.length();
		faceTowards(facing);
		if (state_timer <= 0)
		{
			setState(CHASE);
			target_x = world.player.tileX();
			target_y = world.player.tileY();
		}
		return;
	}

	if (state == SEARCH)
	{
		//look around: down, left, up, right
		moving = false;
		const Vector2 looks[4] = { Vector2(0, 1), Vector2(-1, 0), Vector2(0, -1), Vector2(1, 0) };
		facing = looks[((int)(state_timer * 2.0f) % 4 + 4) % 4];
		faceTowards(facing);
		if (state_timer <= 0)
		{
			setState(RETURN);
			target_x = home_x;
			target_y = home_y;
		}
		return;
	}

	if (state == PATROL && state_timer > 0)
	{
		moving = false;
		return;
	}

	float speed = cfg.guard_speed;
	if (state == CHASE) speed = cfg.guard_chase_speed;
	else if (state == INVESTIGATE) speed = (cfg.guard_speed + cfg.guard_chase_speed) * 0.5f;

	Vector2 goal((float)next_x * TILE_SIZE, (float)next_y * TILE_SIZE);
	Vector2 delta = goal - pos;
	float dist = (float)delta.length();
	float step = speed * dt;
	if (dist <= step)
	{
		pos = goal;
		chooseNextTile(world);
		moving = false;
	}
	else
	{
		pos = pos + delta * (step / dist);
		facing = delta / dist;
		faceTowards(facing);
		moving = true;
		anim_time += dt;
	}
}
