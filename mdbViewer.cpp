#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string.h>
#include <codecvt>
#include <locale>

//OPENGL INCLUDE
#include <SFML/Window.hpp>

//Probably doesnt work.
#ifdef WINDOWS
#include <gl/GLEW.h>
#include <SFML/OpenGL.hpp>

//#include <glad/glad.h>
//#include <GLFW/glfw3.h>
#else
#include <SFML/OpenGL.hpp>
#include <GLES3/gl3.h>
#endif

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

#include "GUI.h"

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

		//Generate GUI elements:
		//mainUI = CUITitledContainer( font, "Displaying Model", 800, 64, true );
		//mainUI.SetActive( true );

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

		cameraRotationX = 90;
		cameraRotationY = 0;
		cameraDistance = 2;

		cameraPosition = glm::vec3( 0, 0, 0 );

		float cameraRadiansX = cameraRotationX * 3.14159f / 180.f;
		float cameraRadiansY = cameraRotationY * 3.14159f / 180.f;
		cameraPosition.x = sin( cameraRadiansX ) * sin( cameraRadiansY ) * cameraDistance;
		cameraPosition.y = cos( cameraRadiansX ) * cameraDistance;
		cameraPosition.z = sin( cameraRadiansX ) * cos( cameraRadiansY ) * cameraDistance;
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

			//for( int i = 0; i < meshs.size(); ++i )
			//	meshs[i]->angles.y -= (float)(mouseOldPos.x - mousePos.x) / 10.0f;

			sf::Vector2i mouseDelta = mousePos - mouseOldPos;

			cameraRotationY -= mouseDelta.x * 0.1f;
			cameraRotationX -= mouseDelta.y * 0.1f;

			if( cameraRotationY > 360 )
				cameraRotationY -= 360;

			if( cameraRotationX > 360 )
				cameraRotationX -= 360;

			float cameraRadiansX = cameraRotationX * 3.14159f / 180.f;
			float cameraRadiansY = cameraRotationY * 3.14159f / 180.f;
			cameraPosition.x = sin( cameraRadiansX ) * sin( cameraRadiansY ) * cameraDistance;
			cameraPosition.y = cos( cameraRadiansX ) * cameraDistance;
			cameraPosition.z = sin( cameraRadiansX ) * cos( cameraRadiansY ) * cameraDistance;

			mouseOldPos = mousePos;
		}
		else if( event.type == sf::Event::MouseWheelMoved )
		{
			cameraDistance -= event.mouseWheel.delta * 0.1f;
			if( cameraDistance < 0.1f )
			{
				cameraDistance = 0.1f;
			}

			float cameraRadiansX = cameraRotationX * 3.14159f / 180.f;
			float cameraRadiansY = cameraRotationY * 3.14159f / 180.f;
			cameraPosition.x = sin( cameraRadiansX ) * sin( cameraRadiansY ) * cameraDistance;
			cameraPosition.y = cos( cameraRadiansX ) * cameraDistance;
			cameraPosition.z = sin( cameraRadiansX ) * cos( cameraRadiansY ) * cameraDistance;
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

		//mainUI.Draw( window );
        window->popGLStates();
    
        window->display();
	}

	void LoadMDB( std::string ipath )
	{
		//Proccess:
		std::filesystem::path fsPath = ipath;
		std::string extension = fsPath.extension().string();
		std::transform( extension.begin(), extension.end(), extension.begin(), ::tolower );

		if( extension == ".mdb" ) //Load directly from MDB file
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
		else if( extension == ".rab" || extension == ".mrab" )
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
			std::wstring strMeshInfo = L"Objects: " + std::to_wstring( model.model.objectscount );
			std::wstring strVertexInfo = L"Number of vertices (MESH 1): " + std::to_wstring( model.model.objects[0].meshs[0].vertexnumber );

			text.setString( L"Displaying: " + strObjName + L"\n" + strMeshInfo + L"\n" + strVertexInfo );

			//Fully construct the object:
			for( int i = 0; i < model.model.objectscount; ++i )
			{
				for( int j = 0; j < model.model.objects[ i ].meshcount; ++j )
				{
					//Test folder names:
					std::wstring folder = L"HD-TEXTURE";
					if( !mdlArc.HasFolder( folder ) )
					{
						folder = L"TEXTURE";
					}

					std::vector< char > textureBytes;
					textureBytes = mdlArc.ReadFile( folder, model.GetColourTextureFilename( i, j ) );

					meshs.push_back( std::make_unique<MeshObject>( model.GetMeshPositionVertices( i, j ), model.GetMeshIndices( i, j ), model.GetMeshUVs( i, j ), programID, LoadDDS_FromBuffer( textureBytes ) ) );
				}
			}

			//Camera angle for looking at humanoid bones.
			//cameraPosition = glm::vec3( 0, 3, 2 );

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
	float cameraRotationX;
	float cameraRotationY;
	float cameraDistance;
	float fov;

	//Control variables
	sf::Vector2i mouseOldPos;
	bool isDragging;

	//GUI
	//CUITitledContainer mainUI;
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

		//sf::Mouse::setPosition( sf::Vector2i(window->getSize().x/2, window->getSize().y/2) , *window );
		//mouseOldPos = sf::Mouse::getPosition( *window );
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
		/*
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
		*/

		for (int i = 0; i < map.modelNames.size(); ++i)
		{
			map.modelNames[i] = map.modelNames2[i] + L".mdb";
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
				std::vector< char > file = mdlArc.ReadFile(L"MODEL", mdbName);
				if (file.size() == 0)
					continue;

				mdbs[mdbName] = MDBReader(file);
				file.clear();

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
					std::vector< char > file = mdlArc.ReadFile(L"MODEL", mdbName);
					if (file.size() == 0)
						continue;

					mdbs[ mdbName ] = MDBReader( file );
					file.clear();

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
			std::string extension = entry.path().extension().string();
			std::transform( extension.begin(), extension.end(), extension.begin(), ::tolower );

			if( extension == ".mdb" || extension == ".rab" || extension == ".mrab" || entry.is_directory() )
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

//File browser UI element.
class CUIFileBrowser : public CBaseUIElement
{
public:
	CUIFileBrowser(){};
	CUIFileBrowser( sf::Font &txtFont, std::string startDirectory, std::string filters )
	{
		path = std::filesystem::canonical( std::filesystem::absolute( startDirectory ) ).generic_string();
		font = txtFont;

		//Generate a file browser UI.

		float xSize = 480;
		float ySize = 480;

		//Background
		background = sf::RectangleShape( sf::Vector2f( xSize, ySize ) );
		background.setFillColor( sf::Color( 128, 128, 128 ) );

		directoryBackground = sf::RectangleShape( sf::Vector2f( xSize, 32 ) );
		directoryBackground.setOutlineThickness( 2 );
		directoryBackground.setOutlineColor( sf::Color( 64, 64, 64 ) );
		directoryBackground.setFillColor( sf::Color( 192, 192, 192 ) );

		shpDirectorySelected = sf::RectangleShape( sf::Vector2f( 32, 32 ) );
		shpDirectorySelected.setFillColor( sf::Color( 192, 192, 255 ) );

		pSlider = new CUISliderVertical( &flScrollProgress, 0, 1, ySize - 64 );
		pSlider->SetPosition( sf::Vector2f( xSize - 32, 32 ) );
		pSlider->SetActive( true );

		GenerateDirectory();
		GenerateFileList();

		//for( const auto & entry : std::filesystem::directory_iterator( path ) )
		//{
		//}

		//directoryList->AddElement();
	}

	~CUIFileBrowser()
	{
		delete pSlider;
	}

	void SetPosition( sf::Vector2f pos )
	{
		vecPosition = pos;
	};

	//Handle SF events.
	void HandleEvent( sf::Event e )
	{
		//Store new mouse position.
		if( e.type == e.MouseMoved )
			mousePos = sf::Vector2f( e.mouseMove.x, e.mouseMove.y );

		if( e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::Up )
		{
			//TODO: Use views. Will handle scrolling much better.
			for( auto& btn : texts )
			{
				btn.setPosition( btn.getPosition() + sf::Vector2f( 0, 1 ) );
			}
		}

		//Left click event, handle button inputs.
		if( e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Button::Left )
		{
			//Iterate through every directory sub-path
			for( int i = 0; i < directoryList.size(); ++i )
			{
				if( directoryList[i].getGlobalBounds().contains(sf::Vector2f(e.mouseButton.x, e.mouseButton.y) ) )
				{
					//Construct new path:
					std::string pathNew = "";
					for( int j = 0; j <= i; ++j )
					{
						pathNew += directoryList[ j ].getString();
					}

					//Set new path.
					path = pathNew;

					//Regenerate directory string.
					GenerateDirectory();
					
					//Repupulate file list
					GenerateFileList();

					break; //Exit loop.
				}
			}
		}

		//directoryList->HandleEvent( e );
		pSlider->HandleEvent( e );
	};

	//Draw elements
	void Draw( sf::RenderTarget *window )
	{
		//Draw background
		window->draw( background );
		window->draw( directoryBackground );

		sf::RenderTexture renderTexture;

		float xSize = 480;
		float ySize = 480 - 64;
		renderTexture.create( xSize, ySize );

		//Draw all other elements as part of a Render Texture (?)
		for( auto& btn : texts )
		{
			renderTexture.draw( btn );
		}

		renderTexture.display();

		sf::Sprite sprite( renderTexture.getTexture() );
		sprite.setPosition( vecPosition + sf::Vector2f( 0, 64 ) );
		window->draw( sprite );

		for( auto &dir : directoryList )
		{
			//Draw a little blue box over the folder in the directory box if applicable.
			if( dir.getGlobalBounds().contains( sf::Vector2f( mousePos ) ) )
			{
				shpDirectorySelected.setPosition( sf::Vector2f( dir.getGlobalBounds().left, dir.getGlobalBounds().top ) );
				shpDirectorySelected.setSize( sf::Vector2f( dir.getGlobalBounds().width, dir.getGlobalBounds().height ) );
				window->draw( shpDirectorySelected );
			}
			window->draw( dir );
		}

		if( pSlider )
			pSlider->Draw( window );

		//directoryList->Draw( window );
	};

	void GenerateFileList()
	{
		if( texts.size() > 0 )
			texts.clear();

		//Files in directory
		for( const auto& entry : std::filesystem::directory_iterator( path ) )
		{
			std::string extension = entry.path().extension().string();
			std::transform( extension.begin(), extension.end(), extension.begin(), ::tolower );

			if( extension == ".mdb" || extension == ".rab" || extension == ".mrab" || entry.is_directory() )
			{
				sf::Text text = sf::Text( entry.path().filename().c_str(), font, 20u );
				text.setFillColor( sf::Color( 0, 255, 0 ) );

				if( texts.size() > 0 )
					text.setPosition( sf::Vector2f( 5, texts.back().getGlobalBounds().top + texts.back().getGlobalBounds().height ) );
				else
					text.setPosition( sf::Vector2f( 5, 0 ) );


				texts.push_back( text );
			}
		}
	}

	void GenerateDirectory()
	{
		if( directoryList.size() > 0 )
			directoryList.clear();

		//Directory List
		float xOfs = 0;

		//Split path string.
		std::string splitstrn = path;
		size_t pos = splitstrn.find( "/" );
		while( pos != std::string::npos )
		{
			std::string folder = splitstrn.substr( 0, pos );
			splitstrn = splitstrn.substr( pos + 1, splitstrn.size() - pos );

			pos = splitstrn.find( "/" );

			if( folder.size() == 0 )
				continue;

			directoryList.push_back( sf::Text( folder + "/", font, 15 ) );
			directoryList.back().setPosition( xOfs, 0 );
			directoryList.back().setFillColor( sf::Color::Black );

			xOfs += directoryList.back().getGlobalBounds().width;
		}

		directoryList.push_back( sf::Text( splitstrn, font, 15 ) );
		directoryList.back().setPosition( xOfs, 0 );
		directoryList.back().setFillColor( sf::Color::Black );
	}

protected:
	std::vector< sf::Text > directoryList;

	//std::vector< sf::Text > files;

	std::vector< sf::Text > texts; //Needs to be renamed.

	//std::vector< CUIButton* > directoryButtons;
	//std::vector< CUIButton * > fileButtons;

	//I don't like this, but we need to use something like this to avoid having a reference to the main window.
	sf::Vector2f mousePos;

	std::string path;

	sf::Font font;

	sf::RectangleShape background;
	sf::RectangleShape directoryBackground;
	sf::RectangleShape shpDirectorySelected;

	float flScrollProgress;

	CUISliderVertical* pSlider;
};

//Temp UI test state
class CStateUITest : public BaseToolState
{
public:
	~CStateUITest()
	{
		if( fb )
			delete fb;
	}

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

		CUIButton *button1 = new CUIButton( font, "Load Model" );
		button1->SetActive( true );
		button1->SetCallback( [&]{Test();} );

		fileUI.AddElement( button1 );

		CUIButton* button2 = new CUIButton(font, "Load MAC");
		button2->SetActive(true);
		button2->SetCallback([&]{ Test2(); });

		fileUI.AddElement(button2);

		fileUI.SetPosition( sf::Vector2f( 0, 64 ) );

		//File Browser is incomplete.
		//fb = new CUIFileBrowser( font, "./", ".rab" );
		//fb->SetActive( true );
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
		//fb->HandleEvent( event );
	}

	void Draw()
	{
		window->clear();
		window->pushGLStates();

		ui1.Draw( window );
		fileUI.Draw( window );
		//fb->Draw( window );
		
		window->popGLStates();
		window->display();
	};

	sf::Font font;
	CUITitledContainer ui1;
	CUITitledContainer fileUI;
	CUIFileBrowser *fb;
	float testvalue;
};

#ifdef WINDOWS

#include <locale.h>

#ifndef MS_STDLIB_BUGS
#  if ( _MSC_VER || __MINGW32__ || __MSVCRT__ )
#    define MS_STDLIB_BUGS 1
#  else
#    define MS_STDLIB_BUGS 0
#  endif
#endif

#if MS_STDLIB_BUGS
#  include <io.h>
#  include <fcntl.h>
#endif

void init_locale(void)
{
#if MS_STDLIB_BUGS
	char cp_utf16le[] = ".1200";
	setlocale(LC_ALL, cp_utf16le);
	_setmode(_fileno(stdout), _O_WTEXT);
#else
	// The correct locale name may vary by OS, e.g., "en_US.utf8".
	constexpr char locale_name[] = "";
	setlocale(LC_ALL, locale_name);
	std::locale::global(std::locale(locale_name));
	std::wcin.imbue(std::locale())
		std::wcout.imbue(std::locale());
#endif
}

#endif

/*
std::wstring OpenFileDialogue( LPCWSTR filter = L"All Files (*.*)\0*.*\0", HWND owner = NULL )
{
	OPENFILENAMEW ofn;
	wchar_t fileName[ MAX_PATH ] = L"";
	ZeroMemory( &ofn, sizeof( ofn ) );
	ofn.lStructSize = sizeof( OPENFILENAME );
	ofn.hwndOwner = owner;
	ofn.lpstrFilter = filter;
	ofn.lpstrFile = fileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = L"";
	std::wstring fileNameStr;

	if( GetOpenFileNameW( &ofn ) )
		fileNameStr = fileName;
	return fileNameStr;
}
*/

//#include "CAS.h"

//##############################################################
//Program entrypoint.
//##############################################################
int wmain( int argc, wchar_t* argv[] )
{
    //std::cout << "Test:\n";

	//CRMPAReader reader( "EDFData/MISSION.RMPA" );

	//CAS cas = CAS( "EDFData/ARMYSOLDIER.CAS" );
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

#ifdef WINDOWS
	init_locale(); //Init locale
	glewInit(); // Init GLEW.
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		/* Problem: glewInit failed, something is seriously wrong. */
		fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
	}

	//This is an option, but lets not use it yet.
	/*
	// Get the native window handle
	HWND hWnd = window.getSystemHandle();

	HMENU hMenu = CreatePopupMenu();
	AppendMenu( hMenu, MF_STRING, 1, L"New" );
	AppendMenu( hMenu, MF_STRING, 2, L"Open" );
	AppendMenu( hMenu, MF_STRING, 3, L"Save" );
	AppendMenu( hMenu, MF_SEPARATOR, 0, NULL );
	AppendMenu( hMenu, MF_STRING, 4, L"Exit" );

	// Create the "File" menu bar using the Windows API CreateMenu function
	HMENU hMenuBar = CreateMenu();
	AppendMenu( hMenuBar, MF_POPUP, (UINT_PTR)hMenu, L"File" );

	// Set the "File" menu bar as the window menu using the Windows API SetMenu function
	SetMenu( hWnd, hMenuBar );
	*/

#endif

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

	//Attempt to load model based on drag+drop if possible
	if( argc > 1 )
	{
		std::filesystem::path fsPath = argv[ 1 ];

		std::wcout << L"LOADING MODEL: " + fsPath.wstring() + L"\n";

		std::string extension = fsPath.extension().string();
		std::transform( extension.begin(), extension.end(), extension.begin(), ::tolower );

		if( extension == ".rab" || extension == ".mrab" )
		{
			mdlRenderer->LoadMDB( fsPath.string() );
			states->SetState( "viewer" );
		}
	}

	sf::Clock frameTime;

	std::unique_ptr<CMDBRenderer> testMesh = std::make_unique< CMDBRenderer >();

    //MAIN LOOP
    while( window.isOpen() )
    {
        sf::Event event;
        while( window.pollEvent(event) )
        {
			if( event.type == sf::Event::Resized )
			{
				//Resize SF viewport
				sf::FloatRect visibleArea( 0, 0, event.size.width, event.size.height );
    			window.setView( sf::View( visibleArea ) );
			}
            if( event.type == sf::Event::Closed )
                window.close();
            
			states->ProccessEvent( event );
        }

        //states->Draw(); //Todo: Place common draw steps before and after state.

		//TEMP
		Camera cam;
		cam.Projection = glm::perspective( glm::radians( 45.0f ), (float)window.getSize().x / (float)window.getSize().y, 0.1f, 100.0f );
		cam.View = glm::lookAt( glm::vec3( 0, 0, 5 ), glm::vec3( 0, 0, 0 ), glm::vec3( 0, 1, 0 ) );
		window.clear();
		glClear( GL_DEPTH_BUFFER_BIT );
		testMesh->Draw( cam );


		window.display();

		float dT = frameTime.restart().asSeconds();
		//std::wcout << std::to_wstring( dT ) + L"\n";
    }

	//Erase states, calling deconstructors.
	delete states;

    //Todo: Nicely clean up OpenGL and other such memory.

	/*
#ifdef WINDOWS
	// Cleanup
	DestroyMenu( hMenu );
	DestroyMenu( hMenuBar );
#endif
	*/

    return 0;
}
