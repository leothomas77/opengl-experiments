#version 450 core 

in vec2 aPosition;

uniform mat4 uMVP;
uniform mat4 uM;
uniform mat4 uV;
uniform mat4 uP;
uniform float T;

out float 	height;

void main() {

	float A = 5.0;

	float w = 0.2;

	height = A * sin(aPosition.x*w + T) * cos(aPosition.y*w + T);

    gl_Position = uMVP * vec4(aPosition.x, height, aPosition.y, 1.0);

}
