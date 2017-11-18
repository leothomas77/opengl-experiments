#version 450 core 

in vec3 aPosition;
in vec4 aColor;

uniform mat4 uMVP;

out vec4 vColor;

void main() {

	vColor = vec4(aPosition, 1.0);

    gl_Position = uMVP * vec4(aPosition, 1.0);
    
}
