#include "stage.h"
#include "game.h"
#include "world.h"
#include "controls.h"
#include "input.h"
#include <cmath>
#include <algorithm>

// ---------- helpers ----------

void drawTextCentered(Image& fb, const std::string& text, int y, const Image& font, int font_w, int font_h)
{
	fb.drawText(text, (int)(fb.width - text.size() * font_w) / 2, y, font, font_w, font_h);
}

void drawTextScaled(Image& fb, const std::string& text, int x, int y, const Image& font, int scale)
{
	Image tmp((unsigned int)text.size() * 7, 9);
	tmp.fill(Color(0, 0, 0, 0));
	tmp.drawText(text, 0, 0, font);
	fb.drawImage(tmp, x, y, tmp.width * scale, tmp.height * scale);
}

static void blendRect(Image& fb, int x, int y, int w, int h, const Color& c)
{
	for (int j = std::max(0, y); j < std::min((int)fb.height, y + h); ++j)
		for (int i = std::max(0, x); i < std::min((int)fb.width, x + w); ++i)
			fb.blendPixel(i, j, c);
}

static std::string formatTime(float seconds)
{
	int s = (int)seconds;
	char buffer[16];
	snprintf(buffer, sizeof(buffer), "%02d:%02d", s / 60, s % 60);
	return buffer;
}

//dark floor with sweeping searchlights and prison bars, used by the menus
static void drawPrisonBackground(Image& fb, float time, bool bars)
{
	Game* game = Game::instance;
	int scroll = (int)(time * 6.0f) % TILE_SIZE;
	for (int y = -TILE_SIZE; y < (int)fb.height; y += TILE_SIZE)
		for (int x = 0; x < (int)fb.width; x += TILE_SIZE)
			drawTile(fb, game->tileset, ((x / 8 + y / 8) % 5 == 0) ? TILE_FLOOR_ALT : TILE_FLOOR, x, y + scroll);

	float lx = 80 + sin(time * 0.9f) * 70, ly = 70 + sin(time * 1.7f) * 30;
	for (int y = 0; y < (int)fb.height; ++y)
		for (int x = 0; x < (int)fb.width; ++x)
		{
			float d = (float)sqrt((x - lx) * (x - lx) + (y - ly) * (y - ly));
			if (d < 24)
				fb.blendPixel(x, y, Color(255, 240, 170, (unsigned char)(70 * (1 - d / 24))));
		}

	if (bars)
		for (int x = 4; x < (int)fb.width; x += 16)
		{
			fb.drawRectangle(x, 0, 3, fb.height, Color(110, 110, 125));
			fb.drawRectangle(x, 0, 1, fb.height, Color(170, 170, 185));
		}
}

// ---------- IntroStage ----------

static const char* MENU_OPTIONS[] = { "START", "HOW TO PLAY", "EXIT" };
static const int MENU_COUNT = 3;

void IntroStage::onEnter()
{
	selected = 0;
}

void IntroStage::update(float dt)
{
	Game* game = Game::instance;
	if (Controls::upPressed()) { selected = (selected + MENU_COUNT - 1) % MENU_COUNT; game->playSound("select"); }
	if (Controls::downPressed()) { selected = (selected + 1) % MENU_COUNT; game->playSound("select"); }
	if (Controls::backPressed()) game->must_exit = true;

	if (Controls::buttonAPressed())
	{
		game->playSound("confirm");
		if (selected == 0) game->setStage(STAGE_PLAY);
		else if (selected == 1) game->setStage(STAGE_TUTORIAL);
		else game->must_exit = true;
	}
}

void IntroStage::render(Image& fb)
{
	Game* game = Game::instance;
	drawPrisonBackground(fb, game->time, true);

	blendRect(fb, 8, 8, 144, 36, Color(0, 0, 0, 170));
	drawTextScaled(fb, "JAILBREAK", 18, 13, game->font_black, 2);
	drawTextScaled(fb, "JAILBREAK", 17, 12, game->font, 2);
	drawTextCentered(fb, "ESCAPE THE PRISON", 34, game->minifont, 4, 6);

	blendRect(fb, 36, 54, 88, 40, Color(0, 0, 0, 170));
	for (int i = 0; i < MENU_COUNT; ++i)
	{
		int y = 58 + i * 12;
		drawTextCentered(fb, MENU_OPTIONS[i], y, game->font);
		if (i == selected && (int)(game->time * 3) % 2 == 0)
			fb.drawText(">", 40, y, game->font);
	}

	//a prisoner running away from a guard
	float x = fmod(game->time * 30.0f, 220.0f) - 40.0f;
	fb.drawImage(game->player_sheet, (int)x + 16, 102, Area((float)((int)(game->time * 10) % 2) * 8, DIR_RIGHT * 8, 8, 8));
	fb.drawImage(game->guard_sheet, (int)x, 102, Area((float)((int)(game->time * 8) % 2) * 8, DIR_RIGHT * 8, 8, 8));
	drawTile(fb, game->tileset, ICON_ALERT, (int)x, 93);

	blendRect(fb, 0, 112, 160, 8, Color(0, 0, 0, 200));
	drawTextCentered(fb, "ARROWS MOVE - Z SELECT", 113, game->minifont, 4, 6);
}

// ---------- TutorialStage ----------

void TutorialStage::update(float dt)
{
	Game* game = Game::instance;
	if (Controls::backPressed() || Controls::buttonBPressed()) { game->playSound("select"); game->setStage(STAGE_INTRO); }
	else if (Controls::buttonAPressed()) { game->playSound("confirm"); game->setStage(STAGE_PLAY); }
}

void TutorialStage::render(Image& fb)
{
	Game* game = Game::instance;
	drawPrisonBackground(fb, game->time, false);
	blendRect(fb, 0, 0, 160, 120, Color(0, 0, 0, 190));
	drawTextCentered(fb, "HOW TO PLAY", 4, game->font);

	struct Line { int icon; const Image* sheet; const char* text; };
	const Line lines[] = {
		{ -1, &game->player_sheet, "ESCAPE ALL 3 CELL BLOCKS" },
		{ TILE_EXIT, nullptr, "REACH THE GREEN HATCH" },
		{ ICON_KEY, nullptr, "KEYS OPEN LOCKED DOORS" },
		{ -1, &game->guard_sheet, "STAY OUT OF THEIR SIGHT" },
		{ ICON_DUST, nullptr, "Z/A: SPRINT (GUARDS HEAR)" },
		{ ICON_PEBBLE, nullptr, "X/B: THROW PEBBLE" },
		{ ICON_QUESTION, nullptr, "NOISE MAKES GUARDS LOOK" },
		{ ICON_HEART, nullptr, "3 LIVES, ESC/START: MENU" },
	};
	for (int i = 0; i < 8; ++i)
	{
		int y = 18 + i * 11;
		if (lines[i].sheet)
			fb.drawImage(*lines[i].sheet, 8, y, Area(0, 0, 8, 8));
		else
			drawTile(fb, game->tileset, lines[i].icon, 8, y);
		fb.drawText(lines[i].text, 20, y + 1, game->minifont, 4, 6);
	}

	if ((int)(game->time * 2) % 2 == 0)
		drawTextCentered(fb, "Z: PLAY   X: BACK", 110, game->minifont, 4, 6);
}

// ---------- PlayStage ----------

void PlayStage::onEnter()
{
	Game::instance->world->newGame();
	startLevel(0);
}

void PlayStage::startLevel(int index)
{
	World& world = *Game::instance->world;
	if (index != world.level_index)
		world.loadLevel(index);
	banner_timer = 1.5f;
	caught_timer = exit_timer = shake_timer = 0.0f;
	updateCamera(0, true);
}

void PlayStage::updateCamera(float dt, bool snap)
{
	World& world = *Game::instance->world;
	float map_w = (float)world.map.width * TILE_SIZE;
	float map_h = (float)world.map.height * TILE_SIZE;
	const float hud_h = 9;

	Vector2 target = world.player.center() - Vector2(80, 60);
	target.x = map_w <= 160 ? (map_w - 160) * 0.5f : clamp(target.x, 0, map_w - 160);
	target.y = map_h + hud_h <= 120 ? (map_h - 120 - hud_h) * 0.5f : clamp(target.y, -hud_h, map_h - 120);

	if (snap)
		camera = target;
	else
		camera = camera + (target - camera) * std::min(1.0f, dt * 6.0f);
}

static void goToEnd(bool won)
{
	Game* game = Game::instance;
	((EndStage*)game->stages[STAGE_END])->won = won;
	game->setStage(STAGE_END);
}

void PlayStage::update(float dt)
{
	Game* game = Game::instance;
	World& world = *game->world;

	if (Controls::backPressed()) { game->setStage(STAGE_INTRO); return; }

	//debug keys for the presentation
	if (Input::wasKeyPressed(SDL_SCANCODE_F1))
	{
		if (world.isLastLevel()) goToEnd(true);
		else startLevel(world.level_index + 1);
		return;
	}
	if (Input::wasKeyPressed(SDL_SCANCODE_F2))
		world.god_mode = !world.god_mode;

	shake_timer -= dt;

	if (banner_timer > 0)
	{
		banner_timer -= dt;
		return;
	}

	if (caught_timer > 0)
	{
		caught_timer -= dt;
		if (caught_timer <= 0)
		{
			if (world.lives <= 0) goToEnd(false);
			else { world.respawn(); banner_timer = 0.8f; updateCamera(0, true); }
		}
		return;
	}

	if (exit_timer > 0)
	{
		exit_timer -= dt;
		if (exit_timer <= 0)
		{
			if (world.isLastLevel()) goToEnd(true);
			else startLevel(world.level_index + 1);
		}
		return;
	}

	if (Controls::buttonBPressed())
		world.throwPebble();

	world.update(dt);

	if (world.player_caught)
	{
		world.lives--;
		world.times_caught++;
		caught_timer = 1.2f;
		shake_timer = 0.4f;
		game->playSound("caught");
	}
	else if (world.level_completed)
	{
		exit_timer = 1.2f;
		game->playSound(world.isLastLevel() ? "win" : "exit");
	}

	updateCamera(dt, false);
}

void PlayStage::render(Image& fb)
{
	Game* game = Game::instance;
	World& world = *game->world;

	Vector2 cam = camera;
	if (shake_timer > 0)
		cam = cam + Vector2(random(4, -2), random(4, -2));

	fb.fill(Color(16, 16, 22));
	world.render(fb, Vector2(floor(cam.x), floor(cam.y)));

	if (caught_timer > 0)
	{
		fb.fillBlend(Color(200, 0, 0, (unsigned char)(caught_timer / 1.2f * 100)));
		blendRect(fb, 0, 52, 160, 14, Color(0, 0, 0, 180));
		drawTextCentered(fb, world.lives > 0 ? "CAUGHT!" : "BUSTED!", 55, game->font);
	}
	else if (exit_timer > 0)
	{
		blendRect(fb, 0, 52, 160, 14, Color(0, 0, 0, 180));
		drawTextCentered(fb, "ESCAPED!", 55, game->font);
	}
	else if (banner_timer > 0)
	{
		blendRect(fb, 0, 50, 160, 20, Color(0, 0, 0, 180));
		drawTextCentered(fb, "BLOCK " + std::to_string(world.level_index + 1) + "/" + std::to_string(world.levels.size()), 52, game->font);
		drawTextCentered(fb, "GET THE KEY, FIND THE HATCH", 62, game->minifont, 4, 6);
	}

	renderHUD(fb);
}

void PlayStage::renderHUD(Image& fb)
{
	Game* game = Game::instance;
	World& world = *game->world;

	blendRect(fb, 0, 0, 160, 9, Color(0, 0, 0, 190));
	for (int i = 0; i < world.config.lives; ++i)
		drawTile(fb, game->tileset, i < world.lives ? ICON_HEART : ICON_HEART_EMPTY, 1 + i * 7, 1);

	drawTile(fb, game->tileset, ICON_KEY, 26, 0);
	fb.drawText(std::to_string(world.keys), 34, 2, game->minifont, 4, 6);
	drawTile(fb, game->tileset, ICON_PEBBLE, 40, 1);
	fb.drawText(std::to_string(world.pebbles), 48, 2, game->minifont, 4, 6);

	//stamina bar
	float ratio = world.player.stamina / world.config.stamina_max;
	fb.drawRectangle(58, 3, 30, 3, Color(50, 50, 60));
	fb.drawRectangle(58, 3, (int)(30 * ratio), 3, ratio > 0.3f ? Color(80, 200, 110) : Color(230, 180, 40));

	fb.drawText("B" + std::to_string(world.level_index + 1), 94, 2, game->minifont, 4, 6);
	fb.drawText(formatTime(world.total_time), 138, 2, game->minifont, 4, 6);
	if (world.god_mode)
		fb.drawText("GOD", 110, 2, game->minifont, 4, 6);
}

// ---------- EndStage ----------

void EndStage::onEnter()
{
	if (!won)
		Game::instance->playSound("caught");
}

void EndStage::update(float dt)
{
	Game* game = Game::instance;
	if (time_in_stage > 0.8f && (Controls::buttonAPressed() || Controls::backPressed()))
	{
		game->playSound("confirm");
		game->setStage(STAGE_INTRO);
	}
}

void EndStage::render(Image& fb)
{
	Game* game = Game::instance;
	World& world = *game->world;

	drawPrisonBackground(fb, game->time, false);
	blendRect(fb, 0, 0, 160, 120, won ? Color(20, 90, 40, 150) : Color(90, 10, 20, 150));

	drawTextScaled(fb, won ? "FREEDOM!" : "BUSTED!", won ? 24 : 31, 10, game->font, 2);

	//big prisoner sprite, behind bars when lost
	int frame = won ? (int)(game->time * 6) % 2 : 0;
	fb.drawImage(game->player_sheet, frame * 8, (won ? DIR_RIGHT : DIR_DOWN) * 8, 8, 8, 64, 34, 32, 32);
	if (!won)
		for (int x = 62; x < 100; x += 8)
			fb.drawRectangle(x, 32, 2, 36, Color(150, 150, 165));

	drawTextCentered(fb, "TIME " + formatTime(world.total_time), 74, game->font);
	drawTextCentered(fb, "CAUGHT " + std::to_string(world.times_caught) + " TIMES", 85, game->minifont, 4, 6);
	if (!won)
		drawTextCentered(fb, "REACHED BLOCK " + std::to_string(world.level_index + 1), 93, game->minifont, 4, 6);

	if (time_in_stage > 0.8f && (int)(game->time * 2) % 2 == 0)
		drawTextCentered(fb, "PRESS Z", 106, game->font);
}
