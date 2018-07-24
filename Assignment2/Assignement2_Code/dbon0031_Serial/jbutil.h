/*!
 * \file
 *
 * Copyright (c) 2011-2014 Johann A. Briffa
 *
 * This file is part of JButil.
 *
 * JButil is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * JButil is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with JButil.  If not, see <http://www.gnu.org/licenses/>.
 *
 * \section svn Version Control
 */

#ifndef __jbutil_h
#define __jbutil_h

#include <sys/time.h>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <typeinfo>
#include <stdint.h>

// *** Global namespace ***

/*! \name Math constants */

const double pi = 3.14159265358979323846;

// @}

/*! \name Math functions */

template <class real>
inline int round(real x)
   {
   return int(floor(x + 0.5));
   }

// @}

/*! \name Error handling */

// An assertion that is implemented even in release builds

#ifdef NDEBUG
#  define assertalways(_Expression) (void)( (!!(_Expression)) || (jbutil::reportassertionandfail(#_Expression, __FILE__, __LINE__), 0) )
#else
#  define assertalways(_Expression) assert(_Expression)
#endif

// Fail with error

#define failwith(_String) jbutil::reporterrorandfail(_String, __FILE__, __LINE__)

// @}

// *** Within library namespace ***

namespace jbutil {

/*! \name Debugging tools */

inline void reporterrorandfail(const char *expression, const char *file, int line)
   {
   std::cerr << "ERROR (" << file << " line " << line << "): " << expression
         << std::endl;
   exit(1);
   }

inline void reportassertionandfail(const char *expression, const char *file, int line)
   {
   std::string s;
   s = "assertion " + std::string(expression) + " failed.";
   reporterrorandfail(s.c_str(), file, line);
   }

// @}

/*! \name Timing functions */

//! Returns current time in seconds
inline double gettime()
   {
   struct timeval tv;
   struct timezone tz;
   gettimeofday(&tv, &tz);
   return ((double) tv.tv_sec + (double) tv.tv_usec * 1E-6);
   }

// @}

/*! \name Memory allocation */

//! Check for alignment

inline bool isaligned(const void *buf, int bytes)
   {
   return ((long) buf & (bytes - 1)) == 0;
   }

//! Aligned memory allocator

template <class T, size_t alignment>
class aligned_allocator : public std::allocator<T> {
private:
   typedef std::allocator<T> base;
public:
   typedef typename base::pointer pointer;
   typedef typename base::size_type size_type;

   template <class U>
   struct rebind {
      typedef aligned_allocator<U, alignment> other;
   };

   pointer allocate(size_type n, std::allocator<void>::const_pointer hint = 0)
      {
      void *p;
      if (posix_memalign(&p, alignment, n * sizeof(T)) == 0)
         {
         assert(isaligned(p, alignment));
         return (pointer) p;
         }
      throw std::bad_alloc();
      }

   void deallocate(pointer p, size_type n)
      {
      free(p);
      }
};

// @}

/*! \name Containers */

//! Aligned vector class

template <class T, size_t alignment = 128>
class vector : public std::vector<T, aligned_allocator<T, alignment> > {
private:
   typedef std::vector<T, aligned_allocator<T, alignment> > base;
   typedef typename base::size_type size_type;
public:
   // constructors
   vector(size_type size = 0) :
      base(size)
      {
      }
   vector(size_type size, const T& value) :
      base(size, value)
      {
      }
   // override size operator to return an int
   int size() const
      {
      return base::size();
      }
   // element access
   T& operator()(int i)
      {
      assert(i >= 0 && i < size());
      return base::operator [](i);
      }
   const T& operator()(int i) const
      {
      assert(i >= 0 && i < size());
      return base::operator [](i);
      }
   // stream output
   friend std::ostream& operator<<(std::ostream& sout, const vector<T>& data)
      {
      sout << '{';
      for (int i = 0; i < data.size(); i++)
         {
         sout << data(i);
         if (i < data.size() - 1)
            sout << ", ";
         }
      sout << '}';
      return sout;
      }
};

//! Matrix class

template <class T>
class matrix {
private:
   vector<vector<T> > m_data;
   int m_rows;
   int m_cols;
public:
   //! constructor
   matrix() :
      m_rows(0), m_cols(0)
      {
      }
   // note: matrix copy uses default copy assignment operator
   //! matrix resizing
   void resize(int rows, int cols)
      {
      // copy user settings
      m_rows = rows;
      m_cols = cols;
      // allocate memory
      m_data.resize(m_rows);
      for (int i = 0; i < m_rows; i++)
         m_data[i].resize(m_cols);
      }
   //! element access (modifyable)
   T& operator()(int i, int j) // i = row index, j = col index
      {
      assert(i >= 0 && i < m_rows);
      assert(j >= 0 && j < m_cols);
      return m_data[i][j];
      }
   //! element access (read-only)
   const T& operator()(int i, int j) const // i = row index, j = col index
      {
      assert(i >= 0 && i < m_rows);
      assert(j >= 0 && j < m_cols);
      return m_data[i][j];
      }
   //! number of rows in matrix
   int get_rows() const
      {
      return m_rows;
      }
   //! number of cols in matrix
   int get_cols() const
      {
      return m_cols;
      }
   //! compute matrix transpose
   matrix<T> transpose() const
      {
      matrix<T> r;
      r.resize(m_cols, m_rows);
      for (int i = 0; i < m_rows; i++)
         for (int j = 0; j < m_cols; j++)
            r(j, i) = m_data[i][j];
      return r;
      }
   //! stream output
   friend std::ostream& operator<<(std::ostream& sout, const matrix<T>& data)
      {
      sout << '{';
      for (int i = 0; i < data.get_rows(); i++)
         {
         sout << data.m_data(i);
         if (i < data.get_rows() - 1)
            sout << ", ";
         }
      sout << '}';
      return sout;
      }
};

// @}

/*! \name Image processing */

/*! \brief   Image Class.
 *
 * This class encapsulates the data and functions for dealing with a single
 * image, potentially containing a number of channels. According to common
 * convention in image processing, the origin is at the top left, so that
 * row-major order gives the normal raster conversion.
 */

template <class T>
class image {
private:
   //! Internal image representation
   vector<matrix<T> > m_data;
   int m_maxval;
public:
   // Construction / destruction
   explicit image(int rows = 0, int cols = 0, int c = 0, int maxval = 255) :
      m_maxval(maxval)
      {
      m_data.resize(c);
      for (int i = 0; i < c; i++)
         m_data(i).resize(rows, cols);
      }
   virtual ~image()
      {
      }

   /*! \name Information functions */
   //! Maximum pixel value
   int range() const
      {
      return m_maxval;
      }
   //! Number of channels (image planes)
   int channels() const
      {
      return m_data.size();
      }
   //! Number of rows in image
   int get_rows() const
      {
      if (channels() > 0)
         return m_data(0).get_rows();
      return 0;
      }
   //! Number of rows in image
   int get_cols() const
      {
      if (channels() > 0)
         return m_data(0).get_cols();
      return 0;
      }
   // @}

   /*! \name Pixel access */
   //! Pixel access (modifyable)
   // c = channel, i = row index, j = col index
   T& operator()(int c, int i, int j)
      {
      return m_data(c)(i, j);
      }
   //! Pixel access (read-only)
   // c = channel, i = row index, j = col index
   const T& operator()(int c, int i, int j) const
      {
      return m_data(c)(i, j);
      }
   //! Extract channel as a matrix of pixel values
   const matrix<T>& get_channel(int c) const
      {
      assert(c >= 0 && c < channels());
      return m_data(c);
      }
   //! Copy matrix of pixel values to channel
   void set_channel(int c, const matrix<T>& m)
      {
      assert(c >= 0 && c < channels());
      assert(m.get_rows() == get_rows() && m.get_cols() == get_cols());
      m_data(c) = m;
      }
   // @}

   /*! \name Saving/loading functions */

   //! Save image in NetPBM format (PBM/PGM/PPM) - binary only
   void save(std::ostream& sout) const
      {
#ifndef NDEBUG
      std::cerr << "Saving image" << std::flush;
#endif
      // check stream validity
      assertalways(sout);
      // header data
      const int chan = channels();
      assert(chan > 0);
      const int rows = m_data(0).get_rows();
      const int cols = m_data(0).get_cols();
#ifndef NDEBUG
      std::cerr << " (" << cols << "x" << rows << "x" << chan << ")..."
            << std::flush;
#endif
      // write file descriptor
      if (chan == 1 && m_maxval == 1)
         sout << "P4" << std::endl; // bitmap
      else if (chan == 1 && m_maxval > 1)
         sout << "P5" << std::endl; // graymap
      else if (chan == 3)
         sout << "P6" << std::endl; // pixmap
      else
         failwith("Image format not supported");
      // write comment
      sout << "# file written by jbutil" << std::endl;
      // write image size
      sout << cols << " " << rows << std::endl;
      // if needed, write maxval
      if (chan > 1 || m_maxval > 1)
         sout << m_maxval << std::endl;
      // write image data
      for (int i = 0; i < rows; i++)
         for (int j = 0; j < cols; j++)
            for (int c = 0; c < chan; c++)
               {
               int p;
               // Scale from [0,1] if we're using floating point
               if (typeid(T) == typeid(double) || typeid(T) == typeid(float))
                  p = int(round(m_data(c)(i, j) * m_maxval));
               else
                  p = int(m_data(c)(i, j));
               assert(p >= 0 && p <= m_maxval);
               if (m_maxval > 255) // 16-bit binary files (MSB first)
                  {
                  sout.put(p >> 8);
                  p &= 0xff;
                  }
               sout.put(p);
               }
      assertalways(sout);
      // done
#ifndef NDEBUG
      std::cerr << "done" << std::endl;
#endif
      }

   //! Load image in NetPBM format (PBM/PGM/PPM) - ASCII or binary
   void load(std::istream& sin)
      {
#ifndef NDEBUG
      std::cerr << "Loading image" << std::flush;
#endif
      // check stream validity
      assertalways(sin);
      // header data
      int cols, rows, chan;
      bool binary;
      // read file header
      std::string line;
      std::getline(sin, line);
      // read file descriptor
      int descriptor;
      assert(line[0] == 'P');
      std::istringstream(line.substr(1)) >> descriptor;
      assertalways(descriptor >= 1 && descriptor <= 6);
      // determine the number of channels
      if (descriptor == 3 || descriptor == 6)
         chan = 3;
      else
         chan = 1;
      // determine the data format
      if (descriptor >= 4 || descriptor <= 6)
         binary = true;
      else
         binary = false;
      // skip comments or empty lines
      do
         {
         std::getline(sin, line);
         } while (line.size() == 0 || line[0] == '#');
      // read image size
      std::istringstream(line) >> cols >> rows;
      // if necessary read pixel value range
      if (descriptor == 1 || descriptor == 4)
         {
         m_maxval = 1;
         assertalways(!binary); // cannot handle binary bitmaps (packed bits)
         }
      else
         {
         std::getline(sin, line);
         std::istringstream(line) >> m_maxval;
         }
#ifndef NDEBUG
      std::cerr << " (" << cols << "x" << rows << "x" << chan << ")...";
#endif
      // set up space to hold image
      m_data.resize(chan);
      for (int c = 0; c < chan; c++)
         m_data(c).resize(rows, cols);
      // read image data
      for (int i = 0; i < rows; i++)
         for (int j = 0; j < cols; j++)
            for (int c = 0; c < chan; c++)
               {
               if (binary)
                  {
                  int p = sin.get();
                  if (m_maxval > 255) // 16-bit binary files (MSB first)
                     p = (p << 8) + sin.get();
                  // Scale to [0,1] if we're using floating point
                  if (typeid(T) == typeid(double) || typeid(T) == typeid(float))
                     m_data(c)(i, j) = T(p) / T(m_maxval);
                  else
                     m_data(c)(i, j) = T(p);
                  }
               else
                  sin >> m_data(c)(i, j);
               assert(m_data(c)(i, j) >= 0 && m_data(c)(i, j) <= m_maxval);
               }
      assertalways(sin);
      // done
#ifndef NDEBUG
      std::cerr << "done" << std::endl;
#endif
      }
};

// @}

/*! \name Random numbers */

/*! \brief   Random generator, adapted from Numerical Recipes.
 *
 * This class implements a thread-safe random number generator.
 */

class randgen {
private:
   // internal state variables
   uint64_t u, v, w;
public:
   // constructor
   explicit randgen(uint64_t s = 0)
      {
      seed(s);
      }
   // generator initialization
   void seed(uint64_t s)
      {
      v = 4101842887655102017LL;
      w = 1;
      u = s ^ v;
      ival64();
      v = u;
      ival64();
      w = v;
      ival64();
      }
   // state advance
   inline void advance()
      {
      u = u * 2862933555777941757LL + 7046029254386353087LL;
      v ^= v >> 17;
      v ^= v << 31;
      v ^= v >> 8;
      w = 4294957665U * (w & 0xffffffff) + (w >> 32);
      }
   // advance, and return a new random value
   inline uint64_t ival64()
      {
      advance();
      uint64_t x = u ^ (u << 21);
      x ^= x >> 35;
      x ^= x << 4;
      return (x + v) ^ w;
      }
   inline double fval()
      {
      return 5.42101086242752217E-20 * ival64();
      }
   inline double fval(double a, double b)
      {
      return fval() * (b - a) + a;
      }
};

// @}

} // end namespace

#endif
