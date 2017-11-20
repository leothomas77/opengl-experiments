#version 120


attribute vec4 vertexPos;

varying vec4 fragPos;

void main(){
	fragPos = vertexPos;
	gl_Position = vertexPos;
}