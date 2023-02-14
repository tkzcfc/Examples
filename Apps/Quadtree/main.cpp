#include "Application.h"
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <utility>
#include <math.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>


#include <chrono>
#include <thread>
#include <mutex>
#include <set>

#include "texture/TextureCache.h"

#include "Quadtree.h"

#include "windows.h"

bool show_imgui_demo = false;

const char* Application_GetName()
{
    return "Control";
}



struct Rect : ImVec2
{
	float w;
	float h;
	bool quadtree_intersects;
	bool rect_intersects;
	bool isUser;

	float getMaxX() const
	{
		return x + w * 0.5f;
	}

	float getMaxY() const
	{
		return y + h * 0.5f;
	}

	float getMinX() const
	{
		return x - w * 0.5f;
	}

	float getMinY() const
	{
		return y - h * 0.5f;
	}

	bool intersectsRect(const Rect& rect) const
	{
		return !(getMaxX() < rect.getMinX() || rect.getMaxX() < getMinX() || getMaxY() < rect.getMinY() ||
			rect.getMaxY() < getMinY());
	}
};

inline float vec2Length(const ImVec2& pt)
{
	return ::sqrt(pt.x * pt.x + pt.y * pt.y);
}

std::vector<std::shared_ptr<Rect>> rects;
std::set<std::shared_ptr<Rect>> clickRects;

#define RANDOM_RECT_RANGE_W 400
#define RANDOM_RECT_RANGE_H 300

int random(int min, int max)
{
	return min + std::rand() % (max - min);
}

void Application_Initialize()
{
	for (auto i = 0; i < 100; ++i)
	{
		auto rect = std::make_shared<Rect>();
		rect->x = random(-RANDOM_RECT_RANGE_W, RANDOM_RECT_RANGE_W + 100);
		rect->y = random(-RANDOM_RECT_RANGE_H, RANDOM_RECT_RANGE_H + 100);
		rect->w = random(20, 70);
		rect->h = random(20, 70);
		rect->quadtree_intersects = false;
		rect->isUser = false;

		rects.push_back(rect);
	}


	for (auto i = 0; i < 1; ++i)
	{
		auto rect = std::make_shared<Rect>();
		rect->x = random(-200, 200);
		rect->y = random(-200, 200);
		rect->w = random(10, 50);
		rect->h = random(10, 50);
		rect->quadtree_intersects = false;
		rect->isUser = true;
		rects.push_back(rect);
	}
}

void Application_Finalize()
{
	TextureCache::getInstance()->releaseAll();
	TextureCache::destroy();
}



void drawTestWindow()
{
	ImGui::Begin("test");

	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	// Here we are using InvisibleButton() as a convenience to 1) advance the cursor and 2) allows us to use IsItemHovered()
			// But you can also draw directly and poll mouse/keyboard by yourself. You can manipulate the cursor using GetCursorPos() and SetCursorPos().
			// If you only use the ImDrawList API, you can notify the owner window of its extends by using SetCursorPos(max).
	ImVec2 canvas_pos = ImGui::GetCursorScreenPos();            // ImDrawList API uses screen coordinates!
	ImVec2 canvas_size = ImGui::GetContentRegionAvail();        // Resize canvas to what's available
	if (canvas_size.x < 50.0f) canvas_size.x = 50.0f;
	if (canvas_size.y < 50.0f) canvas_size.y = 50.0f;
	draw_list->AddRectFilledMultiColor(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), IM_COL32(50, 50, 50, 255), IM_COL32(50, 50, 50, 255), IM_COL32(50, 50, 50, 255), IM_COL32(50, 50, 50, 255));
	draw_list->AddRect(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), IM_COL32(255, 255, 255, 255));

	bool adding_preview = false;
	ImGui::InvisibleButton("canvas", canvas_size);
	

	draw_list->PushClipRect(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), true);      // clip lines within the canvas (if we resize it, etc.)

	ImVec2 center(canvas_size.x * 0.5, canvas_size.y * 0.5);

	ImVec2 mouse_pos_in_canvas = ImVec2(ImGui::GetIO().MousePos.x - canvas_pos.x, ImGui::GetIO().MousePos.y - canvas_pos.y);
	mouse_pos_in_canvas -= center;

	draw_list->AddLine(ImVec2(canvas_pos.x + center.x - canvas_size.x * 0.5f, canvas_pos.y + center.y), ImVec2(canvas_pos.x + center.x + canvas_size.x * 0.5f, canvas_pos.y + center.y), IM_COL32(255, 255, 255, 255));
	draw_list->AddLine(ImVec2(canvas_pos.x + center.x, canvas_pos.y + center.y - canvas_size.y * 0.5f), ImVec2(canvas_pos.x + center.x, canvas_pos.y + center.y + canvas_size.y * 0.5f), IM_COL32(255, 255, 255, 255));



	Quadtree<std::shared_ptr<Rect>> qtree(QuadRect(-RANDOM_RECT_RANGE_W, -RANDOM_RECT_RANGE_H, RANDOM_RECT_RANGE_W * 2, RANDOM_RECT_RANGE_H * 2));
	for (auto& rect : rects)
	{
		rect->quadtree_intersects = false;
		rect->rect_intersects = false;
		if (!rect->isUser)
		{
			qtree.insert(QuadRect(rect->x - rect->w * 0.5f, rect->y - rect->h * 0.5f, rect->w, rect->h), rect);
		}
	}


	for (auto& rect : rects)
	{
		if (rect->isUser)
		{
			std::vector<std::shared_ptr<Rect>> objects;
			qtree.retrieve(QuadRect(rect->x - rect->w * 0.5f, rect->y - rect->h * 0.5f, rect->w, rect->h), objects);
			for (auto& obj : objects)
			{
				obj->quadtree_intersects = true;
				if (rect->intersectsRect(*obj))
				{
					rect->rect_intersects = true;
					obj->rect_intersects = true;
				}
			}
		}
	}

	qtree.debugDraw(draw_list, canvas_pos + center);

	for (auto& rect : rects)
	{
		ImVec2 v[4];
		v[0].x = center.x + canvas_pos.x + rect->x - rect->w * 0.5f;
		v[0].y = center.y + canvas_pos.y + rect->y + rect->h * 0.5f;

		v[1].x = center.x + canvas_pos.x + rect->x + rect->w * 0.5f;
		v[1].y = center.y + canvas_pos.y + rect->y + rect->h * 0.5f;

		v[2].x = center.x + canvas_pos.x + rect->x + rect->w * 0.5f;
		v[2].y = center.y + canvas_pos.y + rect->y - rect->h * 0.5f;

		v[3].x = center.x + canvas_pos.x + rect->x - rect->w * 0.5f;
		v[3].y = center.y + canvas_pos.y + rect->y - rect->h * 0.5f;

		if (rect->rect_intersects)
		{
			if (rect->isUser)
				draw_list->AddPolyline(v, 4, IM_COL32(0, 255, 0, 255), true, 0.0f);
			else
				draw_list->AddPolyline(v, 4, IM_COL32(255, 0, 0, 255), true, 0.0f);
		}
		else
		{
			if (rect->quadtree_intersects)
			{
				draw_list->AddPolyline(v, 4, IM_COL32(255, 255, 255, 255), true, 0.0f);
			}
			else
			{
				if (rect->isUser)
					draw_list->AddPolyline(v, 4, IM_COL32(0, 255, 0, 255), true, 0.0f);
				else
					draw_list->AddPolyline(v, 4, IM_COL32(200, 200, 0, 255), true, 0.0f);
			}
		}
	}

	if (!ImGui::IsMouseDown(0))
	{
		clickRects.clear();
	}

	if (clickRects.empty() && ImGui::IsItemHovered() && ImGui::IsMouseDown(0))
	{	
		for (auto& rect : rects)
		{
			if (rect->isUser)
			{
				if (std::abs(mouse_pos_in_canvas.x - rect->x) <= rect->w * 0.5f &&
					std::abs(mouse_pos_in_canvas.y - rect->y) <= rect->h * 0.5f)
				{
					if (clickRects.count(rect) <= 0)
					{	
						rect->x = ImGui::GetIO().MousePos.x - canvas_pos.x - center.x;
						rect->y = ImGui::GetIO().MousePos.y - canvas_pos.y - center.y;
						clickRects.insert(rect);
					}
				}
			}
		}
	}

	mouse_pos_in_canvas = ImVec2(ImGui::GetIO().MousePos.x - canvas_pos.x, ImGui::GetIO().MousePos.y - canvas_pos.y);
	mouse_pos_in_canvas -= center;

	const float MAX_SPEED = 10.0f;

	for (auto& rect : clickRects)
	{
		ImVec2 pt = mouse_pos_in_canvas  - *rect;
		if (std::abs(vec2Length(pt) < 0.0001f))
		{
			continue;
		}
		float addx = pt.x * 0.15f;
		float addy = pt.y * 0.15f;
		if (std::abs(addx) > MAX_SPEED)
		{
			if (addx < 0.0f)
				addx = -MAX_SPEED;
			else
				addx = MAX_SPEED;
		}
		if (std::abs(addy) > MAX_SPEED)
		{
			if (addy < 0.0f)
				addy = -MAX_SPEED;
			else
				addy = MAX_SPEED;
		}

		rect->x += addx;
		rect->y += addy;
	}

	draw_list->PopClipRect();

	ImGui::End();
}

void Application_Frame()
{
	auto& io = ImGui::GetIO();
	ImGui::NewLine();
	ImGui::Text("FPS: %.2f (%.2gms)", io.Framerate, io.Framerate ? 1000.0f / io.Framerate : 0.0f);
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("Tool"))
		{
			ImGui::MenuItem("imgui demo", "", &show_imgui_demo);
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	if (show_imgui_demo)
	{
		ImGui::ShowDemoWindow(NULL);
	}

	drawTestWindow();
}

