#version 120

vec4 gl_FragCoord;

 vec4 d_abuffer[600 * 600];
 int d_abufferIdx[600 * 600];

void main(void) {

	ivec2 coords=ivec2(gl_FragCoord.xy);
	
	if(coords.x >= 0 && coords.y >= 0 && coords.x < 600 && coords.y < 600 ){
		d_abufferIdx[coords.x + coords.y * 600] = 0;
		d_abuffer[coords.x + coords.y * 600] = vec4(0.0f);
	}

    //Descarta fragmento fora da tela
	discard;
}