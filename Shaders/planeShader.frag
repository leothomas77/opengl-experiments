#version 450 core 

in vec3 wPos;

out vec4 fColor;

void main() {

    fColor = vec4(wPos.x+10.0 / 20.0, 1.0, wPos.z+10.0 / 20.0, 1.0);

}
