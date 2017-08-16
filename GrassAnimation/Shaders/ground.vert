#version 330 

in vec2 in_texture_ground_coord;
in vec4 point; 

out vec2 texture_ground_coord; 

uniform mat4 camera;

void main() {
	mat4 scaleMatrix = mat4(1.0);
    scaleMatrix[3][3] = 0.95; //размер

	gl_Position = camera * scaleMatrix * point; 
    texture_ground_coord = in_texture_ground_coord;
}