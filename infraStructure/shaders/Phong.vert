#version 120

attribute vec3 aPosition;
attribute vec3 aNormal;
attribute vec3 aColor;

uniform mat4 uM;
uniform mat4 uN;
uniform mat4 uMVP;

varying vec3 vNormal; 
varying vec3 vPosW;

void main(void) { 		
	vPosW = (uM * vec4(aPosition, 1.0)).xyz; 
	vNormal = normalize((uN * vec4(aNormal, 1.0)).xyz); 			
	
	gl_Position = uMVP * vec4( aPosition, 1.0 );  
	} 
