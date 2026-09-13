#pragma once

#include "vector2.h"
#include <vector>

class Path
{
public:
	Path(const std::vector<Vector2>& point_list)
	{
		this->point_list = point_list;

		for (size_t i = 1; i < point_list.size(); ++i)
		{
			float segment_len = (point_list[i] - point_list[i - 1]).length();
			segment_len_list.push_back(segment_len);
			total_length += segment_len;
		}
	}
	~Path() = default;

	// 获取当前进度游戏进度处于游戏的哪个阶段
	Vector2 get_position_at_progress(float progress) const
	{
		if (progress <= 0) return point_list.front();
		if (progress >= 1) return point_list.back();

		float target_distance = total_length * progress;
		float accumulated_len = 0.0f;

		for (size_t i = 1; i < point_list.size(); ++i)
		{
			accumulated_len += segment_len_list[i - 1];

			if (accumulated_len >= target_distance)
			{
				float segment_progress = (target_distance - (accumulated_len - segment_len_list[i - 1])) / segment_len_list[i - 1];
				return point_list[i - 1] + (point_list[i] - point_list[i - 1]) * segment_progress;
			}
		}
		return point_list.back();
	}

private:
	float total_length = 0;
	// 每个顶点的坐标
	std::vector<Vector2> point_list;
	// 每两个顶点之间的路径片段长度
	std::vector<float> segment_len_list;
};
