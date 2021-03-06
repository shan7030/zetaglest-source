// This file is part of ZetaGlest <https://github.com/ZetaGlest>
//
// Copyright (C) 2018  The ZetaGlest team
//
// ZetaGlest is a fork of MegaGlest <https://megaglest.org>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>

#ifndef _SHARED_GRAPHICS_MATHUTIL_H_
#define _SHARED_GRAPHICS_MATHUTIL_H_

#include "math_wrapper.h"
#include "vec.h"
#include "data_types.h"
#include "leak_dumper.h"

using namespace std;
using namespace Shared::Platform;

namespace Shared {
	namespace Graphics {

		const float pi = 3.1415926f;
		const float sqrt2 = 1.41421356f;
		const float zero = 1e-6f;
		const float infinity = 1e6f;

		// =====================================================
		//	class Rect
		// =====================================================

		// 0 +-+
		//   | |
		//   +-+ 1

		template<typename T>
		class Rect2 {
		public:
			Vec2<T> p[2];
		public:
			Rect2() {
			};

			Rect2(const Vec2<T> &p0, const Vec2<T> &p1) {
				this->p[0] = p0;
				this->p[1] = p1;
			}

			Rect2(T p0x, T p0y, T p1x, T p1y) {
				p[0].x = p0x;
				p[0].y = p0y;
				p[1].x = p1x;
				p[1].y = p1y;
			}

			Rect2<T> operator*(T scalar) {
				return Rect2<T>(
					p[0] * scalar,
					p[1] * scalar);
			}

			Rect2<T> operator/(T scalar) {
				return Rect2<T>(
					p[0] / scalar,
					p[1] / scalar);
			}

			bool isInside(const Vec2<T> &p) const {
				return
					p.x >= this->p[0].x &&
					p.y >= this->p[0].y &&
					p.x < this->p[1].x &&
					p.y < this->p[1].y;
			}

			void clamp(T minX, T minY, T  maxX, T maxY) {
				for (int i = 0; i < 2; ++i) {
					if (p[i].x < minX) {
						p[i].x = minX;
					}
					if (p[i].y < minY) {
						p[i].y = minY;
					}
					if (p[i].x > maxX) {
						p[i].x = maxX;
					}
					if (p[i].y > maxY) {
						p[i].y = maxY;
					}
				}
			}

			std::string getString() const {
				std::ostringstream streamOut;
				streamOut << "#1: " << this->p[0].getString();
				streamOut << "#2: " << this->p[1].getString();
				std::string result = streamOut.str();
				streamOut.str(std::string());
				return result;
			}

		};

		typedef Rect2<int> Rect2i;
		typedef Rect2<char> Rect2c;
		typedef Rect2<float> Rect2f;
		typedef Rect2<double> Rect2d;

		// =====================================================
		//	class Quad
		// =====================================================

		// 0 +-+ 2
		//   | |
		// 1 +-+ 3

		template<typename T>
		class Quad2 {
		public:
			Vec2<T> p[4];
		public:
			Quad2() {
			};

			Quad2(const Vec2<T> &p0, const Vec2<T> &p1, const Vec2<T> &p2, const Vec2<T> &p3) {
				this->p[0] = p0;
				this->p[1] = p1;
				this->p[2] = p2;
				this->p[3] = p3;
			}

			explicit Quad2(const Rect2<T> &rect) {
				this->p[0] = rect.p[0];
				this->p[1] = Vec2<T>(rect.p[0].x, rect.p[1].y);
				this->p[2] = rect.p[1];
				this->p[3] = Vec2<T>(rect.p[1].x, rect.p[0].y);
			}

			Quad2<T> operator*(T scalar) {
				return Quad2<T>(
					p[0] * scalar,
					p[1] * scalar,
					p[2] * scalar,
					p[3] * scalar);
			}

			Quad2<T> operator/(T scalar) {
				return Quad2<T>(
					p[0] / scalar,
					p[1] / scalar,
					p[2] / scalar,
					p[3] / scalar);
			}

			bool operator <(const Quad2<T> &v) const {
				if (p[0] < v.p[0]) {
					return true;
				}
				if (p[1] < v.p[1]) {
					return true;
				}
				if (p[2] < v.p[2]) {
					return true;
				}
				if (p[3] < v.p[3]) {
					return true;
				}
				return false;
			}

			bool operator !=(const Quad2<T> &v) const {
				if (p[0] != v.p[0]) {
					return true;
				}
				if (p[1] != v.p[1]) {
					return true;
				}
				if (p[2] != v.p[2]) {
					return true;
				}
				if (p[3] != v.p[3]) {
					return true;
				}
				return false;
			}

			Rect2<T> computeBoundingRect() const {
				return Rect2i(
#ifdef WIN32
					min(p[0].x, p[1].x),
					min(p[0].y, p[2].y),
					max(p[2].x, p[3].x),
					max(p[1].y, p[3].y));
#else
					std::min(p[0].x, p[1].x),
					std::min(p[0].y, p[2].y),
					std::max(p[2].x, p[3].x),
					std::max(p[1].y, p[3].y));
#endif
			}

			bool isInside(const Vec2<T> &pt) const {

				if (!computeBoundingRect().isInside(pt))
					return false;

				bool left[4];

				left[0] = (pt.y - p[0].y)*(p[1].x - p[0].x) - (pt.x - p[0].x)*(p[1].y - p[0].y) < 0;
				left[1] = (pt.y - p[1].y)*(p[3].x - p[1].x) - (pt.x - p[1].x)*(p[3].y - p[1].y) < 0;
				left[2] = (pt.y - p[3].y)*(p[2].x - p[3].x) - (pt.x - p[3].x)*(p[2].y - p[3].y) < 0;
				left[3] = (pt.y - p[2].y)*(p[0].x - p[2].x) - (pt.x - p[2].x)*(p[0].y - p[2].y) < 0;

				return left[0] && left[1] && left[2] && left[3];
			}

			void clamp(T minX, T minY, T maxX, T maxY) {
				for (int i = 0; i < 4; ++i) {
					if (p[i].x < minX) {
						p[i].x = minX;
					}
					if (p[i].y < minY) {
						p[i].y = minY;
					}
					if (p[i].x > maxX) {
						p[i].x = maxX;
					}
					if (p[i].y > maxY) {
						p[i].y = maxY;
					}
				}
			}

			float area() {
				Vec2i v0 = p[3] - p[0];
				Vec2i v1 = p[1] - p[2];

				return 0.5f * ((v0.x * v1.y) - (v0.y * v1.x));
			}

			std::string getString() const {
				std::ostringstream streamOut;
				streamOut << "#1: " << this->p[0].getString();
				streamOut << "#2: " << this->p[1].getString();
				streamOut << "#3: " << this->p[2].getString();
				streamOut << "#4: " << this->p[3].getString();
				std::string result = streamOut.str();
				streamOut.str(std::string());
				return result;
			}

		};

		typedef Quad2<int> Quad2i;
		typedef Quad2<char> Quad2c;
		typedef Quad2<float> Quad2f;
		typedef Quad2<double> Quad2d;

		// =====================================================
		//	Misc
		// =====================================================

		inline int next2Power(int n) {
			int i;
			for (i = 1; i < n; i *= 2);
			return i;
		}

		template<typename T>
		inline T degToRad(T deg) {
			return (deg * 2 * pi) / 360;
		}

		template<typename T>
		inline T radToDeg(T rad) {
			return (rad * 360) / (2 * pi);
		}

		// ====================================================================================================================
		// ====================================================================================================================
		//  Inline implementation
		// ====================================================================================================================
		// ====================================================================================================================
		//#if _xs_BigEndian_
		//	#define _xs_iexp_				0
		//	#define _xs_iman_				1
		//#else
		//	#define _xs_iexp_				1       //intel is little endian
		//	#define _xs_iman_				0
		//#endif //BigEndian_
		//
		////#define finline                     __forceinline
		//#define finline                     inline
		//
		//#ifndef _xs_DEFAULT_CONVERSION
		//#define _xs_DEFAULT_CONVERSION      0
		//#endif
		//
		////typedef long                    int32;
		//typedef double                  real64;
		//const real64 _xs_doublemagic			= real64 (6755399441055744.0); 	    //2^52 * 1.5,  uses limited precisicion to floor
		//
		//finline int32 xs_CRoundToInt(real64 val, real64 dmr = _xs_doublemagic) {
		//#if _xs_DEFAULT_CONVERSION==0
		//    val		= val + dmr;
		//	return ((int32*)&val)[_xs_iman_];
		//    //return 0;
		//#else
		//    return int32(floor(val+.5));
		//#endif
		//}


	}
} //end namespace

#endif
