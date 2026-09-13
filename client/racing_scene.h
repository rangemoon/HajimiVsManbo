#pragma once

#include "scene.h"
#include "player.h"

extern Player* player_1;
extern Player* player_2;
extern Timer timer_countdown;


class RacingScene : public Scene
{
public:
	RacingScene() = default;
	~RacingScene() = default;

	void on_update(int delta)
	{
		player_1->on_update(delta);
		player_2->on_update(delta);

	}

	void on_enter()
	{
		// 计时器的初始化
		timer_countdown.set_one_shot(false);
		timer_countdown.set_wait_time(1.0f);

		timer_countdown.set_on_timeout([&]()
			{
				val_countdown--;

				switch (val_countdown)
				{
				case 3: play_audio(_T("ui_3")); break;
				case 2: play_audio(_T("ui_2")); break;
				case 1: play_audio(_T("ui_1")); break;
				case 0: play_audio(_T("ui_fight")); break;
				case -1:
					//stage = Stage::Racing;
					play_audio(_T("bgm"), true);
					break;
				}
			});
	}

private:
	int val_countdown = 4;
	enum class Stage
	{
		Ready,
		Racing
	};
};