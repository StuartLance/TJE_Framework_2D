/*  Scene characters: the prisoner (player) and the guards with their perception state machine.
*/
#ifndef ENTITY_H
#define ENTITY_H

#include "framework.h"
#include "image.h"

class World;

//rows of the character spritesheets
enum eDir { DIR_DOWN = 0, DIR_UP = 1, DIR_LEFT = 2, DIR_RIGHT = 3 };

class Character
{
public:
	Vector2 pos;			//top-left corner in pixels
	eDir dir = DIR_DOWN;
	bool moving = false;
	float anim_time = 0.0f;

	Vector2 center() const { return Vector2(pos.x + 4, pos.y + 4); }
	int tileX() const { return (int)floor((pos.x + 4) / 8); }
	int tileY() const { return (int)floor((pos.y + 4) / 8); }
	void faceTowards(Vector2 v);
	void render(Image& fb, const Image& sheet, Vector2 camera) const;
};

class Player : public Character
{
public:
	float stamina = 0.0f;
	bool sprinting = false;
	float noise_timer = 0.0f;

	void update(float dt, World& world);
};

class Guard : public Character
{
public:
	enum eState { PATROL, ALERT, CHASE, INVESTIGATE, SEARCH, RETURN };

	eState state = PATROL;
	float state_timer = 0.0f;
	Vector2 facing = Vector2(0, 1);

	int home_x = 0, home_y = 0;
	int patrol_dx = 0, patrol_dy = 0;
	int next_x = 0, next_y = 0;		//tile we are walking towards
	int target_x = 0, target_y = 0;	//destination for CHASE / INVESTIGATE / RETURN

	void spawn(int tx, int ty, bool vertical);
	void update(float dt, World& world);
	void hearNoise(int tx, int ty);
	bool isAlerted() const { return state == ALERT || state == CHASE; }

private:
	void setState(eState s, float timer = 0.0f);
	void chooseNextTile(World& world);
};

#endif
