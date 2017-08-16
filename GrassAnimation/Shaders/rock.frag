#version 330

in vec2 texture_rock_coord;
in float height;

out vec4 outColor;

uniform sampler2D rock_texture; 

void main() {
    outColor = mix(vec4(0.5 + 3 * height, 0.5 + 3 * height, 0.5 + 3 * height, 1),
				   texture(rock_texture, texture_rock_coord), 0.8);
}
