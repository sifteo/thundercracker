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

void main() {
    vec4 texColor = texture2D(texture, texCoord);

    vec2 logoCoord = (texCoord * vec2(1.0 / LOGO_SIZE, LOGO_ASPECT / LOGO_SIZE)) + vec2(0.5);
    float logoShade = texture2D(logo, logoCoord).r - 0.5;

    float light = texture2D(lightmap, eyeCoord).r;

    gl_FragColor = (texColor + logoShade * LOGO_ALPHA) * (AMBIENT + SPECULAR * light);
}
