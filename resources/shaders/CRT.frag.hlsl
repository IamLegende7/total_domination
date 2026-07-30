// Copied from SDL3/tests (and edited)
// TODO: make the scanlines depend on the games pixel size (~ZOOM)

// This shader is adapted from the Lightweight CRT Effect at:
// https://godotshaders.com/shader/lightweight-crt-effect/

cbuffer Context : register(b0, space3) {
    float2 resolution; // (width, height) of the source texture in pixels
};

Texture2D u_texture : register(t0, space2);
SamplerState u_sampler : register(s0, space2);

struct PSInput {
    float4 v_color : COLOR0;
    float2 v_uv : TEXCOORD0;
};

struct PSOutput {
    float4 o_color : SV_Target;
};

static const float PI = 3.14159265f;

// Tweakables (0-1 mostly)
static const float scan_line_amount = 0.5; // Range 0-1
static const float warp_amount      = 0.1; // Range 0-1
static const float vignette_amount  = 0.5; // Range 0-1
static const float vignette_intensity = 0.3; // Range 0-1
static const float grille_amount   = 0.05; // Range 0-1
static const float brightness_boost = 1.2; // Range 1-2

PSOutput main(PSInput input) {
    PSOutput output;

    float2 uv = input.v_uv;

    // Warp
    float2 delta = uv - 0.5;
    float warp_factor = dot(delta, delta) * warp_amount;
    uv += delta * warp_factor;

    uv = saturate(uv);

    // Scanlines
    float scan_y = uv.y * resolution.y;
    float scan_freq = 0.3;
    float scanline = sin(scan_y * scan_freq * PI) * 0.5 + 0.5;
    scanline = lerp(1.0, scanline, scan_line_amount * 0.5);

    // Grille
    float grille_phase = uv.x * resolution.x;
    float grille = fmod(grille_phase, 3.0) < 1.5 ? 0.95 : 1.05;
    grille = lerp(1.0, grille, grille_amount * 0.5);

    float4 color = u_texture.Sample(u_sampler, uv) * input.v_color;

    color.rgb *= scanline * grille;

    // Vignette
    float2 v_uv = uv * (1.0 - uv);
    float vignette = v_uv.x * v_uv.y * 15.0;
    vignette = lerp(1.0, vignette, vignette_amount * 0.7);

    color.rgb *= vignette * brightness_boost;

    output.o_color = color;
    return output;
}
