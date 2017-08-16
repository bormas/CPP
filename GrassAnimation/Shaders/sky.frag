#version 330 

in vec2 texture_sky_coord; 

out vec4 outColor; 

uniform sampler2D sky_texture; 

void main() {
    outColor = texture(sky_texture, texture_sky_coord);
}