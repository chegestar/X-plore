#ifndef __C_BEZIER_HPP
#define __C_BEZIER_HPP

/*----------------------------------------------------------------\
   Template bezier curve class, copyright (c) 1998 Michal Bacik

Abstract:

 Bezier curve template, holding info about all objects describing
 bezier curve and computing an object on arbitrary position.

Functions:
   void Init(const T[4]);
   void Init(const T&, const T&, const T&, const T&);
      - initialize class
   void Evaluate(float t, T&);
      - evaluate vector at position t (t is in range 0.0 to 1.0)

Notes:
 Supports any classes for which following functions are defined:

 T& operator * (float f) const;
 T& operator + (const T&) const;
|\----------------------------------------------------------------*/


template<class T>
class C_bezier_curve{
   T p[4];
public:
//----------------------------
// Initialize 4 points of curve, from an array.
   void Init(const T p1[4]){
      for(int i=0; i<4; i++) p[i] = p1[i];
   }

//----------------------------
// Initialize 4 points of curve, by 4 values.
   void Init(const T &p0, const T &p1, const T &p2, const T &p3){
      p[0] = p0, p[1] = p1, p[2] = p2, p[3] = p3;
   }

//----------------------------
// Get access to control points.
   T &operator [](int i){ return p[i]; }
   const T &operator [](int i) const{ return p[i]; }

//----------------------------
// Evaluate point on curve. The value 't' is normalized position on curve (0 .. 1),
// 'p_ret' is value which contains result after return.
   void Evaluate(C_fixed t, T &p_ret) const{

      C_fixed t_sqr = t*t, t_inv = C_fixed::One() - t, t_inv_sqr = t_inv * t_inv;
      C_fixed val0 = t_inv * t_inv_sqr,
         val1 = C_fixed::Three() * t * t_inv_sqr,
         val2 = C_fixed::Three() * t_inv * t_sqr,
         val3 = t * t_sqr;
      p_ret =
         p[0] * val0 +
         p[1] * val1 +
         p[2] * val2 +
         p[3] * val3;
   }
};

//----------------------------

class C_ease_interpolator: protected C_bezier_curve<C_fixed>{
public:
//----------------------------
//Default constructor - sets maximal interpolation
//for both 'from' and 'to' ends.
   inline C_ease_interpolator(){
      Setup(C_fixed(1), C_fixed(1));
   }
   inline C_ease_interpolator(C_fixed in, C_fixed out){
      Setup(in, out);
   }

//----------------------------
//Setup of interpolator - sets 'from' and 'to'
// ends. Value 0.0f means linear interpolation,
// value 1.0f means lin. acceleration interpolation.
   inline void Setup(C_fixed ip, C_fixed op){
      C_bezier_curve<C_fixed>::Init(C_fixed::Zero(), C_fixed::Third() * (C_fixed::One() - ip), C_fixed::One() - C_fixed::Third() * (C_fixed::One() - op), C_fixed::One());
   }

//----------------------------
//Getting interpolated value on time t. 't' must be in range 0.0f to 1.0f.
   inline C_fixed operator[](C_fixed t) const{
      C_fixed ret;
      C_bezier_curve<C_fixed>::Evaluate(t, ret);
      return ret;
   }
};

//----------------------------

#endif
