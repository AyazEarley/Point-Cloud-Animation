#include "raylib.h"
#include "raymath.h"
#include "happly.h"
#include <array>
#include <cstdio>
#include <string>
#include <vector>
#include <cmath>
#include <random>
#include "FastNoiseLite.h"
#include "DSP.h"
#include "dr_wav.h"

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
    Vector3 vectorSeed = {0,0,0};

    float dy = 0.0f;
    float phase = 0.0f;
};

float randomPhase(){
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(0.0f, 2.0f);
    float random_value = dis(gen);
    
    return random_value * PI;
}

bool loadWavLeft(const char* path, std::vector<float>& left,
                 unsigned int& sampleRate)
{
    drwav wav;
    if (!drwav_init_file(&wav, path, nullptr)) {
        std::fprintf(stderr, "Failed to open %s\n", path);
        return false;
    }

    sampleRate = wav.sampleRate;
    const unsigned int channels = wav.channels;

    left.clear();
    left.reserve(wav.totalPCMFrameCount);

    constexpr drwav_uint64 CHUNK_FRAMES = 4096;
    std::vector<float> chunk(CHUNK_FRAMES * channels);

    for (;;) {
        drwav_uint64 framesRead =
            drwav_read_pcm_frames_f32(&wav, CHUNK_FRAMES, chunk.data());
        if (framesRead == 0) break;

        for (drwav_uint64 i = 0; i < framesRead; ++i)
            left.push_back(chunk[i * channels]);
    }

    drwav_uninit(&wav);
    return true;
}


int main() {
    constexpr int   W            = 1920;
    constexpr int   H            = 1080;
    constexpr int   FPS          = 60;
    constexpr int   DURATION_SEC = 10;
    constexpr int   TOTAL_FRAMES = FPS * DURATION_SEC;
    constexpr float DT           = 1.0f / FPS;
    constexpr float ROT_SPEED    = 0.02f; 
    const float smoothTime = 0.5f;
    const std::string outFile    = "fern.mp4";

    InitWindow(W, H, "pointviz render");
    SetTargetFPS(0);

    Camera3D cam{};
    cam.position = {0.65f, 0.65f, 0.4f};
    cam.target = {0.0f, 0.0f, 0.2f};
    cam.up = {0.0f, 0.0f, 1.0f};
    cam.fovy = 45.0f;
    cam.projection = CAMERA_PERSPECTIVE;

    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFrequency(1.0f);

    happly::PLYData ply("C:/Users/Wreck/OneDrive/Desktop/raylib/data/fern.ply");
    std::vector<std::array<double, 3>> positions = ply.getVertexPositions();
    std::vector<std::array<unsigned char, 3>> colors = ply.getVertexColors();

    std::vector<float> left;
    unsigned int sampleRate;

    if (loadWavLeft("C:/Users/Wreck/OneDrive/Desktop/raylib/data/test.wav", left, sampleRate)) {
        std::printf("%zu samples @ %u Hz\n", left.size(), sampleRate);
    }

    std::mt19937 rng(12345);
    std::uniform_real_distribution<float> dist(0.0f, 1000.0f);

    std::vector<Particle> particles(positions.size());
    for (size_t i = 0; i < positions.size(); i++) {
        particles[i].pos = {(float)positions[i][0], (float)positions[i][1], (float)positions[i][2]};
        particles[i].home = particles[i].pos;
        particles[i].color = {colors[i][0], colors[i][1], colors[i][2], 255};
        particles[i].vectorSeed = { dist(rng), dist(rng), dist(rng) };
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

    float noiseSpeed = 0.3f;
    float targetAmplitude  = 0.001f;
    float currentAmplitude = 0.001f;
    for (int frame = 0; frame < TOTAL_FRAMES && !WindowShouldClose(); frame++) {
        float time = frame * DT;
        float t = time * noiseSpeed;
        float sample = frame * 735;

        if(frame < 200){
            targetAmplitude += 0.0001f;
        }
        else if(frame < 400){
            targetAmplitude -=  0.0001f;
        }

        currentAmplitude += (targetAmplitude - currentAmplitude) * (1.0f - std::exp(-DT / smoothTime));
        

        for (auto& p : particles) {
            float nx = noise.GetNoise(p.vectorSeed.x, t, 0.0f);
            float ny = noise.GetNoise(p.vectorSeed.y, t, 100.0f);
            float nz = noise.GetNoise(p.vectorSeed.z, t, 200.0f);
            p.pos = Vector3Add(p.home, Vector3Scale({nx, ny, nz}, currentAmplitude));
        }

        cam.position = Vector3Add(
            cam.target,
            Vector3RotateByAxisAngle(Vector3Subtract(cam.position, cam.target), cam.up, ROT_SPEED * DT));

        BeginTextureMode(target);
            ClearBackground({5, 5, 12, 255});
            BeginMode3D(cam);
            for (const auto& p : particles) {
                DrawCubeV(p.pos, {s, s, s}, p.color);
            }
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