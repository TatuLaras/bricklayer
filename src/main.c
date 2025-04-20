#include "aseprite_texture.h"
#include "model_vector.h"
#include "path.h"
#include "raylib.h"
#include "raymath.h"
#include "string_vector.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#define GLSL_VERSION 330

#define DRAG_ROTATE_SENSITIVITY_X 0.004
#define DRAG_ROTATE_SENSITIVITY_Y 0.006
#define AUTO_ROTATE_SPEED 0.5
#define ZOOM_SENSITIVITY_SCROLL 0.08
#define ZOOM_SENSITIVITY_MOUSE 0.006
#define MODEL_SHIFT_SENSITIVITY 0.004
#define MODIFIED_CHECK_COOLDOWN_SECONDS 0.5

// Tries to load a texture from aseprite file at `filepath` and applies it
// to `model` mesh at index 0 on success.
static void try_load_corresponding_texture(const char *filepath, Model *model) {
    Image *image = aseprite_load_as_image(filepath);
    if (!image)
        return;

    Texture2D texture = LoadTextureFromImage(*image);
    UnloadImage(*image);
    free(image);
    if (!texture.id)
        return;

    model->materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;
}

// Loads model data from `model_filepaths` supplied and possible aseprite
// texture files of the same name.
static ModelVector load_model_data_from_files(StringVector *model_filepaths) {
    ModelVector vec = modelvec_init();

    size_t i = 0;
    while (1) {
        char *model_filepath = stringvec_get(model_filepaths, i++);
        if (!model_filepath)
            break;
        Model model = LoadModel(model_filepath);

        char *texture_filepath =
            path_get_corresponding_texture_file(model_filepath);
        try_load_corresponding_texture(texture_filepath, &model);
        free(texture_filepath);

        modelvec_append(&vec, model);
    }
    return vec;
}

static inline uint64_t file_last_modified(const char *filepath) {
    struct stat attr;
    if (stat(filepath, &attr))
        return 0;
    return attr.st_mtim.tv_sec;
}

static inline uint64_t max(uint64_t a, uint64_t b) {
    if (a > b)
        return a;
    return b;
}

static uint64_t
get_most_recent_file_modification(StringVector *model_filepaths) {
    size_t i = 0;
    uint64_t last_modification = 0;

    while (1) {
        char *model_filepath = stringvec_get(model_filepaths, i++);
        if (!model_filepath)
            break;

        char *texture_filepath =
            path_get_corresponding_texture_file(model_filepath);

        last_modification =
            max(last_modification, file_last_modified(model_filepath));

        if (!texture_filepath)
            continue;
        last_modification =
            max(last_modification, file_last_modified(texture_filepath));

        free(texture_filepath);
    }

    return last_modification;
}

int main(int argc, char **argv) {
    StringVector model_filepaths = stringvec_init();

    for (int i = 1; i < argc; i++) {
        // CLI args..

        if (*argv[i] == '-') {
            fprintf(stderr, "Unsupported command-line option \"%s\"", argv[i]);
            return 1;
        }

        stringvec_append(&model_filepaths, argv[i], strlen(argv[i]));
    }

    // Alternatively read model filepaths from stdin
    if (!stringvec_count(&model_filepaths)) {
        char input[1000] = {0};
        size_t input_used = 0;
        while (!feof(stdin)) {
            char read_char = 0;
            fread(&read_char, 1, 1, stdin);
            if (read_char == ' ') {
                stringvec_append(&model_filepaths, input, input_used);
                input_used = 0;
            } else {
                input[input_used++] = read_char;
            }
        }

        if (input_used > 0)
            stringvec_append(&model_filepaths, input, input_used - 1);
    }

    if (!stringvec_count(&model_filepaths)) {
        fprintf(stderr, "Error: No model files were supplied as arguments.\n");
        return 1;
    }

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(800, 450, "Bricklayer");
    SetTargetFPS(60);

    uint64_t last_modified =
        get_most_recent_file_modification(&model_filepaths);

    Vector3 camera_arm = {0.0f, 0.0f, 3.0f};
    Camera3D camera = {0};
    camera.position = camera_arm;
    camera.target = (Vector3){0.0f, 0.0f, 0.0f};
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;

    ModelVector models = load_model_data_from_files(&model_filepaths);

    float yaw = 0;
    float pitch = -PI / 8;
    Vector3 model_position = {0};
    int grid_enabled = 1;
    int wireframe_enabled = 0;
    int auto_rotate = 0;
    float time_since_last_modified_check = 0;

    while (!WindowShouldClose()) {
        time_since_last_modified_check += GetFrameTime();
        Vector2 mouse_delta = GetMouseDelta();

        // Modified check if needed
        if (time_since_last_modified_check > MODIFIED_CHECK_COOLDOWN_SECONDS) {
            time_since_last_modified_check = 0;

            uint64_t new_last_modified =
                get_most_recent_file_modification(&model_filepaths);

            // Reload model if needed
            if (new_last_modified > last_modified) {
                modelvec_free(&models);
                models = load_model_data_from_files(&model_filepaths);
                last_modified = new_last_modified;
            }
        }

        if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
            yaw -= mouse_delta.x * DRAG_ROTATE_SENSITIVITY_X;
            pitch -= mouse_delta.y * DRAG_ROTATE_SENSITIVITY_Y;
        }
        if (auto_rotate)
            yaw += AUTO_ROTATE_SPEED * GetFrameTime();

        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            if (IsKeyDown(KEY_X))
                model_position.x -= mouse_delta.y * MODEL_SHIFT_SENSITIVITY;
            else if (IsKeyDown(KEY_Z))
                model_position.z -= mouse_delta.y * MODEL_SHIFT_SENSITIVITY;
            else
                model_position.y -= mouse_delta.y * MODEL_SHIFT_SENSITIVITY;
        }

        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            camera_arm.z +=
                (mouse_delta.y * camera_arm.z) * ZOOM_SENSITIVITY_MOUSE;
        }

        camera_arm.z -=
            (GetMouseWheelMove() * camera_arm.z) * ZOOM_SENSITIVITY_SCROLL;

        if (IsKeyPressed(KEY_G))
            grid_enabled = !grid_enabled;
        if (IsKeyPressed(KEY_W))
            wireframe_enabled = !wireframe_enabled;
        if (IsKeyPressed(KEY_R))
            auto_rotate = !auto_rotate;
        if (IsKeyPressed(KEY_B))
            model_position = Vector3Zero();

        // Clamp values
        if (yaw > 2 * PI)
            yaw = 0;
        if (pitch > 2 * PI)
            pitch = 0;
        if (camera_arm.z < 0.04f)
            camera_arm.z = 0.04f;

        // Calculate camera position
        Vector3 yawed_camera_arm = Vector3RotateByAxisAngle(
            camera_arm, (Vector3){0.0f, 1.0f, 0.0f}, yaw);
        Vector3 pitch_axis = Vector3RotateByAxisAngle(
            (Vector3){1.0f, 0.0f, 0.0f}, (Vector3){0.0f, 1.0f, 0.0f}, yaw);
        Vector3 pitched_camera_arm =
            Vector3RotateByAxisAngle(yawed_camera_arm, pitch_axis, pitch);
        camera.position = Vector3Add(camera.target, pitched_camera_arm);

        // ----- Drawing -----

        BeginDrawing();
        Color col = {.r = 0};
        ClearBackground(col);

        BeginMode3D(camera);

        size_t i = 0;
        while (1) {
            Model *model = modelvec_get(&models, i++);
            if (!model)
                break;
            DrawModel(*model, model_position, 1.0f, RAYWHITE);
            if (wireframe_enabled)
                DrawModelWires(*model, model_position, 1.0f, BLACK);
        }

        if (grid_enabled)
            DrawGrid(20, 1.0f);

        EndMode3D();
        EndDrawing();
    }

    modelvec_free(&models);
    stringvec_free(&model_filepaths);
    CloseWindow();

    return 0;
}
