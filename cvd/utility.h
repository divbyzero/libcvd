#ifndef CVD_UTILITY_H
#define CVD_UTILITY_H

#include <cvd/config.h>
#include <cvd/image.h>
#include <cvd/internal/is_pod.h>
#include <cvd/internal/pixel_traits.h>
#include <cvd/internal/convert_pixel_types.h>

namespace CVD { //begin namespace

  /** Generic image copy function for copying sub rectangles of images into
  other images. This performs pixel type conversion if the input and output
  images are different pixel types.
  @param in input image to copy from
  @param out output image to copy into
  @param size size of the area to copy. By default this is the entirty of the
input image
  @param begin upper left corner of the area to copy, by default the upper left
corner of the input image
  @param dst upper left corner of the destination in the output image, by
default the upper left corner of the output image
  @throw ImageRefNotInImage if either begin is not in the input image or dst not
in the output image
  @ingroup gImageIO
  */
  template<class S, class T> void copy(const BasicImage<S>& in, BasicImage<T>&
out, ImageRef size=ImageRef(-1,-1), ImageRef begin = ImageRef(), ImageRef dst =
ImageRef())
  {
    if (size.x == -1 && size.y == -1)
      size = in.size();
    // FIXME: This should be an exception, but which one do I use? 
    // I refuse to define another "ImageRefNotInImage" in this namespace.
    if (!(in.in_image(begin) && out.in_image(dst) && in.in_image(begin+size - ImageRef(1,1)) && out.in_image(dst+size - ImageRef(1,1)))){	
	std::cerr << "bad copy: " << in.size() << " " << out.size() << " " << size << " " << begin << " " << dst << std::endl;
	int *p = 0;
	*p = 1;
    }
    if (in.size() == out.size() && size == in.size() && begin == ImageRef() && dst == ImageRef()) {
      Pixel::ConvertPixels<S,T>::convert(in.data(), out.data(), in.totalsize());
      return;
    }
    
    const S* from = &in[begin];
    T* to = &out[dst];
    int i = 0;
    while (i++<size.y) {
      Pixel::ConvertPixels<S,T>::convert(from, to, size.x);
      from += in.size().x;
      to += out.size().x;
    }
  }
  
  template <class T, bool pod = Internal::is_POD<T>::is_pod> struct ZeroPixel {
      static void zero(T& t) { 
	  for (unsigned int c=0; c<Pixel::Component<T>::count; c++)
	      Pixel::Component<T>::get(t,c) = 0;
      }
  };
  
  template <class T> struct ZeroPixel<T,true> {
      static void zero(T& t) { memset(&t,0,sizeof(T)); }
  };
  
  template <class T, bool pod = Internal::is_POD<T>::is_pod> struct ZeroPixels {
      static void zero(T* pixels, int count) {
	  if (count) {
	      ZeroPixel<T>::zero(*pixels);
	      std::fill(pixels+1, pixels+count, *pixels);
	  }
      }
  };

  template <class T> struct ZeroPixels<T,true> {
      static void zero(T* pixels, int count) {
	  memset(pixels, 0, sizeof(T)*count);
      }
  };
  

  /// Set a pixel to the default value (typically 0)
  /// For multi-component pixels, this zeros all components (sets them to defaults)
  template <class T> inline void zeroPixel(T& pixel) { ZeroPixel<T>::zero(pixel); }

  /// Set many pixels to the default value (typically 0)
  /// For multi-component pixels, this zeros all components (sets them to defaults)
  template <class T> inline void zeroPixels(T* pixels, int count) {  ZeroPixels<T>::zero(pixels, count);  }
  
  /// Set the one-pixel border (top, bottom, sides) of an image to zero values
  template <class T> void zeroBorders(BasicImage<T>& I)
  {
    if (I.size().y == 0)
      return;
    zeroPixels(I[0], I.size().x);
    for (int r=0;r<I.size().y-1; r++)
	zeroPixels(I[r]+I.size().x-1,2);
    zeroPixels(I[I.size().y-1], I.size().x);
  }

  /// Compute pointwise differences (a_i - b_i) and store in diff_i
  /// This is accelerated using SIMD for some platforms and data types (alignment is checked at runtime)
  /// Do not specify template parameters explicitly so that overloading can choose the right implementation
  template <class A, class B> inline void differences(const A* a, const A* b, B* diff, unsigned int count)
  {
      while (count--)
	  *(diff++) = (B)*(a++) - (B)*(b++);
  }

  /// Compute pointwise (a_i + b_i) * c and add to out_i
  /// This is accelerated using SIMD for some platforms and data types (alignment is checked at runtime)
  /// Do not specify template parameters explicitly so that overloading can choose the right implementation
  template <class A, class B> inline void add_multiple_of_sum(const A* a, const A* b, const A& c,  B* out, unsigned int count)
  {
      while (count--)
	  *(out++) += (*(a++) + *(b++)) * c;
  }
  
  /// Compute pointwise a_i * c and store in out_i
  /// This is accelerated using SIMD for some platforms and data types (alignment is checked at runtime)
  /// Do not specify template parameters explicitly so that overloading can choose the right implementation
  template <class A, class B> inline void assign_multiple(const A* a, const A& c,  B* out, unsigned int count)
  {
      while (count--)
	  *(out++) = *(a++) * c;
  }

  /// Compute sum(a_i*b_i)
  /// This is accelerated using SIMD for some platforms and data types (alignment is checked at runtime)
  /// Do not specify template parameters explicitly so that overloading can choose the right implementation
  template <class T> double inner_product(const T* a, const T* b, unsigned int count) {
      double dot = 0;
      while (count--)
	  dot += *(a++) * *(b++);
      return dot;
  }

  template <class R, class D, class T> struct SumSquaredDifferences {
      static inline R sum_squared_differences(const T* a, const T* b, size_t count) {
	  R ssd = 0;
	  while (count--) {
	      D d = *a++ - *b++;
	      ssd += d*d;
	  }
	  return ssd;
      }
  };
 
  /// Compute sum of (a_i - b_i)^2 (the SSD)
  /// This is accelerated using SIMD for some platforms and data types (alignment is checked at runtime)
  /// Do not specify template parameters explicitly so that overloading can choose the right implementation
  template <class T> inline double sum_squared_differences(const T* a, const T* b, size_t count) {
      return SumSquaredDifferences<double,double,T>::sum_squared_differences(a,b,count);
  }
  
  /// Check if the pointer is aligned to the specified byte granularity
  template<int bytes> bool is_aligned(const void* ptr);
  template<> inline bool is_aligned<8>(const void* ptr) {   return ((reinterpret_cast<size_t>(ptr)) & 0x7) == 0;   }
  template<> inline bool is_aligned<16>(const void* ptr) {  return ((reinterpret_cast<size_t>(ptr)) & 0xF) == 0;   }

  /// Compute the number of pointer increments necessary to yield alignment of A bytes
  template<int A, class T> inline size_t steps_to_align(const T* ptr) 
  {
      return is_aligned<A>(ptr) ? 0 : (A-((reinterpret_cast<size_t>(ptr)) & (A-1)))/sizeof(T); 
  }

#if defined(CVD_HAVE_MMXEXT) && defined(CVD_HAVE_MMINTRIN)
  void differences(const byte* a, const byte* b, short* diff, unsigned int size);
  void differences(const short* a, const short* b, short* diff, unsigned int size);
#endif  


#if defined(CVD_HAVE_SSE) && defined(CVD_HAVE_XMMINTRIN)
  void differences(const float* a, const float* b, float* diff, unsigned int size);
  void add_multiple_of_sum(const float* a, const float* b, const float& c,  float* out, unsigned int count);
  void assign_multiple(const float* a, const float& c,  float* out, unsigned int count);
  double inner_product(const float* a, const float* b, unsigned int count);
  double sum_squared_differences(const float* a, const float* b, size_t count);
#endif

#if defined (CVD_HAVE_SSE2) && defined(CVD_HAVE_EMMINTRIN)
  void differences(const int32_t* a, const int32_t* b, int32_t* diff, unsigned int size);
  void differences(const double* a, const double* b, double* diff, unsigned int size);
  void add_multiple_of_sum(const double* a, const double* b, const float& c,  double* out, unsigned int count);
  void assign_multiple(const double* a, const double& c,  double* out, unsigned int count);
  double inner_product(const double* a, const double* b, unsigned int count);
  double sum_squared_differences(const double* a, const double* b, size_t count);
  long long sum_squared_differences(const byte* a, const byte* b, size_t count);
#else  
  inline long long sum_squared_differences(const byte* a, const byte* b, size_t count) {
      return SumSquaredDifferences<long long,int,byte>::sum_squared_differences(a,b,count);
  }
#endif 
  
}

#endif
