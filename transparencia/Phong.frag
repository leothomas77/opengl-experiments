#version 120

// precision mediump float; 

uniform vec3 uLPos;
uniform vec3 uCamPos;

varying vec3 vNormal; 
varying vec3 vPosW;

vec4 calcularPhong(vec4 lColor, vec4 matAmb, vec4 matDif, vec4 matSpec, 
	vec3 uLPos, vec3 uCamPos, vec3 vNormal, vec3 vPosW);

void main(void) {
	vec4 lColor		= vec4(1.0, 0.0, 0.0, 0.0); 
	vec4 matAmb		= vec4(0.2, 0.0, 0.0, 0.0); 
	vec4 matDif 	= vec4(1.0, 0.0, 0.0, 0.5); //apenas este leva o valor de alpha
	vec4 matSpec	= vec4(1.0, 1.0, 1.0, 0.0);
		
	gl_FragColor = calcularPhong(lColor, matAmb, matDif, matSpec, uLPos, uCamPos, vNormal, vPosW); 
}

vec4 calcularPhong(vec4 lColor, vec4 matAmb, vec4 matDif, vec4 matSpec, 
	vec3 uLPos, vec3 uCamPos, vec3 vNormal, vec3 vPosW) {

	vec4 ambient = vec4(lColor.rgb * matAmb.rgb, matAmb.a); 

	vec3 vL = normalize(uLPos - vPosW); 
	float cTeta = max(dot(vL, vNormal), 0.0); 
			
	vec4 diffuse = vec4(lColor.rgb * matDif.rgb * cTeta, matDif.a); 

	vec3 vV = normalize(uCamPos - vPosW); 
	vec3 vR = normalize(reflect(-vL, vNormal)); 
	float cOmega = max(dot(vV, vR), 0.0); 
	vec4 specular = vec4(lColor.rgb * matSpec.rgb * pow(cOmega,20.0), matSpec.a); 
	return clamp(ambient + diffuse + specular, 0.0, 1.0);
} 	
