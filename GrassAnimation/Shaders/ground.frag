#version 330 

in vec2 texture_ground_coord; 

out vec4 outColor; 

uniform sampler2D ground_texture; 

void main() {
    outColor = texture(ground_texture, texture_ground_coord); 
}