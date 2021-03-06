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

#ifndef FILE_READER_H
#define FILE_READER_H

#include "platform_util.h"
#include <map>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <typeinfo>
#include <vector>
#include "conversion.h"
#include "leak_dumper.h"

using std::map;
using std::string;
using std::vector;
using std::ifstream;
using std::ios;
using std::runtime_error;
using namespace Shared::Util;

using Shared::PlatformCommon::extractExtension;

#define AS_STRING(...) #__VA_ARGS__

namespace Shared {

	// =====================================================
	//	class FileReader
	// =====================================================

	template <class T>
	class FileReader {
	public:
		//string const * extensions;
		std::vector<string> extensions;

		/**Creates a filereader being able to possibly load files
		 * from the specified extension
		 **/
		 //FileReader(string const * extensions);
		FileReader(std::vector<string> extensions);

		/**Creates a low-priority filereader
		 **/
		FileReader();

	public:
		/*Return the - existing and initialized - fileReadersMap
		 */
		static map<string, vector<FileReader<T> const * >* >& getFileReadersMap() {
			static map<string, vector<FileReader<T> const * >* > fileReaderByExtension;
			return fileReaderByExtension;
		}

		static vector<FileReader<T> const * >& getFileReaders() {
			static vector<FileReader<T> const*> fileReaders;
			return fileReaders;
		};

		static vector<FileReader<T> const * >& getLowPriorityFileReaders() {
			static vector<FileReader<T> const*> lowPriorityFileReaders;
			return lowPriorityFileReaders;
		};


	public:

		/**Tries to read a file
		 * This method tries to read the file with the specified filepath.
		 * If it fails, either <code>null</code> is returned or an exception
			 * is thrown*/
		static T* readPath(const string& filepath);

		/**Tries to read a file from an object
		 * This method tries to read the file with the specified filepath.
		 * If it fails, either <code>null</code> is returned or an exception
			 * is thrown*/
		static T* readPath(const string& filepath, T* object);

		/**Gives a quick estimation of whether the specified file
			 * can be read or not depending on the filename*/
		virtual bool canRead(const string& filepath) const;

		/**Gives a better estimation of whether the specified file
		 * can be read or not depending on the file content*/
		virtual bool canRead(ifstream& file) const;

		virtual void cleanupExtensions();

		/**Reads a file
		 * This method tries to read the file with the specified filepath
		 * If it fails, either <code>null</code> is returned or an exception
		 * is thrown
			 */
		virtual T* read(const string& filepath) const;

		/**Reads a file to an object
		 * This method tries to read the file with the specified filepath
		 * If it fails, either <code>null</code> is returned or an exception
		 * is thrown
			 */
		virtual T* read(const string& filepath, T* object) const;

		/**Reads a file
		 * This method tries to read the specified file
		 * If it failes, either <code>null</code> is returned or an exception
		 * Default implementation generates an object using T()
		 * is thrown
		 */
		virtual T* read(ifstream& file, const string& path) const {
			T* obj = new T();
			T* ret = read(file, path, obj);
			if (obj != ret) {
				delete obj;
			}
			return ret;
		}


		/**Reads a file onto the specified object
		 * This method tries to read the specified file
		 * If it failes, either <code>null</code> is returned or an exception
		 * is thrown
		 */
		virtual T* read(ifstream& file, const string& path, T* former) const = 0;

		virtual ~FileReader() {
			cleanupExtensions();
		}; //Well ... these objects aren't supposed to be destroyed
	};

	template <typename T>
	static inline T* readFromFileReaders(vector<FileReader<T> const *>* readers, const string& filepath) {
		//try to assign file
#if defined(WIN32) && !defined(__MINGW32__)
		FILE *fp = _wfopen(utf8_decode(filepath).c_str(), L"rb");
		ifstream file(fp);
#else
		ifstream file(filepath.c_str(), ios::in | ios::binary);
#endif
		if (!file.is_open()) {
			throw game_runtime_error("[#1] Could not open file " + filepath);
		}
		for (typename vector<FileReader<T> const *>::const_iterator i = readers->begin(); i != readers->end(); ++i) {
			T* ret = NULL;
			file.seekg(0, ios::beg); //Set position to first
			try {
				FileReader<T> const * reader = *i;
				ret = reader->read(file, filepath); //It is guaranteed that at least the filepath matches ...
			}
#if defined(WIN32)
			catch (game_runtime_error) {
#else
			catch (game_runtime_error &ex) {
#endif
				throw;
			} catch (...) {
				continue;
			}
			if (ret != NULL) {
				file.close();
#if defined(WIN32) && !defined(__MINGW32__)
				if (fp) fclose(fp);
#endif
				return ret;
			}
			}
		file.close();
#if defined(WIN32) && !defined(__MINGW32__)
		if (fp) fclose(fp);
#endif

		return NULL;
		}

	template <typename T>
	static inline T* readFromFileReaders(vector<FileReader<T> const *>* readers, const string& filepath, T* object) {
		//try to assign file
#if defined(WIN32) && !defined(__MINGW32__)
		wstring wstr = utf8_decode(filepath);
		FILE *fp = _wfopen(wstr.c_str(), L"rb");
		int fileErrno = errno;
		ifstream file(fp);
#else
		ifstream file(filepath.c_str(), ios::in | ios::binary);
#endif
		if (!file.is_open()) {
#if defined(WIN32) && !defined(__MINGW32__)
			DWORD error = GetLastError();
			throw game_runtime_error("[#2] Could not open file, result: " + intToStr(error) + " - " + intToStr(fileErrno) + " [" + filepath + "]");
#else
			throw game_runtime_error("[#2] Could not open file [" + filepath + "]");
#endif
		}
		for (typename vector<FileReader<T> const *>::const_iterator i = readers->begin(); i != readers->end(); ++i) {
			T* ret = NULL;
			file.seekg(0, ios::beg); //Set position to first
			try {
				FileReader<T> const * reader = *i;
				ret = reader->read(file, filepath, object); //It is guaranteed that at least the filepath matches ...
			}
#if defined(WIN32)
			catch (game_runtime_error) {
#else
			catch (game_runtime_error &ex) {
#endif
				throw;
			} catch (...) {
				continue;
			}
			if (ret != NULL) {
				file.close();
#if defined(WIN32) && !defined(__MINGW32__)
				if (fp) fclose(fp);
#endif
				return ret;
			}
			}
		file.close();
#if defined(WIN32) && !defined(__MINGW32__)
		if (fp) fclose(fp);
#endif
		return NULL;
		}

	/**Tries to read a file
	 * This method tries to read the file with the specified filepath.
	 * If it fails, either <code>null</code> is returned or an exception
	 * is thrown*/
	template <typename T>
	T* FileReader<T>::readPath(const string& filepath) {
		const string& extension = extractExtension(filepath);
		vector<FileReader<T> const * >* possibleReaders = (getFileReadersMap())[extension];
		if (possibleReaders != NULL) {
			//Search in these possible readers
			T* ret = readFromFileReaders(possibleReaders, filepath);
			if (ret != NULL) {
				return ret;
			}
		}
		T* ret = readFromFileReaders(&(getFileReaders()), filepath); //Try all other
		if (ret == NULL) {
			std::cerr << "ERROR #1 - Could not parse filepath: " << filepath << std::endl;
			throw game_runtime_error(string("Could not parse ") + filepath + " as object of type " + typeid(T).name());
		}
		return ret;
	}

	/**Tries to read a file
	 * This method tries to read the file with the specified filepath.
	 * If it fails, either <code>null</code> is returned or an exception
	 * is thrown*/
	template <typename T>
	T* FileReader<T>::readPath(const string& filepath, T* object) {
		const string& extension = extractExtension(filepath);
		vector<FileReader<T> const * >* possibleReaders = (getFileReadersMap())[extension];
		if (possibleReaders != NULL) {
			//Search in these possible readers
			T* ret = readFromFileReaders(possibleReaders, filepath, object);
			if (ret != NULL) {
				return ret;
			}
		}
		T* ret = readFromFileReaders(&(getFileReaders()), filepath, object); //Try all other
		if (ret == NULL) {
			std::cerr << "ERROR #2 - Could not parse filepath: " << filepath << std::endl;
			ret = readFromFileReaders(&(getLowPriorityFileReaders()), filepath); //Try to get dummy file
			if (ret == NULL) {
				throw game_runtime_error(string("Could not parse ") + filepath + " as object of type " + typeid(T).name());
			}
		}
		return ret;
	}

	template <typename T>
	FileReader<T>::FileReader(std::vector<string> extensions) : extensions(extensions) {
		getFileReaders().push_back(this);
		std::vector<string> nextExtension = extensions;
		for (unsigned int i = 0; i < nextExtension.size(); ++i) {
			vector<FileReader<T> const* >* curPossibleReaders = (getFileReadersMap())[nextExtension[i]];
			if (curPossibleReaders == NULL) {
				(getFileReadersMap())[nextExtension[i]] = (curPossibleReaders = new vector<FileReader<T> const *>());
			}
			curPossibleReaders->push_back(this);
		}
	}

	template <typename T>
	void FileReader<T>::cleanupExtensions() {
		std::vector<string> nextExtension = extensions;
		for (unsigned int i = 0; i < nextExtension.size(); ++i) {
			vector<FileReader<T> const* >* curPossibleReaders = (getFileReadersMap())[nextExtension[i]];
			if (curPossibleReaders != NULL) {
				delete curPossibleReaders;
				(getFileReadersMap())[nextExtension[i]] = NULL;
			}
		}
	}

	/**Gives a quick estimation of whether the specified file
	 * can be read or not depending on the filename*/
	template <typename T>
	bool FileReader<T>::canRead(const string& filepath) const {
		const string& realExtension = extractExtension(filepath);
		//const string* haveExtension = extensions;
		std::vector<string> haveExtension = extensions;
		//while (*haveExtension != "") {
		for (unsigned int i = 0; i < haveExtension.size(); ++i) {
			//if (realExtension == *haveExtension) {
			if (realExtension == haveExtension[i]) {
				return true;
			}
			//++haveExtension;
		}
		return false;
	}

	/**Gives a better estimation of whether the specified file
	 * can be read or not depending on the file content*/
	template <typename T>
	bool FileReader<T>::canRead(ifstream& file) const {
		try {
			T* wouldRead = read(file, "unknown file");
			bool ret = (wouldRead != NULL);
			delete wouldRead;
			return ret;
		}
#if defined(WIN32)
		catch (game_runtime_error) {
#else
		catch (game_runtime_error &ex) {
#endif
			throw;
		} catch (...) {
			return false;
		}
		}

	/**Reads a file
	 * This method tries to read the file with the specified filepath
	 * If it fails, either <code>null</code> is returned or an exception
	 * is thrown
	 */
	template <typename T>
	T* FileReader<T>::read(const string& filepath) const {
#if defined(WIN32) && !defined(__MINGW32__)
		FILE *fp = _wfopen(utf8_decode(filepath).c_str(), L"rb");
		ifstream file(fp);
#else
		ifstream file(filepath.c_str(), ios::in | ios::binary);
#endif
		if (!file.is_open()) {
			throw game_runtime_error("[#3] Could not open file " + filepath);
		}
		T* ret = read(file, filepath);
		file.close();
#if defined(WIN32) && !defined(__MINGW32__)
		if (fp) fclose(fp);
#endif

		return ret;
	}

	/**Reads a file
	 * This method tries to read the file with the specified filepath
	 * If it fails, either <code>null</code> is returned or an exception
	 * is thrown
	 */
	template <typename T>
	T* FileReader<T>::read(const string& filepath, T* object) const {
#if defined(WIN32) && !defined(__MINGW32__)
		FILE *fp = _wfopen(utf8_decode(filepath).c_str(), L"rb");
		ifstream file(fp);
#else
		ifstream file(filepath.c_str(), ios::in | ios::binary);
#endif
		if (!file.is_open()) {
			throw game_runtime_error("[#4] Could not open file " + filepath);
		}
		T* ret = read(file, filepath, object);
		file.close();
#if defined(WIN32) && !defined(__MINGW32__)
		if (fp) fclose(fp);
#endif

		return ret;
	}


} //end namespace

#endif
