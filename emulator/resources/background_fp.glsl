varying vec2 texCoord;
varying vec2 eyeCoord;

uniform sampler2D texture;
uniform sampler2D lightmap;

void main() {
    gl_FragColor = texture2D(texture, texCoord) * texture2D(lightmap, eyeCoord).r;
}
