#include "aseprite_texture.h"
#include "model_vector.h"
#include "orbital_controls.h"
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

// Default shader with vertex colors disabled
static const char *vertex_shader =
    "#version 330                       \n"
    "in vec3 vertexPosition;            \n"
    "in vec2 vertexTexCoord;            \n"
    "in vec4 vertexColor;               \n"
    "out vec2 fragTexCoord;             \n"
    "out vec4 fragColor;                \n"
    "uniform mat4 mvp;                  \n"
    "void main()                        \n"
    "{                                  \n"
    "    fragTexCoord = vertexTexCoord; \n"
    "    fragColor = vec4(1.0);         \n"
    "    gl_Position = mvp*vec4(vertexPosition, 1.0); \n"
    "}                                  \n";

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
static ModelVector load_model_data_from_files(StringVector *model_filepaths,
                                              Shader *shader) {
    ModelVector vec = modelvec_init();

    size_t i = 0;
    while (1) {
        char *model_filepath = stringvec_get(model_filepaths, i++);
        if (!model_filepath)
            break;
        ModelData model_data = {.model = LoadModel(model_filepath)};

        char *texture_filepath =
            path_get_corresponding_texture_file(model_filepath);
        try_load_corresponding_texture(texture_filepath, &model_data.model);
        free(texture_filepath);

        modelvec_append(&vec, model_data);
        model_data.model.materials[0].shader = *shader;
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
    int grid_enabled = 1;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-skybox")) {
            grid_enabled = 0;
            continue;
        }

        if (*argv[i] == '-') {
            fprintf(stderr, "Unsupported command-line option \"%s\"", argv[i]);
            return 1;
        }

        stringvec_append(&model_filepaths, argv[i], strlen(argv[i]));
    }

    if (!stringvec_count(&model_filepaths)) {
        fprintf(stderr, "Error: No model files were supplied as arguments.\n");
        return 1;
    }

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(800, 450, "Bricklayer");
    SetTargetFPS(60);

    Shader shader = LoadShaderFromMemory(vertex_shader, 0);

    uint64_t last_modified =
        get_most_recent_file_modification(&model_filepaths);

    Camera starting_camera = {
        .position = (Vector3){0.0f, 1.0f, 3.0f},
        .target = (Vector3){0.0f, 0.0f, 0.0f},
        .up = (Vector3){0.0f, 1.0f, 0.0f},
        .fovy = 45.0,
    };
    Camera camera = starting_camera;

    ModelVector models = load_model_data_from_files(&model_filepaths, &shader);

    int wireframe_enabled = 0;
    float time_since_last_modified_check = 0;

    while (!WindowShouldClose()) {
        time_since_last_modified_check += GetFrameTime();

        // Modified check if needed
        if (time_since_last_modified_check > MODIFIED_CHECK_COOLDOWN_SECONDS) {
            time_since_last_modified_check = 0;

            uint64_t new_last_modified =
                get_most_recent_file_modification(&model_filepaths);

            // Reload model if needed
            if (new_last_modified > last_modified) {
                modelvec_free(&models);
                models = load_model_data_from_files(&model_filepaths, &shader);
                last_modified = new_last_modified;
            }
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE))
            DisableCursor();
        if (IsMouseButtonReleased(MOUSE_BUTTON_MIDDLE))
            EnableCursor();
        if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
            orbital_camera_update(&camera);
        }
        orbital_adjust_camera_zoom(&camera, GetMouseWheelMove());

        if (IsKeyPressed(KEY_G))
            grid_enabled = !grid_enabled;
        if (IsKeyPressed(KEY_W))
            wireframe_enabled = !wireframe_enabled;
        if (IsKeyPressed(KEY_B))
            camera = starting_camera;

        // ----- Drawing -----

        BeginDrawing();

        if (IsWindowFocused())
            ClearBackground((Color){0x48, 0x48, 0x48, 0xff});
        else
            ClearBackground(BLACK);

        BeginMode3D(camera);

        size_t i = 0;
        while (1) {
            ModelData *model = modelvec_get(&models, i++);
            if (!model)
                break;
            DrawModel(model->model, Vector3Zero(), 1.0f, RAYWHITE);
            if (wireframe_enabled)
                DrawModelWires(model->model, Vector3Zero(), 1.0f, BLACK);
        }

        if (grid_enabled)
            DrawGrid(20, 1.0f);

        EndMode3D();
        EndDrawing();
    }

    size_t i = 0;
    ModelData *model = 0;
    while ((model = modelvec_get(&models, i++)))
        UnloadModel(model->model);

    modelvec_free(&models);
    stringvec_free(&model_filepaths);
    UnloadShader(shader);
    CloseWindow();

    return 0;
}
