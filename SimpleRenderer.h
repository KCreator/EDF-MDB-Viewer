#pragma once

//Ugly little debug line renderer.
//Todo: Improve this.
class CDebugLine
{
	public:
	CDebugLine( glm::vec3 pointA, glm::vec3 pointB, glm::vec3 colour );
	~CDebugLine();
	
	void Draw( Camera cam );

protected:
	GLuint VAO;
    GLuint VBO;
	std::vector<float> vertices;
	int shaderProgram;
	glm::vec3 col;
};

//Todo: Cube, sphere and textbox renderers.
