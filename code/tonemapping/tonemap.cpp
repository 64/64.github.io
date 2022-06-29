#include <iostream>
#include <vector>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cassert>
#include "vec3.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "camera.h"

float luminance(vec3);
vec3 change_luminance(vec3, float);
float clamp(float, float, float);
size_t clamp(size_t, size_t, size_t);
vec3 clamp(vec3, float, float);
float lerp(float, float, float);
vec3 lerp(vec3, vec3, vec3);
float gamma_correct(float);

struct image {
    float *data;
    size_t width;
    size_t height;
    float max_luminance;
    float max_component;
    float log_average_luminance;
};

struct local_params {
    double alpha_1, alpha_2;
    double threshold;
    double phi;
    double middle_grey;
    double max_scale;
};

vec3 reinhard(vec3 v)
{
    return v / (1.0f + v);
}

vec3 reinhard_extended(vec3 v, float max_white)
{
    vec3 numerator = v * (1.0f + (v / vec3(max_white * max_white)));
    return numerator / (1.0f + v);
}

vec3 reinhard_extended_luminance(vec3 v, float max_white_l)
{
    float l_old = luminance(v);
    float numerator = l_old * (1.0f + (l_old / (max_white_l * max_white_l)));
    float l_new = numerator / (1.0f + l_old);
    return change_luminance(v, l_new);
}

vec3 reinhard_jodie(vec3 v)
{
    float l = luminance(v);
    vec3 tv = v / (1.0f + v);
    return lerp(v / (1.0f + l), tv, tv);
}

vec3 const_luminance_reinhard(vec3 c)
{
    vec3 lv = vec3(0.2126f, 0.7152f, 0.0722f);
    vec3 nv = lv / (1.0f - lv);
    c /= 1.0f + dot(c, vec3(lv));
    vec3 nc = vec3(
        std::max(c.r() - 1.0f, 0.0f),
        std::max(c.g() - 1.0f, 0.0f),
        std::max(c.b() - 1.0f, 0.0f)
    ) * nv;
    return c + vec3(nc.g() + nc.b(), nc.r() + nc.b(), nc.r() + nc.g());
}

vec3 uncharted2_tonemap_partial(vec3 x)
{
    float A = 0.15f;
    float B = 0.50f;
    float C = 0.10f;
    float D = 0.20f;
    float E = 0.02f;
    float F = 0.30f;
    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

vec3 uncharted2_filmic(vec3 v)
{
    float exposure_bias = 2.0f;
    vec3 curr = uncharted2_tonemap_partial(v * exposure_bias);

    vec3 W = vec3(11.2f);
    vec3 white_scale = vec3(1.0f) / uncharted2_tonemap_partial(W);
    return curr * white_scale;
}

static const std::array<vec3, 3> aces_input_matrix =
{
    vec3(0.59719f, 0.35458f, 0.04823f),
    vec3(0.07600f, 0.90834f, 0.01566f),
    vec3(0.02840f, 0.13383f, 0.83777f)
};

static const std::array<vec3, 3> aces_output_matrix =
{
    vec3( 1.60475f, -0.53108f, -0.07367f),
    vec3(-0.10208f,  1.10813f, -0.00605f),
    vec3(-0.00327f, -0.07276f,  1.07602f)
};

vec3 mul(const std::array<vec3, 3>& m, const vec3& v)
{
    float x = m[0][0] * v[0] + m[0][1] * v[1] + m[0][2] * v[2];
    float y = m[1][0] * v[0] + m[1][1] * v[1] + m[1][2] * v[2];
    float z = m[2][0] * v[0] + m[2][1] * v[1] + m[2][2] * v[2];
    return vec3(x, y, z);
}

vec3 rtt_and_odt_fit(vec3 v)
{
    vec3 a = v * (v + 0.0245786f) - 0.000090537f;
    vec3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
    return a / b;
}

vec3 aces_fitted(vec3 v)
{
    v = mul(aces_input_matrix, v);
    v = rtt_and_odt_fit(v);
    v = mul(aces_output_matrix, v);
    return v;
}

vec3 aces_approx(vec3 v)
{
    v *= 0.6f;
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((v*(a*v+b))/(v*(c*v+d)+e), 0.0f, 1.0f);
}

float camera_get_intensity(float f, float iso)
{
    f = clamp(f, 0.0f, iso); // Clamp to [0, iso]
    f /= iso; // Convert to [0, 1]

    // Returns 1.0 if the index is out-of-bounds
    auto get_or_one = [](const auto& arr, size_t index)
    {
        return index < arr.size() ? arr[index] : 1.0;
    };

    // std::upper_bound uses a binary search to find the position of f in camera_irradiance
    auto upper = std::upper_bound(camera_irradiance.begin(), camera_irradiance.end(), f);
    size_t idx = std::distance(camera_irradiance.begin(), upper);

    double low_irradiance = camera_irradiance[idx];
    double high_irradiance = get_or_one(camera_irradiance, idx + 1);
    double lerp_param = (f - low_irradiance) / (high_irradiance - low_irradiance);

    double low_val = camera_intensity[idx];
    double high_val = get_or_one(camera_intensity, idx + 1);

    // Lerping isn't really necessary for RGB8 (as the curve is sampled with 1024 points)
    return clamp(lerp((float)low_val, (float)high_val, (float)lerp_param), 0.0f, 1.0f);
}

vec3 camera_tonemap(vec3 v, float iso)
{
    float r = camera_get_intensity(v.r(), iso);
    float g = camera_get_intensity(v.g(), iso);
    float b = camera_get_intensity(v.b(), iso);
    return vec3(r, g, b);
}

double reinhard_r(double x, double y, double alpha, double s)
{
    double alpha_s_squared = (alpha * s) * (alpha * s);
    return std::exp(-(x * x + y * y) / alpha_s_squared) / (3.14159265358979323846 * alpha_s_squared);
}

double reinhard_vx(const image &img, size_t pixel_x, size_t pixel_y, double alpha, double s)
{
    double accum = 0.0;

    // After 3 standard deviations, r is approximately zero
    // 2 * pi * sigma^2 = pi * (alpha * s)^2
    // sigma^2 = (alpha * s)^2 / 2
    // sigma = (alpha * s) / sqrt(2)
    // Hence we stop caring after 3 * sigma
    int width = (int)(3.0 * alpha * s / std::sqrt(2.0) + 0.5);

    for (int x_rel = -width; x_rel <= width; x_rel++) {
        for (int y_rel = -width; y_rel <= width; y_rel++) {
            int x_abs = pixel_x + x_rel;
            int y_abs = pixel_y + y_rel;

            if (x_abs < 0 || x_abs >= (int)img.width || y_abs < 0 || y_abs >= (int)img.height)
                continue;

            double r = reinhard_r((double)x_rel, (double)y_rel, alpha, s);

            float *f = img.data + (x_abs + y_abs * img.width) * 3;
            accum += r * (double)luminance(vec3(f[0], f[1], f[2]));
        }
    }

    return accum;
}

double reinhard_v(const image &img, const local_params &params, size_t pixel_x, size_t pixel_y, double s)
{
    double v1 = reinhard_vx(img, pixel_x, pixel_y, params.alpha_1, s);
    double v2 = reinhard_vx(img, pixel_x, pixel_y, params.alpha_2, s);
    double denom = std::pow(2.0, params.phi) * params.middle_grey / (s * s) + v1;
    return (v1 - v2) / denom;
}

double select_scale(const image &img, const local_params &params, size_t pixel_x, size_t pixel_y)
{
    for (double scale = 1.0f; scale < params.max_scale; scale *= 2.0) {
        double v = reinhard_v(img, params, pixel_x, pixel_y, scale);
        if (std::abs(v) < params.threshold)
            return scale;
    }

    return params.max_scale;
}

vec3 reinhard_local(const image &img, const local_params &params, size_t pixel_x, size_t pixel_y, vec3 v)
{
    double scale = select_scale(img, params, pixel_x, pixel_y);
    double denominator = 1.0 + reinhard_vx(img, pixel_x, pixel_y, params.alpha_1, scale);

    // Simplification of v * (l_new / l_old) where l_new = l_old / denominator 
    return v / (float)denominator; 
}

float gamma_correct(float f)
{
    if (f <= 0.0031308f) {
        return f * 12.92f;
    } else {
        return 1.055f * std::pow(f, 1.0f / 2.4f) - 0.055f;
    }
}

float luminance(vec3 v)
{
    return dot(v, vec3(0.2126f, 0.7152f, 0.0722f));
}

vec3 change_luminance(vec3 c_in, float l_out)
{
    float l_in = luminance(c_in);
    return c_in * (l_out / l_in);
}

float clamp(float f, float low, float high)
{
    return std::max(std::min(f, high), low);
}

size_t clamp(size_t x, size_t low, size_t high)
{
    return std::max(std::min(x, high), low);
}

vec3 clamp(vec3 v, float min, float max)
{
    return vec3(
        clamp(v.r(), min, max),
        clamp(v.g(), min, max),
        clamp(v.b(), min, max)
    );
}

float lerp(float a, float b, float t)
{
    assert(t <= 1.0f);
    return a * (1.0f - t) + b * t;
}

vec3 lerp(vec3 a, vec3 b, vec3 t)
{
    return vec3(
        lerp(a.r(), b.r(), t.r()),
        lerp(a.g(), b.g(), t.g()),
        lerp(a.b(), b.b(), t.b())
    );
}

uint8_t float_to_byte(float f)
{
    f = gamma_correct(f);
    return uint8_t(clamp(f, 0.0f, 1.0f) * 255.99f);
}

// TODO: why does phi not behave as expected?
vec3 tonemap(const image &img, const local_params &params, size_t pixel_x, size_t pixel_y, vec3 v)
{
    // return clamp(v, 0.0f, 1.0f);
    // return reinhard(v);
    // return reinhard_extended(v, img.max_component);
    // return reinhard_extended_luminance(v, img.max_luminance);
    // return reinhard_jodie(v);
    // return uncharted2_filmic(v);
    return aces_fitted(v);
    // return aces_approx(v);
    // return camera_tonemap(v, 6.0f);
    // return const_luminance_reinhard(v);
    // return reinhard_local(img, params, pixel_x, pixel_y, v);
}

int main()
{
    int img_x, img_y, n;
    float *data = stbi_loadf("memorial.hdr", &img_x, &img_y, &n, 0);
    assert(data && n == 3);

    std::cout << "width: " << img_x << ", height: " << img_y << std::endl;

    std::vector<uint8_t> out(img_x * img_y * 3, 0);

    // Compute global image statistics
    float sum_log_luminance = 0.0f, max_luminance = 0.0f, max_component = 0.0f;
    for (size_t i = 0; i < (size_t)(img_x * img_y); i++) {
        float *f = data + (i * 3);
        vec3 v_in(f[0], f[1], f[2]);

        // Global variables which tonemap() uses
        max_luminance = std::max(max_luminance, luminance(v_in));
        max_component = std::max({ max_component, v_in.r(), v_in.g(), v_in.b() });
        sum_log_luminance += std::log(0.0001f + luminance(v_in));
    }

    struct local_params params;
    params.alpha_1 = 0.354;
    params.alpha_2 = 1.6 * params.alpha_1;
    params.middle_grey = 0.5;
    params.phi = 8.0;
    params.max_scale = 64.0;
    params.threshold = 0.05;

    struct image img;
    img.width = (size_t)img_x;
    img.height = (size_t)img_y;
    img.log_average_luminance = std::exp(sum_log_luminance / (float)(img_x * img_y));
    img.max_component = max_component;
    img.max_luminance = max_luminance;
    img.data = data;

    for (int x = 0; x < img_x; x++) {
        for (int y = 0; y < img_y; y++) {
            size_t idx = (x + y * img_x) * 3;

            vec3 v_in(data[idx + 0], data[idx + 1], data[idx + 2]);
            vec3 v_out = tonemap(img, params, x, y, v_in);

            out[idx + 0] = float_to_byte(v_out.r());
            out[idx + 1] = float_to_byte(v_out.g());
            out[idx + 2] = float_to_byte(v_out.b());
        }
    }

    std::cout << "tonemapped " << out.size() / 3 << " pixels\n";
    std::cout << "max luminance: " << img.max_luminance << std::endl;
    std::cout << "max component: " << img.max_component << std::endl;
    std::cout << "log average luminance: " << img.log_average_luminance << std::endl;

    if (!stbi_write_png("./out.png", img.width, img.height, 3, out.data(), img.width * 3)) {
        std::cerr << "error writing png file";
    }

    stbi_image_free(data);
}
