#version 330

in vec4 point;

in vec2 position;
in vec2 texture_coord;

in float size;
in float angle;

out vec2 texture_rock_coord;
out float height;

uniform mat4 camera;

mat4 rotate_y(float angle)
{
    float s = sin(angle);
    float c = cos(angle);
    
    return mat4(  c,  0.0,   s, 0.0,
                0.0,  1.0, 0.0, 0.0,
                 -s,  0.0,   c, 0.0,
                0.0,  0.0, 0.0, 1.0);
}

void main() {

	mat4 rotationMatrix = rotate_y(angle);

    mat4 scaleMatrix = mat4(0.001);    
	scaleMatrix[3][3] = size * 0.6; //размер

    mat4 positionMatrix = mat4(1.0);
    positionMatrix[3][0] = position.x;
    positionMatrix[3][2] = position.y;

    gl_Position = camera * (positionMatrix * rotationMatrix * scaleMatrix * point);

    texture_rock_coord = texture_coord;

	height = point.y;
}
