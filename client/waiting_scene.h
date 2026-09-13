#pragma once

#include "scene.h"
#include "scene_manager.h"
#include "camera.h"

extern IMAGE img_waiting_background;

class WaitingScene : public Scene
{
public:
	WaitingScene() = default;
	~WaitingScene() = default;

	void on_update(int delta) {}

	void on_draw(const Camera& camera)
	{
		putimage(0, 0, &img_waiting_background);
		settextcolor(RGB(195, 195, 195));
		outtextxy(15, 675, _T("比赛即将开始，等待其他玩家加入..."));
	}

	void on_input(const ExMessage& msg) {}

	void on_enter() {}

	void on_exit() {}
private:

};
