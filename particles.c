#include <stdio.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <emscripten/emscripten.h>

typedef unsigned int being_id_t;

EMSCRIPTEN_KEEPALIVE
float randf() {
    return rand() / (float)RAND_MAX;
}

typedef unsigned int bool;
enum { false, true };

struct Physics_State {
    float x, y;
    float x_accel, y_accel;
};
struct Physics_States {
    float* x;
    float* y;
    float* x_accel;
    float* y_accel;
    // this could be a bit field
    bool* used;
    size_t count;
    size_t max_count;
};
struct Physics_States* physics_states;
void alloc_physics_states(size_t max_count) {
    physics_states = malloc(sizeof(struct Physics_States));
    physics_states->x = malloc(max_count * sizeof(float));
    physics_states->y = malloc(max_count * sizeof(float));
    physics_states->x_accel = malloc(max_count * sizeof(float));
    physics_states->y_accel = malloc(max_count * sizeof(float));
    physics_states->used = malloc(max_count * sizeof(bool));
    for (being_id_t i = 0; i < max_count; i += 1) {
        physics_states->used[i] = false;
    }
    physics_states->max_count = max_count;
}

/* this could be duck typed */
void add_physics_state(struct Physics_States* table, struct Physics_State* row) {
    being_id_t index = 0;
    while (index < physics_states->max_count) {
        if (table->used[index]) {
            index += 1;
            continue;
        }
        else {
            break;
        }
    }
    if (index != physics_states->max_count) {
        table->x[index] = row->x;
        table->y[index] = row->y;
        table->x_accel[index] = row->x_accel;
        table->y_accel[index] = row->y_accel;
        table->used[index] = true;
        physics_states->count += 1;
    }
}

void remove_physics_state(struct Physics_States* table, being_id_t id) {
    table->used[id] = false;
    physics_states->count -= 1;
}

EMSCRIPTEN_KEEPALIVE
struct Physics_States* get_physics_states() {
    return physics_states;
}

EMSCRIPTEN_KEEPALIVE
void create_being(float x, float y, float x_accel, float y_accel) {
    struct Physics_State physics_state;
    physics_state.x = x;
    physics_state.y = y;
    physics_state.x_accel = x_accel;
    physics_state.y_accel = y_accel;
    add_physics_state(physics_states, &physics_state);
}

void move_beings(float delta) {
    for (being_id_t i = 0; i < physics_states->max_count; i += 1) {
        physics_states->x[i] += physics_states->x_accel[i] * delta;
        physics_states->y[i] += physics_states->y_accel[i] * delta;
    }
}

struct Nearest_Being {
    being_id_t* id;
    float* distance;
    float* direction_x;
    float* direction_y;
};
struct Nearest_Being* nearest_being;
void alloc_nearest(size_t max_count) {
    nearest_being = malloc(sizeof(struct Nearest_Being));
    nearest_being->id = malloc(max_count * sizeof(being_id_t));
    nearest_being->distance = malloc(max_count * sizeof(float));
    nearest_being->direction_x = malloc(max_count * sizeof(float));
    nearest_being->direction_y = malloc(max_count * sizeof(float));
}
struct Furthest_Being {
    being_id_t* id;
    float* distance;
    float* direction_x;
    float* direction_y;
};
struct Furthest_Being* furthest_being;
void alloc_furthest(size_t max_count) {
    furthest_being = malloc(sizeof(struct Furthest_Being));
    furthest_being->id = malloc(max_count * sizeof(being_id_t));
    furthest_being->distance = malloc(max_count * sizeof(float));
    furthest_being->direction_x = malloc(max_count * sizeof(float));
    furthest_being->direction_y = malloc(max_count * sizeof(float));
}
void calculate_nearest_and_furthest() {
    for (being_id_t i = 0; i < physics_states->max_count; i += 1) {
        const float x = physics_states->x[i];
        const float y = physics_states->y[i];
        being_id_t nearest_id = 0;
        float nearest_distance = 99999;
        float nearest_direction_x = 99999;
        float nearest_direction_y = 99999;
        being_id_t furthest_id = 0;
        float furthest_distance = 0;
        float furthest_direction_x = 99999;
        float furthest_direction_y = 99999;
        for (being_id_t j = 0; j < physics_states->max_count; j += 1) {
            if (j != i) {
                const float jx = physics_states->x[j];
                const float jy = physics_states->y[j];
                const float dx = x - jx;
                const float dy = y - jy;
                const float curr_distance = sqrt(dx*dx + dy*dy);
                if (curr_distance < nearest_distance) {
                    nearest_distance = curr_distance;
                    nearest_direction_x = dx / curr_distance;
                    nearest_direction_y = dy / curr_distance;
                    nearest_id = j;
                }
                if (curr_distance > furthest_distance) {
                    furthest_distance = curr_distance;
                    furthest_direction_x = dx / curr_distance;
                    furthest_direction_y = dy / curr_distance;
                    furthest_id = j;
                }
            }
        }
        nearest_being->id[i] = nearest_id;
        nearest_being->distance[i] = nearest_distance;
        nearest_being->direction_x[i] = nearest_direction_x;
        nearest_being->direction_y[i] = nearest_direction_y;
        furthest_being->id[i] = furthest_id;
        furthest_being->distance[i] = furthest_distance;
        furthest_being->direction_x[i] = furthest_direction_x;
        furthest_being->direction_y[i] = furthest_direction_y;
    }
}
void accel_toward_nearest(float factor) {
    for (being_id_t i = 0; i < physics_states->max_count; i += 1) {
        physics_states->x_accel[i] -= nearest_being->direction_x[i] * factor;
        physics_states->y_accel[i] -= nearest_being->direction_y[i] * factor;
    }
}
void accel_toward_furthest(float factor) {
    for (being_id_t i = 0; i < physics_states->max_count; i += 1) {
        physics_states->x_accel[i] -= furthest_being->direction_x[i] * factor;
        physics_states->y_accel[i] -= furthest_being->direction_y[i] * factor;
    }
}

void accel_toward_beings(float factor) {
    for (being_id_t i = 0; i < physics_states->max_count; i += 1) {
        const float x = physics_states->x[i];
        const float y = physics_states->y[i];
        for (being_id_t j = 0; j < physics_states->max_count; j += 1) {
            if (j != i) {
                const float jx = physics_states->x[j];
                const float jy = physics_states->y[j];
                const float dx = x - jx;
                const float dy = y - jy;
                const float distance = sqrt(dx*dx + dy*dy);
                float factor2 = 1.0 / distance;
                if (0.001 < factor2 && factor2 < 10) {
                    const float direction_x = dx / distance;
                    const float direction_y = dy / distance;
                    physics_states->x_accel[i] -= direction_x * factor2 * factor;
                    physics_states->y_accel[i] -= direction_y * factor2 * factor;
                }
            }
        }
    }
}

void accel_randomly(float factor) {
    for (being_id_t i = 0; i < physics_states->max_count; i += 1) {
        physics_states->x_accel[i] += (randf() * 2 - 1) * factor;
        physics_states->y_accel[i] += (randf() * 2 - 1) * factor;
    }
}

void damp_accel(float factor) {
    for (being_id_t i = 0; i < physics_states->max_count; i += 1) {
        physics_states->x_accel[i] *= factor;
        physics_states->y_accel[i] *= factor;
    }
}

struct Tool_Status {
    float x;
    float y;
    float radius;
    bool active;
};
struct Tool_Status tool_status;

EMSCRIPTEN_KEEPALIVE
void set_tool_status(float x, float y, bool active) {
    tool_status.x = x;
    tool_status.y = y;
    tool_status.radius = 100.0;
    tool_status.active = active;
}

void accel_toward_tool(float factor) {
    if (tool_status.active) {
        for (being_id_t i = 0; i < physics_states->max_count; i += 1) {
            const float x = physics_states->x[i];
            const float y = physics_states->y[i];
            const float dx = x - tool_status.x;
            const float dy = y - tool_status.y;
            const float distance = sqrt(dx*dx + dy*dy);
            if (distance < tool_status.radius) {
                float factor2 = (distance*distance*distance + 3 * distance*distance) / 1000;
                const float direction_x = dx / distance;
                const float direction_y = dy / distance;
                physics_states->x_accel[i] -= direction_x * factor2 * factor;
                physics_states->y_accel[i] -= direction_y * factor2 * factor;
            }
        }
    }
}

clock_t prev_time;
EMSCRIPTEN_KEEPALIVE
int init(int width, int height, int being_count) {
    srand(time(NULL));
    prev_time = clock();
    set_tool_status(0.0, 0.0, false);

    alloc_physics_states(being_count);
    alloc_nearest(being_count);
    alloc_furthest(being_count);
    
    for (being_id_t i = 0; i < being_count; i += 1) {
        const float x = randf() * width;
        const float y = randf() * height;
        const float x_accel = randf() * 10;
        const float y_accel = randf() * 10;
        create_being(x,       y,
                     x_accel, y_accel);
    }

    return 0;
}

EMSCRIPTEN_KEEPALIVE
void step() {
    clock_t curr_time = clock();
    float delta = (curr_time - prev_time) / 1000.0 / 1000.0;
    prev_time = curr_time;

    calculate_nearest_and_furthest();
    accel_toward_nearest(0.4);
    accel_toward_furthest(0.1);
    accel_toward_beings(0.5);
    accel_randomly(0.001);
    accel_toward_tool(0.01);
    damp_accel(0.999);

    move_beings(delta);
}

int main(int argc, char** argv) {
}