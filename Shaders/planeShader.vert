#version 450 core 

in vec2 aPosition;

uniform mat4 uMVP;
uniform mat4 uM;
uniform mat4 uV;
uniform mat4 uP;

out vec3 wPos;

void main() {
    gl_Position = uP * uV * uM * vec4(aPosition.x, 0.0, aPosition.y, 1.0);

    vec4 p =  uM * vec4(aPosition.x, 0.0, aPosition.y , 1.0);

    wPos = p.xyz;   
}
