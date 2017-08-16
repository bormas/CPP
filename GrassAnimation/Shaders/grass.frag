#version 330

in vec2 texture_grass_coord;
in vec4 height;
in float yellow_flag;

out vec4 outColor;

uniform sampler2D grass_texture; 

void main() {
	float yellow_procent = 0.1;
	if	(yellow_flag < yellow_procent)
	{
		outColor = mix(vec4(5 * yellow_procent + 2.3 * height.y, 5 * yellow_procent + 1.6 * height.y, 0.3 * height.y, 1),
					   texture(grass_texture, texture_grass_coord), 0.6);
	} else {
		float k =  3 * height.y;
		outColor = mix(vec4(0.5 + k, 0.5 + k, k, 1),
					   texture(grass_texture, texture_grass_coord), 0.8);
	}
}
