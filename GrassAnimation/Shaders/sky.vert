#version 330 

in vec4 point; 
in vec2 in_texture_sky_coord;

out vec2 texture_sky_coord; 

uniform mat4 camera;

void main() {
	mat4 scaleMatrix = mat4(1.0);
	scaleMatrix[3][3] = 0.1;

	mat4 positionMatrix = mat4(1.0);
    positionMatrix[3][0] = -1.05;
    positionMatrix[3][2] = -1.05;

    gl_Position = camera * positionMatrix * scaleMatrix * point; 
	

    texture_sky_coord = in_texture_sky_coord;
}