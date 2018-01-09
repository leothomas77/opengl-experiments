///////////////////////////////////////////////////////////////////////////////
/*! \file Vector.h
 *  \brief Framework Nemo, copyright Cyril Crassin 2003
 *  \author C. Crassin
 */
///////////////////////////////////////////////////////////////////////////////


#ifndef NemoGraphics_Vector_h
#define NemoGraphics_Vector_h

#include <cmath>
#include <string>
#include <iostream>

using namespace std;

#define myround(x) (x<0?ceilf((x)-0.5):floorf((x)+0.5))
#define myroundf(x) (x<0.0f?ceilf((x)-0.5f):floorf((x)+0.5f))

namespace NemoGraphics {

inline double log2(double N){
	return (log10(N)/log10(2.0));
}


template<class T>
class Vector2 {

public:

	T x, y;

	Vector2(void){}
	Vector2(T xx, T yy): x(xx), y(yy){}
	Vector2(T xx): x(xx), y(xx){}
	~Vector2(void){}

	Vector2(const Vector2<int> &v) : x((T)v.x), y((T)v.y){ }
	Vector2(const Vector2<float> &v) : x((T)v.x), y((T)v.y){ }
	Vector2(const Vector2<short> &v) : x((T)v.x), y((T)v.y){ }


	Vector2<T> operator-(const Vector2 &v2) const {
		Vector2 res;
		res.x=this->x-v2.x;
		res.y=this->y-v2.y;

		return res;
	}

	Vector2<T> operator+(const Vector2 &v2)const {
		Vector2 res;
		res.x=this->x+v2.x;
		res.y=this->y+v2.y;

		return res;
	}

	Vector2<T> operator/(const Vector2 &v2)const {
		Vector2 res;
		res.x=this->x/v2.x;
		res.y=this->y/v2.y;

		return res;
	}

	Vector2<T> operator*(const Vector2 &v2) const {
		Vector2 res;
		res.x=this->x*v2.x;
		res.y=this->y*v2.y;

		return res;
	}

	Vector2<T> operator%(const Vector2 &v2) const {
		Vector2 res;
		res.x=this->x%v2.x;
		res.y=this->y%v2.y;

		return res;
	}

	Vector2<T> operator/(T nb) const {
		Vector2 res;
		res.x=this->x/nb;
		res.y=this->y/nb;

		return res;
	}

	Vector2<T> operator%(T nb) const {
		Vector2 res;
		res.x=this->x%nb;
		res.y=this->y%nb;

		return res;
	}

	Vector2<T> operator+(T nb) const {
		Vector2 res;
		res.x=this->x+nb;
		res.y=this->y+nb;

		return res;
	}
	Vector2<T> operator-(T nb) const {
		Vector2 res;
		res.x=this->x-nb;
		res.y=this->y-nb;

		return res;
	}

	Vector2<T> operator*(T nb) const {
		Vector2 res;
		res.x=this->x*nb;
		res.y=this->y*nb;

		return res;
	}

	inline bool operator==(const Vector2<T> &v2) const{
		return x==v2.x && y==v2.y;
	}
	inline bool operator!=(const Vector2<T> &v2) const{
		return !(*this==v2);
	}

	friend Vector2<T> vceil(const Vector2<T> &v) {
		Vector2<T> res;
		res.x=ceil((double)v.x);
		res.y=ceil((double)v.y);
		return res;
	}

	friend Vector2<T> abs(const Vector2<T> &v) {
		Vector2<T> res;

		res.x=sqrt(pow(v.x, 2)); //abs( v.x);
		res.y=sqrt(pow(v.y, 2)); //abs( v.y);
		return res;
	}

	T squareDistance(const Vector2<T> &v) const {
		return ( pow(v.x-x, 2) + pow(v.y-y, 2) );
	}

	T distance(const Vector2<T> &v) const {
		return sqrt( pow(v.x-x, 2) + pow(v.y-y, 2) );
	}

	friend std::ostream &operator<<(std::ostream &s, const Vector2<T> &v){
		s<<"("<<v.x<<", "<<v.y<<")";
		return s;
	}
};

template<class T>
class Vector3 {

public:

	/*union {
		T val[3];
		T x, y, z;
	};*/

	T x, y, z;

	Vector3(void){}
	Vector3(T xx, T yy, T zz): x(xx), y(yy), z(zz){}
	Vector3(T xx): x(xx), y(xx), z(xx){}
	Vector3(const float *v) : x(v[0]), y(v[1]), z(v[2]) { }
	Vector3(float *v) : x(v[0]), y(v[1]), z(v[2]) { }
	~Vector3(void){}

	Vector3(const Vector3<int> &v) : x((T)v.x), y((T)v.y), z((T)v.z){ }
	Vector3(const Vector3<float> &v) : x((T)v.x), y((T)v.y), z((T)v.z){ }
	Vector3(const Vector3<short> &v) : x((T)v.x), y((T)v.y), z((T)v.z){ }

	std::string toString(){
		char buff[256];
		sprintf_s(buff, "Vector3(%d,%d,%d)", x, y, z);
		return std::string(buff);
	}

	Vector3<T> operator-(const Vector3<T> &v2) const {
		Vector3 res;
		res.x=this->x-v2.x;
		res.y=this->y-v2.y;
		res.z=this->z-v2.z;

		return res;
	}

	Vector3<T> operator/(int nb) const {
		Vector3 res;
		res.x=this->x/nb;
		res.y=this->y/nb;
		res.z=this->z/nb;

		return res;
	}

	Vector3<float> operator/(float nb) const {
		Vector3<float> res;
		res.x=this->x/nb;
		res.y=this->y/nb;
		res.z=this->z/nb;

		return res;
	}

	Vector3<T> operator+(const Vector3<T> &v2) const {
		Vector3 res;
		res.x=this->x+v2.x;
		res.y=this->y+v2.y;
		res.z=this->z+v2.z;

		return res;
	}

	Vector3<T> operator/(Vector3 &v2) const {
		Vector3 res;
		res.x=this->x/v2.x;
		res.y=this->y/v2.y;
		res.z=this->z/v2.z;

		return res;
	}

	Vector3<T> operator*(Vector3 &v2) const {
		Vector3 res;
		res.x=this->x*v2.x;
		res.y=this->y*v2.y;
		res.z=this->z*v2.z;

		return res;
	}


	Vector3<T> operator%(const Vector3 &v2) const {
		Vector3 res;
		res.x=this->x%v2.x;
		res.y=this->y%v2.y;
		res.z=this->z%v2.z;

		return res;
	}

	Vector3<T> operator+(T nb) const{
		Vector3 res;
		res.x=this->x+nb;
		res.y=this->y+nb;
		res.z=this->z+nb;

		return res;
	}

	Vector3<T> operator-(T nb) const{
		Vector3 res;
		res.x=this->x-nb;
		res.y=this->y-nb;
		res.z=this->z-nb;

		return res;
	}


	Vector3<T> operator*(T nb) const {
		Vector3 res;
		res.x=this->x*nb;
		res.y=this->y*nb;
		res.z=this->z*nb;

		return res;
	}

	T length(){
		return sqrt((float)x*x+y*y+z*z);
	}

	void clamp(const Vector3<T> &v0, const Vector3<T> &v1){
		if(x<v0.x)
			x=v0.x;
		if(y<v0.y)
			y=v0.y;
		if(z<v0.z)
			z=v0.z;


		if(x>v1.x)
			x=v1.x;
		if(y>v1.y)
			y=v1.y;
		if(z>v1.z)
			z=v1.z;

	}

	void normalize(){
		T l=this->length();
		x/=l;
		y/=l;
		z/=l;
	}


	Vector3<T> round(){
		Vector3<T> res;
		res.x=myround(x);
		res.y=myround(y);
		res.z=myround(z);

		return res;
	}

	friend Vector3<T> vceil(Vector3<T> &v){
		Vector3<T> res;
		res.x=ceilf((float)v.x);
		res.y=ceilf((float)v.y);
		res.z=ceilf((float)v.z);

		return res;
	}

	friend Vector3<T> exp(Vector3<T> &v){
		Vector3<T> res;
		res.x=expf((float)v.x);
		res.y=expf((float)v.y);
		res.z=expf((float)v.z);

		return res;
	}

	Vector3<T> abs() const{
		Vector3<T> res;
		res.x=x>0 ? x : 0-x;
		res.y=y>0 ? y : 0-y;
		res.z=z>0 ? z : 0-z;

		return res;
	}

	Vector3<T> floor() const{
		Vector3<T> res;
		res.x=myfloor(x);
		res.y=myfloor(y);
		res.z=myfloor(z);

		return res;
	}
	Vector3<T> ceil() const{
		Vector3<T> res;
		res.x=ceilf((float)x);
		res.y=ceilf((float)y);
		res.z=ceilf((float)z);

		return res;
	}

	T squareDistance(Vector3<T> &v){
		return ( pow(v.x-x, 2) + pow(v.y-y, 2) + pow(v.z-z, 2));
	}

	T distance(Vector3<T> &v){
		return sqrt( pow(v.x-x, 2) + pow(v.y-y, 2) + pow(v.z-z, 2));
	}

	//Produit vectoriel
	Vector3<T> cross(Vector3<T> vVector2) {
		Vector3<T> vCross;								// The vector to hold the cross product
													// Get the X value
		vCross.x = ((this->y * vVector2.z) - (this->z * vVector2.y));
													// Get the Y value
		vCross.y = ((this->z * vVector2.x) - (this->x * vVector2.z));
													// Get the Z value
		vCross.z = ((this->x * vVector2.y) - (this->y * vVector2.x));

		return vCross;								// Return the cross product
	}

	inline T &operator[](int i) { /*return val[i];*/ return ((T*)&x)[i]; }
	inline const T operator[](int i) const { /*return val[i];*/ return ((T*)&x)[i]; }

	inline void cross(const Vector3<T> &v1,const Vector3<T> &v2) {
		x = v1.y * v2.z - v1.z * v2.y;
		y = v1.z * v2.x - v1.x * v2.z;
		z = v1.x * v2.y - v1.y * v2.x;
	}

	inline const Vector3<T> operator-() const { return Vector3<T>(-x,-y,-z); }
	inline Vector3<T> operator-() { return Vector3<T>(-x,-y,-z); }

	inline bool operator<(const Vector3<T> &v2) const{
		return x<v2.x && y<v2.y && z<v2.z;
	}
	inline bool operator==(const Vector3<T> &v2) const{
		return x==v2.x && y==v2.y && z==v2.z;
	}
	inline bool operator!=(const Vector3<T> &v2) const{
		return !(*this==v2);
	}


	inline T maxComp() const{
		if(x>y)
			if(x>z)
				return x;
			else
				return z;
		else
			if(y>z)
				return y;
			else
				return z;
	}

	/*inline Vector3<T> &operator=(const Vector4<T> &v2) {
		x=v2.x; y=v2.y; z=v2.z;
		return *this;
	}*/


	inline void rotate(const Vector3<T> &v, double ang){
	   // First we calculate [w,x,y,z], the rotation quaternion
	   double w,x,y,z;
	   Vector3<T> V=v;
	   V.normalize();
	   w=cos(-ang/2);  // The formula rotates counterclockwise, and I
					   // prefer clockwise, so I change 'ang' sign
	   double s=sin(-ang/2);
	   x=V.x*s;
	   y=V.y*s;
	   z=V.z*s;
	   // now we calculate [w^2, x^2, y^2, z^2]; we need it
	   double w2=w*w;
	   double x2=x*x;
	   double y2=y*y;
	   double z2=z*z;
	   
	   // And apply the formula
	   Vector3<T> res=Vector3<T>((*this).x*(w2+x2-y2-z2) + (*this).y*2*(x*y+w*z)   + (*this).z*2*(x*z-w*y),
			   (*this).x*2*(x*y-w*z)   + (*this).y*(w2-x2+y2-z2) + (*this).z*2*(y*z+w*x),
			   (*this).x*2*(x*z+w*y)   + (*this).y*2*(y*z-w*x)   + (*this).z*(w2-x2-y2+z2));

	   (*this)=res;
	   
	 }

	inline void translate(const Vector3<T> &v){
		this->x+=v.x;
		this->y+=v.y;
		this->z+=v.z;
	}


	Vector3<T> lerp(const Vector3<T> &v2, float a) const{
		return (*this)*a + v2*(1.0f-a);
	}

	Vector3<T> step(Vector3<T> edge){
		Vector3<T> res;

		if(x<edge.x)
			res.x=0;
		else
			res.x=1;

		if(y<edge.y)
			res.y=0;
		else
			res.y=1;

		if(z<edge.z)
			res.z=0;
		else
			res.z=1;

		return res;

	}


	//! Convert cartesian coordinates to spherical coordinates (Phi, Theta, r).
	Vector3<T> sphericalCoords(){
		Vector3<T> res;
		res.z=this->length(); //radius

		res.x=(T)atanf(float(y)/float(x));//Phi
		res.y=(T)acosf(float(z)/float(res.z));//Theta

		return res;
	}
	//! Convert spherical coordinates to cartesian coordinates.
	Vector3<T> cartesianCoords(){
		Vector3<T> res;
		
		res.x=z*sinf(y)*cosf(x);
		res.y=z*sinf(y)*sinf(x);
		res.z=z*cosf(y);

		return res;
	}


	Vector3<T> getMipMapSize(int n, int div=2){
		Vector3<T> res=(*this)/(T)powf((float)div, (float)n);
		return res.ceil();
	}

	int getNumMipMapLevels(int div){
		int maxcomp=0;
		T maxv=0;
		for(int i=0; i<3; i++){
			if(this->operator [](i)>maxv){
				maxcomp=i;
				maxv=this->operator [](i);
			}
		}

		int num=(int)( logf((float)(maxv))/logf((float)(div)) )+1;

		return num; 
	}

	Vector2<T> xy() const{
		return Vector2<T>(x, y);
	}

	///////Casts
	Vector3<float> toFloat() const {
		return Vector3<float>(x, y, z);
	}

	Vector3<int> toInt() const {
		return Vector3<int>((int)x, (int)y, (int)z);
	}

	friend std::ostream &operator<<(std::ostream &s, const Vector3<T> &v){
		s<<"("<<v.x<<", "<<v.y<<", "<<v.z<<")";
		return s;
	}
};

template<class T>
class Vector4 {

public:

	T x;
	T y;
	T z;
	T w;

	Vector4(void){}
	Vector4(T xx, T yy, T zz, T ww): x(xx), y(yy), z(zz), w(ww){}
	Vector4(T xx): x(xx), y(xx), z(xx), w(xx){}
	Vector4(const float *v) : x(v[0]), y(v[1]), z(v[2]), w(v[3]) { }
	Vector4(float *v) : x(v[0]), y(v[1]), z(v[2]), w(v[3]) { }

	~Vector4(void){}

	Vector4(const Vector3<T> &v) : x(v.x), y(v.y), z(v.z), w(0){ }
	Vector4(const Vector3<T> &v, T s) : x(v.x), y(v.y), z(v.z), w(s){ }


	Vector4<T> operator-(Vector4<T> &v2) const {
		Vector4 res;
		res.x=this->x-v2.x;
		res.y=this->y-v2.y;
		res.z=this->z-v2.z;

		res.w=this->w-v2.w;

		return res;
	}

	Vector4<T> operator/(T nb) const{
		Vector4 res;
		res.x=this->x/nb;
		res.y=this->y/nb;
		res.z=this->z/nb;

		res.w=this->w/nb;
		return res;
	}

	Vector4<T> operator+(Vector4<T> &v2) const{
		Vector4 res;
		res.x=this->x+v2.x;
		res.y=this->y+v2.y;
		res.z=this->z+v2.z;

		res.w=this->w+v2.w;
		return res;
	}

	Vector4<T> &operator+=(Vector4<T> &v2) {
		
		this->x=this->x+v2.x;
		this->y=this->y+v2.y;
		this->z=this->z+v2.z;
		this->w=this->w+v2.w;

		return *this;
	}


	Vector4<T> operator/(Vector4 &v2) const{
		Vector4 res;
		res.x=this->x/v2.x;
		res.y=this->y/v2.y;
		res.z=this->z/v2.z;

		res.w=this->w/v2.w;
		return res;
	}


	Vector4<T> operator+(T nb) const{
		Vector4 res;
		res.x=this->x+nb;
		res.y=this->y+nb;
		res.z=this->z+nb;

		res.w=this->w+nb;
		return res;
	}


	Vector4<T> operator-(T nb) const{
		Vector4 res;
		res.x=this->x-nb;
		res.y=this->y-nb;
		res.z=this->z-nb;

		res.w=this->w-nb;
		return res;
	}


	Vector4<T> operator*(T nb) const{
		Vector4<T> res;
		res.x=this->x*nb;
		res.y=this->y*nb;
		res.z=this->z*nb;

		res.w=this->w*nb;
		return res;
	}

	inline const Vector4<T> operator-() const { return Vector4<T>(-x,-y,-z, -w); }

	T length() const{
		return sqrt(x*x+y*y+z*z + w*w);
	}

	void normalize(){
		T l=this->length();
		x/=l;
		y/=l;
		z/=l;
		w/=l;
	}

	void normalizePlane() {
		float mag;
		mag = sqrtf((float)(x * x + y * y + z * z));
		x = x / mag;
		y = y / mag;
		z = z / mag;
		w = w / mag;
	}

	Vector4<T> planeIntersect(Vector4<T> &p2, Vector4<T> &p3) {
		Vector4<T> res;
		Vector4<T> p1=*this;
		res.w=1;

		res.x=-(p1.y*(p2.z*p3.w-p3.z*p2.w)-p2.y*(p1.z*p3.w-p3.z*p1.w)+p3.y*(p1.z*p2.w-p2.z*p1.w))/
			(p1.x*(p2.y*p3.z-p3.y*p2.z)-p2.x*(p1.y*p3.z-p3.y*p1.z)+p3.x*(p1.y*p2.z-p2.y*p1.z));
		res.y=(p1.x*(p2.z*p3.w-p3.z*p2.w)-p2.x*(p1.z*p3.w-p3.z*p1.w)+p3.x*(p1.z*p2.w-p2.z*p1.w))/
			(p1.x*(p2.y*p3.z-p3.y*p2.z)-p2.x*(p1.y*p3.z-p3.y*p1.z)+p3.x*(p1.y*p2.z-p2.y*p1.z));
		res.z=-(p1.x*(p2.y*p3.w-p3.y*p2.w)-p2.x*(p1.y*p3.w-p3.y*p1.w)+p3.x*(p1.y*p2.w-p2.y*p1.w))/
			( p1.x*(p2.y*p3.z-p3.y*p2.z)-p2.x*(p1.y*p3.z-p3.y*p1.z)+p3.x*(p1.y*p2.z-p2.y*p1.z) );
		return res;
	}


	friend Vector4<T> vceil(Vector4<T> v){
		Vector4<T> res;
		res.x=ceil((double)v.x);
		res.y=ceil((double)v.y);
		res.z=ceil((double)v.z);

		res.w=ceil((double)v.w);
		return res;
	}

	T squareDistance(Vector4<T> v){
		return ( pow(v.x-x, 2) + pow(v.y-y, 2) + pow(v.z-z, 2)  + pow(v.w-w, 2));
	}

	T distance(Vector4<T> v){
		return sqrt( pow(v.x-x, 2) + pow(v.y-y, 2) + pow(v.z-z, 2)  + pow(v.w-w, 2));
	}

	//Produit vectoriel
	Vector4<T> cross(Vector4<T> vVector2) {
		Vector4<T> vCross;								// The vector to hold the cross product
													// Get the X value
		vCross.x = ((this->y * vVector2.z) - (this->z * vVector2.y));
													// Get the Y value
		vCross.y = ((this->z * vVector2.w) - (this->x * vVector2.w));
													// Get the Z value
		vCross.z = ((this->x * vVector2.y) - (this->y * vVector2.x));

		vCross.z = ((this->w * vVector2.x) - (this->w * vVector2.z));

		return vCross;								// Return the cross product
	}

	inline T &operator[](int i) { return ((T*)&x)[i]; }
	inline const T operator[](int i) const { return ((T*)&x)[i]; }

	Vector3<T> xyz() const{
		//return *(Vector3<T>*)((this));
		return Vector3<T>(x, y, z);
	}

	Vector4<T> lerp(const Vector4<T> &v2, float a) const{
		return (*this)*(1-a) + v2*(a);
	}


	friend std::ostream &operator<<(std::ostream &s, const Vector4<T> &v){
		s<<"("<<v.x<<", "<<v.y<<", "<<v.z<<", "<<v.w<<")";
		return s;
	}

};

typedef Vector2<int> Vector2i;
typedef Vector2<float> Vector2f;

typedef Vector3<int> Vector3i;
typedef Vector4<int> Vector4i;

typedef Vector3<float> Vector3f;
typedef Vector4<float> Vector4f;

typedef Vector3<short> Vector3s;
typedef Vector4<short> Vector4s;

typedef Vector3<unsigned char> Vector3uc;
typedef Vector4<unsigned char> Vector4uc;


typedef Vector3<float> CVector3;

};

#endif
