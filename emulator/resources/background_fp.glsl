varying vec2 texCoord;
varying vec2 eyeCoord;

uniform sampler2D texture;
uniform sampler2D lightmap;

const float AMBIENT = 0.4;
const float SPECULAR = 0.7;

void main() {
    gl_FragColor = texture2D(texture, texCoord) * (AMBIENT + SPECULAR * texture2D(lightmap, eyeCoord).r);
}
