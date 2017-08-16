#version 330

in vec4 point;
in vec4 variance;

in vec2 position;
in vec2 texture_coord;

in float size;
in float angle;
in float yellow;

out vec2 texture_grass_coord;
out vec4 height;
out float yellow_flag;

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

    mat4 scaleMatrix = mat4(1.0);
    
	scaleMatrix[0][0] = 0.008; //длина по х
    scaleMatrix[1][1] = 0.05;  //высота	
	scaleMatrix[3][3] = size;  //размер

    mat4 positionMatrix = mat4(1.0);
    positionMatrix[3][0] = position.x;
    positionMatrix[3][2] = position.y;

    gl_Position = camera * (positionMatrix * rotationMatrix * scaleMatrix * point  + variance * point.z);

	//out
    texture_grass_coord = texture_coord;
	height = point;
	yellow_flag = yellow;
}
