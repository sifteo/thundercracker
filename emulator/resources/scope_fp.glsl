varying vec2 texCoord;

uniform sampler2D texture;

void main() {
    vec4 texel = texture2D(texture, texCoord);
    gl_FragColor = vec4(texel.x, texel.y, 0.5, 0.75);
}
