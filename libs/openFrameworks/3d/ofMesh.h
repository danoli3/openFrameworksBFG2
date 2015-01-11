// TODO:
// get face info properly
// use edge flags

#pragma once

#include "ofVec3f.h"
#include "ofVec2f.h"
#include "ofColor.h"
#include "ofUtils.h"
#include "ofConstants.h"
#include "ofGLUtils.h"

class ofMeshFace;

/// ##Description

/// An ofMesh represents a set of vertices in 3D spaces, and normals at those points, colors at those points, and texture coordinates at those points. Each of these different properties is stored in a vector.
/// Vertices are passed to your graphics card and your graphics card fill in the spaces in between them in a processing usually called the rendering pipeline. The rendering pipeline goes more or less like this:
///
/// 1. Say how you're going to connect all the points.
///
/// 2. Make some points.
///
/// 3. Say that you're done making points.
///
/// You may be thinking: I'll just make eight vertices and voila: a cube. Not so quick. There's a hitch and that hitch is that the OpenGL renderer has different ways of connecting the vertices that you pass to it and none are as efficient as to only need eight vertices to create a cube.
///
/// You've probably seen a version of the following image somewhere before.
/// ![PRIMATIVES](primitives_new-640x269.gif)
/// Generally you have to create your points to fit the drawing mode that you've selected because of whats called winding. A vertex gets connected to another vertex in the order that the mode does its winding and this means that you might need multiple vertices in a given location to create the shape you want. The cube, for example, requires eighteen vertices, not the eight that you would expect. If you note the order of vertices in the GL chart above you'll see that all of them use their vertices slightly differently (in particular you should make note of the GL_TRIANGLE_STRIP example). Drawing a shape requires that you keep track of which drawing mode is being used and which order your vertices are declared in.
///
/// If you're thinking: it would be nice if there were an abstraction layer for this you're thinking right. Enter the mesh, which is really just an abstraction of the vertex and drawing mode that we started with but which has the added bonus of managing the draw order for you. That may seem insignificant at first, but it provides some real benefits when working with complex geometry.
///
/// A very typical usage is something like the following:
///
/// ~~~~{.cpp}
/// ofMesh mesh;
/// for (int y = 0; y < height; y++){
/// 	for (int x = 0; x<width; x++){
/// 		mesh.addVertex(ofPoint(x,y,0)); // make a new vertex
/// 		mesh.addColor(ofFloatColor(0,0,0));  // add a color at that vertex
/// 	}
/// }

/// // now it's important to make sure that each vertex is correctly connected with the
/// // other vertices around it. This is done using indices, which you can set up like so:
/// for (int y = 0; y<height-1; y++){
/// 	for (int x=0; x<width-1; x++){
/// 		mesh.addIndex(x+y*width);				// 0
/// 		mesh.addIndex((x+1)+y*width);			// 1
/// 		mesh.addIndex(x+(y+1)*width);			// 10
///
/// 		mesh.addIndex((x+1)+y*width);			// 1
/// 		mesh.addIndex((x+1)+(y+1)*width);		// 11
/// 		mesh.addIndex(x+(y+1)*width);			// 10
/// 	}
/// }
/// ~~~~

class ofMesh{
public:

	/// This creates the mesh, using OF_PRIMITIVE_TRIANGLES and without any initial vertices.
	ofMesh();

	/// This allows to you to use one of the other ofPrimitiveModes: OF_PRIMITIVE_TRIANGLES, OF_PRIMITIVE_TRIANGLE_STRIP, OF_PRIMITIVE_TRIANGLE_FAN, OF_PRIMITIVE_LINES, OF_PRIMITIVE_LINE_STRIP, OF_PRIMITIVE_LINE_LOOP, OF_PRIMITIVE_POINTS. See [ofGLUtils](../gl/ofGLUtils.htm) for more information on these types.
	ofMesh(ofPrimitiveMode mode, const vector<ofVec3f>& verts);
	virtual ~ofMesh();


	/// Allows you to set the ofPrimitiveMode. The available modes are OF_PRIMITIVE_TRIANGLES, OF_PRIMITIVE_TRIANGLE_STRIP, OF_PRIMITIVE_TRIANGLE_FAN, OF_PRIMITIVE_LINES, OF_PRIMITIVE_LINE_STRIP, OF_PRIMITIVE_LINE_LOOP, OF_PRIMITIVE_POINTS
	void setMode(ofPrimitiveMode mode);

	/// Returns the primitive mode that the mesh is using.
	ofPrimitiveMode getMode() const;

  /// This removes all the vertices, colors, and indices from the mesh.
	void clear();

	/// Allow you to set up the indices automatically when you add a vertex.
	void setupIndicesAuto();

	/// Returns the vertex at the index.
	ofVec3f getVertex(ofIndexType i) const;

	/// Add a new vertex at the end of the current list of vertices. It is important to remember that the order the vertices are added to the list determines how they link they form the polygons and strips (assuming you do not change their indeces). See the ofMesh class description for details.
	void addVertex(const ofVec3f& v);

	/// Add a vector of vertices to a mesh, allowing you to push out many at once rather than adding one at a time. The vector of vertices is added after the end of the current vertices list.
	void addVertices(const vector<ofVec3f>& verts);

	/// Add an array of vertices to the mesh. Because you are using a pointer to the array you also have to define the length of the array as an int (amt). The vertices are added at the end of the current vertices list.
	void addVertices(const ofVec3f* verts, int amt);

	/// Removes the vertex at the index in the vector.
	void removeVertex(ofIndexType index);
	void setVertex(ofIndexType index, const ofVec3f& v);

	/// Removes all the vertices.
	void clearVertices();

	/// Returns the normal at the index in the normals vector.
	ofVec3f getNormal(ofIndexType i) const;


	/// Add a normal to the mesh as a 3D vector, typically perpendicular to the plane of the face. A normal is a vector that defines how a surface responds to lighting, i.e. how it is lit. The amount of light reflected by a surface is proportional to the angle between the light's direction and the normal. The smaller the angle the brighter the surface will look. See the normalsExample for advice on computing the normals.
	/// addNormal adds the 3D vector to the end of the list, so you need to make sure you add normals at the same index of the matching face.
	void addNormal(const ofVec3f& n);


	/// Add a vector of normals to a mesh, allowing you to push out many normals at once rather than adding one at a time. The vector of normals is added after the end of the current normals list.
	void addNormals(const vector<ofVec3f>& norms);

	// Add an array of normals to the mesh. Because you are using a pointer to the array you also have to define the length of the array as an int (amt). The normals are added at the end of the current normals list.
	void addNormals(const ofVec3f* norms, int amt);

	/// Remove a normal.
	void removeNormal(ofIndexType index);
	void setNormal(ofIndexType index, const ofVec3f& n);

	/// Remove all the normals.
	void clearNormals();

	/// Returns the color at the index in the colors vector.
	ofFloatColor getColor(ofIndexType i) const;

	/// This adds a color to the mesh, the color will be associated with the vertex in the same position.
	void addColor(const ofFloatColor& c);

	/// This adds colors using a reference to a vector of ofColors. For each color in the vector, this will put the colors at the corresponding vertex.
	void addColors(const vector<ofFloatColor>& cols);

	/// This adds a pointer of colors to the ofMesh instance with the amount passed as the second parameter.
	void addColors(const ofFloatColor* cols, int amt);

	/// Remove a color at the index in the colors vector.
	void removeColor(ofIndexType index);

	/// Set the color at the index in the colors vector.
	void setColor(ofIndexType index, const ofFloatColor& c);

	/// Clear all the colors.
	void clearColors();

	/// Returns the Vec2f representing the texture coordinate. Because OF uses ARB textures these are in pixels rather than 0-1 normalized coordinates.
	ofVec2f getTexCoord(ofIndexType i) const;

	/// Add a Vec2f representing the texture coordinate. Because OF uses ARB textures these are in pixels rather than 0-1 normalized coordinates.
	void addTexCoord(const ofVec2f& t);

	/// Add a vector of texture coordinates to a mesh, allowing you to push out many at once rather than adding one at a time. The vector of texture coordinates is added after the end of the current texture coordinates list.
	void addTexCoords(const vector<ofVec2f>& tCoords);

	/// Add an array of texture coordinates to the mesh. Because you are using a pointer to the array you also have to define the length of the array as an int (amt). The texture coordinates are added at the end of the current texture coordinates list.
	void addTexCoords(const ofVec2f* tCoords, int amt);

	/// Remove a Vec2f representing the texture coordinate.
	void removeTexCoord(ofIndexType index);
	void setTexCoord(ofIndexType index, const ofVec2f& t);

	/// Clear all the texture coordinates.
	void clearTexCoords();

	/// Returns the index from the index vector. Each index represents the index of the vertex in the vertices vector. This determines the way that the vertices are connected into the polgoynon type set in the primitiveMode.
	ofIndexType getIndex(ofIndexType i) const;


	/// Add an index to the index vector. Each index represents the order of connection for  vertices. This determines the way that the vertices are connected according to the polygon type set in the primitiveMode. It important to note that a particular vertex might be used for several faces and so would be referenced several times in the index vector.
	/// ~~~~{.cpp}
	/// ofMesh mesh;
	/// mesh.setMode(OF_PRIMITIVE_TRIANGLES);
	/// mesh.addVertex(ofPoint(0,-200,0));
	/// mesh.addVertex(ofPoint(200, 0, 0 ));
	/// mesh.addVertex(ofPoint(-200, 0, 0 ));
	/// mesh.addVertex(ofPoint(0, 200, 0 ));
	/// mesh.addIndex(0); //connect the first vertex we made, v0
	/// mesh.addIndex(1); //to v1
	/// mesh.addIndex(2); //to v2 to complete the face
	/// mesh.addIndex(1); //now start a new face beginning with v1
	/// mesh.addIndex(2); //that is connected to v2
	/// mesh.addIndex(3); //and we complete the face with v3
	/// ~~~~

	/// Will give you this shape:
	/// ![image of basic use of indices](index.jpg)
	void addIndex(ofIndexType i);

	/// This adds a vector of indices.
	void addIndices(const vector<ofIndexType>& inds);

	/// This adds indices to the ofMesh by pointing to an array of indices. The "amt" defines the length of the array.
	void addIndices(const ofIndexType* inds, int amt);

	/// Removes an index.
	void removeIndex(ofIndexType index);

	/// This sets the index at i.
	void setIndex(ofIndexType index, ofIndexType val);

	/// Remove all the indices of the mesh. This means that your mesh will be a point cloud.
	void clearIndices();

	/// Adding a triangle means using three of the vertices that have already been added to create a triangle. This is an easy way to create triangles in the mesh. The indices refer to the index of the vertex in the vector of vertices.
  void addTriangle(ofIndexType index1, ofIndexType index2, ofIndexType index3);


  	/// Returns the size of the vertices vector for the mesh. This will tell you how many vertices are contained in the mesh.
	int getNumVertices() const;

	/// Returns the size of the colors vector for the mesh. This will tell you how many colors are contained in the mesh.
	int getNumColors() const;

	/// Returns the size of the normals vector for the mesh. This will tell you how many normals are contained in the mesh.
	int getNumNormals() const;

	/// Returns the size of the texture coordinates vector for the mesh. This will tell you how many texture coordinates are contained in the mesh.
	int getNumTexCoords() const;

	/// Returns the size of the indices vector for the mesh. This will tell you how many indices are contained in the mesh.
	int getNumIndices() const;

	/// Returns a pointer to the vertices that the mesh contains.
	ofVec3f* getVerticesPointer();

	/// Returns a pointer that contains all of the colors of the mesh, if it has any. Use this if you plan to change the colors as part of this call as it will force a reset of the cache.
	ofFloatColor* getColorsPointer();

	/// Returns a pointer to the normals that the mesh contains.
	ofVec3f* getNormalsPointer();

	/// Returns a pointer to the texture coords that the mesh contains.
	ofVec2f* getTexCoordsPointer();

	/// Returns a pointer to the indices that the mesh contains.
	ofIndexType* getIndexPointer();

	/// Returns a pointer to the vertices that the mesh contains.
	const ofVec3f* getVerticesPointer() const;

	/// Returns a pointer that contains all of the colors of the mesh, if it has any. (read only)
	const ofFloatColor* getColorsPointer() const;

	/// Returns a pointer to the normals that the mesh contains.
	const ofVec3f* getNormalsPointer() const;

	/// Get a pointer to the ofVec2f texture coordinates that the mesh contains.
	const ofVec2f* getTexCoordsPointer() const;

	/// Returns a pointer to the indices that the mesh contains.
	const ofIndexType* getIndexPointer() const;

	/// Returns the vector that contains all of the vertices of the mesh.
	vector<ofVec3f> & getVertices();

	/// Returns the vector that contains all of the colors of the mesh, if it has any. Use this if you plan to change the colors as part of this call as it will force a reset of the cache.
	vector<ofFloatColor> & getColors();

	/// Returns the vector that contains all of the normals of the mesh, if it has any. Use this if you plan to change the normals as part of this call as it will force a reset of the cache.
	vector<ofVec3f> & getNormals();

	/// Returns a vector of Vec2f representing the texture coordinates for the whole mesh. Because OF uses ARB textures these are in pixels rather than 0-1 normalized coordinates. Use this if you plan to change the texture coordinates as part of this call as it will force a reset of the cache.
	vector<ofVec2f> & getTexCoords();

	/// Returns the vector that contains all of the indices of the mesh, if it has any. Use this if you plan to change the indices as part of this call as it will force a reset of the cache.
	vector<ofIndexType> & getIndices();

	/// Returns the vector that contains all of the vertices of the mesh.
	const vector<ofVec3f> & getVertices() const;

	/// Returns the vector that contains all of the colors of the mesh, if it has any. (read only)
	const vector<ofFloatColor> & getColors() const;

	/// Returns the vector that contains all of the normals of the mesh, if it has any. (read only)
	const vector<ofVec3f> & getNormals() const;

	/// Returns a vector of Vec2f representing the texture coordinates for the whole mesh. Because OF uses ARB textures these are in pixels rather than 0-1 normalized coordinates. (read only)
	const vector<ofVec2f> & getTexCoords() const;

	/// Returns the vector that contains all of the indices of the mesh, if it has any. (read only)
	const vector<ofIndexType> & getIndices() const;

	/// Returns the vector that contains all of the faces of the mesh. This isn't currently implemented.
	vector<int>& getFace(int faceId);

	/// Returns a ofVec3f defining the centroid of all the vetices in the mesh.
	ofVec3f getCentroid() const;

	/// If the vertices of the mesh have changed, been added or removed.
	bool haveVertsChanged();

	/// If the colors of the mesh have changed, been added or removed.
	bool haveColorsChanged();

	/// If the normals of the mesh have changed, been added or removed.
	bool haveNormalsChanged();

	/// If the texture coords of the mesh have changed, been added or removed.
	bool haveTexCoordsChanged();

	/// If the indices of the mesh have changed, been added or removed.
	bool haveIndicesChanged();

	/// Whether the mesh has any vertices.
	bool hasVertices() const;

	/// /returns Whether the mesh has any colors.
	bool hasColors() const;

	/// Whether the mesh has any normals.
	bool hasNormals() const;

	/// Whether the mesh has any textures assigned to it.
	bool hasTexCoords() const;

	/// Whether the mesh has any indices assigned to it.
	bool hasIndices() const;

	/// This allows you draw just the vertices, meaning that you'll have a point cloud.
	void drawVertices() const;

	/// This draws the mesh as GL_LINES, meaning that you'll have a wireframe.
	void drawWireframe() const;

	/// This draws the mesh as faces, meaning that you'll have a collection of faces.
	void drawFaces() const;

	/// This draws the mesh using its primitive type, meaning that if you set them up to be triangles, this will draw the triangles.
	void draw() const;

	/// This draws the mesh using a defined renderType, overriding the renderType defined with setMode().
	virtual void draw(ofPolyRenderMode renderType) const;

	/// Loads a mesh from a file located at the provided path into the mesh.
    /// This will replace any existing data within the mesh.
    /// 
    /// It expects that the file will be in the [PLY Format](http://en.wikipedia.org/wiki/PLY_(file_format)).
    /// It will only load meshes saved in the PLY ASCII format; the binary format is not supported.
	void load(string path);


    ///  	Saves the mesh at the passed path in the [PLY Format](http://en.wikipedia.org/wiki/PLY_(file_format)).
    ///  
    ///  There are two format options for PLY: a binary format and an ASCII format.
    ///  By default, it will save using the ASCII format.
    ///  Passing ``true`` into the ``useBinary`` parameter will save it in the binary format.
    ///  
    ///  If you're planning on reloading the mesh into ofMesh, ofMesh currently only supports loading the ASCII format.
    ///  
    ///  For more information, see the [PLY format specification](http://paulbourke.net/dataformats/ply/).
	void save(string path, bool useBinary = false) const;

	/// Enable mesh colors. Use disableColors() to turn colors off. Colors are enabled by default when they are added to the mesh.
    virtual void enableColors();

    /// Enable mesh textures. Use disableTextures() to turn textures off. Textures are enabled by default when they are added to the mesh.
    virtual void enableTextures();

    /// Enable mesh normals. Use disableNormals() to turn normals off. Normals are enabled by default when they are added to the mesh.
    virtual void enableNormals();

    /// Enable mesh indices. Use disableIndices() to turn indices off. Indices are enabled by default when they are added to the mesh.
    virtual void enableIndices();

    /// Disable mesh colors. Use enableColors() to turn colors back on.
    virtual void disableColors();

    /// Disable mesh textures. Use enableTextures() to turn textures back on.
    virtual void disableTextures();

    ///Disable mesh normals. Use enableNormals() to turn normals back on.
    virtual void disableNormals();

    /// Disable mesh indices. Use enableIndices() to turn indices back on.
    virtual void disableIndices();

    virtual bool usingColors() const;
    virtual bool usingTextures() const;
    virtual bool usingNormals() const;
    virtual bool usingIndices() const;

	/// Add the vertices, normals, texture coordinates and indices of one mesh onto another mesh. Everything from the referenced mesh is simply added at the end of the current mesh's lists.

    void append(const ofMesh & mesh);

    void setColorForIndices( int startIndex, int endIndex, ofColor color );
    /// Returns a mesh made up of a range of indices from startIndex to the endIndex. The new mesh includes the mesh mode, colors, textures, and normals of the original mesh (assuming any were added).
    ofMesh getMeshForIndices( int startIndex, int endIndex ) const;
    ofMesh getMeshForIndices( int startIndex, int endIndex, int startVertIndex, int endVertIndex ) const;
    void mergeDuplicateVertices();
    
    
    /// Returns the mesh as a vector of unique ofMeshFaces: a list of triangles that do not share vertices or indices 
    const vector<ofMeshFace> & getUniqueFaces() const;

    /// Returns a vector containing the calculated normals of each face in the mesh. As a default it only calculates the normal for the face as a whole but by setting (perVertex = true) it will return the same normal value for each of the three vertices making up a face.
    vector<ofVec3f> getFaceNormals( bool perVetex=false) const;
    void setFromTriangles( const vector<ofMeshFace>& tris, bool bUseFaceNormal=false );
    void smoothNormals( float angle );

    static ofMesh plane(float width, float height, int columns=2, int rows=2, ofPrimitiveMode mode=OF_PRIMITIVE_TRIANGLE_STRIP);
    static ofMesh sphere(float radius, int res=12, ofPrimitiveMode mode=OF_PRIMITIVE_TRIANGLE_STRIP);
    static ofMesh icosahedron(float radius);
    static ofMesh icosphere(float radius, int iterations=2);
  ///
	///	A helper method that returns a cylinder made of triangles. The resolution settings for the radius, height, and cap are optional (they are set at a default of 12 segments around the radius, 6 segments in the height, and 2 on the cap). You have the option to cap the cylinder or not. The only valid modes are the default OF_PRIMITIVE_TRIANGLE_STRIP and OF_PRIMITIVE_TRIANGLES.
	///	~~~~{.cpp}
	///	ofMesh mesh;
	///	mesh = ofMesh::cylinder(100.0, 200.0);
	///	~~~~
	///	
	///	![image of a simple cylinder](cylinder.jpg)
  ///
    static ofMesh cylinder(float radius, float height, int radiusSegments=12, int heightSegments=6, int numCapSegments=2, bool bCapped = true, ofPrimitiveMode mode=OF_PRIMITIVE_TRIANGLE_STRIP);

		/// A helper method that returns a cone made of triangles. The resolution settings for the radius, height, and cap are optional (they are set at a default of 12 segments around the radius, 6 segments in the height, and 2 on the cap). The only valid modes are the default OF_PRIMITIVE_TRIANGLE_STRIP and OF_PRIMITIVE_TRIANGLES.
		/// ~~~~{.cpp}
		/// ofMesh mesh;
		/// mesh = ofMesh::cone(100.0, 200.0);
		/// ~~~~
		///
		/// ![image of a simple cone](cone.jpg)
		static ofMesh cone(float radius, float height, int radiusSegments=12, int heightSegments=6, int capSegments=2, ofPrimitiveMode mode=OF_PRIMITIVE_TRIANGLE_STRIP);

		/// A helper method that returns a box made of triangles. The resolution settings for the width and height are optional (they are both set at a default of 2 triangles per side).
		/// ~~~~{.cpp}
		/// ofMesh mesh;
		/// mesh = ofMesh::box(200.0, 200.0, 200.0);
		/// ~~~~
		///
		/// ![image of a simple box](box.jpg)
		static ofMesh box(float width, float height, float depth, int resX=2, int resY=2, int resZ=2);

		/// Returns an ofMesh representing an XYZ coordinate system.
		static ofMesh axis(float size=1.0);

private:

	vector<ofVec3f> vertices;
	vector<ofFloatColor> colors;
	vector<ofVec3f> normals;
	vector<ofVec2f> texCoords;
	vector<ofIndexType> indices;

	// this variables are only caches and returned always as const
	// mutable allows to change them from const methods
	mutable vector<ofMeshFace> faces;
	mutable bool bFacesDirty;

	bool bVertsChanged, bColorsChanged, bNormalsChanged, bTexCoordsChanged, bIndicesChanged;
	ofPrimitiveMode mode;

    bool useColors;
    bool useTextures;
    bool useNormals;
    bool useIndices;

//	ofMaterial *mat;
};

// this is always a triangle //
class ofMeshFace {
public:
    ofMeshFace();

    const ofVec3f & getFaceNormal() const;

    void setVertex( int index, const ofVec3f& v );
    const ofVec3f& getVertex( int index ) const;

    void setNormal( int index, const ofVec3f& n );
    const ofVec3f& getNormal( int index ) const;

    void setColor( int index, const ofFloatColor& color );
    const ofFloatColor& getColor(int index) const;

    void setTexCoord( int index, const ofVec2f& tCoord );
    const ofVec2f& getTexCoord( int index ) const;

    void setHasColors( bool bColors );
    void setHasNormals( bool bNormals );
    void setHasTexcoords( bool bTexcoords );

    bool hasColors() const;
    bool hasNormals() const;
    bool hasTexcoords() const;

private:
    void calculateFaceNormal() const;
    bool bHasNormals, bHasColors, bHasTexcoords;

	// this variables are only caches and returned always as const
	// mutable allows to change them from const methods
    mutable bool bFaceNormalDirty;
    mutable ofVec3f faceNormal;
    ofVec3f vertices[3];
    ofVec3f normals[3];
    ofFloatColor colors[3];
    ofVec2f texCoords[3];
};
