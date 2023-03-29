#include <vector>
#include <map>


#include <SFML/Window.hpp>

//OPENGL INCLUDE
#ifndef WINDOWS
#include <GLES3/gl3.h>
#else
#include <GL/glew.h>
#endif

#include <SFML/OpenGL.hpp>

#include <glm/gtc/matrix_transform.hpp>

#include "MeshRenderer.h"
#include "SimpleRenderer.h"

//Not ideal.
void GenerateLineShaders()
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
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	// check for linking errors

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	ShaderList::AddShader( "LineShader", shaderProgram );
};

CDebugLine::CDebugLine( glm::vec3 pointA, glm::vec3 pointB, glm::vec3 colour )
{
	if( !ShaderList::HasShader( "LineShader" ) )
	{
		GenerateLineShaders();
	}
	
	shaderProgram = ShaderList::GetShader( "LineShader" );

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
CDebugLine::~CDebugLine()
{
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	//glDeleteProgram(shaderProgram);
}

void CDebugLine::Draw( Camera cam )
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

