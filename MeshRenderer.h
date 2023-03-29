#pragma once

//###################################################################
//Utility functions for shaders and textures
//###################################################################

//Loads and compiles shaders for OpenGL use.
GLuint LoadShaders( const char * vertex_file_path, const char * fragment_file_path );

//Loads a DDS texture for OpenGL use,
GLuint LoadDDS(const char * imagepath);

//Loads a DDS texture for OpenGL use, Reimplementation that uses a char buffer as a "file".
GLuint LoadDDS_FromBuffer( std::vector< char > byteBuf );

//###################################################################
//Storage singleton class for Shaders
//###################################################################

//Shader container
class ShaderList
{
	public:
	ShaderList(){};
	~ShaderList()
	{
		shaders.clear();
	}

	static GLuint GetShader( std::string name )
	{
		return ShaderList::Get()->shaders[name];
	};
	static void AddShader( std::string name, GLuint id )
	{
		ShaderList::Get()->shaders[name] = id;
	};
	static bool HasShader( std::string name )
	{
		if( ShaderList::Get()->shaders.count( name ) > 0 )
			return true;
		else
			return false;
	};
	static GLuint LoadShader( std::string name, std::string vertexFileName, std::string fragmentFileName )
	{
		if( ShaderList::Get()->shaders.count( name ) > 0 )
			return GetShader( name );
		
		GLuint shaderID = LoadShaders( vertexFileName.c_str(), fragmentFileName.c_str() );
		ShaderList::Get()->AddShader( name, shaderID );
		return shaderID;
	};

	static ShaderList* Get() { return instance_; }
	static void Initialize() { instance_ = new ShaderList(); }

protected:
	static ShaderList* instance_;
	std::map < std::string, GLuint > shaders;
};

//###################################################################
//Model Rendering
//###################################################################

//Todo: Implement camera class
class Camera
{
public:
    glm::mat4 Projection;
    glm::mat4 View;
};

//A simple mesh object rendering system.
class MeshObject
{
public:
	//Constructor.
    MeshObject( std::vector< glm::vec3 > verts, std::vector< unsigned int > ind, std::vector< glm::vec2 > uv, GLuint shader, GLuint tex );

	//Destructor. TODO: Clean up memory and OPENGL states here.
	~MeshObject();

	//Render the mesh using a specified camera.
    void Draw( Camera cam );

    GLuint VAO;

    GLuint VBO, EBO;

    GLuint TextureID, Texture;
    GLuint UVBuffer;

    std::vector< glm::vec3 > vertices;
    std::vector< glm::vec2 > uvs;
    std::vector< unsigned int > indices;
    GLuint vertexbuffer;
    GLuint shaderID;

    //Positioning:
    glm::vec3 position;
    glm::vec3 angles;
    glm::vec3 scale;
};

//Clone of 'MeshRenderer' optimised for MDB data.
//TODO: Implement this.
#include "MDBParser.h"
#include "SimpleRenderer.h"

class CMDBRenderer;

class CMDBMesh
{
public:
	CMDBMesh( MDBMeshInfo *info, CMDBRenderer *parent );
	~CMDBMesh();

	void Draw( Camera cam );

	//Mesh info
	MDBMeshInfo *pMeshInfo = nullptr;
	CMDBRenderer* pParent = nullptr;

	GLuint VAO;
	GLuint VBO, EBO;
	GLuint UVBuffer;
	GLuint BoneIDS;
	GLuint VertexWeights;

	GLuint textureID;
};

class CMDBObject
{
public:
	CMDBObject( MDBObject *obj, CMDBRenderer *parent );
	void Draw( Camera cam );

	std::vector< std::unique_ptr< CMDBMesh > > meshs;

	MDBObject* pObjInfo = nullptr;
};

class CMDBRenderer
{
public:
	CMDBRenderer( );
	~CMDBRenderer( );

    void Draw( Camera cam );

	std::unique_ptr<MDBReader> model;
	std::vector< std::unique_ptr< CMDBObject > > objects;

	//Shader to use.
	GLuint shaderID;

	//Map of textures
	std::map< std::wstring, GLuint > textureMap;

	//Positioning:
	glm::vec3 position;
	glm::vec3 angles;
	glm::vec3 scale;

	std::vector<std::unique_ptr<CDebugLine>> lines;
};
