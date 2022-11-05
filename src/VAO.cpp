#include"VAO.h"

VAO::VAO() { glGenVertexArrays(1, &ID); }

void VAO::link_attrib(VBO& VBO, GLuint layout, GLuint num_components, GLenum type, GLsizeiptr stride, void* offset)
{
	VBO.bind();
	glVertexAttribPointer(layout, num_components, type, GL_FALSE, stride, offset);
	glEnableVertexAttribArray(layout);
	VBO.unbind();
}

void VAO::bind() { glBindVertexArray(ID); }

void VAO::unbind() { glBindVertexArray(0); }

void VAO::cleanup() { glDeleteVertexArrays(1, &ID); }
