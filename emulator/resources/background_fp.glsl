varying vec2 texCoord;
varying vec2 eyeCoord;

uniform sampler2D texture;

const float specular = 0.3;
const float brightness = 0.7;
const vec2 center = vec2(-0.25, -1.5);

void main() {
     vec4 bg = texture2D(texture, texCoord);

     vec2 lv = eyeCoord - center;
     float light = brightness / pow(lv.x * lv.x + lv.y * lv.y, specular);
     gl_FragColor = bg * light;
}
