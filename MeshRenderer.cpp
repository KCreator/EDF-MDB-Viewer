#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string.h>
#include <sstream>

//OPENGL INCLUDE
#include <SFML/Window.hpp>

#ifndef WINDOWS
#include <GLES3/gl3.h>
#else
#include <GL/glew.h>
#endif

#include <SFML/OpenGL.hpp>

#include <glm/gtc/matrix_transform.hpp>

#include "MeshRenderer.h"

//Define "Shader List" Singleton
ShaderList* ShaderList::instance_= nullptr;

//###################################################################
//Utility functions for shaders and textures
//###################################################################

//Most of this code is Shamelessly stolen from an opengl tutorial.
//Load shadersfrom file paths
GLuint LoadShaders( const char * vertex_file_path, const char * fragment_file_path )
{
	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open())
    {
		std::stringstream sstr;
		sstr << VertexShaderStream.rdbuf();
		VertexShaderCode = sstr.str();
		VertexShaderStream.close();
	}
    else
    {
		//printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", vertex_file_path);
		//getchar();
		return 0;
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open())
    {
		std::stringstream sstr;
		sstr << FragmentShaderStream.rdbuf();
		FragmentShaderCode = sstr.str();
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	//printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 )
    {
		std::vector<char> VertexShaderErrorMessage(InfoLogLength+1);
		glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);

		std::wstring wstrn;
		for( int i = 0; i < VertexShaderErrorMessage.size(); ++i )
			wstrn += VertexShaderErrorMessage[i];

		std::wcout << wstrn;
		//printf("%s\n", &VertexShaderErrorMessage[0]);
	}

	// Compile Fragment Shader
	//printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 )
    {
		std::vector<char> FragmentShaderErrorMessage(InfoLogLength+1);
		glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);

		std::wstring wstrn;
		for( int i = 0; i < FragmentShaderErrorMessage.size(); ++i )
			wstrn += FragmentShaderErrorMessage[ i ];

		std::wcout << wstrn;
		// 
		//printf("%s\n", &FragmentShaderErrorMessage[0]);
	}

	// Link the program
	//printf("Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 )
    {
		std::vector<char> ProgramErrorMessage(InfoLogLength+1);
		glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		//printf("%s\n", &ProgramErrorMessage[0]);
	}
	
	glDetachShader(ProgramID, VertexShaderID);
	glDetachShader(ProgramID, FragmentShaderID);
	
	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

#define FOURCC_DXT1 0x31545844 // Equivalent to "DXT1" in ASCII
#define FOURCC_DXT3 0x33545844 // Equivalent to "DXT3" in ASCII
#define FOURCC_DXT5 0x35545844 // Equivalent to "DXT5" in ASCII

//Loads a DDS texture for OpenGL use,
GLuint LoadDDS(const char * imagepath)
{
	unsigned char header[124];

	FILE *fp; 
 
	/* try to open the file */ 
	fopen_s( &fp, imagepath, "rb"); 
	if (fp == NULL)
    {
		printf("%s could not be opened. Are you in the right directory ? Don't forget to read the FAQ !\n", imagepath); getchar(); 
		return 0;
	}
   
	/* verify the type of file */ 
	char filecode[4]; 
	fread(filecode, 1, 4, fp); 
	if (strncmp(filecode, "DDS ", 4) != 0)
    { 
		fclose(fp); 
		return 0; 
	}
	
	/* get the surface desc */ 
	fread(&header, 124, 1, fp); 

	unsigned int height      = *(unsigned int*)&(header[8 ]);
	unsigned int width	     = *(unsigned int*)&(header[12]);
	unsigned int linearSize	 = *(unsigned int*)&(header[16]);
	unsigned int mipMapCount = *(unsigned int*)&(header[24]);
	unsigned int fourCC      = *(unsigned int*)&(header[80]);

 
	unsigned char * buffer;
	unsigned int bufsize;
	/* how big is it going to be including all mipmaps? */ 
	bufsize = mipMapCount > 1 ? linearSize * 2 : linearSize; 
	buffer = (unsigned char*)malloc(bufsize * sizeof(unsigned char)); 
	fread(buffer, 1, bufsize, fp); 
	/* close the file pointer */ 
	fclose(fp);

	unsigned int components  = (fourCC == FOURCC_DXT1) ? 3 : 4; 
	unsigned int format;
	switch(fourCC) 
	{ 
	case FOURCC_DXT1: 
		format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT; 
		break; 
	case FOURCC_DXT3: 
		format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; 
		break; 
	case FOURCC_DXT5: 
		format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; 
		break; 
	default: 
		free(buffer); 
		return 0; 
	}

	// Create one OpenGL texture
	GLuint textureID;
	glGenTextures(1, &textureID);

	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture(GL_TEXTURE_2D, textureID);
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);	
	
	unsigned int blockSize = (format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ? 8 : 16; 
	unsigned int offset = 0;

	/* load the mipmaps */ 
	for (unsigned int level = 0; level < mipMapCount && (width || height); ++level) 
	{ 
		unsigned int size = ((width+3)/4)*((height+3)/4)*blockSize; 
		glCompressedTexImage2D(GL_TEXTURE_2D, level, format, width, height,  
			0, size, buffer + offset); 
	 
		offset += size; 
		width  /= 2; 
		height /= 2; 

		// Deal with Non-Power-Of-Two textures. This code is not included in the webpage to reduce clutter.
		if(width < 1) width = 1;
		if(height < 1) height = 1;

	} 

	free(buffer); 

	return textureID;
}

//Loads a DDS texture for OpenGL use, Reimplementation that uses a char buffer as a "file".
GLuint LoadDDS_FromBuffer( std::vector< char > byteBuf )
{
	unsigned char header[124];

    int pos = 0;
   
	/* verify the type of file */ 
	char filecode[4];
    filecode[0] = byteBuf[0];
    filecode[1] = byteBuf[1];
    filecode[2] = byteBuf[2];
    filecode[3] = byteBuf[3];

    pos += 4;

	if (strncmp(filecode, "DDS ", 4) != 0)
    { 
		return 0; 
	}
	
	/* get the surface desc */ 
    for( int i = 0; i < 124; ++i )
    {
        header[i] = byteBuf[ pos ];
        pos++;
    }

	unsigned int flags = *(unsigned int*)&(header[4]);
	unsigned int height      = *(unsigned int*)&(header[8 ]);
	unsigned int width	     = *(unsigned int*)&(header[12]);
	unsigned int linearSize	 = *(unsigned int*)&(header[16]);
	unsigned int mipMapCount = *(unsigned int*)&(header[24]);
	unsigned int fourCC      = *(unsigned int*)&(header[80]);


	unsigned char * buffer;
	unsigned int bufsize;

	unsigned int components  = (fourCC == FOURCC_DXT1) ? 3 : 4; 
	unsigned int format;
	switch(fourCC) 
	{ 
	case FOURCC_DXT1: 
		format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT; 
		break; 
	case FOURCC_DXT3: 
		format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; 
		break; 
	case FOURCC_DXT5: 
		format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; 
		break; 
	default: 
		return 0; 
	}

	unsigned int blockSize = (format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ? 8 : 16;
	unsigned int offset = 0;

	/* how big is it going to be including all mipmaps? */

	if (flags & 0x80000)
		bufsize = mipMapCount > 1 ? linearSize * 2 : linearSize;
	else
		bufsize = (((width + 3) / 4 * 4) * ((height + 3) / 4 * 4)) / 4 * blockSize;;

	buffer = (unsigned char*)malloc(bufsize * sizeof(unsigned char));

	for (int i = 0; i < bufsize; ++i)
	{
		if (pos >= byteBuf.size())
			break; //ERROR

		buffer[i] = byteBuf[pos];
		pos++;
	}

	// Create one OpenGL texture
	GLuint textureID;
	glGenTextures(1, &textureID);

	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture(GL_TEXTURE_2D, textureID);
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);	
	


	/* load the mipmaps */ 
	for (unsigned int level = 0; level < mipMapCount && (width || height); ++level) 
	{ 
		unsigned int size = ((width+3)/4)*((height+3)/4)*blockSize; 
		glCompressedTexImage2D(GL_TEXTURE_2D, level, format, width, height,  
			0, size, buffer + offset); 
	 
		offset += size; 
		width  /= 2; 
		height /= 2; 

		// Deal with Non-Power-Of-Two textures. This code is not included in the webpage to reduce clutter.
		if(width < 1) width = 1;
		if(height < 1) height = 1;

	} 

	free(buffer); 

	return textureID;
}

//###################################################################
//Model Rendering
//###################################################################

MeshObject::MeshObject( std::vector< glm::vec3 > verts, std::vector< unsigned int > ind, std::vector< glm::vec2 > uv, GLuint shader, GLuint tex )
{
	//Fill in data:
	vertices = verts;
	indices = ind;
	uvs = uv;
	shaderID = shader;

	//Load Texture:
	Texture = tex;

	// Get a handle for our "myTextureSampler" uniform
	TextureID = glGetUniformLocation(shaderID, "myTextureSampler");

	// create buffers/arrays
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	glGenBuffers(1, &UVBuffer );

	glBindVertexArray(VAO);
	
	// load data into vertex buffers
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW); 

	//Bind UVS
	glBindBuffer(GL_ARRAY_BUFFER, UVBuffer);
	glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW); 

	//Buffer indices to EBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);


	// set the vertex attribute pointers
	// vertex Positions
	glEnableVertexAttribArray(0);	
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);

	glBindVertexArray(0);

	//Fill in transforms with 0
	position = glm::vec3( 0, 0, 0 );
	angles = glm::vec3( 0, 0, 0 );
};

//Destructor. TODO: Clean up memory and OPENGL states here.
MeshObject::~MeshObject()
{
	//At least purge our mesh vectors.
	vertices.clear();
	uvs.clear();
	indices.clear();

	//Delete vertex arrays and buffers
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	glDeleteBuffers(1, &UVBuffer);
};

#include <glm/gtx/rotate_vector.hpp>

//Render the mesh using a specified camera.
void MeshObject::Draw( Camera cam )
{
	//Create model matrix from transforms.
	glm::mat4 Model = glm::mat4( 1.0f );

	Model = glm::translate( Model, position );

	//TODO: This needs to be better.
	Model = glm::rotate( Model, glm::radians( angles.x ), glm::vec3( 1, 0, 0 ) );
	Model = glm::rotate( Model, glm::radians( angles.y ), glm::vec3( 0, 1, 0 ) );
	Model = glm::rotate( Model, glm::radians( angles.z ), glm::vec3( 0, 0, 1 ) );

	// Use our shader
	glUseProgram(shaderID);

	//Set up MVP:
	glm::mat4 mvp = cam.Projection * cam.View * Model;

	// Get a handle for our "MVP" uniform
	// Only during the initialisation
	GLuint MatrixID = glGetUniformLocation( shaderID, "MVP" );
	
	// Send our transformation to the currently bound shader, in the "MVP" uniform
	glUniformMatrix4fv( MatrixID, 1, GL_FALSE, &mvp[0][0] );

	// Set our "myTextureSampler" sampler to use Texture Unit 0
	glUniform1i(TextureID, 0);

	// Bind our texture in Texture Unit 0
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, Texture);

	// draw mesh
	glBindVertexArray(VAO);

	// 1rst attribute buffer : vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glVertexAttribPointer(
		0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
	);

	// 2nd attribute buffer : UVs
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, UVBuffer);
	glVertexAttribPointer(
		1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
		2,                                // size : U+V => 2
		GL_FLOAT,                         // type
		GL_FALSE,                         // normalized?
		0,                                // stride
		(void*)0                          // array buffer offset
	);

	//Todo: Send bone wieghts and transforms.

	glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);

	glBindVertexArray(0);

	// always good practice to set everything back to defaults once configured.
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

//##############################################################
//
//##############################################################
#include "RAB.h"
#include "MDBParser.h"

std::vector< glm::vec3 > GetVerticesFromMeshInfo( MDBMeshInfo* mesh )
{
	std::vector< glm::vec3 > returnValue;

	//Find the vertex layout type of 'position'
	int id = 0;
	for( int i = 0; i < mesh->layoutcount; ++i )
	{
		if( mesh->vertexinfo[ i ].name == "position" )
		{
			id = i;
		}
	}

	int ofs = 4;
	if( mesh->vertexinfo[ id ].type == 4 )
		ofs = 3;

	for( int i = 0; i < mesh->vertexdata[ "position0" ].size(); i += ofs ) //This should be fine in most cases. However, I should test for "type"
	{
		float x = mesh->vertexdata[ "position0" ][ i ];
		float y = mesh->vertexdata[ "position0" ][ i + 1 ];
		float z = mesh->vertexdata[ "position0" ][ i + 2 ];

		returnValue.push_back( glm::vec3( x, y, z ) );
	}

	return returnValue;
}

std::vector< int > GetBlendIndicesFromMeshInfo( MDBMeshInfo* mesh )
{
	std::vector< int > returnValue;

	for( int i = 0; i < mesh->vertexdata[ "BLENDINDICES0" ].size(); i += 1 ) //This should be fine in most cases. However, I should test for "type"
	{
		int index = mesh->vertexdata[ "BLENDINDICES0" ][ i ];

		returnValue.push_back( index );
	}

	return returnValue;

	//Find the vertex layout type of 'position'
	int id = 0;
	for( int i = 0; i < mesh->layoutcount; ++i )
	{
		if( mesh->vertexinfo[ i ].name == "BLENDINDICES" )
		{
			id = i;
		}
	}

	int ofs = 4;
	if( mesh->vertexinfo[ id ].type == 4 )
		ofs = 3;

	for( int i = 0; i < mesh->vertexdata[ "BLENDINDICES0" ].size(); i += ofs ) //This should be fine in most cases. However, I should test for "type"
	{
		int index = mesh->vertexdata[ "BLENDINDICES0" ][ i ];

		returnValue.push_back( index );
	}

	return returnValue;
}

std::vector< float > GetBlendWeightFromMeshInfo( MDBMeshInfo* mesh )
{
	std::vector< float > returnValue;

	for( int i = 0; i < mesh->vertexdata[ "BLENDWEIGHT0" ].size(); i += 1 )
	{
		float weight = mesh->vertexdata[ "BLENDWEIGHT0" ][ i ];

		returnValue.push_back( weight );
	}

	return returnValue;

	//Find the vertex layout type of 'position'
	int id = 0;
	for( int i = 0; i < mesh->layoutcount; ++i )
	{
		if( mesh->vertexinfo[ i ].name == "BLENDWEIGHT" )
		{
			id = i;
		}
	}

	int ofs = 4;
	if( mesh->vertexinfo[ id ].type == 4 )
		ofs = 3;

	for( int i = 0; i < mesh->vertexdata[ "BLENDWEIGHT0" ].size(); i += ofs ) //This should be fine in most cases. However, I should test for "type"
	{
		float weight = mesh->vertexdata[ "BLENDWEIGHT0" ][ i ];

		returnValue.push_back( weight );
	}

	return returnValue;
}

//I don't like it much, but...
std::wstring GetTextureName( MDB *model, MDBMeshInfo* mesh )
{
	std::wstring returnValue;

	//Look up Material:
	int materialID = mesh->material;

	//Safty check.
	if( materialID > model->materialscount )
		return returnValue;

	for( int i = 0; i < model->materials[ materialID ].textureCount; ++i )
	{
		if( model->materials[ materialID ].textures[ i ].type == "albedo" )
		{
			int textureID = model->materials[ materialID ].textures[ i ].textureIndex;
			return model->textures[ textureID ].filename;
		}
	}

	return returnValue;
}

CMDBMesh::CMDBMesh( MDBMeshInfo* info, CMDBRenderer *parent )
{
	pMeshInfo = info;
	pParent = parent;

	glUseProgram( pParent->shaderID );

	// Get a handle for our "myTextureSampler" uniform
	textureID = glGetUniformLocation( pParent->shaderID, "myTextureSampler" );

	//Create buffers/arrays
	glGenVertexArrays( 1, &VAO );
	glGenBuffers( 1, &VBO );
	glGenBuffers( 1, &UVBuffer );
	glGenBuffers( 1, &BoneIDS );
	glGenBuffers( 1, &VertexWeights );
	glGenBuffers( 1, &EBO );

	glBindVertexArray( VAO );

	std::vector< glm::vec3 > vertices = GetVerticesFromMeshInfo( pMeshInfo );
	std::vector< int > bones = GetBlendIndicesFromMeshInfo( pMeshInfo );
	std::vector< float > blendweights = GetBlendWeightFromMeshInfo( pMeshInfo );

	// load data into vertex buffers
	glBindBuffer( GL_ARRAY_BUFFER, VBO );
	glBufferData( GL_ARRAY_BUFFER, vertices.size() * sizeof( glm::vec3 ), &vertices[ 0 ], GL_STATIC_DRAW );

	//Bind UVS
	glBindBuffer( GL_ARRAY_BUFFER, UVBuffer );
	glBufferData( GL_ARRAY_BUFFER, pMeshInfo->vertexdata[ "texcoord0" ].size() * sizeof( float ), pMeshInfo->vertexdata[ "texcoord0" ].data(), GL_STATIC_DRAW);

	//Bind Bones
	glBindBuffer( GL_ARRAY_BUFFER, BoneIDS );
	glBufferData( GL_ARRAY_BUFFER, bones.size() * sizeof( int ), &bones[0], GL_STATIC_DRAW);

	//Bind weights
	glBindBuffer( GL_ARRAY_BUFFER, VertexWeights );
	glBufferData( GL_ARRAY_BUFFER, blendweights.size() * sizeof( float ), &blendweights[ 0 ], GL_STATIC_DRAW );

	//Buffer indices to EBO
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, EBO );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, pMeshInfo->indicedata.size() * sizeof( unsigned int ), &pMeshInfo->indicedata[ 0 ], GL_STATIC_DRAW );


	// set the vertex attribute pointers
	// vertex Positions
	glEnableVertexAttribArray( 0 );
	glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, nullptr );

	glEnableVertexAttribArray( 1 );
	glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, 0, nullptr );

	glEnableVertexAttribArray( 2 );
	glVertexAttribIPointer( 2, 4, GL_INT, 0, nullptr );

	glEnableVertexAttribArray( 3 );
	glVertexAttribPointer( 3, 4, GL_FLOAT, GL_FALSE, 0, nullptr );

	glDisableVertexAttribArray( 0 );
	glDisableVertexAttribArray( 1 );
	glDisableVertexAttribArray( 2 );
	glDisableVertexAttribArray( 3 );

	glBindVertexArray( 0 );
};

CMDBMesh::~CMDBMesh( )
{
	//Delete vertex arrays and buffers
	glDeleteVertexArrays( 1, &VAO );
	glDeleteBuffers( 1, &VBO );
	glDeleteBuffers( 1, &EBO );
	glDeleteBuffers( 1, &UVBuffer );
	glDeleteBuffers( 1, &VertexWeights );
	glDeleteBuffers( 1, &textureID );
};

#include <glm/gtc/type_ptr.hpp>
void CMDBMesh::Draw( Camera cam )
{
	//Create model matrix from transforms.
	glm::mat4 Model = glm::mat4( 1.0f );

	Model = glm::translate( Model, pParent->position );

	//TODO: This needs to be better.
	Model = glm::rotate( Model, glm::radians( pParent->angles.x ), glm::vec3( 1, 0, 0 ) );
	Model = glm::rotate( Model, glm::radians( pParent->angles.y ), glm::vec3( 0, 1, 0 ) );
	Model = glm::rotate( Model, glm::radians( pParent->angles.z ), glm::vec3( 0, 0, 1 ) );

	// Use our shader
	glUseProgram( pParent->shaderID );

	//Set up MVP:
	glm::mat4 mvp = cam.Projection * cam.View * Model;

	// Get a handle for our "MVP" uniform
	// Only during the initialisation
	GLuint MatrixID = glGetUniformLocation( pParent->shaderID, "MVP" );
	
	//construct bone matrix:
	std::vector< glm::mat4 > boneWorldTransforms;
	for( int i = 0; i < pParent->model->model.bonescount; ++i )
	{
		if( pParent->model->model.bones[ i ].boneParent != -1 )
		{
			//Obvious, in retrospect. Each bone transforms itself relative to its parent to get its actual coords.
			//However, the extra data is unknown.

			int parentIndex = pParent->model->model.bones[ pParent->model->model.bones[ i ].boneParent ].boneIndex;
			boneWorldTransforms.push_back( ( boneWorldTransforms[ parentIndex ] * pParent->model->model.bones[ i ].matrix ) );
		}
		else
		{
			boneWorldTransforms.push_back( pParent->model->model.bones[ i ].matrix );
		}
	}

	//Apply bind pose
	for( int i = 0; i < pParent->model->model.bonescount; ++i )
	{
		boneWorldTransforms[ i ] *= pParent->model->model.bones[ i ].matrix2;
	}

	// Send our transformation to the currently bound shader, in the "MVP" uniform
	glUniformMatrix4fv( MatrixID, 1, GL_FALSE, &mvp[ 0 ][ 0 ] );

	/*
	for( int i = 0; i < boneWorldTransforms.size(); ++i )
	{
		// Send our bone matrix to the currently bound shader, in the "finalBonesMatrices" uniform
		std::string str = "finalBonesMatrices[" + std::to_string( i ) + "]";
		GLuint location = glGetUniformLocation( pParent->shaderID, str.c_str() );
		glUniformMatrix4fv( location, 1, GL_FALSE, glm::value_ptr( boneWorldTransforms[ i ] ) );
	}
	*/

	GLuint location = glGetUniformLocation( pParent->shaderID, "finalBonesMatrices" );
	glUniformMatrix4fv( location, boneWorldTransforms.size(), GL_FALSE, glm::value_ptr( boneWorldTransforms[0] ) );

	// Set our "myTextureSampler" sampler to use Texture Unit 0
	glUniform1i( textureID, 0 );


	// Bind our texture in Texture Unit 0
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, pParent->textureMap[ GetTextureName( &pParent->model->model, pMeshInfo ) ]);

	// draw mesh
	glBindVertexArray( VAO );

	// 1rst attribute buffer : vertices
	glEnableVertexAttribArray( 0 );
	glBindBuffer( GL_ARRAY_BUFFER, VBO );
	glVertexAttribPointer(
		0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
	);

	// 2nd attribute buffer : UVs
	glEnableVertexAttribArray( 1 );
	glBindBuffer( GL_ARRAY_BUFFER, UVBuffer );
	glVertexAttribPointer(
		1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
		2,                                // size : U+V => 2
		GL_FLOAT,                         // type
		GL_FALSE,                         // normalized?
		0,                                // stride
		(void*)0                          // array buffer offset
	);

	// 2rd attribute buffer : BlendIndices
	glEnableVertexAttribArray( 2 );
	glBindBuffer( GL_ARRAY_BUFFER, BoneIDS );
	glVertexAttribIPointer(
		2,                                // attribute. No particular reason for 1, but must match the layout in the shader.
		4,                                // size :
		GL_INT,                         // type
		0,                                // stride
		(void*)0                          // array buffer offset
	);

	// 4th attribute buffer : BlendWeights
	glEnableVertexAttribArray( 3 );
	glBindBuffer( GL_ARRAY_BUFFER, VertexWeights );
	glVertexAttribPointer(
		3,                                // attribute. No particular reason for 1, but must match the layout in the shader.
		4,                                // size: 
		GL_FLOAT,                         // type
		GL_FALSE,                         // normalized?
		0,                                // stride
		(void*)0                          // array buffer offset
	);

	glDrawElements( GL_TRIANGLES, static_cast<unsigned int>(pMeshInfo->indicedata.size()), GL_UNSIGNED_INT, 0 );

	glDisableVertexAttribArray( 0 );
	glDisableVertexAttribArray( 1 );
	glDisableVertexAttribArray( 2 );
	glDisableVertexAttribArray( 3 );

	glBindVertexArray( 0 );

	// always good practice to set everything back to defaults once configured.
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, 0 );
}

CMDBObject::CMDBObject( MDBObject* obj, CMDBRenderer *parent )
{
	pObjInfo = obj;

	for( int i = 0; i < pObjInfo->meshcount; ++i )
	{
		meshs.push_back( std::make_unique< CMDBMesh >( &pObjInfo->meshs[i], parent ) );
	}
};

void CMDBObject::Draw( Camera cam )
{
	for( auto& mesh : meshs )
	{
		mesh->Draw( cam );
	}
}

#include "CAS.h"
#include<glm/gtc/quaternion.hpp>

CMDBRenderer::CMDBRenderer(  )
{
	//TEMP: For now, hardcode this.
	//Load data
	RABReader mdlArc( "EDFData/ARMYSOLDIER.MRAB" );
	model = std::make_unique<MDBReader>( mdlArc.ReadFile( L"MODEL", L"p501_ranger.mdb" ) );
	CAS cas = CAS( "EDFData/ARMYSOLDIER.CAS" );

	shaderID = ShaderList::LoadShader( "SimpleTexturedBones", "SimpleVertexShaderBones.txt", "SimpleTexturedFragShader.txt" ); //Todo: Common rendering library?

	//Obtain textures
	//Todo: Optimise this by running it concurrently with the object loader?
	for( int i = 0; i < model->model.objectscount; ++i )
	{
		for( int j = 0; j < model->model.objects[ i ].meshcount; ++j )
		{
			//Test folder names:
			std::wstring folder = L"HD-TEXTURE";
			if( !mdlArc.HasFolder( folder ) )
			{
				folder = L"TEXTURE";
			}
			//Add to map if non duplicate
			if( textureMap.count( model->GetColourTextureFilename( i, j ) ) == 0 )
			{
				std::vector< char > textureBytes;
				std::wstring texFileName = model->GetColourTextureFilename( i, j );
				textureBytes = mdlArc.ReadFile( L"HD-TEXTURE", texFileName );
				textureMap[ texFileName ] = LoadDDS_FromBuffer( textureBytes );
				textureBytes.clear();
			}
		}
	}

	//Create all objects
	objects.push_back( std::make_unique< CMDBObject >( &model->model.objects[0], this ));

	//construct bone matrix:
	std::vector< glm::mat4 > boneWorldTransforms;

	for( int i = 0; i < model->model.bonescount; ++i )
	{
		if( model->model.bones[ i ].boneParent != -1 )
		{
			//Obvious, in retrospect. Each bone transforms itself relative to its parent to get its actual coords.
			//However, the extra data is unknown.
			int anim = 11;

			for( int j = 0; j < cas.canm.animData[ anim ].dataCount; ++j )
			{
				if( model->model.bones[ i ].name == cas.canm.vecBoneList[ cas.canm.animData[ anim ].dataGroup[j].boneIndex ] )
				{
					float x = cas.canm.animPoints[ cas.canm.animData[ anim ].dataGroup[ j ].rotateAnimPointIndex ].x;
					float y = cas.canm.animPoints[ cas.canm.animData[ anim ].dataGroup[ j ].rotateAnimPointIndex ].y;
					float z = cas.canm.animPoints[ cas.canm.animData[ anim ].dataGroup[ j ].rotateAnimPointIndex ].z;

					glm::quat rotation = glm::quat( glm::vec3( x, y, z ) );
					glm::mat4 matrix = glm::mat4_cast( rotation );

					model->model.bones[ i ].matrix *= matrix;

					//pParent->model->model.bones[ i ].matrix = glm::rotate( pParent->model->model.bones[ i ].matrix, glm::radians( 1.0f ), glm::vec3( 1.0f, 0.0f, 1.0f ) );
				}
			}

			int parentIndex = model->model.bones[ model->model.bones[ i ].boneParent ].boneIndex;
			glm::vec4 pos1 = glm::vec4( 0.0f );
			glm::vec4 pos2 = glm::vec4( 0.0f );

			pos1.w = 1;
			pos2.w = 1;

			boneWorldTransforms.push_back( boneWorldTransforms[ parentIndex ] * model->model.bones[ i ].matrix );

			pos1 = boneWorldTransforms[ parentIndex ] * pos1;
			pos2 = boneWorldTransforms[ i ] * pos2;

			lines.push_back( std::make_unique<CDebugLine>( pos1, pos2, glm::vec3( 0, 255, 0 ) ) );

			//Visualise the "Reset to zero" transform
			//pos2 = model.model.bones[i].matrix2 * pos2;
			//lines.push_back( std::make_unique<CDebugLine>( pos1, pos2, glm::vec3( 255, 255, 0 ) ) );
		}
		else
		{
			boneWorldTransforms.push_back( model->model.bones[ i ].matrix );
		}
	}
};

//Destructor. TODO: Clean up memory and OPENGL states here.
CMDBRenderer::~CMDBRenderer()
{
};


//Render the mesh using a specified camera.
void CMDBRenderer::Draw( Camera cam )
{
	for( auto & obj : objects )
	{
		obj->Draw( cam );
	}

	for( auto& l : lines )
	{
		l->Draw( cam );
	}
}
