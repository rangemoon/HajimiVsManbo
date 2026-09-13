/*
 * 1. 需求分析：
 * - 绘制客户端的画面需要所有玩家在地图位置的实时坐标和两个玩家的打字进度
 * - 因为游戏玩家的游戏进度更新依据的是固定的文本内容，而且不同客户端玩家的地图路径内容是完全相同的，我们只需要同步完成度即可。
 * - 各个玩家的位置和数据只需要交给客户端进行渲染即可
 * 2. 专业术语：序列化和反序列化：
 * - 序列化：将玩家类的字段序列化成一定格式的字符串的过程
 * - 反序列化：将玩家的状态信息和位置信息字符串解析到游戏对象的过程是反序列化的过程
 */

/*
 * ？？多线程？？
 * 
 * 同一时刻有多个客户端访问了同一个服务器的hello路由，会造成数据竞争的问题产生
 * 我们需要对临界区的数据进行保护，保证在同一时间内只有一个线程进入临界区。
 * 互斥锁:mutex可以进行lock和unlock操作
 * 失去unlock就会造成死锁
 * 
 * 为了避免出现死锁的现象，我们引入了RAII(Resource Acquisition Is Initialization)资源获取及初始化
 * 将锁的创建和销毁于对象的构造和析构绑定(lock_guard即为定义好的对象)
 * 
 */
#include "../thirdparty/httplib.h"
#include <mutex>
#include <string>
#include <fstream>

// 全局互斥锁和全局文本内容
std::string str_text;
std::mutex g_mutex;

// 两位玩家的进度表示
int progress_1 = -1;
int progress_2 = -1;

//void on_hello(const httplib::Request& req, httplib::Response& res) {
//	std::lock_guard<std::mutex> lock(g_mutex);
//	str_text = req.body;
//	std::cout << "Hello From Client!" << std::endl;
//	res.set_content("Hello From Server!", "text/plain");
//}

int main(int argc, char** argv) {
	//httplib::Server server;

	///*server.Post("/hello", [&](const httplib::Request& req, httplib::Response& res) {
	//	std::cout << "Hello From Client" << std::endl;

	//	res.set_content("Hello From Server", "text/plain");
	//	});*/

	//server.Post("/hello", on_hello);

	//server.listen("localhost", 25565);
	// 从名为text.txt中读取内容，创建一个文件输入流
	// 如果打开不成功，产生消息弹框
	// 如果打开成功，则创建字符串流str_stream，并且将缓存区的字符串读入到str_stream中，给全局变量str_text赋值
	std::ifstream file("text.txt");
	if (!file.good()) {
		MessageBox(nullptr, L"无法打开文本文件 text.txt", L"启动失败", MB_OK | MB_ICONERROR);
		return -1;
	}
	std::stringstream str_stream;
	str_stream << file.rdbuf();
	str_text = str_stream.str();

	file.close();

	httplib::Server server;

	server.Post("/login", [&](const httplib::Request& req, httplib::Response& res)
		{
			std::lock_guard<std::mutex> lock(g_mutex);
			// 如果检测到两个玩家都进入游戏，对后面进入游戏的玩家进行返回操作
			if (progress_1 >= 0 && progress_2 >= 0)
			{
				res.set_content("-1", "text/plain");
				return;
			}

			res.set_content(progress_1 >= 0 ? "2" : "1", "text/plain");
			// 初始化游戏进度
			(progress_1 >= 0) ? (progress_2 = 0) : (progress_1 = 0);
		});

	// 向客户端提供接口，方便客户端向服务器发送请求，找到游戏的文本字段
	server.Post("/query_text", [&](const httplib::Request& req, httplib::Response& res)
		{
			res.set_content(str_text, "text/plain");
		});

	// 玩家1和玩家2 读取当前玩家的进度，将它转化为整形，然后获得另一个玩家的进度
	server.Post("/update_1", [&](const httplib::Request& req, httplib::Response& res)
		{
			std::lock_guard<std::mutex> lock(g_mutex);
			progress_1 = std::stoi(req.body);
			res.set_content(std::to_string(progress_2), "text/plain");
		});

	server.Post("/update_2", [&](const httplib::Request& req, httplib::Response& res)
		{
			std::lock_guard<std::mutex> lock(g_mutex);
			progress_2 = std::stoi(req.body);
			res.set_content(std::to_string(progress_1), "text/plain");
		});

	server.listen("0.0.0.0", 25565);
	return 0;
}   