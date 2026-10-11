#include "raylib.h"
#include <vector>
#include <cstdlib>
#include <cmath>

inline double frameToTime(size_t frame, double fps) {
    return frame / fps;
}

inline size_t timeToFrame(double seconds, double fps) {
    return (size_t)std::floor(seconds * fps);
}

struct Particle {
    Vector3 home = {0.0f, 0.0f, 0.0f};
    Vector3 pos  = {0.0f, 0.0f, 0.0f};
    float   size = 1.0f;
};

int main() {
    InitWindow(1920, 1080, "pointviz test");
    SetTargetFPS(60);

    Camera3D cam{};
    cam.position   = {8.0f, 6.0f, 8.0f};
    cam.target     = {0.0f, 0.0f, 0.0f};
    cam.up         = {0.0f, 1.0f, 0.0f};
    cam.fovy       = 45.0f;
    cam.projection = CAMERA_PERSPECTIVE;

    std::vector<Particle> particles(2000);
    for (auto& p : particles) {
        auto r = [] { return (std::rand() / (float)RAND_MAX) * 6.0f - 3.0f; };
        p.home = {r(), r(), r()};
        p.pos  = p.home;
    }

    int frame = 0;
    auto jitter = [] { return (std::rand() / (float)RAND_MAX) * 2.0f - 1.0f; };
    while (!WindowShouldClose()) {

        for (auto& p : particles) {
            p.pos.x = p.home.x + jitter();
            p.pos.y = p.home.y + jitter();
            p.pos.z = p.home.z + jitter();
        }

        BeginDrawing();
        ClearBackground({5, 5, 12, 255});
        BeginMode3D(cam);
        for (const auto& p : particles) DrawPoint3D(p.pos, {120, 200, 255, 255});
        EndMode3D();
        EndDrawing();
        

        ++frame;
    }

    CloseWindow();
}