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

#include "MeshRenderer.h"

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
#include "Util.h"

//###################################################
//RMPA (EDF Waypoint Data) reader
//###################################################

//Supporting data structures.
struct RMPASpawnpoint
{
	float x;
	float y;
	float z;

	float fx;
	float fy;
	float fz;

	std::wstring name;
};

class CRMPAReader
{
public:
	CRMPAReader ( const char *path )
	{
		//Create input stream from path
		std::ifstream file( path, std::ios::binary | std::ios::ate );
		std::streamsize size = file.tellg( );
		file.seekg( 0, std::ios::beg );

		std::vector<char> buffer( size );
		if( file.read( buffer.data( ), size ) )
		{
			int pos = 0x0;

			//TODO: Read file header, determine if valid.

			//Read spawnpoints:
			pos = 0x20;
			bool bParseSpawns = ReadInt( &buffer, pos );

			pos = 0x24;
			int offsSpawnTable = ReadInt( &buffer, pos, true );

			//Read header 1.
			pos = offsSpawnTable;

			//Type header
			int subheaderCount = ReadInt( &buffer, offsSpawnTable, true ); //Subheader Count
			int subheaderOffs = ReadInt( &buffer, offsSpawnTable + 0x4, true ); //Subheader offset.
			int dataEndOffs = ReadInt( &buffer, offsSpawnTable + 0xc, true ); //Data end offset
			int typeHeaderID = ReadInt( &buffer, offsSpawnTable + 0x10, true ); //Type header identifier
			int stringTableOffs = ReadInt( &buffer, offsSpawnTable + 0x18, true ); //Offset to strings

			//Read subheaders
			pos = offsSpawnTable + subheaderOffs;
			for( int i = 0; i < subheaderCount; ++i )
			{
				int subheaderEndOffs = ReadInt( &buffer, pos + 0x8, true ); //Subheader Data End Point
				int subheaderNameLen = ReadInt( &buffer, pos + 0x10, true ); //Subheader Name Length
				int subheaderNameOffs = ReadInt( &buffer, pos + 0x14, true ); //Subheader Name offset
				int subheaderDataCount = ReadInt( &buffer, pos + 0x18, true ); //Subheader Data Count
				int subheaderDataOffs = ReadInt( &buffer, pos + 0x1c, true ); //Subheader Data Start Pos

				std::wstring subheaderName = ReadUnicode( &buffer, pos + subheaderNameOffs, true ); //Get our actual name.

				//Read spawn points.
				for( int j = 0; j < subheaderDataCount; ++j )
				{
					int ofs = pos + subheaderDataOffs + (j * 0x40);

					int spawnpointID = ReadInt( &buffer, ofs + 0x8, true );

					//Position
					float x = ReadFloat( &buffer, ofs + 0x0c );
					float y = ReadFloat( &buffer, ofs + 0x10 );
					float z = ReadFloat( &buffer, ofs + 0x14 );

					//Orientation
					float fx = ReadFloat( &buffer, ofs + 0x1c );
					float fy = ReadFloat( &buffer, ofs + 0x20 );
					float fz = ReadFloat( &buffer, ofs + 0x24 );

					int spawnpointNameSize = ReadInt( &buffer, ofs + 0x30, true );
					int spawnpointNameOffs = ReadInt( &buffer, ofs + 0x34, true );

					std::wstring spawnpointName = ReadUnicode( &buffer, ofs + spawnpointNameOffs, true ); //Get our actual name.

					//Store spawn point. This is largely temporary.
					RMPASpawnpoint point;

					point.x = x;
					point.y = y;
					point.z = z;

					point.fx = fx;
					point.fy = fy;
					point.fz = fz;

					point.name = spawnpointName;

					spawnpoints.push_back( point );
				}

				pos += 0x20;
			}
		}

		//Clear buffers
		buffer.clear( );
		file.close( );
	};

	//Temp:
	std::vector< RMPASpawnpoint > spawnpoints;
};

//###################################################
//MAC (EDF Map File) reader
//###################################################
class MACReader
{
public:
	MACReader( const char *path )
	{
		//Create input stream from path
		std::ifstream file( path, std::ios::binary | std::ios::ate );
		std::streamsize size = file.tellg( );
		file.seekg( 0, std::ios::beg );

		std::vector<char> buffer( size );
		if( file.read( buffer.data( ), size ) )
		{
			//Read MAC file.
			int dataStartOfs = ReadInt( &buffer, 0x8 );
			int unk0 = ReadInt( &buffer, 0xc );
			int unk1 = ReadInt( &buffer, 0x10 );

			//MARC table offset and size
			int iMARCTableOffs = ReadInt( &buffer, 0x14 );
			int iMARCTableSize = ReadInt( &buffer, 0x18 );

			//String Table offset and size
			int iStringTableOffs = ReadInt( &buffer, 0x1c );
			int iStringTableSize = ReadInt( &buffer, 0x20 );

			//Read MARC:
			int pos = 0x14 + iMARCTableOffs;
			for( int i = 0; i < iMARCTableSize; ++i )
			{
				//Get the MARC internal name:
				int ofs = ReadInt( &buffer, pos );
				std::wstring name = ReadUnicode( &buffer, pos + ofs, false ); //String is located relative to read position

				//std::wcout << name + L"\n"; //Debugging.

				pos += 0x4;

				//Block Address (Absolute)
				int blockAddress = ReadInt( &buffer, pos );
				pos += 0x4;

				//Block Size:
				int blockSize = ReadInt( &buffer, pos );
				pos += 0x4;

				//Several unknowns.
				int iMARCUnk0 = ReadInt( &buffer, pos );
				pos += 0x4;

				int iMARCUnk1 = ReadInt( &buffer, pos );
				pos += 0x4;

				int iMARCUnk2 = ReadInt( &buffer, pos );
				pos += 0x4;

				//Attempt to do something with this data
				//TODO: Cleanup, organise, ect.
				int oldPos = pos;
				if( name == L"map.mapo" )
				{
					std::vector<char> mapoBuff( blockSize );
					std::copy( buffer.begin() + blockAddress, buffer.begin() + blockAddress + blockSize, mapoBuff.begin() );
					ParseMapO( mapoBuff );

					mapoBuff.clear();
				}
				if( name == L"map.mapb" )
				{
					std::vector<char> mapbBuff( blockSize );
					std::copy( buffer.begin() + blockAddress, buffer.begin() + blockAddress + blockSize, mapbBuff.begin() );
					ParseMapB( mapbBuff );

					mapbBuff.clear();
				}
				pos = oldPos;
			}
		}

		//Clear buffers
		buffer.clear( );
		file.close( );
	}

	void ParseMapB( std::vector< char> buffer )
	{
		int pos = 0;

		int iStringOffs = ReadInt( &buffer, pos + 0x8 );
		int iStringCount = ReadInt( &buffer, pos + 0xc );

		int iMAPBTable1Offs = ReadInt( &buffer, pos + 0x18 );
		int iMAPBTable1Count = ReadInt( &buffer, pos + 0x1c );

		int iMAPBTable2Offs = ReadInt( &buffer, pos + 0x20 );
		int iMAPBTable2Count = ReadInt( &buffer, pos + 0x24 );

		/*
		pos = iMAPBTable1Offs;
		for( int i = 0; i < iMAPBTable1Count; ++i )
		{
			int unk0 = ReadInt( &buffer, pos );
			pos += 0x4;

			int sgoOffs = ReadInt( &buffer, pos );
			pos += 0x4;

			int unk1 = ReadInt( &buffer, pos );
			pos += 0x4;

			//Read SGO?

			int strOffs = ReadInt( &buffer, pos );
			std::wstring name = ReadUnicode( &buffer, pos + strOffs, false );
			std::wcout << name + L"\n"; //Debugging.

			pos += 0x4;

			pos += 0x38 - ( 0x4 * 4 );
		}
		*/

		pos = iMAPBTable2Offs;

		for( int i = 0; i < iMAPBTable2Count; ++i )
		{
			//Offset to Table
			int offs = ReadInt( &buffer, pos );

			//Attempt to use this table offset.
			int table1Pos = pos + offs;

			int unk0 = ReadInt( &buffer, table1Pos );

			int sgoOffs = ReadInt( &buffer, table1Pos + 0x4 );
			int strOffs = ReadInt( &buffer, table1Pos + 0xc );
			std::wstring name = ReadUnicode( &buffer, table1Pos + 0xc + strOffs, false );

			size_t dotPos = name.find_last_of( L"." );
			std::wstring test = name.substr( 0, dotPos );
			modelNames2.push_back( test );
			//std::wcout << name + L"\n"; //Debugging.

			//Try to read some key data from the SGO (TODO: Write SGO handler)
			int sgoPos = table1Pos + 0x4 + sgoOffs;
			
			/*
			//This confirms we have an SGO file here.
			char bytes[4];
			bytes[0] = buffer[sgoPos];
			bytes[1] = buffer[sgoPos+1];
			bytes[2] = buffer[sgoPos+2];
			bytes[3] = buffer[sgoPos+3];

			int numSGOVars = ReadInt( &buffer, sgoPos + 0x8 );
			int SGOVarsStart = ReadInt( &buffer, sgoPos + 0xc );
			int numVarNames = ReadInt( &buffer, sgoPos + 0x10 );
			int SGOVarNamesStart = ReadInt( &buffer, sgoPos + 0x14 );

			//Read Variable Names
			std::vector< std::wstring > variableNames;
			for( int j = 0; j < numVarNames; ++j )
			{
				int stroffs = ReadInt( &buffer, sgoPos + SGOVarNamesStart + ( j * 0x8 ) );
				int id = ReadInt( &buffer, sgoPos + SGOVarNamesStart + ( j * 0x8 ) + 0x4  );
				
				std::wstring vName = ReadUnicode( &buffer, sgoPos + SGOVarNamesStart + ( j * 8 ) + stroffs, false );
				variableNames.push_back( vName );
			}

			for( int j = 0; j < numSGOVars; ++j )
			{
				if( variableNames[j] == L"filename" )
				{
					int locOfs = sgoPos + SGOVarsStart + ( j * 0xc ); //position + varStart + j * Size of SGO Node
					int type = ReadInt( &buffer, locOfs );
					if( type == 3 )
					{
						int strSize = ReadInt( &buffer, locOfs + 0x4 );
						std::wcout << L"Variable '" + variableNames[j] + L"' size =  '" + std::to_wstring( strSize ) + L"'\n";

						int stroffs = ReadInt( &buffer, locOfs + 0x8 );
						std::wstring value = ReadUnicode( &buffer, locOfs + stroffs, false );
						std::wcout << L"Variable '" + variableNames[j] + L"' = '" + value + L"'\n";
					}
				}
			}
			*/

			pos += 0x4;

			//X Pos
			float xPos = ReadFloat( &buffer, pos, true );
			pos += 0x4;
			//Y Pos
			float yPos = ReadFloat( &buffer, pos, true );
			pos += 0x4;
			//Z Pos
			float zPos = ReadFloat( &buffer, pos, true );
			pos += 0x4;

			//RotationX
			float xRot = ReadFloat( &buffer, pos, true );
			pos += 0x4;
			//RotationY
			float yRot = ReadFloat( &buffer, pos, true );
			pos += 0x4;
			//RotationZ
			float zRot = ReadFloat( &buffer, pos, true );
			pos += 0x4;

			//Store this for later?
			positions.push_back( glm::vec3( xPos, yPos, zPos ) );
			rotations.push_back( glm::vec3( xRot, yRot, zRot ) );

			//Ignore unknowns.
			pos += 0x4 * 10;
		}
	}

	void ParseMapO( std::vector< char> buffer )
	{
		//Can't seem to figure this out right now. Fukou da.
		int pos = 0;
					
		int objectTable1Offs = ReadInt( &buffer, pos + 0x8 );
		int objectTable1Count = ReadInt( &buffer, pos + 0xc );

		int objectStringTableOffs = ReadInt( &buffer, pos + 0x48 );
		int objectStringTableCount = ReadInt( &buffer, pos + 0x4C );

		int objectTable3Offs = ReadInt( &buffer, pos + 0x20 );
		int objectTable3Count = ReadInt( &buffer, pos + 0x24 );

		int objectTable5Offs = ReadInt( &buffer, pos + 0x38 );
		int objectTable5Size = ReadInt( &buffer, pos + 0x3c );

		//return;

		pos = objectTable1Offs;
		for( int j = 0; j < objectTable1Count; ++j )
		{
			int base = pos;

			//Read:
			//std::cout << "Var: ";
			//std::cout << std::to_string( ReadInt( &buffer, pos + 0x00 ) ) + ",";
			//std::cout << std::to_string( ReadInt( &buffer, pos + 0x04 ) ) + ","; //Apparently offset to 4th data table?
			//std::cout << std::to_string( ReadInt( &buffer, pos + 0x08 ) ) + ",";
			//std::cout << std::to_string( ReadInt( &buffer, pos + 0x0c ) ) + ",";
			//std::cout << std::to_string( ReadInt( &buffer, pos + 0x10 ) ) + ",";
			//std::cout << std::to_string( ReadInt( &buffer, pos + 0x14 ) ) + ",";
			//std::cout << std::to_string( ReadInt( &buffer, pos + 0x18 ) ) + "\n";

			int unk0 = ReadInt( &buffer, pos );
			pos += 0x4;

			int offset1 = ReadInt( &buffer, pos );

			if( offset1 != 0 )
			{
				//std::cout << "4th table value: " + std::to_string( ReadInt( &buffer, pos + offset1 ) ) + "\n";
				int test = offset1 + ReadInt( &buffer, pos + offset1 );

				int positionsOffs = ReadInt( &buffer, pos + test + 0x10 );
				if( positionsOffs != 0 )
				{
					int localPos = pos + test + positionsOffs;
					float x = ReadFloat( &buffer, localPos, true );
					float y = ReadFloat( &buffer, localPos + 0x4, true );
					float z = ReadFloat( &buffer, localPos + 0x8, true );

					submodelPositionOffsets.push_back( glm::vec3( x, y, z ) );
				}
				else
				{
					submodelPositionOffsets.push_back( glm::vec3( 0, 0, 0 ) );
				}

				int namOfs = ReadInt( &buffer, pos + test + 0x20 );
				std::wstring tname = ReadUnicode( &buffer, pos + test + namOfs, false );
				//std::wcout << tname + L"\n"; //Debugging.

				modelNames.push_back( tname ); //Store for later...
			}
			else
			{
				modelNames.push_back( L"" ); //Store for later...
			}

			//pos += 0x1c;
			//continue;

			pos += 0x4;

			int unk1 = ReadInt( &buffer, pos );
			pos += 0x4;

			float unk2 = ReadFloat( &buffer, pos );
			pos += 0x4;

			//Sub model name?
			int mdbNameOfs = ReadInt( &buffer, pos );
			std::wstring name = ReadUnicode( &buffer, base + mdbNameOfs, false );
			//std::wcout << name + L"\n"; //Debugging.
			subModelNames.push_back( name );

			pos += 0x4;

			int hktNameOfs = ReadInt( &buffer, pos );
			name = ReadUnicode( &buffer, base + hktNameOfs, false );
			//std::wcout << name + L"\n"; //Debugging.

			pos += 0x4;
			pos += 0x4;
		}

		return; //TEMP!

		//Test:
		pos = objectTable3Offs;
		for( int i = 0; i < objectTable3Count; ++i )
		{
			int base = pos;

			//Get object .hkt name
			int hktSringOffs = ReadInt( &buffer, pos + 0x04 );
			std::wstring hktString = ReadUnicode( &buffer, base + hktSringOffs, false );

			//Seems there is only 1 value at 0xC that matters. Everything else is zero... Very different compared to the 4.1 notes...

			//Get object hk.mdb name
			int hkmdbSringOffs = ReadInt( &buffer, pos + 0x14 );
			std::wstring hkmdbString = ReadUnicode( &buffer, base + hkmdbSringOffs, false );

			//This seems to get object names now.
			int test = ReadInt( &buffer, pos + 0x20 );
			std::wstring name = ReadUnicode( &buffer, base + test, false );
			std::wcout << name + L"\n"; //Debugging.

			modelNames.push_back( name ); //Store for later...

			//Apparently the 5 map format has an extra byte?
			pos += 0x38 + 0x4;
		}
	}

	//Temp:
	//This needs to be redone...
	std::vector< std::wstring > modelNames;
	std::vector< std::wstring > modelNames2;
	std::vector< std::wstring > subModelNames;
	std::vector< glm::vec3 > positions;
	std::vector< glm::vec3 > submodelPositionOffsets;
	std::vector< glm::vec3 > rotations;
};

//###################################################
//Tool State System
//###################################################

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

	CToolStateHandler * GetOwner()
	{
		return pOwner;
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
		programID = ShaderList::LoadShader( "SimpleTextured", "SimpleVertexShader.txt", "SimpleTexturedFragShader.txt" ); //Todo: Common rendering library?
		shader_Untextured = ShaderList::LoadShader( "Untextured", "SimpleVertexShader.txt", "SimpleFragmentShader.txt" );

		//mesh = std::make_unique<MeshObject>( reader.GetMeshPositionVertices(0, 0), reader.GetMeshIndices(0, 0), reader.uvs, programID, LoadDDS("texture1.dds") ); //Todo: Perhaps use new/pointers/ect.
		
		//Try to form a bone map:
		//lines.push_back( std::make_unique< CDebugLine >( reader.bonepos[1], reader.bonepos[0], glm::vec3( 0, 0, 255 ) ) );
		//lines.push_back( std::make_unique< CDebugLine >( reader.bonepos[2], reader.bonepos[0], glm::vec3( 0, 0, 255 ) ) );
		//lines.push_back( std::make_unique< CDebugLine >( reader.bonepos[3], reader.bonepos[2], glm::vec3( 0, 0, 255 ) ) );

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

				//Toggle culling.
				if( bUseWireframe )
					glDisable( GL_CULL_FACE );
				else
					glEnable( GL_CULL_FACE );

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
				meshs[i]->angles.y -= (float)(mouseOldPos.x - mousePos.x) / 10.0f;

			mouseOldPos = mousePos;
		}
	}

	void Draw()
	{
		// Projection matrix : 45° Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
        cam.Projection = glm::perspective( glm::radians(45.0f), (float)window->getSize().x / (float)window->getSize().y, 0.1f, 100.0f );
        
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

		//Render these above all else
		glDepthFunc( GL_ALWAYS );
		for( const auto & line : lines )
		{
			line->Draw( cam );
		}
		glDepthFunc( GL_LEQUAL );

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

			//find first model in RAB archive. TODO: Allow user to select this, somehow?
			for( int i = 0; i < mdlArc.numFiles; ++i )
			{
				if( mdlArc.files[i].folder == L"MODEL" )
				{
					fileName = mdlArc.files[i].name;
					break;
				}
			}
			
			MDBReader model( mdlArc.ReadFile( L"MODEL", fileName ) );

			//Text information about the viewer
			std::wstring strObjName = model.model.names[ model.model.objects[0].nameindex ];
			std::wstring strMeshInfo = L"Object 1 of " + std::to_wstring( model.model.objectscount );
			std::wstring strVertexInfo = L"Number of vertices (MESH 1): " + std::to_wstring( model.model.objects[0].meshs[0].vertexnumber );

			text.setString( L"Displaying: " + strObjName + L"\n" + strMeshInfo + L"\n" + strVertexInfo );

			//Fully construct the object:
			for( int i = 0; i < model.model.objects[0].meshcount; ++i )
			{
				std::vector< char > textureBytes;
				textureBytes = mdlArc.ReadFile( L"HD-TEXTURE", model.GetColourTextureFilename( 0, i ) );

				meshs.push_back( std::make_unique<MeshObject>( model.GetMeshPositionVertices(0, i), model.GetMeshIndices(0, i), model.GetMeshUVs( 0, i ), programID, LoadDDS_FromBuffer(textureBytes) ) );
			}

			//Camera angle for looking at humanoid bones.
			cameraPosition = glm::vec3( 0, 3, 2 );

			//Attempt to generate bones.
			std::vector< glm::mat4 > boneWorldTransforms;

			for( int i = 0; i < model.model.bonescount; ++i )
			{
				if( model.model.bones[i].boneParent != -1 )
				{
					//Obvious, in retrospect. Each bone transforms itself relative to its parent to get its actual coords.
					//However, the extra data is unknown.

					int parentIndex = model.model.bones[ model.model.bones[i].boneParent ].boneIndex;
					glm::vec4 pos1 = glm::vec4( 0.0f );
					glm::vec4 pos2 = glm::vec4( 0.0f );

					pos1.w = 1;
					pos2.w = 1;

					boneWorldTransforms.push_back( boneWorldTransforms[parentIndex] * model.model.bones[i].matrix );

					pos1 = boneWorldTransforms[parentIndex] * pos1;
					pos2 = boneWorldTransforms[i] * pos2;

					lines.push_back( std::make_unique<CDebugLine>( pos1, pos2, glm::vec3( 0, 255, 0 ) ) );

					//Visualise the "Reset to zero" transform
					//pos2 = model.model.bones[i].matrix2 * pos2;
					//lines.push_back( std::make_unique<CDebugLine>( pos1, pos2, glm::vec3( 255, 255, 0 ) ) );
				}
				else
				{
					boneWorldTransforms.push_back( model.model.bones[i].matrix );
				}
			}
		}
	}

	protected:

	//Renderables:
	sf::Font font;
	sf::Text text;
	sf::Text meshObjReadoutText;

	std::vector< std::unique_ptr<MeshObject>> meshs;

	std::vector< std::unique_ptr<CDebugLine> > lines;

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

class CStateSceneRenderer : public BaseToolState
{
	public:
	~CStateSceneRenderer()
	{
		meshs.clear(); //Lets hope this call the destructor.
		lines.clear();

		//TODO: Kill shaders?
	};

	void Init( sf::RenderWindow *iwindow )
	{
		window = iwindow;

		//Init OPENGL related stuff;

		//Enable Culling
		glEnable( GL_CULL_FACE );
		glFrontFace( GL_CCW );
		
		// Create and compile our GLSL program from the shaders
		programID = ShaderList::LoadShader( "SimpleTextured", "SimpleVertexShader.txt", "SimpleTexturedFragShader.txt" ); //Todo: Common rendering library?
		shader_Untextured = ShaderList::LoadShader( "Untextured", "SimpleVertexShader.txt", "SimpleFragmentShader.txt" );

		// Enable depth test
		glEnable( GL_DEPTH_TEST );
		// Accept fragment if it closer to the camera than the former one
		glDepthFunc( GL_LEQUAL );

		//Final var init;
		bUseWireframe = false;
		fov = 45.0f;
		cameraPosition = glm::vec3( 2, 0, 0 );
		pitch = 0;
		yaw = 180;

		isMouseCamControl = false;

		sf::Mouse::setPosition( sf::Vector2i(window->getSize().x/2, window->getSize().y/2) , *window );
		mouseOldPos = sf::Mouse::getPosition( *window );
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

				//Toggle culling.
				if( bUseWireframe )
					glDisable( GL_CULL_FACE );
				else
					glEnable( GL_CULL_FACE );
				
				//Not the best solution, but here we are.
				for( int i = 0; i < meshs.size(); ++i )
				{
					meshs[i]->shaderID = bUseWireframe ? shader_Untextured : programID;
					meshs[i]->TextureID = glGetUniformLocation(meshs[i]->shaderID, "myTextureSampler");
				}
			}
			else if( event.key.code == sf::Keyboard::Space )
			{
				isMouseCamControl = !isMouseCamControl;
				window->setMouseCursorVisible( !isMouseCamControl );

				if( isMouseCamControl )
				{
					sf::Mouse::setPosition( sf::Vector2i( window->getSize().x/2, window->getSize().y/2 ) , *window );
					mouseOldPos = sf::Mouse::getPosition( *window );
				}
			}
		}
		else if (event.type == sf::Event::MouseMoved )
		{
			if( isMouseCamControl )
			{
				if( event.mouseMove.x != mouseOldPos.x || event.mouseMove.y != mouseOldPos.y ) //Only care if the muse has actually moved.
				{
					pitch -= (event.mouseMove.y - mouseOldPos.y) / 10.0f;
					yaw += (event.mouseMove.x - mouseOldPos.x) / 10.0f;
					sf::Mouse::setPosition( sf::Vector2i(window->getSize().x/2, window->getSize().y/2) , *window );
				}
			}
		}
	}

	void Draw()
	{
		// Projection matrix : 45° Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
        cam.Projection = glm::perspective( glm::radians(45.0f), (float)window->getSize().x / (float)window->getSize().y, 0.1f, 1000.0f );
        
		//pitch = 0;
		//yaw = 180.0;

		glm::vec3 direction;
		direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		direction.y = sin(glm::radians(pitch));
		direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		//cameraFront = glm::normalize(direction);

		//We have the movement code here, as we want to run it each frame.
		//TODO: Scale with a deltatime value.
		float speed = cameraSpeed;

		//Get needed vectors for camera movement.
		glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f); 
		glm::vec3 cameraRight = glm::normalize(glm::cross(up, direction));

		if( sf::Keyboard::isKeyPressed( sf::Keyboard::W ) )
			cameraPosition += direction * speed;
		if( sf::Keyboard::isKeyPressed( sf::Keyboard::S ) )
			cameraPosition -= direction * speed;
		if( sf::Keyboard::isKeyPressed( sf::Keyboard::A ) )
			cameraPosition += cameraRight * speed;
		if( sf::Keyboard::isKeyPressed( sf::Keyboard::D ) )
			cameraPosition -= cameraRight * speed;

        // Camera matrix
        cam.View = glm::lookAt(
            cameraPosition, // Camera position, in World Space
            cameraPosition + direction, // Camera Look At position
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

		for( const auto & line : lines )
		{
			line->Draw( cam );
		}

		glUseProgram(0);
    
        window->display();
	}

	void LoadScene( )
	{
		//Proccess:

		std::string path = "EDFData/IG_BASE502";
		//std::string path = "EDFData/IG_EDFROOM01";

		//Load MAC.
		MACReader map( ( path + ".MAC" ).c_str() );

		//Load RAB archive
		RABReader mdlArc( ( path + ".RAB" ).c_str() );

		//Compare map object strings, replace with "true" object when I can.
		if( map.modelNames.size() == map.modelNames2.size() )
		{
			for( int i = 0; i < map.modelNames.size(); ++i )
			{
				for( int j = 0; j < mdlArc.files.size(); ++j )
				{
					if( mdlArc.files[j].folder == L"MODEL" && mdlArc.files[j].name == map.modelNames2[i] + L".mdb" )
					{
						//Should do this elsewere.
						map.modelNames[i] = map.modelNames2[i] + L".mdb";
					}
				}
			}
		}

		//Lets be more effecient about this.
		//Determine "Unique" models
		std::map < std::wstring, MDBReader > mdbs;
		std::map < std::wstring, GLuint > mdbTextures;
		for( int i = 0; i < map.modelNames.size(); ++i )
		{
			if( map.modelNames[i].size() == 0 )
				continue;

			if( mdbs.count( map.modelNames[i] ) == 0 )
			{
				std::wstring mdbName = map.modelNames[i];
				mdbs[ mdbName ] = MDBReader( mdlArc.ReadFile( L"MODEL", mdbName ) );

				if( mdbs[ mdbName ].model.objectscount == 0 ) //Bizzare edge case where a null model exists.
					continue;

				for( int o = 0; o < mdbs[ mdbName ].model.objectscount; ++o )
				{
					for( int j = 0; j < mdbs[ mdbName ].model.objects[o].meshcount; ++j )
					{
						if( mdbTextures.count( mdbs[ mdbName ].GetColourTextureFilename( o, j ) ) == 0 )
						{
							std::vector< char > textureBytes;
							std::wstring texFileName = mdbs[ mdbName ].GetColourTextureFilename( o, j );
							textureBytes = mdlArc.ReadFile( L"HD-TEXTURE", texFileName );
							mdbTextures[ texFileName ] = LoadDDS_FromBuffer(textureBytes);
							textureBytes.clear();
						}
					}
				}
			}

			//Load submodel, if available.
			if( map.subModelNames[i].size() > 0 )
			{
				if( mdbs.count( map.subModelNames[i] ) == 0 )
				{
					std::wstring mdbName = map.subModelNames[i];
					mdbs[ mdbName ] = MDBReader( mdlArc.ReadFile( L"MODEL", mdbName ) );

					if( mdbs[ mdbName ].model.objectscount == 0 ) //Bizzare edge case where a null model exists.
						continue;

					for( int o = 0; o < mdbs[ mdbName ].model.objectscount; ++o )
					{
						for( int j = 0; j < mdbs[ mdbName ].model.objects[o].meshcount; ++j )
						{
							if( mdbTextures.count( mdbs[ mdbName ].GetColourTextureFilename( o, j ) ) == 0 )
							{
								std::vector< char > textureBytes;
								std::wstring texFileName = mdbs[ mdbName ].GetColourTextureFilename( o, j );
								textureBytes = mdlArc.ReadFile( L"HD-TEXTURE", texFileName );
								mdbTextures[ texFileName ] = LoadDDS_FromBuffer(textureBytes);
								textureBytes.clear();
							}
						}
					}
				}
			}
		}

		//Least effecient modelloader ever made :)
		for( int iter = 0; iter < map.modelNames.size(); ++iter )
		{
			if( map.modelNames[iter].size() == 0 )
				continue;

			if( mdbs[ map.modelNames[iter] ].model.objectscount == 0 ) //Bizzare edge case where a null model exists.
				continue;

			for( int o = 0; o < mdbs[ map.modelNames[iter] ].model.objectscount; ++o )
			{
				for( int i = 0; i < mdbs[ map.modelNames[iter] ].model.objects[o].meshcount; ++i )
				{
					meshs.push_back( std::make_unique<MeshObject>( 
						mdbs[ map.modelNames[iter] ].GetMeshPositionVertices(o, i), 
						mdbs[ map.modelNames[iter] ].GetMeshIndices(o, i), 
						mdbs[ map.modelNames[iter] ].GetMeshUVs( o, i ), 
						programID, 
						mdbTextures[mdbs[ map.modelNames[iter] ].GetColourTextureFilename( o, i ) ] ) );

					meshs.back()->position = map.positions[iter];
					meshs.back()->angles = map.rotations[iter];
				}
			}

			//Load submodel, if available.
			if( map.subModelNames[iter].size() > 0 )
			{
				for( int o = 0; o < mdbs[ map.subModelNames[iter] ].model.objectscount; ++o )
				{
					for( int i = 0; i < mdbs[ map.subModelNames[iter] ].model.objects[o].meshcount; ++i )
					{
						meshs.push_back( std::make_unique<MeshObject>( 
							mdbs[ map.subModelNames[iter] ].GetMeshPositionVertices(o, i), 
							mdbs[ map.subModelNames[iter] ].GetMeshIndices(o, i), 
							mdbs[ map.subModelNames[iter] ].GetMeshUVs( o, i ), 
							programID, 
							mdbTextures[mdbs[ map.subModelNames[iter] ].GetColourTextureFilename( o, i ) ] ) );

						meshs.back()->position = map.positions[iter] + map.submodelPositionOffsets[iter];
						meshs.back()->angles = map.rotations[iter];
					}
				}
			}
		}

		cameraSpeed = 0.5;

		//Load RMPA
		CRMPAReader rmpa( "EDFData/MISSION.RMPA" );
		for( int i = 0; i < rmpa.spawnpoints.size(); ++i )
		{
			glm::vec3 pos( rmpa.spawnpoints[i].x, rmpa.spawnpoints[i].y, rmpa.spawnpoints[i].z );
			glm::vec3 dir( rmpa.spawnpoints[i].fx, rmpa.spawnpoints[i].fy, rmpa.spawnpoints[i].fz );

			lines.push_back( std::make_unique<CDebugLine>( pos, dir, glm::vec3( 0, 255, 0 ) ) );

			//Create secondary lines to make the spoint point more obvious.
			//Get needed vectors for camera movement.
			glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f); 
			glm::vec3 lineRight = glm::normalize(glm::cross(up, pos - dir));
			glm::vec3 lineUp = glm::normalize(glm::cross(lineRight, pos - dir));

			lines.push_back( std::make_unique<CDebugLine>( pos - (lineRight), pos + (lineRight), glm::vec3( 0, 0, 255 ) ) );
			lines.push_back( std::make_unique<CDebugLine>( pos - (lineUp), pos + (lineUp), glm::vec3( 255, 255, 0 ) ) );

		}

		return;

		std::wstring fileName;
		for( int i = 0; i < mdlArc.numFiles; ++i )
		{
			if( mdlArc.files[i].folder == L"MODEL" )
			{
				fileName = mdlArc.files[i].name;

				MDBReader model( mdlArc.ReadFile( L"MODEL", L"ya_edfroom_rooma.mdb" ) );

				for( int i = 0; i < model.model.objects[0].meshcount; ++i )
				{
					std::vector< char > textureBytes;
					textureBytes = mdlArc.ReadFile( L"HD-TEXTURE", model.GetColourTextureFilename( 0, i ) );

					meshs.push_back( std::make_unique<MeshObject>( model.GetMeshPositionVertices(0, i), model.GetMeshIndices(0, i), model.GetMeshUVs( 0, i ), programID, LoadDDS_FromBuffer(textureBytes) ) );
				}

				//meshs.push_back( std::make_unique<MeshObject>( model.GetMeshPositionVertices(0, 0), model.GetMeshIndices(0, 0), model.uvs, programID, LoadDDS_FromBuffer(textureBytes) ) );
			
				break;
			}
		}
	}

	protected:
	//Renderables:
	std::vector< std::unique_ptr<MeshObject>> meshs;

	std::vector< std::unique_ptr<CDebugLine> > lines;

	//Variables:
	GLuint programID;
	GLuint shader_Untextured;
	Camera cam;

	bool bUseWireframe;

	//TODO: This shouldnt be here, it should be in the camera class.
	glm::vec3 cameraPosition;
	float yaw;
	float pitch;

	float fov;

	//Control variables
	sf::Vector2i mouseOldPos;
	float cameraSpeed;
	bool isMouseCamControl;
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
						//Can't say I am happy with this. Feels inelegant, but it works.
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

#include "GUI.h"

//Simple labeled value implementation
class CUILabelledValue : public CBaseUIElement
{
public:
	CUILabelledValue(){};
	CUILabelledValue( sf::Font &txtFont, std::string name, unsigned int fontSize = 20U )
	{
		txtLabelText = sf::Text( name, txtFont, fontSize );
		txtLabelText.setFillColor( sf::Color( 0, 255, 0 ) );

		labelString = name;

		recBounds = txtLabelText.getGlobalBounds();
	};

	void SetTrackedValue( float *value )
	{
		trackedValue = value;

		oldValue = *trackedValue;
		txtLabelText.setString( labelString + std::to_string( *value ) );
	};

	void Draw( sf::RenderWindow *window )
	{
		if( !bActive )
			return;

		//Check if tracked value has changed.
		if( trackedValue != NULL )
		{
			if( *trackedValue != oldValue )
			{
				txtLabelText.setString( labelString + std::to_string( *trackedValue ) );
				oldValue = *trackedValue;
			}
		}

		window->draw( txtLabelText );
	};

	void SetPosition( sf::Vector2f pos )
	{
		txtLabelText.setPosition( pos );
		vecPosition = pos;

		recBounds = txtLabelText.getGlobalBounds();
	};

protected:
	float oldValue;
	float *trackedValue;

	std::string labelString;
	sf::Text txtLabelText;
};


//Temp UI test state
class CStateUITest : public BaseToolState
{
public:
	void Init( sf::RenderWindow *iwindow )
	{
		window = iwindow;

    	font.loadFromFile( "Font.ttf" );

		ui1 = CUITitledContainer( font, "Menu 1", 200, 400 );
		ui1.SetActive( true );

		CUIButton *button1 = new CUIButton( font, "Load Model" );
		button1->SetActive( true );
		button1->SetCallback( [&]{Test();} );

		ui1.AddElement( button1 );

		CUIButton *button2 = new CUIButton( font, "Map Viewer" );
		button2->SetActive( true );
		button2->SetCallback( [&]{Test2();} );

		ui1.AddElement( button2 );

		testvalue = 45;

		CUISlider *slider = new CUISlider( &testvalue, 1, 90, 150 );
		slider->SetActive( true );

		ui1.AddElement( slider );

		CUILabelledValue *label = new CUILabelledValue( font, "Value: " );
		label->SetActive( true );
		label->SetTrackedValue( &testvalue );

		ui1.AddElement( label );
	}

	void Test( )
	{
		GetOwner()->SetState( "browser" );
	};

	void Test2( )
	{
		//Nasty :)
		CStateSceneRenderer *sceneRenderer = new CStateSceneRenderer( );
		sceneRenderer->Init( window ); //Feed the state a ptr to the sf window and initialise.
		sceneRenderer->LoadScene();

		GetOwner()->AddState( "scene", sceneRenderer );
		GetOwner()->SetState( "scene" );
	};

	void ProccessEvent( sf::Event event )
	{
		ui1.HandleEvent( event );
	}

	void Draw()
	{
		window->clear();
		window->pushGLStates();

		ui1.Draw( window );
		
		window->popGLStates();
		window->display();
	};

	sf::Font font;
	CUITitledContainer ui1;
	float testvalue;
};

//##############################################################
//Program entrypoint.
//##############################################################
int main()
{
    //std::cout << "Test:\n";

	//CRMPAReader reader( "EDFData/MISSION.RMPA" );

	//return 0;

    //SFML Setup.
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    settings.antialiasingLevel = 4;
    settings.majorVersion = 3;
    settings.minorVersion = 0;

	//TODO: Load from config file
	int resolutionX = 800;
	int resolutionY = 600;

    sf::RenderWindow window( sf::VideoMode(resolutionX, resolutionY), "MDB Viewer", sf::Style::Default, settings );

	//Init shader list:
	ShaderList::Initialize();

	//State manager;
	CToolStateHandler *states = new CToolStateHandler();

	//Create states:
	CStateModelRenderer *mdlRenderer = new CStateModelRenderer( );
	mdlRenderer->Init( &window ); //Feed the state a ptr to the sf window and initialise.
	//mdlRenderer->LoadMDB( "model2.mdb" );

	BaseToolState *fileBrowser = new CStateFileBrowser( );
	fileBrowser->Init( &window ); //Feed the state a ptr to the sf window and initialise.

	//CStateSceneRenderer *sceneRenderer = new CStateSceneRenderer( );
	//sceneRenderer->Init( &window ); //Feed the state a ptr to the sf window and initialise.
	//sceneRenderer->LoadScene();

	BaseToolState *testState = new CStateUITest( );
	testState->Init( &window ); //Feed the state a ptr to the sf window and initialise.

	states->AddState( "browser", fileBrowser );
	states->AddState( "viewer", mdlRenderer );
	states->AddState( "uitest", testState );
	//states->AddState( "scene", sceneRenderer );

	//Set default state
	states->SetState( "uitest" );

    //MAIN LOOP
    while (window.isOpen())
    {
        sf::Event event;
        while( window.pollEvent(event) )
        {
			if( event.type == sf::Event::Resized )
			{
				//Resize SF viewport
				sf::FloatRect visibleArea(0, 0, event.size.width, event.size.height);
    			window.setView( sf::View(visibleArea) );
			}
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
