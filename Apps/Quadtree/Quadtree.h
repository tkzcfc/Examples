#pragma once

#include <string>
#include <memory>
#include <vector>
#include <array>

#include "imgui.h"


struct QuadRect
{
	QuadRect(float _x, float _y, float _w, float _h)
		: x(_x)
		, y(_y)
		, width(_w)
		, height(_h)
	{
	}
	float x;
	float y;
	float width;
	float height;
};

template<typename T, uint32_t MaxObjects = 10, uint32_t MaxLevels = 4>
class Quadtree
{
public:

	Quadtree(QuadRect bounds, int level = 0)
		: m_bounds(bounds)
		, m_level(level)
	{
	}

	~Quadtree()
	{
		clear();
	}

	void insert(const QuadRect& rect, const T& data)
	{
		if (this->m_nodes[0] != NULL)
		{
			auto indexes = this->getIndex(rect);
			for (auto index : indexes)
			{
				this->m_nodes[index]->insert(rect, data);
			}
			return;
		}

		this->m_objects.push_back({ rect, data });

		if (this->m_objects.size() > MaxObjects && this->m_level < MaxLevels)
		{
			if (this->m_nodes[0] == NULL)
			{
				split();
			}

			for (auto& obj : this->m_objects)
			{
				auto indexes = this->getIndex(obj.bounds);
				for (auto& index : indexes)
				{
					this->m_nodes[index]->insert(obj.bounds, obj.data);
				}
			}

			this->m_objects.clear();
		}
	}

	void retrieve(const QuadRect& rect, std::vector<T>& returnObjects)
	{
		for (auto& obj : m_objects)
		{
			if (std::find(returnObjects.begin(), returnObjects.end(), obj.data) == returnObjects.end())
			{
				returnObjects.push_back(obj.data);
			}
		}

		if (this->m_nodes[0] != NULL)
		{
			auto indexes = this->getIndex(rect);
			for (auto index : indexes)
			{
				this->m_nodes[index]->retrieve(rect, returnObjects);
			}
		}
	}

	void clear()
	{
		this->m_objects.clear();
		for (auto& node : this->m_nodes)
		{
			node.release();
		}
	}

	void debugDraw(ImDrawList* draw_list, ImVec2 canvas_pos, int index = 0)
	{
		const float offset_value = 0.5f;

		ImVec2 v[4];
		v[0].x = m_bounds.x + canvas_pos.x + offset_value;
		v[0].y = m_bounds.y + canvas_pos.y + offset_value;

		v[1].x = m_bounds.x + canvas_pos.x + offset_value;
		v[1].y = m_bounds.y + canvas_pos.y + m_bounds.height - offset_value;

		v[2].x = m_bounds.x + canvas_pos.x + m_bounds.width - offset_value;
		v[2].y = m_bounds.y + canvas_pos.y + m_bounds.height - offset_value;

		v[3].x = m_bounds.x + canvas_pos.x + m_bounds.width - offset_value;
		v[3].y = m_bounds.y + canvas_pos.y + offset_value;

		draw_list->AddPolyline(v, 4, IM_COL32(0, 200, 200, 255), true, 0.0f);

		//static unsigned int colorArray[4] = {
		//	IM_COL32(100, 100, 0, 200),
		//	IM_COL32(150, 0, 0, 200),
		//	IM_COL32(0, 150, 0, 200),
		//	IM_COL32(0, 0, 150, 200),
		//};

		//draw_list->AddPolyline(v, 4, colorArray[index], true, 0.0f);

		if (this->m_nodes[0] != NULL)
		{
			index = 0;
			for(auto& node : this->m_nodes)
			{
				node->debugDraw(draw_list, canvas_pos, index++);
			}
		}
	}

private:

	void split()
	{
		auto nextLevel = this->m_level + 1;
		auto subWidth = this->m_bounds.width / 2;
		auto subHeight = this->m_bounds.height / 2;
		auto x = this->m_bounds.x;
		auto y = this->m_bounds.y;

		// top right
		this->m_nodes[0] = std::make_unique<Quadtree>(QuadRect(x + subWidth, y, subWidth, subHeight), nextLevel);
		// top left
		this->m_nodes[1] = std::make_unique<Quadtree>(QuadRect(x, y, subWidth, subHeight), nextLevel);
		// bottom left
		this->m_nodes[2] = std::make_unique<Quadtree>(QuadRect(x, y + subHeight, subWidth, subHeight), nextLevel);
		// bottom right
		this->m_nodes[3] = std::make_unique<Quadtree>(QuadRect(x + subWidth, y + subHeight, subWidth, subHeight), nextLevel);
	}

	std::vector<int> getIndex(const QuadRect& rect)
	{
		std::vector<int> indexes;
		auto verticalMidpoint = this->m_bounds.x + (this->m_bounds.width / 2);
		auto horizontalMidpoint = this->m_bounds.y + (this->m_bounds.height / 2);

		auto startIsNorth = rect.y < horizontalMidpoint;
		auto startIsWest = rect.x < verticalMidpoint;
		auto endIsEast = rect.x + rect.width > verticalMidpoint;
		auto endIsSouth = rect.y + rect.height > horizontalMidpoint;

		//top-right quad
		if (startIsNorth && endIsEast) {
			indexes.push_back(0);
		}

		//top-left quad
		if (startIsWest && startIsNorth) {
			indexes.push_back(1);
		}

		//bottom-left quad
		if (startIsWest && endIsSouth) {
			indexes.push_back(2);
		}

		//bottom-right quad
		if (endIsEast && endIsSouth) {
			indexes.push_back(3);
		}

		return indexes;
	}

private:
	int m_level;

	struct ObjectData
	{
		QuadRect bounds;
		T data;
	};

	std::vector<ObjectData> m_objects;
	std::array<std::unique_ptr<Quadtree>, 4> m_nodes;
	QuadRect m_bounds;
};
