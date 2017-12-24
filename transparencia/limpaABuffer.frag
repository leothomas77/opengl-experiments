#version 120

vec4 gl_FragCoord;

vec4 d_abuffer[3600];
int d_abufferIdx[3600];

void main(void) {
	
	ivec2 coords=ivec2(gl_FragCoord.xy);
	if(coords.x >= 0 && coords.y >= 0 && coords.x < 600 && coords.y < 600 ){
		d_abufferIdx[coords.x + 599 * coords.y] = 0;
		d_abuffer[coords.x + 599 * coords.y] = vec4(0.0f);
	}
    // Descarta fragmento fora da tela 
	discard;
}