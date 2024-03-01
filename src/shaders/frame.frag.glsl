#version 330 core
out vec4 FragColor;
in vec2 Texcoord;
uniform sampler2D texture0;
uniform sampler1D textureC;
uniform vec2 minmax;
uniform int opacity_function;

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
        switch (opacity_function) {
            case 0: // Linear
                FragColor.a = transfer_fct_linear(val);
                break;
            case 1: // Linear_r
                FragColor.a = 1. - transfer_fct_linear(val);
                break;
            case 2: // Centered
                FragColor.a = transfer_fct_diff(val);
                break;
            case 3: // 1.0
                FragColor.a = 1.0;
                break;
            case 4: // 0.75
                FragColor.a = 0.75;
                break;
            case 5: // 0.5
                FragColor.a = 0.5;
                break;
            case 6: // 0.25
                FragColor.a = 0.25;
                break;
            default:
                FragColor.a = 0.0;
                break;
        }
    }
}