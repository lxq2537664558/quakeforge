#include "gui/Point.h"

Point makePoint (integer x, integer y)
{
	Point p = {x, y};
	return p;
}

Point addPoint (Point a, Point b)
{
	Point c = {a.x + b.x, a.y + b.y};
	return c;
}

Point subtractPoint (Point a, Point b)
{
	Point c = {a.x - b.x, a.y - b.y};
	return c;
}
