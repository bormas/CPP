#version 330

in vec4 point;
in vec4 variance;

in vec2 position;

in float size;
in float angle;

out vec4 height;

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

    mat4 scaleMatrix = mat4(0.01); 
    scaleMatrix[1][1] = 0.03; //высота	
	scaleMatrix[3][3] = size / 2; //размер

    mat4 positionMatrix = mat4(1.0);
    positionMatrix[3][0] = position.x;
    positionMatrix[3][2] = position.y;

    gl_Position = camera * (positionMatrix * rotationMatrix * scaleMatrix * point  + variance / 150 * point.y);

	height = point;
}
