
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/


# include <errno.h>
# include <aleph.H>
# include <socket_wrappers.H>

extern "C"
{

ssize_t readn(int fd, void *vptr, size_t n)
{
  ssize_t nread;

  char * ptr = static_cast<char *>(vptr);
  size_t nleft = n;

  while (nleft > 0) 
    {
      if ( (nread = read(fd, ptr, nleft)) < 0) 
	{
          if (errno == EINTR)
	    nread = 0; /* and call read() again */
	  else
	    return -1;
	} 
      else 
	if (nread == 0)
	  break; /* EOF */

      nleft -= nread;
      ptr   += nread;
    }

  return n - nleft;              /* return >= 0 */
}


ssize_t writen(int fd, const void *vptr, size_t n)
{
  ssize_t         nwritten;

  const char * ptr = static_cast<const char *>(vptr);
  size_t nleft = n;

  while (nleft > 0) 
    {
      nwritten = write(fd, ptr, nleft);
      if ( nwritten <= 0) 
	{
	  if (errno == EINTR)
	    nwritten = 0;           /* call write() again */
	  else
	    return -1;
	}
      nleft -= nwritten;
      ptr   += nwritten;
    }

  return n;
}


static const size_t buffer_size = 1024;

ssize_t flushn(int fd, size_t n)
{
  char buffer[buffer_size];

  int num_reads      = n / buffer_size;
  int last_read_size = n % buffer_size;

  int read_size;

  for (int i = 0; i < num_reads; i++)
    {
      read_size = readn(fd, &buffer[0], buffer_size);
      if (read_size < 0)
	return read_size;
    }

  if (last_read_size == 0)
    return n;

  read_size = readn(fd, &buffer[0], last_read_size);

  return read_size;
}


}
