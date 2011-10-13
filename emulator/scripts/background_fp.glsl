varying vec2 texCoord;

uniform float scale;
uniform sampler2D texture;

const float specular = 0.5;
const float brightness = 0.7;
const vec2 center = vec2(0.7, 1.4);

void main() {
     vec4 bg = texture2D(texture, (texCoord - 0.5) * scale);

     vec2 lv = texCoord - center;
     float light = brightness / pow(lv.x * lv.x + lv.y * lv.y, specular);
     gl_FragColor = bg * light;
}
