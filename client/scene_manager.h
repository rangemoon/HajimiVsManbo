#pragma once
#include "scene.h"
#include "camera.h"

class SceneManager
{
public:
	SceneManager() = default;
	~SceneManager() = default;

	enum class SceneType
	{
		Waiting,
		Racing,
	};

public:
	void set_current_scene(Scene* scene)
	{
		current_scene = scene;
		current_scene->on_enter();
	}

	// 这里需要传入对应的对象场景指针，进行场景的切换
	void switch_scene(SceneType type)
	{
		current_scene->on_exit();
		switch (type)
		{
		case SceneManager::SceneType::Waiting:

			break;
		case SceneManager::SceneType::Racing:
			break;
		default:
			break;
		}
		current_scene->on_enter();
	}

	void on_update(int delta)
	{
		current_scene->on_update(delta);
	}

	void on_draw(const Camera& camera) {
		current_scene->on_draw(camera);
	}

	void on_input(const ExMessage& msg) {
		current_scene->on_input(msg);
	}

private:
	Scene* current_scene  =nullptr;
};
