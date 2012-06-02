varying vec2 texCoord;

uniform sampler2D texture;

const float numChannels = 8.0;
const float focus = 30.0;

const vec4 bg = vec4(0.00, 0.19, 0.39, 0.4);
const vec4 fg = vec4(1.00, 1.50, 1.00, 1.0);

const vec4 shadow = vec4(0.0, 0.0, 0.0, 1.0);

void main()
{
    float x = texCoord.x * numChannels;
    float channelNum = floor(x);
    float samplePos = x - channelNum;

    float sample = texture2D(texture, vec2(samplePos, channelNum / numChannels)).r;
    float closeness = pow(1.0 + abs(sample - texCoord.y), -focus);

    float shadowAlpha = (samplePos + texCoord.y) * 0.2;

    gl_FragColor = mix(mix(bg, fg, closeness), shadow, shadowAlpha);
}
