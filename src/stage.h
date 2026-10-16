/*  Stages split the application states: title menu, tutorial, gameplay and end screen.
	Game only forwards update/render to the current stage.
*/
#ifndef STAGE_H
#define STAGE_H

#include "image.h"

enum eStageId { STAGE_INTRO, STAGE_TUTORIAL, STAGE_PLAY, STAGE_END, STAGE_COUNT };

class Stage
{
public:
	float time_in_stage = 0.0f;

	virtual ~Stage() {}
	virtual void onEnter() {}
	virtual void update(float dt) = 0;
	virtual void render(Image& fb) = 0;
};

class IntroStage : public Stage
{
public:
	int selected = 0;

	void onEnter() override;
	void update(float dt) override;
	void render(Image& fb) override;
};

class TutorialStage : public Stage
{
public:
	void update(float dt) override;
	void render(Image& fb) override;
};

class PlayStage : public Stage
{
public:
	Vector2 camera;
	float banner_timer = 0.0f;	//"LEVEL N" intro, gameplay frozen
	float caught_timer = 0.0f;	//freeze after being caught
	float exit_timer = 0.0f;	//freeze after reaching the exit
	float shake_timer = 0.0f;

	void onEnter() override;
	void update(float dt) override;
	void render(Image& fb) override;

private:
	void startLevel(int index);
	void updateCamera(float dt, bool snap);
	void renderHUD(Image& fb);
};

class EndStage : public Stage
{
public:
	bool won = false;

	void onEnter() override;
	void update(float dt) override;
	void render(Image& fb) override;
};

//text helpers shared by the stages
void drawTextCentered(Image& fb, const std::string& text, int y, const Image& font, int font_w = 7, int font_h = 9);
void drawTextScaled(Image& fb, const std::string& text, int x, int y, const Image& font, int scale);

#endif
