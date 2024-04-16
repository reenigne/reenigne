// Sign of the z projection of the cross product of two vectors
bool orientation(Vector a, Vector b)
{
  return a.x*b.y-b.x*a.y > 0;
}

// Orientation of triangle abc. Or, is point c on the inside or the outside of
// the half plane described by the directed line containing points a and b.
bool orientation(Vector a, Vector b, Vector c)
{
  return orientation(a-c, b-c);
  // == (a-c).x*(b-c).y-(b-c).x*(a-c).y
  // == (a.x-c.x)*(b.y-c.y)-(b.x-c.x)*(a.y-c.y)
  // == (a.x*(b.y-c.y)-c.x*(b.y-c.y))-(b.x*(a.y-c.y)-c.x*(a.y-c.y))
  // == ((a.x*b.y-a.x*c.y)-(c.x*b.y-c.x*c.y))-((b.x*a.y-b.x*c.y)-(c.x*a.y-c.x*c.y))
  // == (a.x*b.y-a.x*c.y-c.x*b.y+c.x*c.y)-(b.x*a.y-b.x*c.y-c.x*a.y+c.x*c.y)
  // == a.x*b.y-a.x*c.y-c.x*b.y+c.x*c.y-b.x*a.y+b.x*c.y+c.x*a.y-c.x*c.y
  // == a.x*b.y-a.x*c.y-c.x*b.y-b.x*a.y+b.x*c.y+c.x*a.y
  // == a.x*b.y-b.x*a.y+(b.x-a.x)*c.y+c.x*(a.y-b.y)
}

// Is point p inside triangle abc?
bool insideTriangle(Vector a, Vector b, Vector c, Vector p)
{
  return orientation(b-a,p-a) && orientation(c-b,p-b) && orientation(a-c,p-c);
  // == orientation(b,p,a) && orientation(c,p,b) && orientation(a,p,c)
  // == orientation(a,b,p) && orientation(b,c,p) && orientation(c,a,p)
}

Verticalish line: divide through by spany

Suppose ax,ay == 1,1 and bx,by == 1,0
spanx = ax - bx = 0                                     a
spany = ay - by = 1                                    cb
intersect = ax*by - bx*ay = 1*0-1*1 = -1
Test point c at 0,0 => intersect = -1  so negative means abc is in clockwise order

Horizontalish:
  c.x is multiplied by span and added
  c.y is added
Verticalish:
  c.x is added
  c.y is multiplied by span and added
