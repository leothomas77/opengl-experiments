#version 450 core 

in float height;

out vec4 fColor;

void main() {

	if (height > 0.0)
		fColor = vec4( 0.0, height, 0.0, 1.0); 
	else
		fColor = vec4( 0.0, 0.0, -height, 1.0); 

}
