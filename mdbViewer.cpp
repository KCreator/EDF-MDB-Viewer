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
#include "RMPA.h"

#include "MeshRenderer.h"
#include "SimpleRenderer.h"

#include <SFML/Graphics.hpp>

#include "RAB.h"
#include <filesystem>
#include "Util.h"

#include "ToolState.h"
#include "MapViewer.h"

//###################################################
//Tool State System
//###################################################

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
		// Projection matrix : 45?? Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
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
		// Projection matrix : 45?? Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
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

//Temp UI test state
class CStateUITest : public BaseToolState
{
public:
	void Init( sf::RenderWindow *iwindow )
	{
		window = iwindow;

    	font.loadFromFile( "Font.ttf" );

		/*
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

		CUITextfield *txt = new CUITextfield( font, "100.0", 100 );
		txt->SetActive( true );

		ui1.AddElement( txt );

		//ui1.SetPosition( sf::Vector2f( 100, 10 ) );
		*/

		ui1 = CUITitledContainer( font, "Earth Defence Force Model, Map and RMPA Viewer", 800, 64, true );
		ui1.SetActive( true );

		CUIButton *file = new CUIButton( font, "FILE", 80, 20, 15 );
		file->SetActive( true );
		file->SetCallback( [&]{PressedFile();} );


		ui1.AddElement( file );

		CUIButton *options = new CUIButton( font, "OPTIONS", 80, 20, 15 );
		options->SetActive( true );

		ui1.AddElement( options );

		CUIButton *about = new CUIButton( font, "ABOUT", 80, 20, 15 );
		about->SetActive( true );

		ui1.AddElement( about );

		fileUI = CUITitledContainer( font, "FILE", 158, 300, false );
		fileUI.SetActive( false );

		CUIButton *button1 = new CUIButton( font, "Load MDB" );
		button1->SetActive( true );
		button1->SetCallback( [&]{Test2();} );

		fileUI.AddElement( button1 );

		fileUI.SetPosition( sf::Vector2f( 0, 64 ) );
	}

	void PressedFile()
	{
		fileUI.SetActive( true );
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
		fileUI.HandleEvent( event );
	}

	void Draw()
	{
		window->clear();
		window->pushGLStates();

		ui1.Draw( window );
		fileUI.Draw( window );
		
		window->popGLStates();
		window->display();
	};

	sf::Font font;
	CUITitledContainer ui1;
	CUITitledContainer fileUI;
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
