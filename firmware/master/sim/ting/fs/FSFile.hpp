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



/**
 * @file Ordinary file system File implementation
 * @author Ivan Gagis <igagis@gmail.com>
 */

#pragma once

#include <cstdio>

#include "../debug.hpp"

#include "File.hpp"



namespace ting{
namespace fs{



/**
 * @brief Native OS file system implementation of File interface.
 * Implementation of a ting::File interface for native file system of the OS.
 */
class FSFile : public File{
	std::string rootDir;

	ting::Inited<FILE*, 0> handle;

protected:

	inline std::string TruePath()const{
		return this->rootDir + this->Path();
	}

public:
	/**
	 * @brief Constructor.
     * @param pathName - initial path to set passed to File constructor.
     */
	FSFile(const std::string& pathName = std::string()) :
			File(pathName)
	{}
	
	/**
	 * @brief Destructor.
	 * This destructor calls the Close() method.
	 */
	~FSFile()throw(){
		this->Close();
	}

	/**
	 * @brief Set root directory.
	 * Sets the root directory which holds the file system subtree. The file path
	 * set by SetPath() method will refer to a file path relative to the root directory.
	 * That means that all file operations like opening the file and other will be 
	 * performed on the actual file/directory referred by the final path which is a concatenation of
	 * the root directory and the path returned by Path() method. 
     * @param dir - path to the root directory to set. It should have trailing '/' character.
     */
	void SetRootDir(const std::string &dir);

	/**
	 * @brief Get current root directory.
	 * Returns the current root directory. See description of SetRootDir() method
	 * for more details.
     * @return Current root directory.
     */
	inline const std::string& GetRootDir()const throw(){
		return this->rootDir;
	}


	//override
	virtual void Open(EMode mode);



	//override
	virtual void Close()throw();



	//override
	virtual size_t ReadInternal(ting::Buffer<ting::u8>& buf);



	//override
	virtual size_t WriteInternal(const ting::Buffer<ting::u8>& buf);



	//override
	virtual void SeekForward(size_t numBytesToSeek);
	
	
	
	//override
	virtual void SeekBackward(size_t numBytesToSeek);

	
	
	/**
	 * @brief Seek forward or backward.
     * @param numBytesToSeek - number of bytes to seek.
     * @param seekForward - if true, then the seeking will be done forward.
	 *                      If false, then the seeking will be done backward.
     */
	void Seek(size_t numBytesToSeek, bool seekForward);
	
	
	//override
	virtual void Rewind();
	


	//override
	virtual bool Exists()const;



	//override
	virtual void MakeDir();



public:
	/**
	 * @brief Get user home directory.
	 * Returns an absolute path to the current user's home directory.
	 * On *nix systems it will be something like "/home/user/".
     * @return Absolute path to the user's home directory.
     */
	static std::string GetHomeDir();



	//override
	virtual ting::Array<std::string> ListDirContents(size_t maxEntries = 0);
};



}//~namespace
}//~namespace
