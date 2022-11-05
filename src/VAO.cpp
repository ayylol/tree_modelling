#include"VAO.h"

VAO::VAO() { glGenVertexArrays(1, &ID); }

void VAO::linkVBO(VBO& VBO, GLuint layout)
{
	VBO.bind();
	glVertexAttribPointer(layout, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glEnableVertexAttribArray(layout);
	VBO.unbind();
}

void VAO::bind() { glBindVertexArray(ID); }

void VAO::unbind() { glBindVertexArray(0); }

void VAO::cleanup() { glDeleteVertexArrays(1, &ID); }
