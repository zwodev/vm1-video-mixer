// https://www.shadertoy.com/view/XsBfRW

#version 310 es

precision mediump float;

in vec2 texCoord;
out vec4 fragColor;

const vec2 OUT_TEX_SIZE = vec2(1920.0f, 1080.0f);
uniform float iTime;
uniform float speed;    // { "name": "Speed", "default": 1.0, "min": 0.01, "max": 10.0, "step": 0.01 }
uniform float offsetX;  // { "name": "Offset X", "default": 0.0, "min": -2.0, "max": 2.0, "step": 0.01 }
uniform float offsetY;  // { "name": "Offset Y", "default": 0.0, "min": -2.0, "max": 2.0, "step": 0.01 }
uniform float scale;    // { "name": "Scale", "default": 1.0, "min": 0.01, "max": 10.0, "step": 0.01 }

void main() {
	vec2 fragCoord = gl_FragCoord.xy;
	vec2 iResolution = OUT_TEX_SIZE;
	
    float aspect = iResolution.y/iResolution.x;
    float value;
	vec2 uv = fragCoord.xy / iResolution.x;
    uv *= scale;
    vec2 offset = vec2(offsetX, offsetY);
    uv -= vec2(0.5, 0.5*aspect) + offset;
    float t = iTime * speed;
    float rot = radians(45.0); // radians(45.0*sin(t));
    mat2 m = mat2(cos(rot), -sin(rot), sin(rot), cos(rot));
   	uv  = m * uv;
    uv += vec2(0.5, 0.5*aspect);
    uv.y+=0.5*(1.0-aspect);
    vec2 pos = 10.0*uv;
    vec2 rep = fract(pos);
    float dist = 2.0*min(min(rep.x, 1.0-rep.x), min(rep.y, 1.0-rep.y));
    float squareDist = length((floor(pos)+vec2(0.5)) - vec2(5.0) );
    
    float edge = sin(t-squareDist*0.5)*0.5+0.5;
    
    edge = (t-squareDist*0.5)*0.5;
    edge = 2.0*fract(edge*0.5);
    //value = 2.0*abs(dist-0.5);
    //value = pow(dist, 2.0);
    value = fract (dist*2.0);
    value = mix(value, 1.0-value, step(1.0, edge));
    //value *= 1.0-0.5*edge;
    edge = pow(abs(1.0-edge), 2.0);
    
    //edge = abs(1.0-edge);
    value = smoothstep( edge-0.05, edge, 0.95*value);
    
    
    value += squareDist*.1;
    //fragColor = vec4(value);
    fragColor = mix(vec4(1.0,1.0,1.0,1.0),vec4(0.5,0.75,1.0,1.0), value);
    //fragColor.a = 0.25*clamp(value, 0.0, 1.0);
}


