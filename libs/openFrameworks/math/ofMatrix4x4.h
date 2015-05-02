/*
 *  ofMatrix4x4.h
 *  
 *  Created by Aaron Meyers on 6/22/09 -- modified by Arturo Castro, Zach Lieberman, Memo Akten
 *  based on code from OSG - 
 *  see OSG license for more details: 
 *  http://www.openscenegraph.org/projects/osg/wiki/Legal
 * 
 */

#pragma once

#include "ofVec3f.h"
#include "ofVec4f.h"
#include "ofQuaternion.h"
#include "ofConstants.h"
#include <cmath>

/// \brief The ofMatrix4x4 is the big class of the math part of openFrameworks. You'll
/// sometimes see it used for doing things like setting where the camera in OepnGL
/// (the mathematically calculated one, not the ofCamera one) is looking or is
/// pointedA, or figuring how to position something in 3d space, doing scaling,
/// etc. The great thing about the 4x4 matrix is that it can do all these things
/// at the same time. A single ofMatrix4x4 can represent a ton of different
/// information about a stuff that goes on in doing 3d programming: where an
/// object is, how you want to scale an object, where a camera is. Let's look at a
/// few really basic examples:
/// 
/// ![MATS](math/mats.png)
/// 
/// Not particularly exciting, but you can see how they'd be useful. Luckily most
/// of the need to transform, rotate, scale, shear, or further bazzlemunge (just
/// kidding, bazzlemunging is not a thing) stuff in oF is handled internally by
/// objects like ofNode or ofCamera.
/// 
/// oF uses row-vector style by default, meaning that when transforming a vector
/// by multiplying with a matrix, you should put the vector on the left side and
/// the matrix (or matrices) to its right. When multiplying by multiple matrices,
/// the order of application of the transforms is left-to-right. This means that
/// the standard order of manipulation operations is vector * scale * rotate * translate.
/// 
/// Note that in GLSL, this convention is reversed, and column-vector style is used.
/// oF uploads the matrices correctly, but you should reverse the order of your manipulations
/// to, e.g. translate * rotate * scale * vector.
/// 
/// oF still lets you do matrix-vector multiplication with the vector
/// on the right if that's your preferred style. You can perform matrix transformations
/// that use the OpenGL style by using functions like glTranslate, glRotate, and glScale.
/// 
class ofMatrix4x4 {
public:
//	float _mat[4][4];
	
	/// \cond INTERNAL
	// Should this be moved to private?
	// The values of the matrix, stored in row-major style
	ofVec4f _mat[4];
	/// \endcond



	//---------------------
	/// \name Construct a matrix
	/// \{

	// \brief The default constructor provides an identity matrix 
	ofMatrix4x4() {
		makeIdentityMatrix();
	}

	// \brief You can pass another ofMatrix4x4 to create a copy
	ofMatrix4x4( const ofMatrix4x4& mat) {
		set(mat.getPtr());
	}

	// \brief You can pass a pointer to floats, and the first 16 contents will be extracted into this matrix.
	// Note: the validity of these values is not checked!
	ofMatrix4x4( float const * const ptr ) {
		set(ptr);
	}

	// \brief Rotation matrices can be constructed from a quaternion
	ofMatrix4x4( const ofQuaternion& quat ) {
		makeRotationMatrix(quat);
	}

	// \brief You can pass all 16 values of the matrix as positional arguments (in row-major order)
	ofMatrix4x4(	float a00, float a01, float a02, float a03,
	              float a10, float a11, float a12, float a13,
	              float a20, float a21, float a22, float a23,
	              float a30, float a31, float a32, float a33);

	// \brief destructor
	~ofMatrix4x4() {}

	// Init matrix as identity, scale, translation...
	// all make* methods delete the current data
	
	// \brief Matrix becomes the identity matrix
	void makeIdentityMatrix();

	// \brief Matrix becomes a scale matrix with values only in the diagonal.
	// Accepts x, y, z scale values as a vector or separately
	void makeScaleMatrix( const ofVec3f& );
	void makeScaleMatrix( float, float, float );

	// \brief Matrix becomes a translation matrix with values only in the fourth column.
	// Accepts x, y, z translation values as a vector or separately
	void makeTranslationMatrix( const ofVec3f& );
	void makeTranslationMatrix( float, float, float );

	// \brief Matrix becomes a rotation matrix with values in the first three rows and columns.
	// \brief Matrix becomes a rotation from the first vector direction to the second
	void makeRotationMatrix( const ofVec3f& from, const ofVec3f& to );
	// \brief Matrix becomes a rotation of angle (degrees) around the vector axis.
	void makeRotationMatrix( float angle, const ofVec3f& axis );
	// \brief Matrix becomes a rotation of angle (degrees) around the axis specified positionally
	void makeRotationMatrix( float angle, float x, float y, float z );
	// \brief Matrix becomes a rotation that matches the quaternion's orientation.
	void makeRotationMatrix( const ofQuaternion& );
	// \brief Matrix becomes a rotation that is the result of computing a rotation
	// for each of the provided axes, in order of parameters.
	void makeRotationMatrix( float angle1, const ofVec3f& axis1,
	                 float angle2, const ofVec3f& axis2,
	                 float angle3, const ofVec3f& axis3);


	// init related to another matrix
	// \brief Matrix becomes the inverse of the provided matrix.
	bool makeInvertOf( const ofMatrix4x4& rhs);
	// \brief Matrix becomes an orthonormalized version of the provided matrix.
	// The basis vectors (the 3x3 chunk embedded in the upper left of the matrix)
	// are normalized and it is ensured that they are orthogonal (perpendicular)
	// to each other. This means the resulting matrix will only create rotations,
	// and "removes" any scaling effect.
	void makeOrthoNormalOf(const ofMatrix4x4& rhs);
	// \brief Matrix becomes the result of the multiplication of two other matrices
	void makeFromMultiplicationOf( const ofMatrix4x4&, const ofMatrix4x4& );
	
	//---------------------------------------------
	// init as opengl related matrix for perspective settings
	// see opengl docs of the related funciton for further details

	// glOrtho
	// \brief Matrix becomes an orthographic projection matrix with a
	// box-shaped viewing volume described by the six parameters.
	// left, right, bottom, and top essentially specify coordinates in
	// the zNear clipping plane.
	void makeOrthoMatrix(double left,   double right,
	               double bottom, double top,
	               double zNear,  double zFar);

	// glOrtho2D
	// \brief Matrix becomes a 2D orthographic projection matrix with a 
	// box-shaped viewing volume described by the four parameters
	// plus, implicitly, a zNear of -1 and a zFar of 1
	void makeOrtho2DMatrix(double left,   double right,
	                        double bottom, double top);

	// glFrustum
	// \brief Matrix becomes a perspective projection matrix with a frustum-shaped
	// viewing volume defined by the six parameters.
	// left, right, top, and bottom specify coordinates in the zNear
	// clipping plane, and the zNear and zFar parameters define the distances
	// to the edges of the view volume. Note that the resulting volume can be
	// vertically and horizontally asymmetrical around the center of the near plane.
	void makeFrustumMatrix(double left,   double right,
	                 double bottom, double top,
	                 double zNear,  double zFar);

	// gluPerspective
	// Aspect ratio is defined as width/height.
	// Matrix becomes a perspective projection matrix with a frustum-shaped
	// viewing volume defined by the four parameters. The fovy and aspect ratio
	// are used to compute the positions of the left, right, top, and bottom sides
	// of the viewing volume in the zNear plane. Note that the resulting volume is
	// both vertically and horizontally symmetrical around the center of the near plane.
	void makePerspectiveMatrix(double fovy,  double aspectRatio,
						 double zNear, double zFar);


	// makeLookAtMatrix:
	// creates a transformation matrix positioned at 'eye'
	// pointing at (along z axis) 'center'
	// this is what you use if you want an object to look at a point
	// Matrix becomes a combination of a translation to 'eye' and a rotation matrix which
	// orients an object to look towards 'center' along its z-axis.
	// \cond INTERNAL:
	// Does this orient the object along its positive or its negative z-axis?
	// \endcond
	void makeLookAtMatrix(const ofVec3f& eye, const ofVec3f& center, const ofVec3f& up);
	
	
	// makeLookAtViewMatrix:
	// creates *the inverse of* a transformation matrix positioned at 'eye'
	// pointing at (along z axis) 'center'
	// this is what you use when you want your view matrix looking at a point
	// (the inverse of makeLookAtMatrix), same as gluLookAt
	// This matrix will also cause a translation equal to -eye.
	void makeLookAtViewMatrix(const ofVec3f& eye, const ofVec3f& center, const ofVec3f& up);

	// static utility functions to create new matrices. These functions are, generally,
	// the equivalent of declaring a matrix, calling the corresponding "make..." function on it,
	// and returning it.
	inline static ofMatrix4x4 newIdentityMatrix( void );
	inline static ofMatrix4x4 newScaleMatrix( const ofVec3f& sv);
	inline static ofMatrix4x4 newScaleMatrix( float sx, float sy, float sz);
	inline static ofMatrix4x4 newTranslationMatrix( const ofVec3f& dv);
	inline static ofMatrix4x4 newTranslationMatrix( float x, float y, float z);
	inline static ofMatrix4x4 newRotationMatrix( const ofVec3f& from, const ofVec3f& to);
	inline static ofMatrix4x4 newRotationMatrix( float angle, float x, float y, float z);
	inline static ofMatrix4x4 newRotationMatrix( float angle, const ofVec3f& axis);
	inline static ofMatrix4x4 newRotationMatrix( float angle1, const ofVec3f& axis1,
	                                   float angle2, const ofVec3f& axis2,
	                                   float angle3, const ofVec3f& axis3);
	inline static ofMatrix4x4 newRotationMatrix( const ofQuaternion& quat);

	// create new matrices related to glFunctions. See the description of the make* methods for more info.

	// glOrtho
	inline static ofMatrix4x4 newOrthoMatrix(double left,   double right,
	                                 double bottom, double top,
	                                 double zNear,  double zFar);

	// glOrtho2D
	inline static ofMatrix4x4 newOrtho2DMatrix(double left,   double right,
	                                   double bottom, double top);

	// glFrustum
	inline static ofMatrix4x4 newFrustumMatrix(double left,   double right,
	                                   double bottom, double top,
	                                   double zNear,  double zFar);

	// gluPerspective
	inline static ofMatrix4x4 newPerspectiveMatrix(double fovy,  double aspectRatio,
	                                       double zNear, double zFar);

	// gluLookAt
	inline static ofMatrix4x4 newLookAtMatrix(const ofVec3f& eye,
	                                  const ofVec3f& center,
	                                  const ofVec3f& up);

	//---------------------
	/// \name Accessors
	/// \{

	/// \brief Write data with `matrix(row,col)=number`
	float& operator()(int row, int col) {
		return _mat[row][col];
	}

	// \brief Read data with `matrix(row, col)`
	float operator()(int row, int col) const {
		return _mat[row][col];
	}

	// \brief returns a copy of row i
	ofVec3f getRowAsVec3f(int i) const {
		return ofVec3f(_mat[i][0], _mat[i][1], _mat[i][2]);
	}
	
	// \brief returns a copy of row i
	ofVec4f getRowAsVec4f(int i) const {
		return _mat[i];
	}
	
	/// \cond INTERNAL
	friend ostream& operator<<(ostream& os, const ofMatrix4x4& M);
	friend istream& operator>>(istream& is, ofMatrix4x4& M);
	/// \endcond
	
	// \brief Access the internal data in `float*` format
	// useful for opengl matrix transformations
	float * getPtr() {
		return (float*)_mat;
	}
	const float * getPtr() const {
		return (const float *)_mat;
	}
	
	/// \}

	//---------------------
	/// \name Checking
	/// \{

	
	/// \brief Check if the matrix is valid
	bool isValid() const {
		return !isNaN();
	}

	bool isNaN() const;

	/// \brief Check matrix identity
	bool isIdentity() const;

	/// \}

	//---------------------
	/// \name Setters
	/// \{

	/// \brief Copy a matrix using `=` operator
	ofMatrix4x4& operator = (const ofMatrix4x4& rhs);

	
	/// \brief Set the data of the matrix
	/// These functions are analogous to the corresponding constructors.
	void set(const ofMatrix4x4& rhs);
	void set(float const * const ptr);
	void set(double const * const ptr);
	void set(float a00, float a01, float a02, float a03,
	         float a10, float a11, float a12, float a13,
	         float a20, float a21, float a22, float a23,
	         float a30, float a31, float a32, float a33);

	/// \}

	//---------------------
	/// \name Getters
	/// \{

	/// \brief Get the inverse matrix
	ofMatrix4x4 getInverse() const;


	/// \brief Get the perspective components from a matrix
	/// this only works with pure perspective projection matrices
	bool getOrtho(double& left,   double& right,
	              double& bottom, double& top,
	              double& zNear,  double& zFar) const;

	
	bool getFrustum(double& left,   double& right,
	                double& bottom, double& top,
	                double& zNear,  double& zFar) const;

	/// \brief Get the frustum settings of a symmetric perspective projection
	/// matrix.
	/// 
	/// Note, if matrix is not a symmetric perspective matrix then the
	/// shear will be lost.
	/// Asymmetric matrices occur when stereo, power walls, caves and
	/// reality center display are used.
	/// In these configuration one should use the getFrustum method instead.
	///
	/// \returns false if matrix is not a perspective matrix,
	/// where parameter values are undefined.
	bool getPerspective(double& fovy,  double& aspectRatio,
	                    double& zNear, double& zFar) const;

	// will only work for modelview matrices
	void getLookAt(ofVec3f& eye, ofVec3f& center, ofVec3f& up,
	               float lookDistance = 1.0f) const;



	/// \brief Decompose the matrix into translation, rotation,
	/// scale and scale orientation.
	void decompose( ofVec3f& translation,
					ofQuaternion& rotation,
					ofVec3f& scale,
					ofQuaternion& so ) const;

	// create new matrices as transformation of another
	inline static ofMatrix4x4 getInverseOf( const ofMatrix4x4& matrix);
	inline static ofMatrix4x4 getTransposedOf( const ofMatrix4x4& matrix);
	inline static ofMatrix4x4 getOrthoNormalOf(const ofMatrix4x4& matrix);


	/// \}

	//---------------------
	/// \name Matrix multiplication
	/// \{

	/// \brief Vector multiplication
	/// although opengl uses premultiplication
	/// because of the way matrices are used in opengl:
	///
	/// ofMatrix4x4:
	/// 
	/// |   |   |   |   |
	/// |:-:|:-:|:-:|:-:|
	/// | 0 | 1 | 2 | 3 |
	/// | 4 | 5 | 6 | 7 |
	/// | 8 | 9 | 10| 11|
	/// | 12| 13| 14| 15|
	///
	/// 
	/// openGL:
	///
	/// |   |   |   |   |
	/// |:-:|:-:|:-:|:-:|
	/// | 0 | 4 | 8 | 12|
	/// | 1 | 5 | 9 | 13|
	/// | 2 | 6 | 10| 14|
	/// | 3 | 7 | 11| 15|
	///
	/// in memory though both are layed in the same way
	/// so when uploading a matrix it just works without
	/// needing to transpose
	///
	/// so although opengl docs explain transformations
	/// like:
	///
	/// ~~~~~{.cpp}
	/// ofVec3f c = rotateZ 30º ofVec3f a around ofVec3f b
	/// ~~~~~
	///
	/// openGL docs says: `c = T(b)*R(30)*a`
	///
	/// with ofMatrix4x4:
	/// ~~~~~{.cpp}
	/// ofMatrix4x4 R = ofMatrix4x4::newRotationMatrix(30,0,0,1);
	/// ofMatrix4x4 T = ofMatrix4x4::newTranlationMatrix(b);
	/// ofVec3f c = a*R*T;
	/// ~~~~~
	/// where `*` is calling preMult()
	/// 

	/// \brief Matrix * vector multiplication. This operation
	/// implicitly treat vectors as column-matrices
	inline ofVec3f postMult( const ofVec3f& v ) const;
	inline ofVec4f postMult( const ofVec4f& v ) const;

	/// \brief post-multiply by another matrix. This matrix becomes `this * other`
	void postMult( const ofMatrix4x4& );

	// \brief Vector * matrix multiplication. This
	// operation implicitly treats vectors as row-matrices
	inline ofVec3f preMult( const ofVec3f& v ) const;
	inline ofVec4f preMult( const ofVec4f& v ) const;

	// \brief pre-multiply by another matrix. This matrix becomes `other * this`
	void preMult( const ofMatrix4x4& );

	// \brief Equivalent to calling postMult(other) but you can do
	// someMatrix *= someMatrix without breaking const-correctness
	inline void operator *= ( const ofMatrix4x4& other ) {
		if ( this == &other ) {
			ofMatrix4x4 temp(other);
			postMult( temp );
		} else postMult( other );
	}

	// \brief creates a new matrix from the product of two matrices
	inline ofMatrix4x4 operator * ( const ofMatrix4x4 &m ) const {
		ofMatrix4x4 r;
		r.makeFromMultiplicationOf(*this, m);
		return  r;
	}
	

	/// \brief Matrix * vector multiplication. Calls postMult()
	inline ofVec3f operator* (const ofVec3f& v) const {
		return postMult(v);
	}
	
	/// \brief Matrix * vector multiplication. Calls postMult()
	inline ofVec4f operator* (const ofVec4f& v) const {
		return postMult(v);
	}


	//---------------------------------------------
	// specialized postMult methods, usually you want to use this
	// for transforming ofVec not preMult
	// equivalent to postMult(newTranslationMatrix(v)); */
	inline void postMultTranslate( const ofVec3f& v );
	// equivalent to postMult(scale(v));
	inline void postMultScale( const ofVec3f& v );
	// equivalent to postMult(newRotationMatrix(q));
	inline void postMultRotate( const ofQuaternion& q );

	// AARON METHODS
	inline void postMultTranslate(float x, float y, float z);
	inline void postMultRotate(float angle, float x, float y, float z);
	inline void postMultScale(float x, float y, float z);


	//---------------------------------------------
	// equivalent to preMult(newScaleMatrix(v));
	inline void preMultScale( const ofVec3f& v );
	// equivalent to preMult(newTranslationMatrix(v));
	inline void preMultTranslate( const ofVec3f& v );
	// equivalent to preMult(newRotationMatrix(q));
	inline void preMultRotate( const ofQuaternion& q );


	/// \}

	//---------------------
	/// \name Matrix transformation
	/// \{

	// set methods: all these alter the components
	// deleting the previous data only in that components
	void setRotate(const ofQuaternion& q);
	void setTranslation( float tx, float ty, float tz );
	void setTranslation( const ofVec3f& v );

	// all these apply the transformations over the
	// current one, it's actually postMult... and behaves
	// the opposite to the equivalent gl functions
	// glTranslate + glRotate == rotate + translate
	void rotate(float angle, float x, float y, float z);
	void rotateRad(float angle, float x, float y, float z);
	void rotate(const ofQuaternion& q);
	void translate( float tx, float ty, float tz );
	void translate( const ofVec3f& v );
	void scale(float x, float y, float z);
	void scale( const ofVec3f& v );

	// all these apply the transformations over the
	// current one, it's actually preMult... and behaves
	// the the same the equivalent gl functions
	void glRotate(float angle, float x, float y, float z);
	void glRotateRad(float angle, float x, float y, float z);
	void glRotate(const ofQuaternion& q);
	void glTranslate( float tx, float ty, float tz );
	void glTranslate( const ofVec3f& v );
	void glScale(float x, float y, float z);
	void glScale( const ofVec3f& v );

	//---------------------------------------------
	// get methods: return matrix components
	// rotation and scale can only be used if the matrix
	// only has rotation or scale.
	// for matrices with both use decompose instead.
	ofQuaternion getRotate() const;
	ofVec3f getTranslation() const;
	ofVec3f getScale() const;


	//---------------------------------------------
	// apply a 3x3 transform of v*M[0..2,0..2].
	inline static ofVec3f transform3x3(const ofVec3f& v, const ofMatrix4x4& m);

	// apply a 3x3 transform of M[0..2,0..2]*v.
	inline static ofVec3f transform3x3(const ofMatrix4x4& m, const ofVec3f& v);


	/// \}
	
};

/// \cond INTERNAL

//--------------------------------------------------
// implementation of inline methods

inline bool ofMatrix4x4::isNaN() const {
	
#if (_MSC_VER) || defined (TARGET_ANDROID)
#ifndef isnan
#define isnan(a) ((a) != (a))
#endif

return isnan(_mat[0][0]) || isnan(_mat[0][1]) || isnan(_mat[0][2]) || isnan(_mat[0][3]) ||
	       isnan(_mat[1][0]) || isnan(_mat[1][1]) || isnan(_mat[1][2]) || isnan(_mat[1][3]) ||
	       isnan(_mat[2][0]) || isnan(_mat[2][1]) || isnan(_mat[2][2]) || isnan(_mat[2][3]) ||
	       isnan(_mat[3][0]) || isnan(_mat[3][1]) || isnan(_mat[3][2]) || isnan(_mat[3][3]);

#else
return std::isnan(_mat[0][0]) || std::isnan(_mat[0][1]) || std::isnan(_mat[0][2]) || std::isnan(_mat[0][3]) ||
	       std::isnan(_mat[1][0]) || std::isnan(_mat[1][1]) || std::isnan(_mat[1][2]) || std::isnan(_mat[1][3]) ||
	       std::isnan(_mat[2][0]) || std::isnan(_mat[2][1]) || std::isnan(_mat[2][2]) || std::isnan(_mat[2][3]) ||
	       std::isnan(_mat[3][0]) || std::isnan(_mat[3][1]) || std::isnan(_mat[3][2]) || std::isnan(_mat[3][3]);

#endif
	
}



inline ostream& operator<<(ostream& os, const ofMatrix4x4& M) {
	int w = 8;
	os	<< setw(w)
		<< M._mat[0][0] << ", " << setw(w)
		<< M._mat[0][1] << ", " << setw(w)
		<< M._mat[0][2] << ", " << setw(w) 
		<< M._mat[0][3] << std::endl;
		
	os	<< setw(w)
		<< M._mat[1][0] << ", " << setw(w) 
		<< M._mat[1][1] << ", " << setw(w)
		<< M._mat[1][2] << ", " << setw(w) 
		<< M._mat[1][3] << std::endl;
	
	os	<< setw(w)
		<< M._mat[2][0] << ", " << setw(w) 
		<< M._mat[2][1] << ", " << setw(w)
		<< M._mat[2][2] << ", " << setw(w) 
		<< M._mat[2][3] << std::endl;
	
	os	<< setw(w)
		<< M._mat[3][0] << ", " << setw(w) 
		<< M._mat[3][1] << ", " << setw(w)
		<< M._mat[3][2] << ", " << setw(w) 
		<< M._mat[3][3];
	
	return os;
}

inline istream& operator>>(istream& is, ofMatrix4x4& M) {
	is >> M._mat[0][0]; is.ignore(2); 
	is >> M._mat[0][1]; is.ignore(2);
	is >> M._mat[0][2]; is.ignore(2);
	is >> M._mat[0][3]; is.ignore(1);
	
	is >> M._mat[1][0]; is.ignore(2); 
	is >> M._mat[1][1]; is.ignore(2);
	is >> M._mat[1][2]; is.ignore(2);
	is >> M._mat[1][3]; is.ignore(1);
	
	is >> M._mat[2][0]; is.ignore(2); 
	is >> M._mat[2][1]; is.ignore(2);
	is >> M._mat[2][2]; is.ignore(2);
	is >> M._mat[2][3]; is.ignore(1);
	
	is >> M._mat[3][0]; is.ignore(2); 
	is >> M._mat[3][1]; is.ignore(2);
	is >> M._mat[3][2]; is.ignore(2);
	is >> M._mat[3][3];
	return is;
}


inline ofMatrix4x4& ofMatrix4x4::operator = (const ofMatrix4x4& rhs) {
	if ( &rhs == this ) return *this;
	set(rhs.getPtr());
	return *this;
}

inline void ofMatrix4x4::set(const ofMatrix4x4& rhs) {
	set(rhs.getPtr());
}

inline void ofMatrix4x4::set(float const * const ptr) {
	float* local_ptr = (float*)_mat;
	for (int i = 0;i < 16;++i) local_ptr[i] = (float)ptr[i];
}

inline void ofMatrix4x4::set(double const * const ptr) {
	float* local_ptr = (float*)_mat;
	for (int i = 0;i < 16;++i) local_ptr[i] = (float)ptr[i];
}

inline bool ofMatrix4x4::isIdentity() const {
	return _mat[0][0] == 1.0f && _mat[0][1] == 0.0f && _mat[0][2] == 0.0f &&  _mat[0][3] == 0.0f &&
		   _mat[1][0] == 0.0f && _mat[1][1] == 1.0f && _mat[1][2] == 0.0f &&  _mat[1][3] == 0.0f &&
		   _mat[2][0] == 0.0f && _mat[2][1] == 0.0f && _mat[2][2] == 1.0f &&  _mat[2][3] == 0.0f &&
		   _mat[3][0] == 0.0f && _mat[3][1] == 0.0f && _mat[3][2] == 0.0f &&  _mat[3][3] == 1.0f;
}

inline void ofMatrix4x4::makeOrtho2DMatrix(double left,   double right,
	                        double bottom, double top) {
	makeOrthoMatrix(left, right, bottom, top, -1.0, 1.0);
}

inline ofVec3f ofMatrix4x4::getTranslation() const {
	return ofVec3f(_mat[3][0], _mat[3][1], _mat[3][2]);
}

inline ofVec3f ofMatrix4x4::getScale() const {
	ofVec3f x_vec(_mat[0][0], _mat[1][0], _mat[2][0]);
	ofVec3f y_vec(_mat[0][1], _mat[1][1], _mat[2][1]);
	ofVec3f z_vec(_mat[0][2], _mat[1][2], _mat[2][2]);
	return ofVec3f(x_vec.length(), y_vec.length(), z_vec.length());
}

//static utility methods
inline ofMatrix4x4 ofMatrix4x4::newIdentityMatrix(void) {
	ofMatrix4x4 m;
	m.makeIdentityMatrix();
	return m;
}

inline ofMatrix4x4 ofMatrix4x4::newScaleMatrix(float sx, float sy, float sz) {
	ofMatrix4x4 m;
	m.makeScaleMatrix(sx, sy, sz);
	return m;
}

inline ofMatrix4x4 ofMatrix4x4::newScaleMatrix(const ofVec3f& v ) {
	return newScaleMatrix(v.x, v.y, v.z );
}

inline ofMatrix4x4 ofMatrix4x4::newTranslationMatrix(float tx, float ty, float tz) {
	ofMatrix4x4 m;
	m.makeTranslationMatrix(tx, ty, tz);
	return m;
}

inline ofMatrix4x4 ofMatrix4x4::newTranslationMatrix(const ofVec3f& v ) {
	return newTranslationMatrix(v.x, v.y, v.z );
}

inline ofMatrix4x4 ofMatrix4x4::newRotationMatrix( const ofQuaternion& q ) {
	return ofMatrix4x4(q);
}
inline ofMatrix4x4 ofMatrix4x4::newRotationMatrix(float angle, float x, float y, float z ) {
	ofMatrix4x4 m;
	m.makeRotationMatrix(angle, x, y, z);
	return m;
}
inline ofMatrix4x4 ofMatrix4x4::newRotationMatrix(float angle, const ofVec3f& axis ) {
	ofMatrix4x4 m;
	m.makeRotationMatrix(angle, axis);
	return m;
}
inline ofMatrix4x4 ofMatrix4x4::newRotationMatrix(	float angle1, const ofVec3f& axis1,
    float angle2, const ofVec3f& axis2,
    float angle3, const ofVec3f& axis3) {
	ofMatrix4x4 m;
	m.makeRotationMatrix(angle1, axis1, angle2, axis2, angle3, axis3);
	return m;
}
inline ofMatrix4x4 ofMatrix4x4::newRotationMatrix(const ofVec3f& from, const ofVec3f& to ) {
	ofMatrix4x4 m;
	m.makeRotationMatrix(from, to);
	return m;
}

inline ofMatrix4x4 ofMatrix4x4::getInverseOf( const ofMatrix4x4& matrix) {
	ofMatrix4x4 m;
	m.makeInvertOf(matrix);
	return m;
}

inline ofMatrix4x4 ofMatrix4x4::getTransposedOf( const ofMatrix4x4& matrix) {
	ofMatrix4x4 m(matrix._mat[0][0], matrix._mat[1][0], matrix._mat[2][0],
	               matrix._mat[3][0], matrix._mat[0][1], matrix._mat[1][1], matrix._mat[2][1],
	               matrix._mat[3][1], matrix._mat[0][2], matrix._mat[1][2], matrix._mat[2][2],
	               matrix._mat[3][2], matrix._mat[0][3], matrix._mat[1][3], matrix._mat[2][3],
	               matrix._mat[3][3]);
	return m;
}

inline ofMatrix4x4 ofMatrix4x4::getOrthoNormalOf(const ofMatrix4x4& matrix) {
	ofMatrix4x4 m;
	m.makeOrthoNormalOf(matrix);
	return m;
}

inline ofMatrix4x4 ofMatrix4x4::newOrthoMatrix(double left, double right,
                                        double bottom, double top,
                                        double zNear, double zFar) {
	ofMatrix4x4 m;
	m.makeOrthoMatrix(left, right, bottom, top, zNear, zFar);
	return m;
}

inline ofMatrix4x4 ofMatrix4x4::newOrtho2DMatrix(double left, double right,
    double bottom, double top) {
	ofMatrix4x4 m;
	m.makeOrtho2DMatrix(left, right, bottom, top);
	return m;
}

inline ofMatrix4x4 ofMatrix4x4::newFrustumMatrix(double left, double right,
    double bottom, double top,
    double zNear, double zFar) {
	ofMatrix4x4 m;
	m.makeFrustumMatrix(left, right, bottom, top, zNear, zFar);
	return m;
}

inline ofMatrix4x4 ofMatrix4x4::newPerspectiveMatrix(double fovy, double aspectRatio,
    double zNear, double zFar) {
	ofMatrix4x4 m;
	m.makePerspectiveMatrix(fovy, aspectRatio, zNear, zFar);
	return m;
}

inline ofMatrix4x4 ofMatrix4x4::newLookAtMatrix(const ofVec3f& eye, const ofVec3f& center, const ofVec3f& up) {
	ofMatrix4x4 m;
	m.makeLookAtMatrix(eye, center, up);
	return m;
}

inline ofVec3f ofMatrix4x4::postMult( const ofVec3f& v ) const {
	float d = 1.0f / (_mat[3][0] * v.x + _mat[3][1] * v.y + _mat[3][2] * v.z + _mat[3][3]) ;
	return ofVec3f( (_mat[0][0]*v.x + _mat[0][1]*v.y + _mat[0][2]*v.z + _mat[0][3])*d,
	                 (_mat[1][0]*v.x + _mat[1][1]*v.y + _mat[1][2]*v.z + _mat[1][3])*d,
	                 (_mat[2][0]*v.x + _mat[2][1]*v.y + _mat[2][2]*v.z + _mat[2][3])*d) ;
}

inline ofVec3f ofMatrix4x4::preMult( const ofVec3f& v ) const {
	float d = 1.0f / (_mat[0][3] * v.x + _mat[1][3] * v.y + _mat[2][3] * v.z + _mat[3][3]) ;
	return ofVec3f( (_mat[0][0]*v.x + _mat[1][0]*v.y + _mat[2][0]*v.z + _mat[3][0])*d,
	                 (_mat[0][1]*v.x + _mat[1][1]*v.y + _mat[2][1]*v.z + _mat[3][1])*d,
	                 (_mat[0][2]*v.x + _mat[1][2]*v.y + _mat[2][2]*v.z + _mat[3][2])*d);
}

// \brief post-multiplies the vector by the matrix (i.e. returns M mult v).
// The vector is implicitly treated as a column-matrix
inline ofVec4f ofMatrix4x4::postMult( const ofVec4f& v ) const {
	return ofVec4f( (_mat[0][0]*v.x + _mat[0][1]*v.y + _mat[0][2]*v.z + _mat[0][3]*v.w),
	                 (_mat[1][0]*v.x + _mat[1][1]*v.y + _mat[1][2]*v.z + _mat[1][3]*v.w),
	                 (_mat[2][0]*v.x + _mat[2][1]*v.y + _mat[2][2]*v.z + _mat[2][3]*v.w),
	                 (_mat[3][0]*v.x + _mat[3][1]*v.y + _mat[3][2]*v.z + _mat[3][3]*v.w)) ;
}

// \brief pre-multiplies the vector by the matrix (i.e. returns v mult M)
// The vector is implicitly treated as a row-matrix
inline ofVec4f ofMatrix4x4::preMult( const ofVec4f& v ) const {
	return ofVec4f( (_mat[0][0]*v.x + _mat[1][0]*v.y + _mat[2][0]*v.z + _mat[3][0]*v.w),
	                 (_mat[0][1]*v.x + _mat[1][1]*v.y + _mat[2][1]*v.z + _mat[3][1]*v.w),
	                 (_mat[0][2]*v.x + _mat[1][2]*v.y + _mat[2][2]*v.z + _mat[3][2]*v.w),
	                 (_mat[0][3]*v.x + _mat[1][3]*v.y + _mat[2][3]*v.z + _mat[3][3]*v.w));
}

// \brief performs a pre-multiplication transformation on the vector using only the
// upper left 3x3 portion of the matrix (i.e. only the rotation part).
inline ofVec3f ofMatrix4x4::transform3x3(const ofVec3f& v, const ofMatrix4x4& m) {
	return ofVec3f( (m._mat[0][0]*v.x + m._mat[1][0]*v.y + m._mat[2][0]*v.z),
	                 (m._mat[0][1]*v.x + m._mat[1][1]*v.y + m._mat[2][1]*v.z),
	                 (m._mat[0][2]*v.x + m._mat[1][2]*v.y + m._mat[2][2]*v.z));
}

// \brief performs a post-multiplication transformation on the vector using only the
// upper left 3x3 portion of the matrix (i.e. only the rotation part).
inline ofVec3f ofMatrix4x4::transform3x3(const ofMatrix4x4& m, const ofVec3f& v) {
	return ofVec3f( (m._mat[0][0]*v.x + m._mat[0][1]*v.y + m._mat[0][2]*v.z),
	                 (m._mat[1][0]*v.x + m._mat[1][1]*v.y + m._mat[1][2]*v.z),
	                 (m._mat[2][0]*v.x + m._mat[2][1]*v.y + m._mat[2][2]*v.z) ) ;
}

// \brief translates this matrix by treating the ofVec3f like a translation matrix,
// and multiplying this Matrix by it in a pre-multiplication manner (T mult M)
inline void ofMatrix4x4::preMultTranslate( const ofVec3f& v ) {
	for (unsigned i = 0; i < 3; ++i) {
		float tmp = v.getPtr()[i];
		if (tmp == 0)
			continue;
		_mat[3][0] += tmp * _mat[i][0];
		_mat[3][1] += tmp * _mat[i][1];
		_mat[3][2] += tmp * _mat[i][2];
		_mat[3][3] += tmp * _mat[i][3];
	}
}

// \brief translates this matrix by treating the ofVec3f like a translation matrix,
// and multiplying this Matrix by it in a post-multiplication manner (M mult T)
inline void ofMatrix4x4::postMultTranslate( const ofVec3f& v ) {
	for (unsigned i = 0; i < 3; ++i) {
		float tmp = v.getPtr()[i];
		if (tmp == 0)
			continue;
		_mat[0][i] += tmp * _mat[0][3];
		_mat[1][i] += tmp * _mat[1][3];
		_mat[2][i] += tmp * _mat[2][3];
		_mat[3][i] += tmp * _mat[3][3];
	}
}

// AARON METHOD
// \brief the positional argument version of the above
inline void ofMatrix4x4::postMultTranslate( float x, float y, float z) {
	if (x != 0) {
		_mat[0][0] += x * _mat[0][3];
		_mat[1][0] += x * _mat[1][3];
		_mat[2][0] += x * _mat[2][3];
		_mat[3][0] += x * _mat[3][3];
	}
	if (y != 0) {
		_mat[0][1] += y * _mat[0][3];
		_mat[1][1] += y * _mat[1][3];
		_mat[2][1] += y * _mat[2][3];
		_mat[3][1] += y * _mat[3][3];
	}
	if (z != 0) {
		_mat[0][2] += z * _mat[0][3];
		_mat[1][2] += z * _mat[1][3];
		_mat[2][2] += z * _mat[2][3];
		_mat[3][2] += z * _mat[3][3];
	}
}

// \brief treats the ofVec3f like a scaling matrix and edits this Matrix
// by multiplying the vector with it in a pre-multiplication style (V mult M)
inline void ofMatrix4x4::preMultScale( const ofVec3f& v ) {
	_mat[0][0] *= v.getPtr()[0];
	_mat[0][1] *= v.getPtr()[0];
	_mat[0][2] *= v.getPtr()[0];
	_mat[0][3] *= v.getPtr()[0];
	_mat[1][0] *= v.getPtr()[1];
	_mat[1][1] *= v.getPtr()[1];
	_mat[1][2] *= v.getPtr()[1];
	_mat[1][3] *= v.getPtr()[1];
	_mat[2][0] *= v.getPtr()[2];
	_mat[2][1] *= v.getPtr()[2];
	_mat[2][2] *= v.getPtr()[2];
	_mat[2][3] *= v.getPtr()[2];
}

// \brief treats the ofVec3f like a scaling matrix and edits this Matrix
// by multiplying the vector with it in a post-multiplication style (M mult V)
inline void ofMatrix4x4::postMultScale( const ofVec3f& v ) {
	_mat[0][0] *= v.getPtr()[0];
	_mat[1][0] *= v.getPtr()[0];
	_mat[2][0] *= v.getPtr()[0];
	_mat[3][0] *= v.getPtr()[0];
	_mat[0][1] *= v.getPtr()[1];
	_mat[1][1] *= v.getPtr()[1];
	_mat[2][1] *= v.getPtr()[1];
	_mat[3][1] *= v.getPtr()[1];
	_mat[0][2] *= v.getPtr()[2];
	_mat[1][2] *= v.getPtr()[2];
	_mat[2][2] *= v.getPtr()[2];
	_mat[3][2] *= v.getPtr()[2];
}

// \brief rotates this Matrix by the provided quaternion
inline void ofMatrix4x4::rotate(const ofQuaternion& q){
	postMultRotate(q);
}

// \brief rotates this Matrix by the provided angle (in degrees) around an axis defined by the three values
inline void ofMatrix4x4::rotate(float angle, float x, float y, float z){
	postMultRotate(angle,x,y,z);
}

// \brief Rotates this Matrix by the provided angle (in Radians) around an axis defined by the three values
inline void ofMatrix4x4::rotateRad(float angle, float x, float y, float z){
	postMultRotate(angle*RAD_TO_DEG,x,y,z);
}

// \brief Translates this matrix by the provided amount
inline void ofMatrix4x4::translate( float tx, float ty, float tz ){
	postMultTranslate(tx,ty,tz);
}

// \brief Translates this matrix by the provided vector
inline void ofMatrix4x4::translate( const ofVec3f& v ){
	postMultTranslate(v);
}

// \brief scales this matrix by the provided scales
inline void ofMatrix4x4::scale(float x, float y, float z){
	postMultScale(x,y,z);
}

// \brief scales this matrix, treating the ofVec3f as the diagonal of a scaling matrix.
inline void ofMatrix4x4::scale( const ofVec3f& v ){
	postMultScale(v);
}

// implementation of the gl-style pre-multiplication versions of the above functions
inline void ofMatrix4x4::glRotate(float angle, float x, float y, float z){
	preMultRotate(ofQuaternion(angle,ofVec3f(x,y,z)));
}

inline void ofMatrix4x4::glRotateRad(float angle, float x, float y, float z){
	preMultRotate(ofQuaternion(angle*RAD_TO_DEG,ofVec3f(x,y,z)));
}

inline void ofMatrix4x4::glRotate(const ofQuaternion& q){
	preMultRotate(q);
}

inline void ofMatrix4x4::glTranslate( float tx, float ty, float tz ){
	preMultTranslate(ofVec3f(tx,ty,tz));
}

inline void ofMatrix4x4::glTranslate( const ofVec3f& v ){
	preMultTranslate(v);
}

inline void ofMatrix4x4::glScale(float x, float y, float z){
	preMultScale(ofVec3f(x,y,z));
}

inline void ofMatrix4x4::glScale( const ofVec3f& v ){
	preMultScale(v);
}

// AARON METHOD
inline void ofMatrix4x4::postMultScale( float x, float y, float z ) {
	_mat[0][0] *= x;
	_mat[1][0] *= x;
	_mat[2][0] *= x;
	_mat[3][0] *= x;
	_mat[0][1] *= y;
	_mat[1][1] *= y;
	_mat[2][1] *= y;
	_mat[3][1] *= y;
	_mat[0][2] *= z;
	_mat[1][2] *= z;
	_mat[2][2] *= z;
	_mat[3][2] *= z;
}


inline void ofMatrix4x4::preMultRotate( const ofQuaternion& q ) {
	if (q.zeroRotation())
		return;
	ofMatrix4x4 r;
	r.setRotate(q);
	preMult(r);
}

inline void ofMatrix4x4::postMultRotate( const ofQuaternion& q ) {
	if (q.zeroRotation())
		return;
	ofMatrix4x4 r;
	r.setRotate(q);
	postMult(r);
}

// AARON METHOD
inline void ofMatrix4x4::postMultRotate(float angle, float x, float y, float z) {
	ofMatrix4x4 r;
	r.makeRotationMatrix(angle, x, y, z);
	postMult(r);
}

// \brief provides Vector3 * Matrix multiplication. Vectors are implicitly treated as row-matrices.
inline ofVec3f operator* (const ofVec3f& v, const ofMatrix4x4& m ) {
	return m.preMult(v);
}
// \brief provides Vector4 * Matrix multiplication. Vectors are implicitly treated as row-matrices.
inline ofVec4f operator* (const ofVec4f& v, const ofMatrix4x4& m ) {
	return m.preMult(v);
}


/// \endcond