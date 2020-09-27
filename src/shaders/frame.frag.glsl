#version 330 core
out vec4 FragColor;
in vec2 Texcoord;
uniform sampler2D texture0;
uniform sampler1D textureC;
uniform vec2 minmax;
uniform bool use_transfer_fct;
uniform int transfer_fct_version;

float transfer_fct_linear(float x) {
    return smoothstep(0., 1., x);
}
float transfer_fct_diff_pos(float x) {
    if (x < 0.5) return 0.;
    return smoothstep(0.5, 1., abs(x));
}
float transfer_fct_diff_neg(float x) {
    if (x > 0.5) return 0.;
    x = x - 0.5;
    return smoothstep(0., 0.5, abs(x));
}
float transfer_fct_diff(float x) {
    if (x < 0.5) {
        return transfer_fct_diff_neg(x);
    } else {
        return transfer_fct_diff_pos(x);
    }
}
void main() {
    float val = texture(texture0, Texcoord).r;
    if (isnan(val)) {
        //discard;
        FragColor = vec4(0.0, 0.0, 0.0, 0.0);
    } else {
        val = (val - minmax.x) / (minmax.y - minmax.x);
        val = clamp(val, 0.0, 1.0);
        FragColor = texture(textureC, val);
        if (use_transfer_fct) {
            switch (transfer_fct_version) {
                case 1:
                FragColor.a = transfer_fct_diff(val);
                break;
                case 2:
                FragColor.a = transfer_fct_diff_pos(val);
                break;
                case 3:
                FragColor.a = transfer_fct_diff_neg(val);
                break;
                default :
                FragColor.a = transfer_fct_linear(val);
                break;
            }
        }
    }
}