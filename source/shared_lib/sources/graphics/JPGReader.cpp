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

#include "JPGReader.h"
#include "FileReader.h"

#include "data_types.h"
#include "pixmap.h"
#include <stdexcept>
#include <jpeglib.h>
#include <setjmp.h>

#include "util.h"
#include "leak_dumper.h"

using std::runtime_error;
using std::ios;

namespace Shared {
	namespace Graphics {

		// =====================================================
		//	Methods used for JPG-Decompression
		// =====================================================


		//Methods used by jpeglib
		static void init_source(j_decompress_ptr cinfo) {
			//It already is initialized
		}
		static boolean fill_input_buffer(j_decompress_ptr cinfo) {
			//it is already filled
			return boolean(true);
		}
		static void skip_input_data(j_decompress_ptr cinfo, long num_bytes) {
			if (num_bytes > 0) {
				jpeg_source_mgr* This = cinfo->src;
				This->bytes_in_buffer -= num_bytes;
				This->next_input_byte += num_bytes;
			}
		}
		static void term_source(j_decompress_ptr cinfo) {
		}

		// =====================================================
		//	class JPGReader
		// =====================================================

		/**Return an array containing the used extensions,
		  * initialized*/
		  //static inline const string* getExtensions() {
			  //static const string extensions[] = {"jpg", "jpeg", ""};
		static inline std::vector<string> getExtensions() {
			static std::vector<string> extensions;
			if (extensions.empty() == true) {
				extensions.push_back("jpg");
				extensions.push_back("jpeg");
			}

			return extensions;
		}

		JPGReader::JPGReader() : FileReader<Pixmap2D>(getExtensions()) {
		}

		Pixmap2D* JPGReader::read(ifstream& is, const string& path, Pixmap2D* ret) const {
			if (GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
				throw game_runtime_error("Loading graphics in headless server mode not allowed!");
			}

			//Read file
			is.seekg(0, ios::end);
			streampos length = is.tellg();
			if (length < 8) {
				return NULL;
			}
			is.seekg(0, ios::beg);
			uint8 *buffer = new uint8[(unsigned int) length];
			is.read((char*) buffer, (std::streamsize)length);
			static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
			if (bigEndianSystem == true) {
				Shared::PlatformByteOrder::fromEndianTypeArray<uint8>(buffer, (size_t) length);
			}
			if (length < 2) {
				throw game_runtime_error("length < 2", true);
			}
			//Check buffer (weak jpeg check)
			//if (buffer[0] != 0x46 || buffer[1] != 0xA0) {
			// Proper header check found from: http://www.fastgraph.com/help/jpeg_header_format.html
			if (buffer[0] != 0xFF || buffer[1] != 0xD8) {
				std::cout << "0 = [" << std::hex << (int) buffer[0] << "] 1 = [" << std::hex << (int) buffer[1] << "]" << std::endl;
				delete[] buffer;
				throw game_runtime_error(path + " is not a jpeg", true);
			}

			struct jpeg_decompress_struct cinfo;
			struct jpeg_error_mgr jerr;

			JSAMPROW row_pointer[1];
			row_pointer[0] = NULL;
			cinfo.err = jpeg_std_error(&jerr); //Standard error handler
			jpeg_create_decompress(&cinfo); //Create decompressing structure
			struct jpeg_source_mgr source;

			jmp_buf error_buffer; //Used for saving/restoring context
			// Set up data pointer
			source.bytes_in_buffer = (size_t) length;
			source.next_input_byte = (JOCTET*) buffer;
			cinfo.src = &source;

			if (setjmp(error_buffer)) { //Longjump was called --> an exception was thrown
				delete[] buffer;
				jpeg_destroy_decompress(&cinfo);
				if (row_pointer[0] != NULL) {
					delete[] row_pointer[0];
				}
				throw game_runtime_error(path + " is a corrupt(1) jpeg", true);
			}

			source.init_source = init_source;
			source.fill_input_buffer = fill_input_buffer;
			source.resync_to_restart = jpeg_resync_to_restart;
			source.skip_input_data = skip_input_data;
			source.term_source = term_source;

			/* reading the image header which contains image information */
			if (jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK) {
				delete[] buffer;
				jpeg_destroy_decompress(&cinfo);
				throw game_runtime_error(path + " is a corrupt(1) jpeg", true);
			}


			/*std::cout << "JPEG FILE Information: " << std::endl;
			std::cout << "Image width and height: " << cinfo.image_width <<" pixels and " << cinfo.image_height <<" pixels." << std::endl;

			std::cout << "Color components per fixel: " << cinfo.num_components << std::endl;
			std::cout << "Color space: " << cinfo.jpeg_color_space << std::endl;*/
			int picComponents = (ret->getComponents() == -1) ? cinfo.num_components : ret->getComponents();
			//std::cout << "JPG-Components: Pic: " << picComponents << " old: " << (ret->getComponents()) << " File: " << cinfo.num_components << std::endl;
			//picComponents = 4;

			// Start decompression jpeg here
			jpeg_start_decompress(&cinfo);
			ret->init(cinfo.image_width, cinfo.image_height, picComponents);
			uint8* pixels = ret->getPixels();
			//std::cout << "output width and height: " << cinfo.output_width <<" pixels and " << cinfo.output_height <<" pixels." << std::endl;
			/* now actually read the jpeg into the raw buffer */
			row_pointer[0] = new unsigned char[cinfo.output_width*cinfo.num_components];
			/* read one scan line at a time */
			/* Again you need to invert the lines unfortunately*/
			while (cinfo.output_scanline < cinfo.output_height) {
				jpeg_read_scanlines(&cinfo, row_pointer, 1);
				//Current pixel
				size_t location = (cinfo.output_height - cinfo.output_scanline) * cinfo.output_width * picComponents;
				if (picComponents == cinfo.num_components) {
					memcpy(pixels + location, row_pointer[0], cinfo.output_width*cinfo.num_components);
				} else {
					int r, g, b, a, l;
					for (unsigned int xPic = 0, xFile = 0; xPic < cinfo.output_width*picComponents; xPic += picComponents, xFile += cinfo.num_components) {
						switch (cinfo.num_components) {
							case 3:
								r = row_pointer[0][xFile];
								g = row_pointer[0][xFile + 1];
								b = row_pointer[0][xFile + 2];
								l = (r + g + b + 2) / 3;
								a = 255;
								break;
							case 4:
								r = row_pointer[0][xFile];
								g = row_pointer[0][xFile + 1];
								b = row_pointer[0][xFile + 2];
								l = (r + g + b + 2) / 3;
								a = row_pointer[0][xFile + 3];
								break;
							default:
								// Possible Error
							case 1:
								r = g = b = l = row_pointer[0][xFile];
								a = 255;
								break;
						}
						switch (picComponents) {
							case 1:
								pixels[location + xPic] = l;
								break;
							case 4:
								pixels[location + xPic + 3] = a; //Next case
							case 3:
								pixels[location + xPic] = r;
								pixels[location + xPic + 1] = g;
								pixels[location + xPic + 2] = b;
								break;
							default:
								//just so at least something works
								for (int i = 0; i < picComponents; ++i) {
									pixels[location + xPic + i] = l;
								}
								break;
								// Possible Error
						}
					}
				}
			}
			/*for(int i = 0; i < cinfo.image_width*cinfo.image_height*picComponents; ++i) {
				if (i%39 == 0) std::cout << std::endl;
				int first = pixels[i]/16;
				if (first < 10)
					std:: cout << first;
				else
					std::cout << (char)('A'+(first-10));
				first = pixels[i]%16;
				if (first < 10)
					std:: cout << first;
				else
					std::cout << (char)('A'+(first-10));
				std::cout << " ";
			}*/
			/* wrap up decompression, destroy objects, free pointers and close open files */
			jpeg_finish_decompress(&cinfo);
			jpeg_destroy_decompress(&cinfo);
			delete[] row_pointer[0];

			delete[] buffer;

			return ret;
		}

	}
} //end namespace
