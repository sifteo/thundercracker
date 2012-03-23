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



#include <list>

#include "File.hpp"

#include "../util.hpp"



using namespace ting::fs;



std::string File::ExtractExtension()const{
	size_t dotPos = this->Path().rfind('.');
	if(dotPos == std::string::npos || dotPos == 0){//NOTE: dotPos is 0 for hidden files in *nix systems
		return std::string();
	}else{
		ASSERT(dotPos > 0)
		ASSERT(this->Path().size() > 0)
		ASSERT(this->Path().size() >= dotPos + 1)
		
		//Check for hidden file on *nix systems
		if(this->Path()[dotPos - 1] == '/'){
			return std::string();
		}
		
		return std::string(this->Path(), dotPos + 1, this->Path().size() - (dotPos + 1));
	}
	ASSERT(false)
}



bool File::IsDir()const throw(){
	if(this->Path().size() == 0){
		return true;
	}

	ASSERT(this->Path().size() > 0)
	if(this->Path()[this->Path().size() - 1] == '/'){
		return true;
	}

	return false;
}



ting::Array<std::string> File::ListDirContents(size_t maxEntries){
	throw File::Exc("File::ListDirContents(): not supported for this File instance");
}



size_t File::Read(
			ting::Buffer<ting::u8>& buf,
			size_t numBytesToRead,
			size_t offset
		)
{
	if(!this->IsOpened()){
		throw File::IllegalStateExc("Cannot read, file is not opened");
	}

	size_t actualNumBytesToRead =
			numBytesToRead == 0 ? buf.Size() : numBytesToRead;

	if(offset > buf.Size()){
		throw File::Exc("offset is out of buffer bounds");
	}

	if(actualNumBytesToRead > buf.Size() - offset){
		throw File::Exc("attempt to read more bytes than the number of bytes from offset to the buffer end");
	}

	ASSERT(actualNumBytesToRead + offset <= buf.Size())
	ting::Buffer<ting::u8> b(buf.Begin() + offset, actualNumBytesToRead);
	return this->ReadInternal(b);
}



size_t File::Write(
			const ting::Buffer<ting::u8>& buf,
			size_t numBytesToWrite,
			size_t offset
		)
{
	if(!this->IsOpened()){
		throw File::IllegalStateExc("Cannot write, file is not opened");
	}

	if(this->ioMode != WRITE){
		throw File::Exc("file is opened, but not in WRITE mode");
	}

	size_t actualNumBytesToWrite =
			numBytesToWrite == 0 ? buf.SizeInBytes() : numBytesToWrite;

	if(offset > buf.Size()){
		throw File::Exc("offset is out of buffer bounds");
	}

	if(actualNumBytesToWrite > buf.Size() - offset){
		throw File::Exc("attempt to write more bytes than passed buffer contains");
	}

	ASSERT(actualNumBytesToWrite + offset <= buf.SizeInBytes())
	return this->WriteInternal(ting::Buffer<ting::u8>(const_cast<ting::u8*>(buf.Begin() + offset), actualNumBytesToWrite));
}



void File::SeekForward(size_t numBytesToSeek){
	if(!this->IsOpened())
		throw File::Exc("SeekForward(): file is not opened");
	
	ting::StaticBuffer<ting::u8, 0xfff> buf;//4kb buffer
	
	for(size_t bytesRead = 0; bytesRead != numBytesToSeek;){
		size_t curNumToRead = numBytesToSeek - bytesRead;
		ting::util::ClampTop(curNumToRead, buf.Size());
		size_t res = this->Read(buf, curNumToRead);
		if(res != curNumToRead){//if end of file reached
			throw ting::Exc("File::SeekForward(): end of file reached, seeking did not complete");
		}
		ASSERT(bytesRead < numBytesToSeek)
		ASSERT(numBytesToSeek >= res)
		ASSERT(bytesRead <= numBytesToSeek - res)
		bytesRead += res;
	}
}



void File::SeekBackward(size_t numBytesToSeek){
	throw ting::Exc("SeekBackward(): unsupported");
}


void File::Rewind(){
	throw ting::Exc("Rewind(): unsupported");
}



void File::MakeDir(){
	throw File::Exc("Make directory is not supported");
}



namespace{
const size_t DReadBlockSize = 4 * 1024;

//Define a class derived from StaticBuffer. This is just to define custom
//copy constructor which will do nothing to avoid unnecessary buffer copying when
//inserting new element to the list of chunks.
struct Chunk : public ting::StaticBuffer<ting::u8, DReadBlockSize>{
	inline Chunk(){}
	inline Chunk(const Chunk&){}
};
}



ting::Array<ting::u8> File::LoadWholeFileIntoMemory(size_t maxBytesToLoad){
	if(this->IsOpened()){
		throw File::IllegalStateExc("file should not be opened");
	}

	File::Guard fileGuard(*this, File::READ);//make sure we close the file upon exit from the function
	
	std::list<Chunk> chunks;
	
	size_t res;
	size_t bytesRead = 0;
	
	for(;;){
		if(bytesRead == maxBytesToLoad){
			break;
		}
		
		chunks.push_back(Chunk());
		
		ASSERT(maxBytesToLoad > bytesRead)
		
		size_t numBytesToRead = maxBytesToLoad - bytesRead;
		ting::util::ClampTop(numBytesToRead, chunks.back().Size());
		
		res = this->Read(chunks.back(), numBytesToRead);

		bytesRead += res;
		
		if(res != numBytesToRead){
			ASSERT(res < chunks.back().Size())
			ASSERT(res < numBytesToRead)
			if(res == 0){
				chunks.pop_back();//pop empty chunk
			}
			break;
		}
	}
	
	ASSERT(maxBytesToLoad >= bytesRead)
	
	if(chunks.size() == 0){
		return ting::Array<ting::u8>();
	}
	
	ASSERT(chunks.size() >= 1)
	
	ting::Array<ting::u8> ret((chunks.size() - 1) * chunks.front().Size() + res);
	
	ting::u8* p;
	for(p = ret.Begin(); chunks.size() > 1; p += chunks.front().Size()){
		ASSERT(p < ret.End())
		memcpy(p, chunks.front().Begin(), chunks.front().Size());
		chunks.pop_front();
	}
	
	ASSERT(chunks.size() == 1)
	ASSERT(res <= chunks.front().Size())
	memcpy(p, chunks.front().Begin(), res);
	ASSERT(p + res == ret.End())
	
	return ret;
}



bool File::Exists()const{
	if(this->IsDir()){
		throw File::Exc("File::Exists(): Checking for directory existence is not supported");
	}

	if(this->IsOpened()){
		return true;
	}

	//try opening and closing the file to find out if it exists or not
	ASSERT(!this->IsOpened())
	try{
		File::Guard fileGuard(const_cast<File&>(*this), File::READ);
	}catch(File::Exc &e){
		return false;//file opening failed, assume the file does not exist
	}
	return true;//file open succeeded => file exists
}



File::Guard::Guard(File& file, EMode mode) :
		f(file)
{
	if(this->f.IsOpened())
		throw File::Exc("File::Guard::Guard(): file is already opened");

	this->f.Open(mode);
}



File::Guard::~Guard(){
	this->f.Close();
}
