#version 330

in vec4 height;

out vec4 outColor;

void main() {
	if(height.y > 1)
	{
		outColor = vec4( 0.8 + height.y / 0.15, 0.8 + height.y / 0.15, 0.1 + height.y / 0.15, 1);
	} else 
	{
		if(height.y == 1)
			outColor = vec4(1, 0.8, 0.1, 1);
		else
			outColor = vec4(0.6 + height.y / 2, 0.8 + height.y / 2, 0.2 + height.y / 2, 1);
	}
}
