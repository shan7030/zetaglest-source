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

#include "display.h"

#include "metrics.h"
#include "command_type.h"
#include "util.h"
#include "leak_dumper.h"

using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Game {
	// =====================================================
	// 	class Display
	// =====================================================

	Display::Display() {
		if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem, "In [%s::%s Line: %d]\n", __FILE__, __FUNCTION__, __LINE__);

		colors[0] = Vec4f(1.f, 1.f, 1.f, 1.0f);
		colors[1] = Vec4f(1.f, 0.5f, 0.5f, 1.0f);
		colors[2] = Vec4f(0.5f, 0.5f, 1.0f, 1.0f);
		colors[3] = Vec4f(0.5f, 1.0f, 0.5f, 1.0f);
		colors[4] = Vec4f(0.0f, 0.0f, 0.0f, 1.0f);
		colors[5] = Vec4f(0.0f, 0.0f, 1.0f, 1.0f);
		colors[6] = Vec4f(1.0f, 0.0f, 0.0f, 1.0f);
		colors[7] = Vec4f(0.0f, 1.0f, 0.0f, 1.0f);
		colors[8] = Vec4f(1.0f, 1.0f, 1.0f, 1.0f);

		currentColor = 0;
		clear();

		if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem, "In [%s::%s Line: %d]\n", __FILE__, __FUNCTION__, __LINE__);
	}

	void Display::calculateUpDimensions(int index) {
		if (index > maxUpIndex) {
			maxUpIndex = index;
			if (maxUpIndex + 1 > upCellSideCount*upCellSideCount) {
				upCellSideCount = upCellSideCount + 1;
				upImageSize = static_cast<float>(imageSize) * static_cast<float>(cellSideCount) / static_cast<float>(upCellSideCount) + 0.9f;
			}
		}
	}

	Vec4f Display::getColor() const {
		if (currentColor < 0 || currentColor >= colorCount) {
			throw game_runtime_error("currentColor >= colorCount");
		}
		return colors[currentColor];
	}

	void Display::setUpImage(int i, const Texture2D *image) {
		if (i >= upCellCount)
			return;
		upImages[i] = image;
		calculateUpDimensions(i);
	}

	//misc
	void Display::clear() {
		downSelectedPos = invalidPos;
		for (int i = 0; i < upCellCount; ++i) {
			upImages[i] = NULL;
		}

		for (int i = 0; i < downCellCount; ++i) {
			downImages[i] = NULL;
			downLighted[i] = true;
			commandTypes[i] = NULL;
			commandClasses[i] = ccNull;
		}

		title.clear();
		text.clear();
		progressBar = -1;

		upCellSideCount = cellSideCount;
		upImageSize = imageSize;
		maxUpIndex = 0;
	}
	void Display::switchColor() {
		currentColor = (currentColor + 1) % colorCount;
	}

	int Display::computeDownIndex(int x, int y) const {
		y = y - (downY - cellSideCount * imageSize);

		if (y > imageSize*cellSideCount || y < 0) {
			return invalidPos;
		}

		int cellX = x / imageSize;
		int cellY = (y / imageSize) % cellSideCount;
		int index = (cellSideCount - cellY - 1)*cellSideCount + cellX;;

		if (index < 0 || index >= downCellCount || downImages[index] == NULL) {
			index = invalidPos;
		}

		return index;
	}

	int Display::computeDownX(int index) const {
		return (index % cellSideCount) * imageSize;
	}

	int Display::computeDownY(int index) const {
		return Display::downY - (index / cellSideCount)*imageSize - imageSize;
	}

	int Display::computeUpX(int index) const {
		return (index % upCellSideCount) * upImageSize;
	}

	int Display::computeUpY(int index) const {
		return Metrics::getInstance().getDisplayH() - (index / upCellSideCount)*upImageSize - upImageSize;
	}

	void Display::saveGame(XmlNode *rootNode) const {
		std::map<string, string> mapTagReplacements;
		XmlNode *displayNode = rootNode->addChild("Display");

		//	string title;
		displayNode->addAttribute("title", title, mapTagReplacements);
		//	string text;
		displayNode->addAttribute("text", text, mapTagReplacements);
		//	string infoText;
		displayNode->addAttribute("infoText", infoText, mapTagReplacements);
		//	const Texture2D *upImages[upCellCount];
		//	const Texture2D *downImages[downCellCount];
		//	bool downLighted[downCellCount];
		//	const CommandType *commandTypes[downCellCount];
		//	CommandClass commandClasses[downCellCount];
		//	int progressBar;
		displayNode->addAttribute("progressBar", intToStr(progressBar), mapTagReplacements);
		//	int downSelectedPos;
		displayNode->addAttribute("downSelectedPos", intToStr(downSelectedPos), mapTagReplacements);
		//	Vec4f colors[colorCount];
		//	int currentColor;
		displayNode->addAttribute("currentColor", intToStr(currentColor), mapTagReplacements);
		//	int upCellSideCount;
		//	int upImageSize;
		//	int maxUpIndex;
	}

	void Display::loadGame(const XmlNode *rootNode) {
		const XmlNode *displayNode = rootNode->getChild("Display");

		//	string title;
		title = displayNode->getAttribute("title")->getValue();
		//	string text;
		text = displayNode->getAttribute("text")->getValue();
		//	string infoText;
		infoText = displayNode->getAttribute("infoText")->getValue();
		//	const Texture2D *upImages[upCellCount];
		//	const Texture2D *downImages[downCellCount];
		//	bool downLighted[downCellCount];
		//	const CommandType *commandTypes[downCellCount];
		//	CommandClass commandClasses[downCellCount];
		//	int progressBar;
		progressBar = displayNode->getAttribute("progressBar")->getIntValue();
		//	int downSelectedPos;
		//displayNode->addAttribute("downSelectedPos",intToStr(downSelectedPos), mapTagReplacements);
		//	Vec4f colors[colorCount];
		//	int currentColor;
		//currentColor = displayNode->getAttribute("progressBar")->getIntValue();
		//	int upCellSideCount;
		//	int upImageSize;
		//	int maxUpIndex;
	}

} //end namespace
