varying vec2 texCoord;
varying vec2 eyeCoord;

uniform sampler2D texture;
uniform sampler2D lightmap;
uniform sampler2D logo;

const float AMBIENT = 0.4;
const float SPECULAR = 0.7;

const float LOGO_ALPHA = 0.25;
const float LOGO_ASPECT = 4.0;
const float LOGO_SIZE = 0.5;
const vec2 LOGO_DISPLACEMENT = vec2(0.02, 0);

void main() {
    vec2 logoCoord = (texCoord * vec2(1.0 / LOGO_SIZE, LOGO_ASPECT / LOGO_SIZE)) + vec2(0.5);
    vec4 logoSample = texture2D(logo, logoCoord);
    float logoShade = logoSample.r * LOGO_ALPHA;
    float logoDisp = logoSample.g;

    float light = texture2D(lightmap, eyeCoord).r;

    vec4 texFront = texture2D(texture, texCoord);
    vec4 texBack = texture2D(texture, texCoord + LOGO_DISPLACEMENT);

    gl_FragColor = (mix(texFront, texBack, logoDisp) + logoShade) * (AMBIENT + SPECULAR * light);
}
