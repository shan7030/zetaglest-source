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

#ifndef _SHARED_PLATFORM_SDL_GLOBALS_H_
#define _SHARED_PLATFORM_SDL_GLOBALS_H_

// This header contains things that should not be used outside the platform/sdl
// directory

#include "leak_dumper.h"

namespace Shared {
	namespace PlatformCommon {
		namespace Private {

			extern bool shouldBeFullscreen;
			extern int ScreenWidth;
			extern int ScreenHeight;

		}
	}
}

#endif

