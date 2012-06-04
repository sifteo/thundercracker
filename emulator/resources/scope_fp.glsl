varying vec2 texCoord;
varying float alpha;

// X = sample number, Y = channel number, L = unsigned sample value
uniform sampler2D sampleBuffer; 

// RGB = background color, A = trace mask
uniform sampler2D background;

const float numChannels = 8.0;
const float focus = 60.0;
const vec3 fg = vec3(1.8, 2.7, 1.8);

void main()
{
    float x = texCoord.x * numChannels;
    float channelNum = floor(x);
    float samplePos = x - channelNum;

    float sample = 1.0 - texture2D(sampleBuffer, vec2(samplePos, channelNum / numChannels)).r;
    float closeness = pow(1.0 + abs(sample - texCoord.y), -focus);

    vec4 bgImage = texture2D(background, vec2(x, texCoord.y));

    gl_FragColor = mix(vec4(bgImage.rgb, alpha),
                       vec4(fg, alpha * 2.0),
                       closeness * bgImage.a);
}
