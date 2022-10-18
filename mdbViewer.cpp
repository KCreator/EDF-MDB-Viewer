#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string.h>
#include <codecvt>
#include <locale>

//OPENGL INCLUDE
#include <SFML/OpenGL.hpp>
#include <SFML/Window.hpp>
#include <GLES3/gl3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <sstream>

#include "MDBParser.h"

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
		printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", vertex_file_path);
		getchar();
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
	printf("Compiling shader : %s\n", vertex_file_path);
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
		printf("%s\n", &VertexShaderErrorMessage[0]);
	}

	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
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
		printf("%s\n", &FragmentShaderErrorMessage[0]);
	}

	// Link the program
	printf("Linking program\n");
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
		printf("%s\n", &ProgramErrorMessage[0]);
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
	fp = fopen(imagepath, "rb"); 
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

    int pos;
   
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

    for( int i = 0; i < bufsize; ++i )
    {
        buffer[i] = byteBuf[ pos ];
        pos++;
    }

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
    MeshObject( std::vector< glm::vec3 > verts, std::vector< unsigned int > ind, std::vector< glm::vec2 > uv, GLuint shader, GLuint tex )
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
	~MeshObject()
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

	//Render the mesh using a specified camera.
    void Draw( Camera cam )
    {
        //Create model matrix from transforms.
        glm::mat4 Model = glm::mat4( 1.0f );

        Model = glm::translate( Model, position );

        //TODO: This needs to be better.
        Model = glm::rotate( Model, angles.x, glm::vec3( 1, 0, 0 ) );
        Model = glm::rotate( Model, angles.y, glm::vec3( 0, 1, 0 ) );
        Model = glm::rotate( Model, angles.z, glm::vec3( 0, 0, 1 ) );

        // Use our shader
		glUseProgram(shaderID);

        //Set up MVP:
        glm::mat4 mvp = cam.Projection * cam.View * Model;

        // Get a handle for our "MVP" uniform
        // Only during the initialisation
        GLuint MatrixID = glGetUniformLocation( shaderID, "MVP" );
        
        // Send our transformation to the currently bound shader, in the "MVP" uniform
        // This is done in the main loop since each model will have a different MVP matrix (At least for the M part)
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


        glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);

        glBindVertexArray(0);

        // always good practice to set everything back to defaults once configured.
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

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

//Todo: Implement this so that we can draw lines in 3D space.
//Ugly little debug line renderer.
class CDebugLine
{
	public:
	CDebugLine( glm::vec3 pointA, glm::vec3 pointB, glm::vec3 colour )
	{
		//Hardcoded shaders. Ideally we would move all this junk somewhere else.
		const char *vertexShaderSource = "#version 330 core\n"
            "layout (location = 0) in vec3 aPos;\n"
            "uniform mat4 MVP;\n"
            "void main()\n"
            "{\n"
            "   gl_Position = MVP * vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
            "}\0";
        const char *fragmentShaderSource = "#version 330 core\n"
            "out vec4 FragColor;\n"
            "uniform vec3 color;\n"
            "void main()\n"
            "{\n"
            "   FragColor = vec4(color, 1.0f);\n"
            "}\n\0";

        // vertex shader
        int vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);
        // check for shader compile errors

        // fragment shader
        int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);
        // check for shader compile errors

        // link shaders
        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);
        // check for linking errors

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        vertices = {
             pointA.x, pointA.y, pointA.z,
             pointB.x, pointB.y, pointB.z,

        };
        
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0); 
        glBindVertexArray(0);

		col = colour;
	};
	~CDebugLine()
	{
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteProgram(shaderProgram);
    }
	
	void Draw( Camera cam )
	{
        glm::mat4 Model = glm::mat4( 1.0f ); //We dont need to do much here.
		glm::mat4 mvp = cam.Projection * cam.View * Model;

		glUseProgram(shaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "MVP"), 1, GL_FALSE, &mvp[0][0]);
        glUniform3fv(glGetUniformLocation(shaderProgram, "color"), 1, &col[0]);

        glBindVertexArray(VAO);
        glDrawArrays(GL_LINES, 0, 2);
        glBindVertexArray(0);
	};

protected:
	GLuint VAO;
    GLuint VBO;
	std::vector<float> vertices;
	int shaderProgram;
	glm::vec3 col;
};

//Todo: Primitive renderers for if we implement RMPA node viewing?

#include <SFML/Graphics.hpp>

#include "RAB.h"
#include <filesystem>

//Tool state system, will allow the tool to have multiple "screens", IE, a file browser, that it can swap between
class CToolStateHandler;

class BaseToolState
{
public:
	BaseToolState()
	{
		pOwner = NULL;
	};

	virtual void Init( sf::RenderWindow *iwindow )
	{
		window = iwindow;
	};
	virtual void ProccessEvent( sf::Event event ){};
	virtual void Draw(){};

	void SetOwner( CToolStateHandler *owner )
	{
		pOwner = owner;
	};

protected:
	sf::RenderWindow *window;
	CToolStateHandler *pOwner;
};

//Ugly little State manager system
class CToolStateHandler
{
public:
	~CToolStateHandler()
	{
		state_map.clear(); //Purge map and delete elements.
	}

	void AddState( std::string name, BaseToolState *state )
	{
		if( state_map.count( name ) == 0 ) //We do not already have a state of this name:
		{
			state_map[ name ] = state;
			state->SetOwner( this );
		}
	};
	BaseToolState * GetState( std::string name )
	{
		return state_map[ name ];
	};

	void SetState( std::string state )
	{
		curState = state;
	};

	void ProccessEvent( sf::Event e )
	{
		if( state_map.count( curState ) > 0 )
		{
			state_map[ curState ]->ProccessEvent( e );
		}
	};
	void Draw()
	{
		if( state_map.count( curState ) > 0 )
		{
			state_map[ curState ]->Draw();
		}
	};

	std::map< std::string, BaseToolState * > state_map;

private:
	std::string curState;
};

//Tool state is in "Model Renderer"
class CStateModelRenderer : public BaseToolState
{
	public:
	~CStateModelRenderer()
	{
		meshs.clear(); //Lets hope this call the destructor.

		//TODO: Kill shaders?
	};

	void Init( sf::RenderWindow *iwindow )
	{
		window = iwindow;

		//Todo: Reference font in some kind of common way.
    	font.loadFromFile( "Font.ttf" );

		text = sf::Text( L"NO MESH LOADED", font, 20u );
		text.setFillColor( sf::Color( 0, 255, 0 ) );
		text.setPosition( sf::Vector2f( 5, 5 ) );

		strReadoutText = "Rendering 1 Mesh(s) at position:";
		meshObjReadoutText = sf::Text( strReadoutText, font, 15U );
		meshObjReadoutText.setFillColor( sf::Color( 255, 0, 0 ) );
		meshObjReadoutText.setPosition( sf::Vector2f( 5, 600-15-5 ) );

		//Init OPENGL related stuff;

		//Enable Culling
		glEnable( GL_CULL_FACE );
		glFrontFace( GL_CCW );
		
		// Create and compile our GLSL program from the shaders
		programID = LoadShaders( "SimpleVertexShader.txt", "SimpleTexturedFragShader.txt" ); //Todo: Common rendering library?
		shader_Untextured = LoadShaders( "SimpleVertexShader.txt", "SimpleFragmentShader.txt" );

		//mesh = std::make_unique<MeshObject>( reader.GetMeshPositionVertices(0, 0), reader.GetMeshIndices(0, 0), reader.uvs, programID, LoadDDS("texture1.dds") ); //Todo: Perhaps use new/pointers/ect.
		
		//Try to form a bone map:
		//lines.push_back( std::make_unique< CDebugLine >( reader.bonepos[1], reader.bonepos[0], glm::vec3( 0, 0, 255 ) ) );
		//lines.push_back( std::make_unique< CDebugLine >( reader.bonepos[2], reader.bonepos[0], glm::vec3( 0, 0, 255 ) ) );
		//lines.push_back( std::make_unique< CDebugLine >( reader.bonepos[3], reader.bonepos[2], glm::vec3( 0, 0, 255 ) ) );

		//mesh.Texture = loadDDS_FromBuffer( modelArc.ReadFile( L"HD-TEXTURE", reader.model.textures[0].filename ) );
		//MeshObject mesh2( reader.test, reader.test2, reader.uvs, programID );
		//mesh.shaderID = programID;

		// Enable depth test
		glEnable( GL_DEPTH_TEST );
		// Accept fragment if it closer to the camera than the former one
		glDepthFunc( GL_LEQUAL );

		//Final var init;
		isDragging = false;
		bUseWireframe = false;
		fov = 45.0f;
		cameraPosition = glm::vec3( 2, 0, 0 );
	};

	void ProccessEvent( sf::Event event )
	{
		if( event.type == sf::Event::Resized )
		{
			// adjust the viewport when the window is resized
			glViewport(0, 0, event.size.width, event.size.height);
		}
		else if( event.type == sf::Event::KeyPressed ) //Temp:
		{
			if( event.key.code == sf::Keyboard::F )
			{
				bUseWireframe = !bUseWireframe;
				//Not the best solution, but here we are.
				for( int i = 0; i < meshs.size(); ++i )
				{
					meshs[i]->shaderID = bUseWireframe ? shader_Untextured : programID;
					meshs[i]->TextureID = glGetUniformLocation(meshs[i]->shaderID, "myTextureSampler");
				}
			}
			else if( event.key.code == sf::Keyboard::Down )
				cameraPosition.x += 0.1;
		}

		if( event.type == sf::Event::MouseButtonPressed )
		{
			//Additional checks can go here.

			if( event.mouseButton.button == sf::Mouse::Button::Left )
			{
				mouseOldPos = sf::Mouse::getPosition( *window );
				isDragging = true;
			}
		}
		else if( event.type == sf::Event::MouseButtonReleased )
		{
			//Additional checks can go here.

			if( event.mouseButton.button == sf::Mouse::Button::Left && isDragging )
			{
				isDragging = false;
			}
		}
		else if( event.type == sf::Event::MouseMoved && isDragging )
		{
			sf::Vector2i mousePos = sf::Mouse::getPosition( *window );
			for( int i = 0; i < meshs.size(); ++i )
				meshs[i]->angles.y -= (float)(mouseOldPos.x - mousePos.x) / 1000.0f;

			mouseOldPos = mousePos;
		}
	}

	void Draw()
	{
		// Projection matrix : 45° Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
        cam.Projection = glm::perspective( glm::radians(45.0f), (float)800 / (float)600, 0.1f, 100.0f );
        
        // Camera matrix
        cam.View = glm::lookAt(
            cameraPosition, // Camera position, in World Space
            glm::vec3(0,0,0), // Camera Look At position
            glm::vec3(0,1,0)  // Head is up (set to 0,-1,0 to look upside-down)
            );
        
        //strReadoutText = "Rendering 1 'object' at position: " + std::to_string( mesh->position.x ) + ", " +
        //std::to_string( mesh->position.y ) + ", " + std::to_string( mesh->position.z );
        //meshObjReadoutText.setString( strReadoutText );

        window->clear();

        glClear( GL_DEPTH_BUFFER_BIT );

        //Turn on wireframe mode
		if( bUseWireframe )
        	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        
		//Draw mesh(s)
		for( int i = 0; i < meshs.size(); ++i )
        	meshs[i]->Draw( cam );

		//std::cout << glGetError() + "\n";

        //Turn off wireframe mode
		if( bUseWireframe )
        	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		//for( const auto & line : lines )
		//{
		//	line->Draw( cam );
		//}

		glUseProgram(0);

        window->pushGLStates();
        window->draw( text );
        window->draw( meshObjReadoutText );
        window->popGLStates();
    
        window->display();
	}

	void LoadMDB( std::string ipath )
	{
		//Proccess:
		std::filesystem::path fsPath = ipath;

		if( fsPath.extension() == ".mdb" ) //Load directly from MDB file
		{
			//Load mdb
			//TODO: Load this more dynamically, through some kind of user input.
			MDBReader reader( ipath.c_str() ); //Todo: Memory management.

			//Text information about the viewer
			std::wstring strObjName = reader.model.names[ reader.model.objects[0].nameindex ];
			std::wstring strMeshInfo = L"Object 1 of " + std::to_wstring( reader.model.objectscount );
			std::wstring strVertexInfo = L"Number of vertices (MESH 1): " + std::to_wstring( reader.model.objects[0].meshs[0].vertexnumber );

			text.setString( L"Displaying: " + strObjName + L"\n" + strMeshInfo + L"\n" + strVertexInfo );

			meshs.clear();
			//meshs.push_back( std::make_unique<MeshObject>( reader.GetMeshPositionVertices(0, 0), reader.GetMeshIndices(0, 0), reader.uvs, programID, LoadDDS("texture1.dds") ) );
			
			for( int i = 0; i < reader.model.objects[0].meshcount; ++i )
			{
				meshs.push_back( std::make_unique<MeshObject>( reader.GetMeshPositionVertices(0, i), reader.GetMeshIndices(0, i), reader.uvs, programID, LoadDDS("texture1.dds") ) );
			}
		}
		else if( fsPath.extension() == ".RAB" )
		{
			//Load RAB archive
			RABReader mdlArc( ipath.c_str() );
			std::wstring fileName;
			//find first model in RAB archive.
			for( int i = 0; i < mdlArc.numFiles; ++i )
			{
				if( mdlArc.files[i].folder == L"MODEL" )
				{
					fileName = mdlArc.files[i].name;
					break;
				}
			}
			MDBReader model( mdlArc.ReadFile( L"MODEL", fileName ) );
			std::vector< char > textureBytes;
			textureBytes = mdlArc.ReadFile( L"HD-TEXTURE", model.model.textures[0].filename );

			meshs.push_back( std::make_unique<MeshObject>( model.GetMeshPositionVertices(0, 0), model.GetMeshIndices(0, 0), model.uvs, programID, LoadDDS_FromBuffer(textureBytes) ) );
		}
	}

	protected:
	//Renderables:
	sf::Font font;
	sf::Text text;
	sf::Text meshObjReadoutText;

	std::vector< std::unique_ptr<MeshObject>> meshs;

	//std::vector< std::unique_ptr<CDebugLine> > lines;

	//Variables:
	std::string strReadoutText;
	GLuint programID;
	GLuint shader_Untextured;
	Camera cam;

	bool bUseWireframe;

	//TODO: This shouldnt be here, it should be in the camera class.
	glm::vec3 cameraPosition;
	float fov;

	//Control variables
	sf::Vector2i mouseOldPos;
	bool isDragging;
};

//Tool state is in "File Browser"
class CStateFileBrowser : public BaseToolState
{
public:
	void Init( sf::RenderWindow *iwindow )
	{
		window = iwindow;

    	font.loadFromFile( "Font.ttf" );

		path = "./";

		PopulateTexts();
	};
	void ProccessEvent( sf::Event event )
	{
		if( event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Button::Left )
		{
			for( int i = 1; i < texts.size(); ++i )
			{
				if( texts[i].getGlobalBounds().contains( sf::Vector2f( sf::Mouse::getPosition( *window ) ) ) )
				{
					std::string newpath;
					newpath = texts[i].getString();

					if( std::filesystem::is_directory( newpath ) )
					{
						path = newpath;
						PopulateTexts();
					}
					else
					{
						//Fuck you.
						CStateModelRenderer * renderer = (CStateModelRenderer*)pOwner->GetState( "viewer" );
						renderer->LoadMDB( newpath );
						pOwner->SetState( "viewer" );
					}
				}
			}
		}
	};
	void Draw()
	{
		window->clear();
		window->pushGLStates();
		for( int i = 0; i < texts.size(); ++i )
		{
			if( i > 0 && texts[i].getGlobalBounds().contains( sf::Vector2f( sf::Mouse::getPosition( *window ) ) ) )
				texts[i].setFillColor( sf::Color( 255, 0, 0 ) );
			else
				texts[i].setFillColor( sf::Color( 0, 255, 0 ) );
			window->draw(texts[i]);
		}
		window->popGLStates();
		window->display();
	};
protected:

	void PopulateTexts()
	{
		texts.clear();

		sf::Text headerText = sf::Text( "DIRECTORY:", font, 20u );
		headerText.setFillColor( sf::Color( 0, 255, 0 ) );
		headerText.setPosition( sf::Vector2f( 5, 5 ) );

		texts.push_back( headerText );

		for( const auto & entry : std::filesystem::directory_iterator( path ) )
		{
			if( entry.path().extension() == ".mdb" || entry.path().extension() == ".RAB" || entry.is_directory() )
			{
				sf::Text text = sf::Text( entry.path().c_str(), font, 20u );
				text.setFillColor( sf::Color( 0, 255, 0 ) );
				text.setPosition( sf::Vector2f( 5, texts.back().getGlobalBounds().top + texts.back().getGlobalBounds().height ) );

				texts.push_back( text );
			}
		}
	}

	sf::Font font;
	std::vector< sf::Text > texts;
	std::string path;
};


//##############################################################
//Program entrypoint.
//##############################################################
int main()
{
    //std::cout << "Test:\n";

    //RABReader modelArc( "AIRTORTOISE_MISSILE.RAB" );

    //MDBReader reader( "model2.mdb" );
    //MDBReader reader( modelArc.ReadFile( L"MODEL", L"airtortoise_missile.mdb" ) );

    //return 0;
	

    //SFML Setup.
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    settings.antialiasingLevel = 4;
    settings.majorVersion = 3;
    settings.minorVersion = 0;

    sf::RenderWindow window( sf::VideoMode(800, 600), "MDB Viewer", sf::Style::Default, settings );

	//State manager;
	CToolStateHandler *states = new CToolStateHandler();

	//Create states:
	CStateModelRenderer *mdlRenderer = new CStateModelRenderer( );
	mdlRenderer->Init( &window ); //Feed the state a ptr to the sf window and initialise.
	//mdlRenderer->LoadMDB( "model2.mdb" );

	BaseToolState *fileBrowser = new CStateFileBrowser( );
	fileBrowser->Init( &window ); //Feed the state a ptr to the sf window and initialise.

	states->AddState( "browser", fileBrowser );
	states->AddState( "viewer", mdlRenderer );

	//Set default state
	states->SetState( "browser" );

    //MAIN LOOP
    while (window.isOpen())
    {
        sf::Event event;
        while( window.pollEvent(event) )
        {
            if( event.type == sf::Event::Closed )
                window.close();
            
			states->ProccessEvent( event );
        }

        states->Draw(); //Todo: Place common draw steps before and after state.
    }

	//Erase states, calling deconstructors.
	delete states;

    //Todo: Nicely clean up OpenGL and other such memory.

    return 0;
}