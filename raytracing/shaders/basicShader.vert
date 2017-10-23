#version 120

attribute vec3 aPosition;
attribute vec4 aColor;

//uniform mat4 uMVP;

varying vec4 vColor;

void main() {
	vColor = aColor;
  gl_Position =  vec4(aPosition, 1.0);
}
