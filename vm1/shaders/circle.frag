#version 310 es

precision mediump float;

in vec2 texCoord;
out vec4 fragColor;

const vec2 OUT_TEX_SIZE = vec2(1920.0f, 1080.0f);
uniform float iTime;
uniform float offsetX;
uniform float offsetY;
uniform float scale;
///uniform float radius;

float sdCircle( in vec2 p, in float r ) 
{
    return length(p)-r;
}

void main() {
	vec2 fragCoord = gl_FragCoord.xy;
	vec2 iResolution = OUT_TEX_SIZE;

    vec2 p = (2.0*fragCoord-iResolution.xy)/iResolution.y;
    p.x -= offsetX;
    p.y += offsetY;
	float effectiveScale = scale + 0.1;

    float d = sdCircle(p, abs(sin(iTime)) * effectiveScale);
	vec3 col = mix(vec3(0.9, 0.6, 0.3), vec3(0.65, 0.85, 1.0), 1.0-smoothstep(0.0, 0.001, d));

	fragColor = vec4(col,1.0);
}