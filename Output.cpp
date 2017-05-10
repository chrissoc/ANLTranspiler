
#include <string>

const static std::string OutputString = R"abc(
#include <bitset>
#include <string>
#include <cmath>
#include <accidental-noise-library/anl.h>

double hex_function(double x, double y);// from vm.cpp

struct Point
{
	double x, y, z, w, u, v;
	int dimensions = 0;

	Point()
	{
	}

	Point(double x, double y, double z, double w, double u, double v)
		: x(x), y(y), z(z), w(w), u(u), v(v)
	{
	}

	double Length() const {
		return std::sqrt(x * x + y * y + z * z + w * w + u * u + v * v);
	}

	Point Scale(double d) const {
		Point p;
		p.z = p.w = p.u = p.v = 0.0;
		p.dimensions = dimensions;
		switch (dimensions)
		{
		default:
			p.v = v * d;
			p.u = u * d;
		case 4:
			p.w = w * d;
		case 3:
			p.z = z * d;
		case 2:
			p.y = y * d;
			p.x = x * d;
		}
		return p;
	}

	Point& ScaleX(double d) {
		x *= d;
		return *this;
	}
	
	Point& ScaleY(double d) {
		y *= d;
		return *this;
	}
	
	Point& ScaleZ(double d) {
		z *= d;
		return *this;
	}
	
	Point& ScaleW(double d) {
		w *= d;
		return *this;
	}

	Point& ScaleU(double d) {
		u *= d;
		return *this;
	}

	Point& ScaleV(double d) {
		v *= d;
		return *this;
	}

	Point Translate(double d) {
		Point p = *this;
		p.dimensions = dimensions;
		switch (dimensions)
		{
		default:
			p.v = v + d;
			p.u = u + d;
		case 4:
			p.w = w + d;
		case 3:
			p.z = z + d;
		case 2:
			p.y = y + d;
			p.x = x + d;
		}
		return p;
	}

	Point& TranslateX(double d) {
		x += d;
		return *this;
	}

	Point& TranslateY(double d) {
		y += d;
		return *this;
	}

	Point& TranslateZ(double d) {
		z += d;
		return *this;
	}

	Point& TranslateW(double d) {
		w += d;
		return *this;
	}

	Point& TranslateU(double d) {
		u += d;
		return *this;
	}

	Point& TranslateV(double d) {
		v += d;
		return *this;
	}
};

TileCoord  CalcHexPointTile(float px, float py)
{
	TileCoord tile;
	float rise = 0.5f;
	float slope = rise / 0.8660254f;
	int X = (int)(px / 1.732051f);
	int Y = (int)(py / 1.5f);

	float offsetX = px - (float)X*1.732051f;
	float offsetY = py - (float)Y*1.5f;

	if (fmod(Y, 2) == 0)
	{
		if (offsetY < (-slope * offsetX + rise))
		{
			--X;
			--Y;
		}
		else if (offsetY < (slope*offsetX - rise))
		{
			--Y;
		}
	}
	else
	{
		if (offsetX >= 0.8660254f)
		{
			if (offsetY <(-slope*offsetX + rise * 2))
			{
				--Y;
			}
		}
		else
		{
			if (offsetY<(slope*offsetX))
			{
				--Y;
			}
			else
			{
				--X;
			}
		}
	}
	tile.x = X;
	tile.y = Y;
	return tile;
}

CoordPair CalcHexTileCenter(int tx, int ty)
{
	CoordPair origin;
	float ymod = (float)fmod(ty, 2.0f);
	if (ymod == 1.0f) ymod = 0.8660254f;
	else ymod = 0.0f;
	origin.x = (float)tx*1.732051 + ymod;
	origin.y = (float)ty*1.5;
	CoordPair center;
	center.x = origin.x + 0.8660254;
	center.y = origin.y + 1.0;
	return center;
}

double HexTile(Point p, unsigned int seed)
{
	TileCoord tile = CalcHexPointTile((float)p.x, (float)p.y);
	unsigned int hash = hash_coords_2(tile.x, tile.y, seed);
	return (double)hash / 255.0;
}

double HexBump(Point p)
{
	TileCoord tile = CalcHexPointTile((float)p.x, (float)p.y);
	CoordPair center = CalcHexTileCenter(tile.x, tile.y);
	double dx = p.x - center.x;
	double dy = p.y - center.y;
	return hex_function(dx, dy);
}

double SmoothTiers(double Value, int NumberOfSteps)
{
	NumberOfSteps -= 1;

	double Tb = std::floor(Value * (double)NumberOfSteps);
	double Tt = Tb + 1.0;
	double t = quintic_blend(Value * (double)NumberOfSteps - Tb);

	Tb /= (double)NumberOfSteps;
	Tt /= (double)NumberOfSteps;

	return Tb + t * (Tt - Tb);
}

double CellularBasis(Point p, unsigned int dist,
	double f1, double f2, double f3, double f4,
	double d1, double d2, double d3, double d4,
	unsigned int seed)
{
	double f[4], d[4];
	switch (p.dimensions)
	{
	case 2:
		switch (dist)
		{
		case 0: anl::cellular_function2D(p.x, p.y, seed, f, d, anl::distEuclid2); break;
		case 1: anl::cellular_function2D(p.x, p.y, seed, f, d, anl::distManhattan2); break;
		case 2: anl::cellular_function2D(p.x, p.y, seed, f, d, anl::distGreatestAxis2); break;
		case 3: anl::cellular_function2D(p.x, p.y, seed, f, d, anl::distLeastAxis2); break;
		default: anl::cellular_function2D(p.x, p.y, seed, f, d, anl::distEuclid2); break;
		}; break;
	case 3:
		switch (dist)
		{
		case 0: anl::cellular_function3D(p.x, p.y, p.z, seed, f, d, anl::distEuclid3); break;
		case 1: anl::cellular_function3D(p.x, p.y, p.z, seed, f, d, anl::distManhattan3); break;
		case 2: anl::cellular_function3D(p.x, p.y, p.z, seed, f, d, anl::distGreatestAxis3); break;
		case 3: anl::cellular_function3D(p.x, p.y, p.z, seed, f, d, anl::distLeastAxis3); break;
		default: anl::cellular_function3D(p.x, p.y, p.z, seed, f, d, anl::distEuclid3); break;
		}; break;
	case 4:
		switch (dist)
		{
		case 0: anl::cellular_function4D(p.x, p.y, p.z, p.w, seed, f, d, anl::distEuclid4); break;
		case 1: anl::cellular_function4D(p.x, p.y, p.z, p.w, seed, f, d, anl::distManhattan4); break;
		case 2: anl::cellular_function4D(p.x, p.y, p.z, p.w, seed, f, d, anl::distGreatestAxis4); break;
		case 3: anl::cellular_function4D(p.x, p.y, p.z, p.w, seed, f, d, anl::distLeastAxis4); break;
		default: anl::cellular_function4D(p.x, p.y, p.z, p.w, seed, f, d, anl::distEuclid4); break;
		}; break;
	default:
		switch (dist)
		{
		case 0: anl::cellular_function6D(p.x, p.y, p.z, p.w, p.u, p.v, seed, f, d, anl::distEuclid6); break;
		case 1: anl::cellular_function6D(p.x, p.y, p.z, p.w, p.u, p.v, seed, f, d, anl::distManhattan6); break;
		case 2: anl::cellular_function6D(p.x, p.y, p.z, p.w, p.u, p.v, seed, f, d, anl::distGreatestAxis6); break;
		case 3: anl::cellular_function6D(p.x, p.y, p.z, p.w, p.u, p.v, seed, f, d, anl::distLeastAxis6); break;
		default: anl::cellular_function6D(p.x, p.y, p.z, p.w, p.u, p.v, seed, f, d, anl::distEuclid6); break;
		}; break;
	};

	return f1*f[0] + f2*f[1] + f3*f[2] + f4*f[3] + d1*d[0] + d2*d[1] + d3*d[2] + d4*d[3];
}

double SimplexBasis(Point p, unsigned int seed)
{
	switch (p.dimensions)
	{
	case 2: return anl::simplex_noise2D(p.x, p.y, seed, anl::noInterp);
	case 3: return anl::simplex_noise3D(p.x, p.y, p.z, seed, anl::noInterp);
	case 4: return anl::simplex_noise4D(p.x, p.y, p.z, p.w, seed, anl::noInterp);
	default: return anl::simplex_noise6D(p.x, p.y, p.z, p.w, p.u, p.v, seed, anl::noInterp);
	}
}

double GradientBasis(Point p, int Interpolation, unsigned int seed)
{
	switch (p.dimensions)
	{
	case 2:
		switch (Interpolation)
		{
		case 0: return anl::gradient_noise2D(p.x, p.y, seed, anl::noInterp);
		case 1: return anl::gradient_noise2D(p.x, p.y, seed, anl::linearInterp);
		case 2: return anl::gradient_noise2D(p.x, p.y, seed, anl::hermiteInterp);
		default: return anl::gradient_noise2D(p.x, p.y, seed, anl::quinticInterp);
		}; break;
	case 3:
		//std::cout << "(" << p.x << "," << p.y << "," << p.z << std::endl;
		switch (Interpolation)
		{
		case 0: return anl::gradient_noise3D(p.x, p.y, p.z, seed, anl::noInterp);
		case 1: return anl::gradient_noise3D(p.x, p.y, p.z, seed, anl::linearInterp);
		case 2: return anl::gradient_noise3D(p.x, p.y, p.z, seed, anl::hermiteInterp);
		default: return anl::gradient_noise3D(p.x, p.y, p.z, seed, anl::quinticInterp);
		}; break;
	case 4:
		switch (Interpolation)
		{
		case 0: return anl::gradient_noise4D(p.x, p.y, p.z, p.w, seed, anl::noInterp);
		case 1: return anl::gradient_noise4D(p.x, p.y, p.z, p.w, seed, anl::linearInterp);
		case 2: return anl::gradient_noise4D(p.x, p.y, p.z, p.w, seed, anl::hermiteInterp);
		default: return anl::gradient_noise4D(p.x, p.y, p.z, p.w, seed, anl::quinticInterp);
		}; break;
	default:
		switch (Interpolation)
		{
		case 0: return anl::gradient_noise6D(p.x, p.y, p.z, p.w, p.u, p.v, seed, anl::noInterp);
		case 1: return anl::gradient_noise6D(p.x, p.y, p.z, p.w, p.u, p.v, seed, anl::linearInterp);
		case 2: return anl::gradient_noise6D(p.x, p.y, p.z, p.w, p.u, p.v, seed, anl::hermiteInterp);
		default: return anl::gradient_noise6D(p.x, p.y, p.z, p.w, p.u, p.v, seed, anl::quinticInterp);
		}; break;
	}
}

double ValueBasis(Point p, int Interpolation, unsigned int seed)
{
	switch (p.dimensions)
	{
	case 2:
		switch (Interpolation)
		{
		case 0: return anl::value_noise2D(p.x, p.y, seed, anl::noInterp);
		case 1: return anl::value_noise2D(p.x, p.y, seed, anl::linearInterp);
		case 2: return anl::value_noise2D(p.x, p.y, seed, anl::hermiteInterp);
		default: return anl::value_noise2D(p.x, p.y, seed, anl::quinticInterp);
		}; break;
	case 3:
		switch (Interpolation)
		{
		case 0: return anl::value_noise3D(p.x, p.y, p.z, seed, anl::noInterp);
		case 1: return anl::value_noise3D(p.x, p.y, p.z, seed, anl::linearInterp);
		case 2: return anl::value_noise3D(p.x, p.y, p.z, seed, anl::hermiteInterp);
		default: return anl::value_noise3D(p.x, p.y, p.z, seed, anl::quinticInterp);
		}; break;
	case 4:
		switch (Interpolation)
		{
		case 0: return anl::value_noise4D(p.x, p.y, p.z, p.w, seed, anl::noInterp);
		case 1: return anl::value_noise4D(p.x, p.y, p.z, p.w, seed, anl::linearInterp);
		case 2: return anl::value_noise4D(p.x, p.y, p.z, p.w, seed, anl::hermiteInterp);
		default: return anl::value_noise4D(p.x, p.y, p.z, p.w, seed, anl::quinticInterp);
		}; break;
	default:
		switch (Interpolation)
		{
		case 0: return anl::value_noise6D(p.x, p.y, p.z, p.w, p.u, p.v, seed, anl::noInterp);
		case 1: return anl::value_noise6D(p.x, p.y, p.z, p.w, p.u, p.v, seed, anl::linearInterp);
		case 2: return anl::value_noise6D(p.x, p.y, p.z, p.w, p.u, p.v, seed, anl::hermiteInterp);
		default: return anl::value_noise6D(p.x, p.y, p.z, p.w, p.u, p.v, seed, anl::quinticInterp);
		}; break;
	}
}

Point RotateDomain(Point EvalPoint, double angle, double ax, double ay, double az)
{
	double len = std::sqrt(ax * ax + ay * ay + az * az);
	ax /= len;
	ay /= len;
	az /= len;

	double cosangle = cos(angle);
	double sinangle = sin(angle);

	double rotmatrix[3][3];

	rotmatrix[0][0] = 1.0 + (1.0 - cosangle) * (ax * ax - 1.0);
	rotmatrix[1][0] = -az * sinangle + (1.0 - cosangle) * ax * ay;
	rotmatrix[2][0] = ay * sinangle + (1.0 - cosangle) * ax * az;

	rotmatrix[0][1] = az * sinangle + (1.0 - cosangle) * ax * ay;
	rotmatrix[1][1] = 1.0 + (1.0 - cosangle) * (ay * ay - 1.0);
	rotmatrix[2][1] = -ax * sinangle + (1.0 - cosangle) * ay * az;

	rotmatrix[0][2] = -ay * sinangle + (1.0 - cosangle) * ax * az;
	rotmatrix[1][2] = ax * sinangle + (1.0 - cosangle) * ay * az;
	rotmatrix[2][2] = 1.0 + (1.0 - cosangle) * (az * az - 1.0);

	double nx, ny, nz;
	nx = (rotmatrix[0][0] * EvalPoint.x) + (rotmatrix[1][0] * EvalPoint.y) + (rotmatrix[2][0] * EvalPoint.z);
	ny = (rotmatrix[0][1] * EvalPoint.x) + (rotmatrix[1][1] * EvalPoint.y) + (rotmatrix[2][1] * EvalPoint.z);
	nz = (rotmatrix[0][2] * EvalPoint.x) + (rotmatrix[1][2] * EvalPoint.y) + (rotmatrix[2][2] * EvalPoint.z);
	EvalPoint.x = nx;
	EvalPoint.y = ny;
	EvalPoint.z = nz;
	return EvalPoint;
}

double Select_Blend(double low, double high, double control, double threshold, double falloff)
{
	double lower = threshold - falloff;
	double upper = threshold + falloff;
	double blend = quintic_blend((control - lower) / (upper - lower));
	return low + (high - low) * blend;
}

double ANL_CPP_Evaluate(const Point EvalPoint)
{
<THIS_IS_WHERE_THE_CODE_GOES>
	return FinalResult;
}

double ANL_CPP_EvalScalar(double x, double y)
{
	Point p;
	p.x = p.y = p.z = p.w = p.u = p.v = 0.0;
	p.dimensions = 2;
	p.x = x;
	p.y = y;
	return ANL_CPP_Evaluate(p);
}

double ANL_CPP_EvalScalar(double x, double y, double z)
{
	Point p;
	p.x = p.y = p.z = p.w = p.u = p.v = 0.0;
	p.dimensions = 3;
	p.x = x;
	p.y = y;
	p.z = z;
	return ANL_CPP_Evaluate(p);
}
)abc";

static const std::string CodeReplaceToken = "<THIS_IS_WHERE_THE_CODE_GOES>";

std::string OutputFullCppFile(std::string CppExpressionToExecute)
{
	std::string Result = OutputString;
	std::size_t Offset = Result.find(CodeReplaceToken);
	Result.replace(Offset, CodeReplaceToken.size(), CppExpressionToExecute);
	return Result;
}