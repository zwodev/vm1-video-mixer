bool enabled = true;
float r = 0.2;
float strokeWidth = 0.2;
float shapeValue = 0.0;

float sdCircle(vec2 p, float r) {
    return length(p) - r;
}

float sdBox(vec2 p, float r) {
    vec2 q = abs(p) - r;
    return length(max(q,0.0)) + min(max(q.x,q.y),0.0);
}

float sdTriangle(vec2 p, float r) {
    const float k = sqrt(3.0);
    p.x = abs(p.x) - r;
    p.y = p.y + r/k;
    if( p.x+k*p.y > 0.0 ) p=vec2(p.x-k*p.y, -k*p.x-p.y) / 2.0;
    p.x = p.x - clamp( p.x, -2.0*r, 0.0 );
    return -length(p) * sign(p.y);
}

float shapeSDF(vec2 p, float shape, float r) {
    float d1 = sdCircle(p, r);
    float d2 = sdBox(p, r * 0.8);
    float d3 = sdTriangle(p, r);
    
    // 0=circle → 1=box → 2=triangle
    float t1 = smoothstep(0.0, 1.0, shape);
    float t2 = smoothstep(1.0, 2.0, shape);
    
    return mix(mix(d1, d2, t1), d3, t2);
}

float shapeMask(vec2 p, float r, float strokeWidth, float shape) {
    float dist = shapeSDF(p, shape, r);
    float outerMask = smoothstep(0.0, 0.001, -dist);
    float innerMask = smoothstep(0.0, 0.001, dist + strokeWidth);
    return outerMask * innerMask;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord/iResolution.xy;
    
    float aspectRatio = iResolution.y / iResolution.x;

    vec3 texColor = texture(iChannel0, uv).rgb;
    
    vec2 p = uv - 0.5;
    p.y *=aspectRatio;
    float maskValue = shapeMask(p, r, strokeWidth, shapeValue);
    
    
    vec3 finalColor = mix(texColor, 1.0 - texColor, maskValue * float(enabled));
    
    /*float debugColor = float(strokeWidth);
    
    if (fragCoord.x < iResolution.x * 0.2 && fragCoord.y < iResolution.y * 0.2) {
        finalColor = finalColor * 0.0 + vec3(debugColor);
    }*/
    fragColor = vec4(finalColor,1.0);
}