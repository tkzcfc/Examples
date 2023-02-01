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


bool show_imgui_demo = false;

const char* Application_GetName()
{
    return "Control";
}



struct Rect : ImVec2
{
	float w;
	float h;
	bool intersects;

	float minx() const
	{
		return this->x - this->w * 0.5f;
	}
	float miny() const
	{
		return this->y - this->h * 0.5f;
	}

	float maxx() const
	{
		return this->x + this->w * 0.5f;
	}
	float maxy() const
	{
		return this->y + this->h * 0.5f;
	}

	float halfWidth() const
	{
		return this->w * 0.5f;
	}

	float halfHeight() const
	{
		return this->h * 0.5f;
	}
};
struct Circle : ImVec2
{
	bool intersects;
	float radius;
	ImVec2 prePoint;
};

inline float vec2Length(const ImVec2& pt)
{
	return ::sqrt(pt.x * pt.x + pt.y * pt.y);
}

inline void vec2Normal(ImVec2& pt)
{
	auto length = vec2Length(pt);
	if (length == 0.0f)
	{
		pt.x = 1.0f;
		pt.y = 0.0f;
	}
	else
	{
		pt.x = pt.x / length;
		pt.y = pt.y / length;
	}
}


std::vector<std::shared_ptr<Rect>> rects;
std::vector<std::shared_ptr<Circle>> circles;
std::set<std::shared_ptr<Circle>> clickCircles;

void Application_Initialize()
{
	{
		auto rect = std::make_shared<Rect>();
		rect->x = 0.0f;
		rect->y = 0.0f;
		rect->w = 300.0f;
		rect->h = 150.0f;
		rect->intersects = false;
		rects.push_back(rect);
	}

	{
		auto rect = std::make_shared<Rect>();
		rect->x = 200.0f;
		rect->y = 200.0f;
		rect->w = 200.0f;
		rect->h = 150.0f;
		rect->intersects = false;
		rects.push_back(rect);
	}

	{
		auto circle = std::make_shared<Circle>();
		circle->x = 0.0f;
		circle->y = 10.0f;
		circle->radius = 50.0f;
		circle->intersects = false;
		circles.push_back(circle);
	}


	{
		auto circle = std::make_shared<Circle>();
		circle->x = 100.0f;
		circle->y = 50.0f;
		circle->radius = 50.0f;
		circle->intersects = false;
		circles.push_back(circle);
	}

	{
		auto circle = std::make_shared<Circle>();
		circle->x = -100.0f;
		circle->y = 150.0f;
		circle->radius = 50.0f;
		circle->intersects = false;
		circles.push_back(circle);
	}
}

void Application_Finalize()
{
	TextureCache::getInstance()->releaseAll();
	TextureCache::destroy();
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////


bool CircleToBox(float cx, float cy, float radius, float bx, float by, float bw, float bh)
{
	const auto halfw = bw * 0.5f;
	const auto halfh = bh * 0.5f;

	float dx = std::abs(cx - bx);
	if (dx > halfw + radius)
		return false;

	float dy = std::abs(cy - by);
	if (dy > halfh + radius)
		return false;

	if (dx <= halfw || dy <= halfh)
		return true;

	const float xCornerDist = dx - halfw;
	const float yCornerDist = dy - halfh;
	const float xCornerDistSq = xCornerDist * xCornerDist;
	const float yCornerDistSq = yCornerDist * yCornerDist;
	const float maxCornerDistSq = radius * radius;
	return xCornerDistSq + yCornerDistSq <= maxCornerDistSq;
}


#if 0

#define OFFSET_VALUE 1.0f
void collisionDetection(std::shared_ptr<Rect>& rect, std::shared_ptr<Circle>& circle)
{
	if (!CircleToBox(circle->x, circle->y, circle->radius, rect->x, rect->y, rect->w, rect->h))
	{
		return;
	}

	auto diffx = circle->prePoint.x - circle->x;
	auto diffy = circle->prePoint.y - circle->y;

	bool crossx = false;
	bool crossy = false;

	if (circle->x >= rect->minx() && circle->x <= rect->maxx())
	{
		crossx = true;
	}
	if (circle->y >= rect->miny() && circle->y <= rect->maxy())
	{
		crossy = true;
	}

	if (crossx && crossy)
	{
		// 计算离那条边最近
		float distancex, distancey;
		if (circle->x >= rect->x)
		{
			distancex = rect->maxx() - (circle->x + circle->radius);
		}
		else
		{
			distancex = (circle->x - circle->radius) - rect->minx();
		}
		if (circle->y >= rect->y)
		{
			distancey = rect->maxy() - (circle->y + circle->radius);
		}
		else
		{
			distancey = (circle->y - circle->radius) - rect->miny();
		}

		crossx = distancex > distancey;
		crossy = !crossx;
	}

	if (crossx)
	{
		if (diffy == 0.0f)
		{
			if (circle->y >= rect->y)
				circle->y = rect->maxy() + circle->radius + OFFSET_VALUE;
			else
				circle->y = rect->miny() - circle->radius - OFFSET_VALUE;
		}
		else if (diffy > 0.0f)
			circle->y = rect->maxy() + circle->radius + OFFSET_VALUE;
		else
			circle->y = rect->miny() - circle->radius - OFFSET_VALUE;

		return;
	}

	if (crossy)
	{
		if (diffx == 0.0f)
		{
			if (circle->x >= rect->x)
				circle->x = rect->maxx() + circle->radius + OFFSET_VALUE;
			else
				circle->x = rect->minx() - circle->radius - OFFSET_VALUE;
		}
		else if (diffx > 0.0f)
			circle->x = rect->maxx() + circle->radius + OFFSET_VALUE;
		else
			circle->x = rect->minx() - circle->radius - OFFSET_VALUE;
		return;
	}


	/*
	1	________________	2
		|				|
		|			    |
		|               |
		|_______________|
	4						3
	*/

	ImVec2 crossPoint;
	if (circle->x < rect->x)
	{
		if (circle->y < rect->y)
		{
			// 1
			crossPoint.x = rect->minx() - OFFSET_VALUE;
			crossPoint.y = rect->miny() - OFFSET_VALUE;
		}
		else
		{
			// 4
			crossPoint.x = rect->minx() - OFFSET_VALUE;
			crossPoint.y = rect->maxy() + OFFSET_VALUE;
		}
	}
	else
	{
		if (circle->y < rect->y)
		{
			// 2
			crossPoint.x = rect->maxx() + OFFSET_VALUE;
			crossPoint.y = rect->miny() - OFFSET_VALUE;
		}
		else
		{
			// 3
			crossPoint.x = rect->maxx() + OFFSET_VALUE;
			crossPoint.y = rect->maxy() + OFFSET_VALUE;
		}
	}

	ImVec2 pt = *circle - crossPoint;
	vec2Normal(pt);

	pt.x *= circle->radius;
	pt.y *= circle->radius;

	circle->x = crossPoint.x + pt.x;
	circle->y = crossPoint.y + pt.y;
}
#undef OFFSET_VALUE

void testUpdate()
{
	for (auto& circle : circles)
	{
		circle->intersects = false;
	}
	for (auto& rect : rects)
	{
		rect->intersects = false;
		for (auto& circle : circles)
		{
			if (CircleToBox(circle->x, circle->y, circle->radius, rect->x, rect->y, rect->w, rect->h))
			{
				circle->intersects = true;
				rect->intersects = true;
			}
		}
	}


	if (clickCircles.empty())
	{
		for (auto& rect : rects)
		{
			for (auto& circle : circles)
			{
				collisionDetection(rect, circle);
			}
		}
	}


	for (auto& circle : circles)
	{
		circle->prePoint.x = circle->x;
		circle->prePoint.y = circle->y;
	}
}

#else

#define OFFSET_VALUE 0.0f
bool CircleToBoxCollision(float& cx, float& cy, float radius, float bx, float by, float bw, float bh)
{
	const auto halfw = bw * 0.5f;
	const auto halfh = bh * 0.5f;

	float dx = std::abs(cx - bx);
	if (dx > halfw + radius)
		return false;

	float dy = std::abs(cy - by);
	if (dy > halfh + radius)
		return false;

	if (dx <= halfw || dy <= halfh)
	{
		// change cx or cy
		if (halfw - dx > halfh - dy) {
			if (by > cy) {
				cy = by - halfh - radius - OFFSET_VALUE;
			}
			else {
				cy = by + halfh + radius + OFFSET_VALUE;
			}
		}
		else {
			if (bx > cx) {
				cx = bx - halfw - radius - OFFSET_VALUE;
			}
			else {
				cx = bx + halfw + radius + OFFSET_VALUE;
			}
		}
		return true;
	}

	const float xCornerDist = dx - halfw;
	const float yCornerDist = dy - halfh;
	const float xCornerDistSq = xCornerDist * xCornerDist;
	const float yCornerDistSq = yCornerDist * yCornerDist;
	const float maxCornerDistSq = radius * radius;
	if (xCornerDistSq + yCornerDistSq <= maxCornerDistSq)
	{
		// change cx & cy
		auto incX = xCornerDist, incY = yCornerDist;
		float dSeq = xCornerDistSq + yCornerDistSq;
		if (dSeq == 0.0f) {
			incX = halfw + radius / 1.414213562373095f + OFFSET_VALUE;
			incY = halfh + radius / 1.414213562373095f + OFFSET_VALUE;
		}
		else {
			auto d = ::sqrt(dSeq);
			incX = halfw + radius * xCornerDist / d + OFFSET_VALUE;
			incY = halfh + radius * yCornerDist / d + OFFSET_VALUE;
		}
		if (cx < bx) {
			incX = -incX;
		}
		if (cy < by) {
			incY = -incY;
		}
		cx = bx + incX;
		cy = by + incY;
		return true;
	}
	return false;
}
#undef OFFSET_VALUE

void testUpdate()
{
	for (auto& circle : circles)
	{
		circle->intersects = false;
	}
	for (auto& rect : rects)
	{
		rect->intersects = false;
		for (auto& circle : circles)
		{
			if (CircleToBox(circle->x, circle->y, circle->radius, rect->x, rect->y, rect->w, rect->h))
			{
				circle->intersects = true;
				rect->intersects = true;
			}
		}
	}


	//if (clickCircles.empty())
	{
		for (auto& rect : rects)
		{
			for (auto& circle : circles)
			{
				CircleToBoxCollision(circle->x, circle->y, circle->radius, rect->x, rect->y, rect->w, rect->h);
			}
		}
	}
}

#endif


//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////


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

		if(rect->intersects)
			draw_list->AddPolyline(v, 4, IM_COL32(255, 0, 0, 255), true, 0.0f);
		else
			draw_list->AddPolyline(v, 4, IM_COL32(100, 255, 100, 255), true, 0.0f);
	}

	if (!ImGui::IsMouseDown(0))
	{
		clickCircles.clear();
	}

	for (auto& circle : circles)
	{
		bool click = false;
		if (clickCircles.empty() && ImGui::IsItemHovered() && ImGui::IsMouseDown(0))
		{
			ImVec2 circleCenter(circle->x, circle->y);
			auto disV = mouse_pos_in_canvas - circleCenter;
			if ((disV.x * disV.x + disV.y * disV.y) <= (circle->radius * circle->radius))
				click = true;

			if (click && clickCircles.count(circle) <= 0)
			{
				circle->x = ImGui::GetIO().MousePos.x - canvas_pos.x - center.x;
				circle->y = ImGui::GetIO().MousePos.y - canvas_pos.y - center.y;
			}
		}

		if (click || clickCircles.count(circle) > 0)
		{
			clickCircles.insert(circle);
			if (circle->intersects)
				draw_list->AddCircle(ImVec2(center.x + canvas_pos.x + circle->x, center.y + canvas_pos.y + circle->y), circle->radius, IM_COL32(255, 0, 0, 100), 100);
			else
				draw_list->AddCircle(ImVec2(center.x + canvas_pos.x + circle->x, center.y + canvas_pos.y + circle->y), circle->radius, IM_COL32(100, 255, 100, 100), 100);
		}
		else
		{
			if (circle->intersects)
				draw_list->AddCircle(ImVec2(center.x + canvas_pos.x + circle->x, center.y + canvas_pos.y + circle->y), circle->radius, IM_COL32(255, 0, 0, 255), 100);
			else
				draw_list->AddCircle(ImVec2(center.x + canvas_pos.x + circle->x, center.y + canvas_pos.y + circle->y), circle->radius, IM_COL32(100, 255, 100, 255), 100);
		}
	}

	mouse_pos_in_canvas = ImVec2(ImGui::GetIO().MousePos.x - canvas_pos.x, ImGui::GetIO().MousePos.y - canvas_pos.y);
	mouse_pos_in_canvas -= center;

	const float MAX_SPEED = 10.0f;

	for (auto& circle : clickCircles)
	{
		ImVec2 pt = mouse_pos_in_canvas  - *circle;
		if (std::abs(vec2Length(pt) < 0.0001f))
		{
			continue;
		}
		//vec2Normal(pt);

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

		circle->x += addx;
		circle->y += addy;
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
	testUpdate();
}

