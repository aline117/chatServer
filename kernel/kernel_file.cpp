/*
The MIT License (MIT)

Copyright (c) 2013-2015 SRS(ossrs)

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <kernel_file.hpp>

// for srs-librtmp, @see https://github.com/ossrs/srs/issues/213
#ifndef _WIN32
#include <unistd.h>
#include <sys/uio.h>
#endif

#include <fcntl.h>
#include <sstream>
using namespace std;

#include <kernel_log.hpp>
#include <kernel_error.hpp>

FileWriter::FileWriter()
{
    fd = -1;
}

FileWriter::~FileWriter()
{
    close();
}

int FileWriter::open(string p)
{
    int ret = ERROR_SUCCESS;
    
    if (fd > 0) {
        ret = ERROR_SYSTEM_FILE_ALREADY_OPENED;
        srs_error("file %s already opened. ret=%d", path.c_str(), ret);
        return ret;
    }
    
    int flags = O_CREAT|O_WRONLY|O_TRUNC;
    mode_t mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH;

    if ((fd = ::open(p.c_str(), flags, mode)) < 0) {
        ret = ERROR_SYSTEM_FILE_OPENE;
        srs_error("open file %s failed. ret=%d", p.c_str(), ret);
        return ret;
    }
    
    path = p;
    
    return ret;
}

int FileWriter::open_append(string p)
{
    int ret = ERROR_SUCCESS;
    
    if (fd > 0) {
        ret = ERROR_SYSTEM_FILE_ALREADY_OPENED;
        srs_error("file %s already opened. ret=%d", path.c_str(), ret);
        return ret;
    }
    
    int flags = O_APPEND|O_WRONLY;
    mode_t mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH;

    if ((fd = ::open(p.c_str(), flags, mode)) < 0) {
        ret = ERROR_SYSTEM_FILE_OPENE;
        srs_error("open file %s failed. ret=%d", p.c_str(), ret);
        return ret;
    }
    
    path = p;
    
    return ret;
}

void FileWriter::close()
{
    int ret = ERROR_SUCCESS;
    
    if (fd < 0) {
        return;
    }
    
    if (::close(fd) < 0) {
        ret = ERROR_SYSTEM_FILE_CLOSE;
        srs_error("close file %s failed. ret=%d", path.c_str(), ret);
        return;
    }
    fd = -1;
    
    return;
}

bool FileWriter::is_open()
{
    return fd > 0;
}

void FileWriter::lseek(int64_t offset)
{
    ::lseek(fd, (off_t)offset, SEEK_SET);
}

int64_t FileWriter::tellg()
{
    return (int64_t)::lseek(fd, 0, SEEK_CUR);
}

int FileWriter::write(void* buf, size_t count, ssize_t* pnwrite)
{
    int ret = ERROR_SUCCESS;
    
    ssize_t nwrite;
    // TODO: FIXME: use st_write.
    if ((nwrite = ::write(fd, buf, count)) < 0) {
        ret = ERROR_SYSTEM_FILE_WRITE;
        srs_error("write to file %s failed. ret=%d", path.c_str(), ret);
        return ret;
    }
    
    if (pnwrite != NULL) {
        *pnwrite = nwrite;
    }
    
    return ret;
}

int FileWriter::writev(iovec* iov, int iovcnt, ssize_t* pnwrite)
{
    int ret = ERROR_SUCCESS;
    
    ssize_t nwrite = 0;
    for (int i = 0; i < iovcnt; i++) {
        iovec* piov = iov + i;
        ssize_t this_nwrite = 0;
        if ((ret = write(piov->iov_base, piov->iov_len, &this_nwrite)) != ERROR_SUCCESS) {
            return ret;
        }
        nwrite += this_nwrite;
    }
    
    if (pnwrite) {
        *pnwrite = nwrite;
    }
    
    return ret;
}

FileReader::FileReader()
{
    fd = -1;
}

FileReader::~FileReader()
{
    close();
}

int FileReader::open(string p)
{
    int ret = ERROR_SUCCESS;
    
    if (fd > 0) {
        ret = ERROR_SYSTEM_FILE_ALREADY_OPENED;
        srs_error("file %s already opened. ret=%d", path.c_str(), ret);
        return ret;
    }

    if ((fd = ::open(p.c_str(), O_RDONLY)) < 0) {
        ret = ERROR_SYSTEM_FILE_OPENE;
        srs_error("open file %s failed. ret=%d", p.c_str(), ret);
        return ret;
    }
    
    path = p;
    
    return ret;
}

void FileReader::close()
{
    int ret = ERROR_SUCCESS;
    
    if (fd < 0) {
        return;
    }
    
    if (::close(fd) < 0) {
        ret = ERROR_SYSTEM_FILE_CLOSE;
        srs_error("close file %s failed. ret=%d", path.c_str(), ret);
        return;
    }
    fd = -1;
    
    return;
}

bool FileReader::is_open()
{
    return fd > 0;
}

int64_t FileReader::tellg()
{
    return (int64_t)::lseek(fd, 0, SEEK_CUR);
}

void FileReader::skip(int64_t size)
{
    ::lseek(fd, (off_t)size, SEEK_CUR);
}

int64_t FileReader::lseek(int64_t offset)
{
    return (int64_t)::lseek(fd, (off_t)offset, SEEK_SET);
}

int64_t FileReader::filesize()
{
    int64_t cur = tellg();
    int64_t size = (int64_t)::lseek(fd, 0, SEEK_END);
    ::lseek(fd, (off_t)cur, SEEK_SET);
    return size;
}

int FileReader::read(void* buf, size_t count, ssize_t* pnread)
{
    int ret = ERROR_SUCCESS;
    
    ssize_t nread;
    // TODO: FIXME: use st_read.
    if ((nread = ::read(fd, buf, count)) < 0) {
        ret = ERROR_SYSTEM_FILE_READ;
        srs_error("read from file %s failed. ret=%d", path.c_str(), ret);
        return ret;
    }
    
    if (nread == 0) {
        ret = ERROR_SYSTEM_FILE_EOF;
        return ret;
    }
    
    if (pnread != NULL) {
        *pnread = nread;
    }
    
    return ret;
}

