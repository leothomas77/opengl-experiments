#version 450 core

in vec3 aPosition;
in vec3 aNormal;

uniform mat4 uM;
uniform mat4 uN;
uniform mat4 uMVP;

uniform vec3 uLPos;
uniform vec3 uCamPos;
	
out vec4 vColor; 
	
void main(void) { 
	vec4 lColor		= vec4(1.0, 1.0, 1.0, 1.0); 
	vec4 matAmb		= vec4(0.1, 0.1, 0.1, 1.0); 
	vec4 matDif 	= vec4(0.3, 1.0, 0.6, 1.0); 
	vec4 matSpec	= vec4(1.0, 1.0, 1.0, 1.0); 
	
	gl_Position = uMVP * vec4( aPosition, 1.0 ); 
	
	vec3 vPosW = (uM * vec4(aPosition, 1.0)).xyz; 
	vec3 vNormal = normalize((uN * vec4(aNormal, 1.0)).xyz); 
	
	vec4 ambient = vec4(lColor.rgb * matAmb.rgb, matAmb.a); 
	
	vec3 vL = normalize(uLPos - vPosW); 
	float cTeta = max(dot(vL, vNormal), 0.0); 
			
	vec4 diffuse = vec4(lColor.rgb * matDif.rgb * cTeta, matDif.a); 
	
	vec3 vV = normalize(uCamPos - vPosW); 
	vec3 vR = normalize(reflect(-vL, vNormal)); 
	float cOmega = max(dot(vV, vR), 0.0); 
	vec4 specular = vec4(lColor.rgb * matSpec.rgb * pow(cOmega,20.0), matSpec.a); 
	
	vColor = clamp(ambient + diffuse + specular, 0.0, 1.0); 
	} 
