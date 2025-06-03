#define FIREWATCH_IMPLEMENTATION
#include "firewatch.h"

#include "aseprite_texture.h"
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

static Shader shader = {0};
static Model *models = 0;
static pthread_mutex_t lock;
static size_t model_count = 0;

typedef enum {
    FILE_KIND_MODEL,
    FILE_KIND_TEXTURE,
} FileKind;

void load_model(const char *filepath, uint64_t model_index) {
    pthread_mutex_lock(&lock);

    Texture texture = {0};
    if (models[model_index].materialCount)
        texture =
            models[model_index].materials[0].maps[MATERIAL_MAP_DIFFUSE].texture;

    if (models[model_index].meshCount)
        UnloadModel(models[model_index]);

    models[model_index] = LoadModel(filepath);
    assert(models[model_index].meshCount);
    models[model_index].materials[0].shader = shader;
    models[model_index].materials[0].maps[MATERIAL_MAP_DIFFUSE].texture =
        texture;

    pthread_mutex_unlock(&lock);
}

void load_texture(const char *filepath, uint64_t model_index) {
    pthread_mutex_lock(&lock);

    ImageData image_data = aseprite_load(filepath);
    if (!image_data.base_image.data) {
        pthread_mutex_unlock(&lock);
        return;
    }

    Texture2D texture = LoadTextureFromImage(image_data.base_image);
    UnloadImage(image_data.base_image);
    assert(texture.id);

    if (models[model_index].materialCount) {
        if (models[model_index]
                .materials[0]
                .maps[MATERIAL_MAP_DIFFUSE]
                .texture.id)
            UnloadTexture(models[model_index]
                              .materials[0]
                              .maps[MATERIAL_MAP_DIFFUSE]
                              .texture);

        models[model_index].materials[0].maps[MATERIAL_MAP_DIFFUSE].texture =
            texture;
    }

    pthread_mutex_unlock(&lock);
}

static inline void setup_models(StringVector *model_filepaths) {
    model_count = model_filepaths->indices_used;
    assert(model_count);
    models = calloc(model_count, sizeof(Model));
    assert(models);

    for (size_t i = 0; i < model_count; i++) {
        char *model_filepath = stringvec_get(model_filepaths, i);
        if (!model_filepath)
            break;

        firewatch_new_file_ex(model_filepath, 0, i, FILE_KIND_MODEL);
        load_model(model_filepath, i);

        char *texture_filepath =
            path_get_corresponding_texture_file(model_filepath);
        assert(texture_filepath);
        firewatch_new_file_ex(texture_filepath, 0, i, FILE_KIND_TEXTURE);
        load_texture(texture_filepath, i);
        free(texture_filepath);
    }
}

static inline void unload_models(void) {
    for (size_t i = 0; i < model_count; i++) {
        if (!models[i].meshCount)
            continue;
        if (models[i].materials[0].maps[MATERIAL_MAP_DIFFUSE].texture.id)
            UnloadTexture(
                models[i].materials[0].maps[MATERIAL_MAP_DIFFUSE].texture);
        UnloadModel(models[i]);
    }
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

    shader = LoadShaderFromMemory(vertex_shader, 0);
    setup_models(&model_filepaths);

    Camera starting_camera = {
        .position = (Vector3){0.0f, 1.0f, 3.0f},
        .target = (Vector3){0.0f, 0.0f, 0.0f},
        .up = (Vector3){0.0f, 1.0f, 0.0f},
        .fovy = 45.0,
    };
    Camera camera = starting_camera;

    LoadRequest load_request = {0};

    int wireframe_enabled = 0;

    while (!WindowShouldClose()) {
        // Check for file changes
        while (firewatch_request_stack_pop(&load_request)) {
            switch (load_request.kind) {
            case FILE_KIND_MODEL:
                load_model(load_request.filepath, load_request.cookie);
                break;
            case FILE_KIND_TEXTURE:
                load_texture(load_request.filepath, load_request.cookie);
                break;
            default:
                break;
            }
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE))
            DisableCursor();
        if (IsMouseButtonReleased(MOUSE_BUTTON_MIDDLE))
            EnableCursor();
        if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
            orbital_camera_update(&camera, 0);
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

        for (size_t i = 0; i < model_count; i++) {
            assert(models[i].meshCount);

            DrawModel(models[i], Vector3Zero(), 1.0f, RAYWHITE);
            if (wireframe_enabled)
                DrawModelWires(models[i], Vector3Zero(), 1.0f, BLACK);
        }

        if (grid_enabled)
            DrawGrid(20, 1.0f);

        EndMode3D();
        EndDrawing();
    }

    unload_models();
    stringvec_free(&model_filepaths);
    UnloadShader(shader);
    CloseWindow();

    return 0;
}
