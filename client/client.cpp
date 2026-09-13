//#include "../thirdparty/httplib.h"
//
//int main(int argc, char** argv) {
//	httplib::Client client("localhost:25565");
//	httplib::Result result = client.Post("/hello");
//	if (!result || result->status != 200) {
//		std::cout << "Hello Failed!" << std::endl;
//		return -1;
//	}
//	std::cout << result->body << std::endl;
//	system("pause");
//	return 0;
//}

#include "../thirdparty/httplib.h"
#include "path.h"
#include "player.h"
#include "scene_manager.h"
#include "racing_scene.h"
#include "waiting_scene.h"
#include <chrono>
#include <string>
#include <vector>
#include <thread>
#include <codecvt>
#include <fstream>
#include <sstream>

#pragma region init global var

Player* player_1 = nullptr;
Player* player_2 = nullptr;
SceneManager* scene_manager = nullptr;
Scene* racing_scene = new RacingScene();
Scene* waiting_scene = new WaitingScene();

// 构造和初始化游戏中的对象
ExMessage msg;
Timer timer_countdown;
Camera camera_ui, camera_scene;

// Ready阶段，变成了Racing的一个小过程
enum class Stage
{
	Waiting,						// 等待玩家加入
	Ready,							// 准备起跑倒计时
	Racing							// 正在比赛中
};

//int val_countdown = 4;				// 起跑倒计时 - 直接在游戏场景中定义即可
Stage stage = Stage::Waiting;		// 当前游戏状态
int player_id = 0;
int num_total_char = 0;				// 全部字符数

// 使用多线程中的不可分割的原子操作，防止变量在多线程编程中产生变量冲突问题
std::atomic<int> progress_1 = -1;
std::atomic<int> progress_2 = -1;

Path path = Path({
	{842, 842}, {1322, 842}, {1322, 422},
	{2762, 442}, {2762, 842}, {3162, 842},
	{3162, 1722}, {2122, 1722}, {2122, 1562},
	{842, 1562}, {842, 842}
	});

int idx_line = 0;					// 当前文本行索引
int idx_char = 0;					// 当前行文本字符索引
std::string str_text;				// 文本内容
// 行文本列表
std::vector<std::string> str_line_list;
#pragma endregion

#pragma region init and load resources

Atlas atlas_1P_idle_up;
Atlas atlas_1P_idle_down;
Atlas atlas_1P_idle_left;
Atlas atlas_1P_idle_right;
Atlas atlas_1P_run_up;
Atlas atlas_1P_run_down;
Atlas atlas_1P_run_left;
Atlas atlas_1P_run_right;
Atlas atlas_2P_idle_up;
Atlas atlas_2P_idle_down;
Atlas atlas_2P_idle_left;
Atlas atlas_2P_idle_right;
Atlas atlas_2P_run_up;
Atlas atlas_2P_run_down;
Atlas atlas_2P_run_left;
Atlas atlas_2P_run_right;

IMAGE img_ui_1;						// 界面文本1
IMAGE img_ui_2;						// 界面文本2
IMAGE img_ui_3;						// 界面文本3
IMAGE img_ui_fight;					// 界面文本FIGHT
IMAGE img_ui_textbox;				// 界面文本框
IMAGE img_background;				// 背景图
IMAGE img_waiting_background;		// 等待界面背景图

std::string str_address;			// 服务器地址
httplib::Client* client = nullptr;	// http客户端对象

void load_resource(HWND hwnd)
{
	AddFontResourceEx(_T("resources/IPix.ttf"), FR_PRIVATE, NULL);

	atlas_1P_idle_up.load(_T("resources/hajimi_idle_back_%d.png"), 4);
	atlas_1P_idle_down.load(_T("resources/hajimi_idle_front_%d.png"), 4);
	atlas_1P_idle_left.load(_T("resources/hajimi_idle_left_%d.png"), 4);
	atlas_1P_idle_right.load(_T("resources/hajimi_idle_right_%d.png"), 4);
	atlas_1P_run_up.load(_T("resources/hajimi_run_back_%d.png"), 4);
	atlas_1P_run_down.load(_T("resources/hajimi_run_front_%d.png"), 4);
	atlas_1P_run_left.load(_T("resources/hajimi_run_left_%d.png"), 4);
	atlas_1P_run_right.load(_T("resources/hajimi_run_right_%d.png"), 4);
	atlas_2P_idle_up.load(_T("resources/manbo_idle_back_%d.png"), 4);
	atlas_2P_idle_down.load(_T("resources/manbo_idle_front_%d.png"), 4);
	atlas_2P_idle_left.load(_T("resources/manbo_idle_left_%d.png"), 4);
	atlas_2P_idle_right.load(_T("resources/manbo_idle_right_%d.png"), 4);
	atlas_2P_run_up.load(_T("resources/manbo_run_back_%d.png"), 4);
	atlas_2P_run_down.load(_T("resources/manbo_run_front_%d.png"), 4);
	atlas_2P_run_left.load(_T("resources/manbo_run_left_%d.png"), 4);
	atlas_2P_run_right.load(_T("resources/manbo_run_right_%d.png"), 4);

	loadimage(&img_ui_1, _T("resources/ui_1.png"));
	loadimage(&img_ui_2, _T("resources/ui_2.png"));
	loadimage(&img_ui_3, _T("resources/ui_3.png"));
	loadimage(&img_ui_fight, _T("resources/ui_fight.png"));
	loadimage(&img_ui_textbox, _T("resources/ui_textbox.png"));
	loadimage(&img_background, _T("resources/background.png"));
	loadimage(&img_waiting_background, _T("resources/waiting_background.png"));

	load_audio(_T("resources/bgm.mp3"), _T("bgm"));
	load_audio(_T("resources/1P_win.mp3"), _T("1p_win"));
	load_audio(_T("resources/2P_win.mp3"), _T("2p_win"));
	load_audio(_T("resources/click_1.mp3"), _T("click_1"));
	load_audio(_T("resources/click_2.mp3"), _T("click_2"));
	load_audio(_T("resources/click_3.mp3"), _T("click_3"));
	load_audio(_T("resources/click_4.mp3"), _T("click_4"));
	load_audio(_T("resources/ui_1.mp3"), _T("ui_1"));
	load_audio(_T("resources/ui_2.mp3"), _T("ui_2"));
	load_audio(_T("resources/ui_3.mp3"), _T("ui_3"));
	load_audio(_T("resources/ui_fight.mp3"), _T("ui_fight"));

	std::ifstream file("config.cfg");

	if (!file.good())
	{
		MessageBox(hwnd, L"无法打开配置 config.cfg", L"启动失败", MB_OK | MB_ICONERROR);
		exit(-1);
	}

	std::stringstream str_stream;
	str_stream << file.rdbuf();
	str_address = str_stream.str();

	file.close();
}
#pragma endregion

// 登录到服务器
void login_to_server(HWND hwnd)
{
	client = new httplib::Client(str_address);
	client->set_keep_alive(true);

	httplib::Result result = client->Post("/login");
	// 无响应或者响应异常
	if (!result || result->status != 200)
	{
		MessageBox(hwnd, _T("无法连接到服务器！"), _T("启动失败"), MB_OK | MB_ICONERROR);
		exit(-1);
	}

	player_id = std::stoi(result->body);

	if (player_id <= 0)
	{
		MessageBox(hwnd, _T("比赛已经开始啦!"), _T("拒绝加入"), MB_OK | MB_ICONERROR);
		exit(-1);
	}
	(player_id == 1) ? (progress_1 = 0) : (progress_2 = 0);
	str_text = client->Post("/query_text")->body;

	// 统计游戏所需要输入的文本字符数
	std::stringstream str_stream(str_text);
	std::string str_line;
	while (std::getline(str_stream, str_line))
	{
		str_line_list.push_back(str_line);
		num_total_char += (int)str_line.length();
	}

	// 开辟独立线程,线构造线程对象
	// 使用detach与主线程进行分离，与主线程并行执行
	std::thread([&]()
		{
			// 使用死循环，每过0.1s之后执行下一次循环
			while (true)
			{
				using namespace std::chrono;
				// 更具当前玩家id 选择不同的路由
				std::string route = (player_id == 1) ? "/update_1" : "/update_2";
				// 提交本地玩家的数据过后，返回结果中的另一个玩家结果进行更新
				std::string body = std::to_string((player_id == 1) ? progress_1 : progress_2);

				httplib::Result result = client->Post(route, body, "text/plain");

				if (result && result->status == 200)
				{
					int progress = std::stoi(result->body);
					(player_id == 1) ? (progress_2 = progress) : (progress_1 = progress);
				}

				std::this_thread::sleep_for(nanoseconds(1000000000 / 10));
			}
		}).detach();
}

int main(int argc, char** argv)
{
	// 初始化数据
	#pragma region init resources
	using namespace std::chrono;

	HWND hwnd = initgraph(1280, 720);

	SetWindowText(hwnd, _T("哈基米大冒险！"));
	settextstyle(28, 0, _T("IPix"));

	setbkmode(TRANSPARENT);

	load_resource(hwnd);
	login_to_server(hwnd);

	// 修改为全局变量
	player_1->init(&atlas_1P_idle_up, &atlas_1P_idle_down, &atlas_1P_idle_left, &atlas_1P_idle_right,
		 &atlas_1P_run_up, &atlas_1P_run_down, &atlas_1P_run_left, &atlas_1P_run_right);
	player_2->init(&atlas_2P_idle_up, &atlas_2P_idle_down, &atlas_2P_idle_left, &atlas_2P_idle_right,
	 	 &atlas_2P_run_up, &atlas_2P_run_down, &atlas_2P_run_left, &atlas_2P_run_right);
	camera_ui.set_size({ 1280, 720 });
	camera_scene.set_size({ 1280, 720 });
	player_1->set_position({ 842, 842 });
	player_2->set_position({ 842, 842 });
	scene_manager->set_current_scene(waiting_scene);

	// 计时器的初始化
	// - 每一次重新开始游戏时，都需要重置计时器，所以要将计时器的初始化放置到racing_scene的on_enter方法中
	/*timer_countdown.set_one_shot(false);
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
				stage = Stage::Racing;
				play_audio(_T("bgm"), true);
				break;
			}
		});*/

	const nanoseconds frame_duration(1000000000 / 144);
	steady_clock::time_point last_tick = steady_clock::now();

	BeginBatchDraw();

	#pragma endregion

	while (1)
	{
		#pragma region handle player input

		while (peekmessage(&msg))
		{
			//if (stage != Stage::Racing)
			//	continue;
			//if (msg.message == WM_CHAR && idx_line < str_line_list.size())
			//{
			//	// 将玩家的输入文本内容与target文本内容进行比对，如果正确则进行音频播放和游戏进度的更新操作
			//	const std::string& str_line = str_line_list[idx_line];
			//	if (str_line[idx_char] == msg.ch)
			//	{
			//		switch (rand() % 4)
			//		{
			//		case 0: play_audio(_T("click_1")); break;
			//		case 1: play_audio(_T("click_2")); break;
			//		case 2: play_audio(_T("click_3")); break;
			//		case 3: play_audio(_T("click_4")); break;
			//		}

			//		(player_id == 1) ? progress_1++ : progress_2++;
			//		
			//		// 如果当前行已经输入完毕时，则切换到下一行即可
			//		idx_char++;
			//		if (idx_char >= str_line.length())
			//		{
			//			idx_char = 0;
			//			idx_line++;
			//		}
			//	}
			//}

			scene_manager->on_input(msg);
		}

		#pragma endregion

		#pragma region handle game update

		steady_clock::time_point frame_start = steady_clock::now();
		duration<float> delta = duration<float>(frame_start - last_tick);

		// 修改为player_1和player_2的update set_target look_at方法
		scene_manager->on_update(delta.count());

		//if (stage == Stage::Waiting)
		//{
		//	if (progress_1 >= 0 && progress_2 >= 0)
		//		stage = Stage::Ready;
		//}
		//else 
		//{
		//	if (stage == Stage::Ready)
		//		timer_countdown.on_update(delta.count());

		//	if((player_id == 1 && progress_1 >= num_total_char)
		//		|| (player_id == 2 && progress_2 >= num_total_char))
		//	{
		//		settextcolor(RGB(195, 195, 195));
		//		outtextxy(1280 / 2 - 32, 720 / 2 - 32, _T("恭喜你赢得了比赛"));
		//		stop_audio(_T("bgm"));
		//		play_audio((player_id == 1) ? _T("1p_win") : _T("2p_win"));
		//		// MessageBox(hwnd, _T("恭喜胜利！"), _T("游戏结束！"), MB_OK | MB_ICONINFORMATION);
		//		exit(0);
		//	}
		//	else if ((player_id == 1 && progress_2 >= num_total_char)
		//		|| (player_id == 2 && progress_1 >= num_total_char))
		//	{
		//		settextcolor(RGB(195, 195, 195));
		//		outtextxy(1280 / 2 - 32, 720 / 2 - 32, _T("很遗憾你输了"));
		//		stop_audio(_T("bgm"));
		//		play_audio((player_id == 1) ? _T("1p_win") : _T("2p_win"));
		//		// MessageBox(hwnd, _T("摸头！摸头！"), _T("游戏结束！"), MB_OK | MB_ICONINFORMATION);
		//		exit(0);
		//	}

		//	player_1->set_target(path.get_position_at_progress((float)progress_1 / num_total_char));
		//	player_2->set_target(path.get_position_at_progress((float)progress_2 / num_total_char));

		//	player_1->on_update(delta.count());
		//	player_2->on_update(delta.count());

		//	// 让场景摄像机跟随本地客户端
		//	camera_scene.look_at((player_id == 1)
		//		? player_1->get_position() : player_2->get_position());
		//}

		#pragma endregion

		#pragma region handle game draw
		
		// 清空上一帧的内容
		setbkcolor(RGB(0, 0, 0));
		cleardevice();

		scene_manager->on_draw(camera_scene);

		//if (stage == Stage::Waiting)
		//{
		//	settextcolor(RGB(195, 195, 195));
		//	outtextxy(15, 675, _T("比赛即将开始，等待其他玩家加入..."));
		//}
		//else
		//{
		//	// 绘制背景
		//	static const Rect rect_bg =
		//	{
		//		0, 0,
		//		img_background.getwidth(),
		//		img_background.getheight()
		//	};
		//	putimage_ex(camera_scene, &img_background, &rect_bg);

		//	//putimage_alpha(camera_scene, 0, 0, &img_background);

		//	// 绘制玩家
		//	if (player_1->get_position().y > player_2->get_position().y)
		//	{
		//		player_2->on_render(camera_scene);
		//		player_1->on_render(camera_scene);
		//	}
		//	else
		//	{
		//		player_1->on_render(camera_scene);
		//		player_2->on_render(camera_scene);
		//	}

		//	// 绘制倒计时
		//	switch (val_countdown)
		//	{
		//	case 3:
		//	{
		//		static const Rect rect_ui_3 =
		//		{
		//			1280 / 2 - img_ui_3.getwidth() / 2,
		//			720 / 2 - img_ui_3.getheight() / 2,
		//			img_ui_3.getwidth(), img_ui_3.getheight()
		//		};
		//		putimage_ex(camera_ui, &img_ui_3, &rect_ui_3);
		//	}
		//	// putimage_alpha(camera_ui, 1280 / 2 - img_ui_3.getwidth() / 2, 720 / 2 - img_ui_3.getheight() / 2, &img_ui_3);
		//	break;
		//	case 2:
		//	{
		//		static const Rect rect_ui_2 =
		//		{
		//			1280 / 2 - img_ui_2.getwidth() / 2,
		//			720 / 2 - img_ui_2.getheight() / 2,
		//			img_ui_2.getwidth(), img_ui_2.getheight()
		//		};
		//		putimage_ex(camera_ui, &img_ui_2, &rect_ui_2);
		//	// putimage_alpha(camera_ui, 1280 / 2 - img_ui_2.getwidth() / 2, 720 / 2 - img_ui_2.getheight() / 2, &img_ui_2);
		//	}
		//	break;
		//	case 1:
		//	{
		//		static const Rect rect_ui_1 =
		//		{
		//			1280 / 2 - img_ui_1.getwidth() / 2,
		//			720 / 2 - img_ui_1.getheight() / 2,
		//			img_ui_1.getwidth(), img_ui_1.getheight()
		//		};
		//		putimage_ex(camera_ui, &img_ui_1, &rect_ui_1);
		//	// putimage_alpha(camera_ui, 1280 / 2 - img_ui_1.getwidth() / 2, 720 / 2 - img_ui_1.getheight() / 2, &img_ui_1);
		//	}
		//	break;
		//	case 0:
		//	{
		//		static const Rect rect_ui_fight =
		//		{
		//			1280 / 2 - img_ui_fight.getwidth() / 2,
		//			720 / 2 - img_ui_fight.getheight() / 2,
		//			img_ui_fight.getwidth(), img_ui_fight.getheight()
		//		};
		//		putimage_ex(camera_ui, &img_ui_fight, &rect_ui_fight);
		//	// putimage_alpha(camera_ui, 1280 / 2 - img_ui_fight.getwidth() / 2, 720 / 2 - img_ui_fight.getheight() / 2, &img_ui_fight);
		//	}
		//	break;
		//	default: break;
		//	}

		//	// 绘制界面
		//	if (stage == Stage::Racing)
		//	{
		//		static const Rect rect_textbox =
		//		{
		//			0,
		//			720 - img_ui_textbox.getheight(),
		//			img_ui_textbox.getwidth(),
		//			img_ui_textbox.getheight()
		//		};
		//		static std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> convert;
		//		std::wstring wstr_line = convert.from_bytes(str_line_list[idx_line]);
		//		std::wstring wstr_completed = convert.from_bytes(str_line_list[idx_line].substr(0, idx_char));

		//		putimage_ex(camera_ui, &img_ui_textbox, &rect_textbox);
		//		// putimage_alpha(camera_ui, 0, 720 - img_ui_textbox.getheight(), &img_ui_textbox);
		//		settextcolor(RGB(125, 125, 125));
		//		outtextxy(185 + 2, rect_textbox.y + 65 + 2, wstr_line.c_str());
		//		settextcolor(RGB(25, 25, 25));
		//		outtextxy(185, rect_textbox.y + 65, wstr_line.c_str());
		//		settextcolor(RGB(0, 149, 217));
		//		outtextxy(185, rect_textbox.y + 65, wstr_completed.c_str());
		//	}
		//}

		FlushBatchDraw();

		// 计算动态延时的时间保持帧更新稳定
		last_tick = frame_start;
		nanoseconds sleep_duration = frame_duration - (steady_clock::now() - frame_start);
		if (sleep_duration > nanoseconds(0))
		{
			std::this_thread::sleep_for(sleep_duration);
		}
		#pragma endregion
	}
	EndBatchDraw();
	return 0;
}
