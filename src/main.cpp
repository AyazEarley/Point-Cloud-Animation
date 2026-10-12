#include "raylib.h"
#include "raymath.h"
#include "happly.h"
#include <array>
#include <cstdio>
#include <string>
#include <vector>

#ifdef _WIN32
  #define POPEN  _popen
  #define PCLOSE _pclose
  #define POPEN_MODE "wb"
#else
  #define POPEN  popen
  #define PCLOSE pclose
  #define POPEN_MODE "w"
#endif

struct Particle {
    Vector3 home = {0, 0, 0};
    Vector3 pos = {0, 0, 0};
    Color color = {255, 255, 255, 255};
};

int main() {
    constexpr int   W            = 1920;
    constexpr int   H            = 1080;
    constexpr int   FPS          = 60;
    constexpr int   DURATION_SEC = 10;
    constexpr int   TOTAL_FRAMES = FPS * DURATION_SEC;
    constexpr float DT           = 1.0f / FPS;
    constexpr float ROT_SPEED    = 0.02f; 
    const std::string outFile    = "fern.mp4";

    InitWindow(W, H, "pointviz render");
    SetTargetFPS(0);

    Camera3D cam{};
    cam.position = {0.65f, 0.65f, 0.4f};
    cam.target = {0.0f, 0.0f, 0.2f};
    cam.up = {0.0f, 0.0f, 1.0f};
    cam.fovy = 45.0f;
    cam.projection = CAMERA_PERSPECTIVE;

    happly::PLYData ply("C:/Users/Wreck/OneDrive/Desktop/raylib/data/fern.ply");
    std::vector<std::array<double, 3>> positions = ply.getVertexPositions();
    std::vector<std::array<unsigned char, 3>> colors = ply.getVertexColors();

    std::vector<Particle> particles(positions.size());
    for (size_t i = 0; i < positions.size(); i++) {
        particles[i].pos = {(float)positions[i][0], (float)positions[i][1], (float)positions[i][2]};
        particles[i].home = particles[i].pos;
        particles[i].color = {colors[i][0], colors[i][1], colors[i][2], 255};
    }
    const float s = 0.0005f;

    RenderTexture2D target = LoadRenderTexture(W, H);

    //ffmpeg stuff
    std::string cmd =
        "ffmpeg -y -loglevel error "
        "-f rawvideo -pix_fmt rgba -s " + std::to_string(W) + "x" + std::to_string(H) +
        " -framerate " + std::to_string(FPS) + " -i - "
        "-vf \"vflip,scale=out_color_matrix=bt709:out_range=tv,format=yuv420p\" "
        "-c:v libx264 -preset slow -crf 16 -r " + std::to_string(FPS) + " "
        "-colorspace bt709 -color_primaries bt709 -color_trc bt709 -color_range tv "
        "-movflags +faststart \"" + outFile + "\"";

    FILE* ff = POPEN(cmd.c_str(), POPEN_MODE);
    if (!ff) {
        std::fprintf(stderr, "Could not start ffmpeg (is it on your PATH?)\n");
        CloseWindow();
        return 1;
    }

    for (int frame = 0; frame < TOTAL_FRAMES && !WindowShouldClose(); frame++) {
        cam.position = Vector3Add(
            cam.target,
            Vector3RotateByAxisAngle(Vector3Subtract(cam.position, cam.target), cam.up, ROT_SPEED * DT));

        BeginTextureMode(target);
            ClearBackground({5, 5, 12, 255});
            BeginMode3D(cam);
                for (const auto& p : particles) DrawCubeV(p.pos, {s, s, s}, p.color);
            EndMode3D();
        EndTextureMode();

        Image img = LoadImageFromTexture(target.texture);
        size_t bytes = (size_t)W * H * 4;
        size_t written = fwrite(img.data, 1, bytes, ff);
        UnloadImage(img);
        if (written != bytes) {
            std::fprintf(stderr, "ffmpeg pipe closed early\n");
            break;
        }

        BeginDrawing();
            ClearBackground(BLACK);
            DrawTextureRec(target.texture, {0, 0, (float)W, -(float)H}, {0, 0}, WHITE);
            DrawText(TextFormat("frame %d / %d", frame + 1, TOTAL_FRAMES), 20, 20, 30, RAYWHITE);
        EndDrawing();
    }

    PCLOSE(ff);
    UnloadRenderTexture(target);
    CloseWindow();
    return 0;
}