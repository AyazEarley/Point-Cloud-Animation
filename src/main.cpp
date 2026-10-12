#include "raylib.h"
#include <vector>
#include <cstdlib>
#include <cmath>

inline double frameToTime(size_t frame, double fps) { return frame / fps; }

inline float frand() { return std::rand() / (float)RAND_MAX; }            // 0..1
inline float frandRange(float a, float b) { return a + (b - a) * frand(); }

struct Particle {
    float u0      = 0.0f; 
    float strand  = 0.0f;
    float dTheta  = 0.0f;
    float dRadius = 1.0f;


    float freq[3][2];
    float phase[3][2];
    float amp = 1.0f;

    Vector3 pos = {0, 0, 0};
    Color color = {255, 255, 255, 255};
};

inline float wander(const Particle& p, int ch, float t) {
    return 0.65f * std::sin(p.freq[ch][0] * t + p.phase[ch][0])
         + 0.35f * std::sin(p.freq[ch][1] * t + p.phase[ch][1]);
}

inline float smoothTo(float current, float target, float tau, float dt) {
    float alpha = 1.0f - std::exp(-dt / tau);
    return current + (target - current) * alpha;
}

int main() {
    InitWindow(1920, 1080, "pointviz test");
    SetTargetFPS(60);

    Camera3D cam{};
    cam.position = {8.0f, 6.0f, 8.0f};
    cam.target = {0.0f, 0.0f, 0.0f};
    cam.up = {0.0f, 1.0f, 0.0f};
    cam.fovy = 45.0f;
    cam.projection = CAMERA_PERSPECTIVE;

    const float FPS_F = 60.0f;
    const float v = 0.05f; 
    const float k = 2.0f;
    const float H = 8.0f;
    const float R = 1.5f;

    float driftTheta  = 0.05f;
    float driftRadius = 0.05f;    
    float driftY = 0.25f;

    float targetDriftTheta  = 0.05f;
    float targetDriftRadius = 0.05f;    
    float targetDriftY = 0.25f;

    const float camRotation = 0.2f;

    const float dt  = 1.0f / FPS_F;
    const float tau = 0.5f;

    std::vector<Particle> particles(4000);
    for (auto& p : particles) {
        p.u0 = frand();
        p.strand = frand() > 0.5f ? 0.0f : 1.0f;
        p.color = p.strand == 0.0f ? Color{255, 255, 0, 255} : Color{255, 0, 255, 255};

        p.dTheta  = frandRange(-0.12f, 0.12f);
        p.dRadius = frandRange(0.92f, 1.08f);

        for (int c = 0; c < 3; ++c) {
            p.freq[c][0]  = frandRange(0.2f, 1.0f);
            p.freq[c][1]  = frandRange(0.6f, 2.3f);
            p.phase[c][0] = frandRange(0.0f, 6.2831853f);
            p.phase[c][1] = frandRange(0.0f, 6.2831853f);
        }
        p.amp = frandRange(0.5f, 1.5f);
    }

    size_t frame = 0;
    while (!WindowShouldClose()) {
        float t = (float)frameToTime(frame, FPS_F);
        cam.position = { 11.3f * std::cos(t * camRotation), 6.0f, 11.3f * std::sin(t * camRotation) };

        for (auto& p : particles) {
            float u  = std::fmod (p.u0 + v * t, 1.0f);

            float wTheta = wander(p, 0, t) * driftTheta  * p.amp;
            float wRad   = wander(p, 1, t) * driftRadius * p.amp;
            float wY     = wander(p, 2, t) * driftY * p.amp;

            float th = k * 2.0f * PI * u + PI * p.strand + p.dTheta + wTheta;
            float r  = R * p.dRadius * (1.0f + wRad);

            p.pos = { r * std::cos(th),
                      (u - 0.5f) * H + wY,
                      r * std::sin(th) };
        }

        if (frame >= 2000) {
            targetDriftTheta  = 0.35f * std::cos(2.0f * PI * frame / 240.0f);
            targetDriftRadius = 0.60f * std::cos(2.0f * PI * frame / 240.0f);
            targetDriftY = 0.5f * std::cos(2.0f * PI * frame / 240.0f);
        } 
        else if(frame < 1500){
            targetDriftTheta  = 0.7f;
            targetDriftRadius = 0.7f;
            targetDriftY = 1.0f;
        }
        else {
            targetDriftTheta  = 0.05f;
            targetDriftRadius = 0.05f;
        }

        driftTheta = smoothTo(driftTheta, targetDriftTheta, tau, dt);
        driftRadius = smoothTo(driftRadius, targetDriftRadius, tau, dt);
        driftY = smoothTo(driftY, targetDriftY, tau, dt);

        BeginDrawing();
        ClearBackground({5, 5, 12, 255});
        BeginMode3D(cam);
        for (const auto& p : particles) DrawPoint3D(p.pos, p.color);
        EndMode3D();
        EndDrawing();

        ++frame;
    }

    CloseWindow();
}