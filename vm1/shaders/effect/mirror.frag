// uniform int enabled;        // { "name": "Enabled", "group": "Shape Inverter", "default": 1, "min": 0, "max": 1 }
// uniform int shape;          // { "name": "Shape Type", "group": "Shape Inverter", "default": 0, "min": 0, "max": 2 }
// uniform float px;            // { "name": "pos x", "group": "Shape Inverter", "default": 0.0, "min": -1.0, "max": 1.0, "step": 0.01 }
uniform float xAxis;            // { "name": "horizontal axis", "group": "Mirror", "default": 0.5, "min": 0.0, "max": 1.0, "step": 0.01 }



void extMain(inout vec3 color, in vec2 coord)
{
    float s = min(floor(1.0 + xAxis - coord.y), 1.0);
    vec2 uv = coord;
    
    if(xAxis < 0.5)
        uv.y = mix(coord.y, xAxis*2.0 - coord.y, s);
    else
        uv.y = mix(xAxis*2.0 - coord.y, coord.y, s);

    color = colorAtUV(uv);
}