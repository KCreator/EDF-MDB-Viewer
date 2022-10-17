#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>
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
        GLuint MatrixID = glGetUniformLocation(shaderID, "MVP");
        
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
class CDebugLines
{
	public:

	protected:

};

//Todo: Primitive renderers for if we implement RMPA node viewing?

#include <SFML/Graphics.hpp>

#include "RAB.h"
#include <filesystem>

//Tool state system, will allow the tool to have multiple "screens", IE, a file browser, that it can swap between
class BaseToolState
{
public:
	BaseToolState(){};

	virtual void Init( sf::RenderWindow *iwindow )
	{
		window = iwindow;
	};
	virtual void ProccessEvent( sf::Event event ){};
	virtual void Draw(){};

protected:
	sf::RenderWindow *window;
};

//Tool state is in "Model Renderer"
class CStateModelRenderer : public BaseToolState
{
	public:
	CStateModelRenderer(){};
	
	void Init( sf::RenderWindow *iwindow )
	{
		window = iwindow;

		//Init OPENGL related stuff;
    	MDBReader reader( "model2.mdb" ); //Todo: Memory management.

		//Todo: Reference font in some kind of common way.
    	font.loadFromFile( "Font.ttf" );

		//Text information about the viewer
		std::wstring strObjName = reader.model.names[ reader.model.objects[0].nameindex ];
		std::wstring strMeshInfo = L"Mesh 1 of " + std::to_wstring( reader.model.objects[0].meshcount );
		std::wstring strVertexInfo = L"Number of vertices: " + std::to_wstring( reader.model.objects[0].meshs[0].vertexnumber );

		text = sf::Text( L"Displaying: " + strObjName + L"\n" + strMeshInfo + L"\n" + strVertexInfo, font, 20u );
		text.setFillColor( sf::Color( 0, 255, 0 ) );
		text.setPosition( sf::Vector2f( 5, 5 ) );

		strReadoutText = "Rendering 1 Mesh at position:";
		meshObjReadoutText = sf::Text( strReadoutText, font, 15U );
		meshObjReadoutText.setFillColor( sf::Color( 255, 0, 0 ) );
		meshObjReadoutText.setPosition( sf::Vector2f( 5, 600-15-5 ) );

		//Enable Culling
		glEnable(GL_CULL_FACE);
		glFrontFace(GL_CCW);
		
		// Create and compile our GLSL program from the shaders
		GLuint programID = LoadShaders( "SimpleVertexShader.txt", "SimpleTexturedFragShader.txt" ); //Todo: Common rendering library?
		GLuint shader_Untextured = LoadShaders( "SimpleVertexShader.txt", "SimpleFragmentShader.txt" );

		mesh = std::make_unique<MeshObject>( reader.test, reader.test2, reader.uvs, programID, LoadDDS("texture1.dds") ); //Todo: Perhaps use new/pointers/ect.
		
		//mesh.Texture = loadDDS_FromBuffer( modelArc.ReadFile( L"HD-TEXTURE", reader.model.textures[0].filename ) );
		//MeshObject mesh2( reader.test, reader.test2, reader.uvs, programID );
		//mesh.shaderID = programID;

		// Enable depth test
		glEnable(GL_DEPTH_TEST);
		// Accept fragment if it closer to the camera than the former one
		glDepthFunc(GL_LEQUAL);
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
			if( event.key.code == sf::Keyboard::Right )
			{
				mesh->angles.y += 0.01;
			}
		}
	}

	void Draw()
	{
		// Projection matrix : 45° Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
        cam.Projection = glm::perspective( glm::radians(45.0f), (float)800 / (float)600, 0.1f, 100.0f );
        
        // Camera matrix
        cam.View = glm::lookAt(
            glm::vec3(2,0,0), // Camera position, in World Space
            glm::vec3(0,0,0), // Camera Look At position
            glm::vec3(0,1,0)  // Head is up (set to 0,-1,0 to look upside-down)
            );
        
        strReadoutText = "Rendering 1 Mesh at position: " + std::to_string( mesh->position.x ) + ", " +
        std::to_string( mesh->position.y ) + ", " + std::to_string( mesh->position.z );
        meshObjReadoutText.setString( strReadoutText );

        window->clear();

        glClear( GL_DEPTH_BUFFER_BIT );

        //Turn on wireframe mode
        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        
        mesh->Draw(cam);

        //Turn off wireframe mode
        //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		glUseProgram(0);

        window->pushGLStates();
        window->draw( text );
        window->draw( meshObjReadoutText );
        window->popGLStates();
    
        window->display();
	}

	protected:
	//Renderables:
	sf::Font font;
	sf::Text text;
	sf::Text meshObjReadoutText;
	std::unique_ptr<MeshObject> mesh;

	//Variables:
	std::string strReadoutText;
	GLuint programID;
	GLuint shader_Untextured;
	Camera cam;
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
				}
			}
		}
	};
	void Draw()
	{
		window->clear();
		for( int i = 0; i < texts.size(); ++i )
		{
			if( i > 0 && texts[i].getGlobalBounds().contains( sf::Vector2f( sf::Mouse::getPosition( *window ) ) ) )
				texts[i].setFillColor( sf::Color( 255, 0, 0 ) );
			else
				texts[i].setFillColor( sf::Color( 0, 255, 0 ) );
			window->draw(texts[i]);
		}
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
			sf::Text text = sf::Text( entry.path().c_str(), font, 20u );
			text.setFillColor( sf::Color( 0, 255, 0 ) );
			text.setPosition( sf::Vector2f( 5, texts.back().getGlobalBounds().top + texts.back().getGlobalBounds().height ) );

			texts.push_back( text );
		}
	}

	sf::Font font;
	std::vector< sf::Text > texts;
	std::string path;
};

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

	//Create states:
	std::unique_ptr< BaseToolState > state = std::make_unique< CStateModelRenderer >( ); //Might not need unique_ptr here.
	state->Init( &window ); //Feed the state a ptr to the sf window and initialise.

    //MAIN LOOP
    while (window.isOpen())
    {
        sf::Event event;
        while( window.pollEvent(event) )
        {
            if( event.type == sf::Event::Closed )
                window.close();
            
			state->ProccessEvent( event );
        }

        state->Draw(); //Todo: Place common draw steps before and after state.
    }

	//Erase states, calling deconstructors.

    //Todo: Nicely clean up OpenGL and other such memory.

    return 0;
}