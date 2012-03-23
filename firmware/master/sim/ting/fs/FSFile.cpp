/* The MIT License:

Copyright (c) 2009-2012 Ivan Gagis

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE. */

// Home page: http://ting.googlecode.com



#ifdef WIN32
#include <windows.h>

#elif defined(__linux__)
#include <dirent.h>
#include <sys/stat.h>
#include <cerrno>

#endif

#include <vector>
#include <cstdlib>
#include <sstream>

#include "FSFile.hpp"



using namespace ting::fs;



void FSFile::SetRootDir(const std::string &dir){
	if(this->IsOpened()){
		throw File::IllegalStateExc("cannot set root directory when file is opened");
	}
	
	if(dir.size() > 0){
		if(dir[dir.size() - 1] != '/'){
			throw File::Exc("FSFile::SetRootDir(): argument is not a directory, should have trailing '/'");
		}
	}
	this->rootDir = std::string(dir.c_str());//immediate copy (we do not want copy-on-write)
}



//override
void FSFile::Open(EMode mode){
	if(this->IsOpened()){
		throw File::IllegalStateExc("cannot open, file is already opened.");
	}

	if(this->IsDir()){
		throw File::Exc("path refers to a directory, directories can't be opened");
	}
	
	const char* modeStr;
	switch(mode){
		case File::WRITE:
			modeStr="r+b";
			break;
		case File::CREATE:
			modeStr="w+b";
			break;
		case File::READ:
			modeStr="rb";
			break;
		default:
			throw File::Exc("unknown mode");
			break;
	}
	this->handle = fopen(this->TruePath().c_str(), modeStr);
	if(!this->handle){
		TRACE(<< "FSFile::Open(): TruePath() = " << this->TruePath().c_str() << std::endl)
		throw File::Exc("fopen() failed");
	}

	//set open mode
	if(mode == CREATE){
		this->ioMode = WRITE;
	}else{
		this->ioMode = mode;
	}

	this->isOpened = true;//set "opened" flag
}



//override
void FSFile::Close()throw(){
	if(!this->IsOpened()){
		return;
	}

	ASSERT(this->handle)

	fclose(this->handle);
	this->handle = 0;

	this->isOpened = false;
}



//override
size_t FSFile::ReadInternal(ting::Buffer<ting::u8>& buf){
	ASSERT(this->handle)
	size_t numBytesRead = fread(buf.Begin(), 1, buf.Size(), this->handle);
	if(numBytesRead != buf.Size()){//something happened
		if(!feof(this->handle)){
			throw File::Exc("fread() error");//if it is not an EndOfFile then it is error
		}
	}
	return numBytesRead;
}



//override
size_t FSFile::WriteInternal(const ting::Buffer<ting::u8>& buf){
	ASSERT(this->handle)
	size_t bytesWritten = fwrite(buf.Begin(), 1, buf.Size(), this->handle);
	if(bytesWritten != buf.Size()){//something bad has happened
		throw File::Exc("fwrite error");
	}

	return bytesWritten;
}



void FSFile::Seek(size_t numBytesToSeek, bool seekForward){
	if(!this->IsOpened()){
		throw File::IllegalStateExc("cannot seek, file is not opened");
	}

	ASSERT(this->handle)
	
	//NOTE: fseek() accepts 'long int' as offset argument which is signed and can be
	//      less than size_t value passed as argument to this function.
	//      Therefore, do several seek operations with smaller offset if necessary.
	
	typedef long int T_FSeekOffset;
	const size_t DMax = ((size_t(T_FSeekOffset(-1))) >> 1);
	ASSERT((size_t(1) << ((sizeof(T_FSeekOffset) * 8) - 1)) - 1 == DMax)
	STATIC_ASSERT(size_t(-(-T_FSeekOffset(DMax))) == DMax)
	
	for(size_t numBytesLeft = numBytesToSeek; numBytesLeft != 0;){
		ASSERT(numBytesLeft <= numBytesToSeek)
		
		T_FSeekOffset offset;
		if(numBytesLeft > DMax){
			offset = T_FSeekOffset(DMax);
		}else{
			offset = T_FSeekOffset(numBytesLeft);
		}
		
		ASSERT(offset > 0)
		
		if(fseek(this->handle, seekForward ? offset : (-offset), SEEK_CUR) != 0){
			throw File::Exc("fseek() failed");
		}
		
		ASSERT(size_t(offset) < size_t(-1))
		ASSERT(numBytesLeft >= size_t(offset))
		
		numBytesLeft -= size_t(offset);
	}
}



//override
void FSFile::SeekForward(size_t numBytesToSeek){
	this->Seek(numBytesToSeek, true);
}



//override
void FSFile::SeekBackward(size_t numBytesToSeek){
	this->Seek(numBytesToSeek, false);
}



//override
void FSFile::Rewind(){
	if(!this->IsOpened()){
		throw File::IllegalStateExc("cannot rewind, file is not opened");
	}

	ASSERT(this->handle)
	if(fseek(this->handle, 0, SEEK_SET) != 0){
		throw File::Exc("fseek() failed");
	}
}



//override
bool FSFile::Exists()const{
	if(this->IsOpened()){ //file is opened => it exists
		return true;
	}
	
	if(this->Path().size() == 0){
		return false;
	}

	//if it is a directory, check directory existence
	if(this->Path()[this->Path().size() - 1] == '/'){
#if defined(__linux__)
		DIR *pdir = opendir(this->TruePath().c_str());
		if(!pdir){
			return false;
		}else{
			closedir(pdir);
			return true;
		}
#else
		throw File::Exc("Checking for directory existence is not supported");
#endif
	}else{
		return this->File::Exists();
	}
}



//override
void FSFile::MakeDir(){
	if(this->IsOpened()){
		throw File::IllegalStateExc("cannot make directory when file is opened");
	}

	if(this->Path().size() == 0 || this->Path()[this->Path().size() - 1] != '/'){
		throw File::Exc("invalid directory name");
	}

#if defined(__linux__)
//	TRACE(<< "creating directory = " << this->Path() << std::endl)
	umask(0);//clear umask for proper permissions of newly created directory
	if(mkdir(this->TruePath().c_str(), 0777) != 0){
		throw File::Exc("mkdir() failed");
	}
#else
	throw File::Exc("creating directory is not supported");
#endif
}



//static
std::string FSFile::GetHomeDir(){
	std::string ret;
	
#if defined(__linux__) || defined(WIN32)
	
#if defined(__linux__)
	char * home = getenv("HOME");
#elif defined(WIN32)
	char * home = getenv("USERPROFILE");
#else
#error "ASSERTION FAILURE: should never get here"
#endif
	
	if(!home){
		throw File::Exc("HOME environment variable does not exist");
	}

	ret = std::string(home);
#else
#error "unsupported os"
#endif
	
	//append trailing '/' if needed
	if(ret.size() == 0 || ret[ret.size() - 1] != '/'){
		ret += '/';
	}

	return ret;
}



//override
ting::Array<std::string> FSFile::ListDirContents(size_t maxEntries){
	if(!this->IsDir())
		throw File::Exc("FSFile::ListDirContents(): this is not a directory");

	std::vector<std::string> files;

#ifdef __WIN32__
	{
		std::string pattern = this->TruePath();
		pattern += '*';

		TRACE(<< "FSFile::ListDirContents(): pattern = " << pattern << std::endl)

		WIN32_FIND_DATA wfd;
		HANDLE h = FindFirstFile(pattern.c_str(), &wfd);
		if(h == INVALID_HANDLE_VALUE)
			throw File::Exc("ListDirContents(): cannot find first file");

		//create Find Closer to automatically call FindClose on exit from the function in case of exceptions etc...
		{
			struct FindCloser{
				HANDLE hnd;
				FindCloser(HANDLE h) :
					hnd(h)
				{}
				~FindCloser(){
					FindClose(this->hnd);
				}
			} findCloser(h);

			do{
				std::string s(wfd.cFileName);
				ASSERT(s.size() > 0)

				//do not add ./ and ../ directories, we are not interested in them
				if(s == "." || s == "..")
					continue;

				if(((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) && s[s.size() - 1] != '/')
					s += '/';
				files.push_back(s);
				
				if(files.size() == maxEntries){
					break;
				}
			}while(FindNextFile(h, &wfd) != 0);

			if(GetLastError() != ERROR_NO_MORE_FILES)
				throw File::Exc("ListDirContents(): find next file failed");
		}
	}
#elif defined(__linux__)
	{
		DIR *pdir = opendir(this->TruePath().c_str());

		if(!pdir){
			std::stringstream ss;
			ss << "FSFile::ListDirContents(): opendir() failure, error code = " << strerror(errno);
			throw File::Exc(ss.str());
		}

		//create DirentCloser to automatically call closedir on exit from the function in case of exceptions etc...
		struct DirCloser{
			DIR *pdir;

			DirCloser(DIR *pDirToClose) :
					pdir(pDirToClose)
			{}

			~DirCloser(){
				int ret;
				do{
					ret = closedir(this->pdir);
					ASSERT_INFO(ret == 0 || errno == EINTR, "FSFile::ListDirContents(): closedir() failed: " << strerror(errno))
				}while(ret != 0 && errno == EINTR);
			}
		} dirCloser(pdir);

		errno = 0;//clear errno
		while(dirent *pent = readdir(pdir)){
			std::string s(pent->d_name);
			if(s == "." || s == "..")
				continue;//do not add ./ and ../ directories, we are not interested in them

			struct stat fileStats;
			//TRACE(<< s << std::endl)
			if(stat((this->TruePath() + s).c_str(), &fileStats) < 0){
				std::stringstream ss;
				ss << "FSFile::ListDirContents(): stat() failure, error code = " << strerror(errno);
				throw File::Exc(ss.str());
			}

			if(fileStats.st_mode & S_IFDIR)//if this entry is a directory append '/' symbol to its end
				s += "/";

			files.push_back(s);
			
			if(files.size() == maxEntries){
				break;
			}
		}//~while()

		//check if we exited the while() loop because of readdir() failed
		if(errno != 0){
			std::stringstream ss;
			ss << "FSFile::ListDirContents(): readdir() failure, error code = " << strerror(errno);
			throw File::Exc(ss.str());
		}
	}
#elif defined(__APPLE__)

#error "FSFile::ListDirContents(): MacOSx version is not implemented yet"

#else

#error "FSFile::ListDirContents(): version is not implemented yet for this os"

#endif
	
	ting::Array<std::string> filesArray(files.size());
	
	for(size_t i = 0; i < files.size(); ++i)
		filesArray[i] = files[i];

	return filesArray;
}//~ListDirContents()
