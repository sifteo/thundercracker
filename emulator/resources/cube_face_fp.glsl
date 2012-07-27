const float HILIGHT = 1.0;

varying vec2 lcdCoord;
varying vec2 hilightCoord;

uniform sampler2D lcd;
uniform sampler2D hilight;

vec4 lcdPixel(vec2 coord)
{
#ifdef FILTER
     vec2 p = coord*128.0+0.5;
     vec2 i = floor(p);
     vec2 f = p-i;
     f = f*f*f*(f*(f*6.0-15.0)+10.0);
     p = i+f;
     p = (p-0.5)/128.0;
     return texture2D(lcd, p);
#else
     return texture2D(lcd, coord);
#endif
}

vec4 lcdEdgeFilter(vec4 lcdColor, vec4 bgColor, float lcdDist)
{
#ifdef FILTER
    return mix(bgColor, lcdColor, clamp((0.5 - lcdDist) * 192.0, 0.0, 1.0));
#else
    return lcdDist > 0.5 ? bgColor : lcdColor;
#endif
}

void main() {
     vec4 hilight = texture2D(hilight, hilightCoord) * HILIGHT;

     vec2 lcdCoordAbs = abs(lcdCoord);
     float lcdDist = max(lcdCoordAbs.x, lcdCoordAbs.y);

     gl_FragColor = lcdEdgeFilter(lcdPixel(lcdCoord + 0.5), hilight, lcdDist);
}
