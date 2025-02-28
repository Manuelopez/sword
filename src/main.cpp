#include "./helper.cpp"
#include "raylib.h"
#include "raymath.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

// :const
#define TILE_WIDTH 15
#define MAX_ENTITY_COUNT 1024
// :Range util

struct Range1f {
  float min;
  float max;
};

struct Range2f {
  Vector2 min;
  Vector2 max;
};

inline Range2f range2f_make(Vector2 min, Vector2 max) { return {min, max}; }

Range2f range2f_shift(Range2f r, Vector2 shift) {
  r.min = Vector2Add(r.min, shift);
  r.max = Vector2Add(r.max, shift);
  return r;
}

Range2f range2f_make_bottom_center(Vector2 size) {
  Range2f range = {0};
  range.max = size;
  range = range2f_shift(range, {float(0), 0.0});
  return range;
}

Range2f range2f_make_center_center(Vector2 size) {
  Range2f range = {0};
  range.max = size;
  range = range2f_shift(range, {-(size.x / 2), -(size.y / 2)});
  return range;
}

Vector2 range2f_size(Range2f range) {
  Vector2 size = {0};
  size = Vector2Subtract(range.min, range.max);
  size.x = fabsf(size.x);
  size.y = fabsf(size.y);
  return size;
}

bool range2f_contains(Range2f range, Vector2 v) {
  return v.x >= range.min.x && v.x <= range.max.x && v.y >= range.min.y &&
         v.y <= range.max.y;
}

// :tile :util
int world_position_to_tile_position(float world_position) {
  return roundf(world_position / (float)TILE_WIDTH);
}

Vector2 world_position_to_tile_position(Vector2 world_position) {
  world_position.x = world_position_to_tile_position(world_position.x);
  world_position.y = world_position_to_tile_position(world_position.y);
  return world_position;
}

float tile_position_to_world_position(float tile_position) {
  return tile_position * TILE_WIDTH;
}

Vector2 tile_position_to_world_position(Vector2 tile_position) {
  tile_position.x = tile_position_to_world_position(tile_position.x);
  tile_position.y = tile_position_to_world_position(tile_position.y);
  return tile_position;
}

Vector2 round_vector2_world_position_to_tile_position(Vector2 world_position) {
  world_position.x = tile_position_to_world_position(
      world_position_to_tile_position(world_position.x));
  world_position.y = tile_position_to_world_position(
      world_position_to_tile_position(world_position.y));

  return world_position;
}

// :camera util

bool almost_equals(float a, float b, float epsilon) {
  return fabs(a - b) <= epsilon;
}
bool animate_to_target(float *value, float target, float delta_time,
                       float rate) {
  *value += (target - *value) * (1.0 - pow(2.0f, -rate * delta_time));
  if (almost_equals(*value, target, 0.001f)) {
    *value = target;
    return true;
  } else {

    return false;
  }
}

void animate_to_target(Vector2 *value, Vector2 target, float delta_time,
                       float rate) {
  animate_to_target(&value->x, target.x, delta_time, rate);
  animate_to_target(&value->y, target.y, delta_time, rate);
}

enum class TextureId { NIL, PLAYER, GOBLIN, TROLL, MAX };
Texture2D textures[(int)TextureId::MAX] = {};
Texture2D *get_texture(TextureId id) { ; }

enum class EntityArcheType {
  PLAYER = 0,
  GOBLIN = 1,
  TROLL = 2,
  MAX = 3,
};

// attacks
enum class AttackArcheType {

  NIL = 0,
  SWIPE_LEFT = 1,
  SWIPE_RIGHT = 2,
  SWIPE_UP = 3,
  SWIPE_DOWN = 4,
};

enum class BlockArcheType {
  NIL = 0,
  LEFT = 1,
  RIGHT = 2,
  UP = 3,
  DOWN = 4,
};

// blocks

struct Entity {
  bool is_valid;
  EntityArcheType type;
  TextureId textureId;
  Vector2 position;

  Vector2 *before_attack_position;
  Vector2 *end_movement_position;
  Vector2 *end_attack_position;
  float movement_speed;
  float attack_speed;
  float attack_range;
  float attack_omega; // for rotating whiles attacking

  Vector2 input_axis;
  Vector2 attack_input_axis;
  float angle;
  float angle_before_attack;
  int health;
  int damage;

  AttackArcheType attack;
  BlockArcheType block;

  bool is_attacking() { return this->attack != AttackArcheType::NIL; }
  bool is_blocking() { return this->attack != AttackArcheType::NIL; }
  void rectangle() {
    Texture2D *texture = this->get_texture();
    Range2f bounds = range2f_make_bottom_center(
        Vector2{(float)(texture->width), (float)texture->height});
    bounds = range2f_shift(bounds, this->position);
    Color color = RED;
    color.a = 100;

    Vector2 size = range2f_size(bounds);
    DrawRectanglePro({bounds.min.x, bounds.min.y, size.x, size.y},
                     {size.x / 2, size.y / 2}, 0, color);
  }

  Texture2D *get_texture() {
    if ((int)this->textureId < 0 || this->textureId >= TextureId::MAX) {
      return &textures[(int)TextureId::NIL];
    }
    return &textures[(int)this->textureId];
  }
};

struct World {
  Entity entities[MAX_ENTITY_COUNT];
};

World *world = 0;

Entity *create_entity() {
  assert(world && "wolrd is null");

  Entity *entity_found = 0;
  for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
    Entity *existing_entity = &world->entities[i];
    if (!existing_entity->is_valid) {
      entity_found = existing_entity;
    }
  }
  assert(entity_found && "no more enities memory full");
  entity_found->is_valid = true;

  assert(entity_found->is_valid == true && "entity not valid");
  return entity_found;
}

void entity_destroy(Entity *entity) { std::memset(entity, 0, sizeof(Entity)); }

void setup_player(Entity *entity) {
  entity->type = EntityArcheType::GOBLIN;
  entity->textureId = TextureId::PLAYER;
  entity->attack_speed = 200;
  entity->movement_speed = 100;
  entity->attack_range = 50;
}
void setup_goblin(Entity *entity) {
  entity->type = EntityArcheType::GOBLIN;
  entity->textureId = TextureId::GOBLIN;
}
void setup_troll(Entity *entity) {
  entity->type = EntityArcheType::TROLL;
  entity->textureId = TextureId::TROLL;
}

int main(void) {
  // :Initialization
  //--------------------------------------------------------------------------------------
  const int screenWidth = 800;
  const int screenHeight = 450;
  world = new World{};
  SearchAndSetResourceDir("resources");
  InitWindow(screenWidth, screenHeight, "raylib [core] example - basic window");

  Texture2D sprite_player = LoadTexture("character-001-idle.png");
  Texture2D sprite_goblin = LoadTexture("goblin-001-idle.png");
  Texture2D sprite_troll = LoadTexture("troll-001-idle.png");
  textures[(int)TextureId::PLAYER] = sprite_player;
  textures[(int)TextureId::GOBLIN] = sprite_goblin;
  textures[(int)TextureId::TROLL] = sprite_troll;
  std::string attack_1 = "Q - Left";
  std::string attack_2 = "W - Right";
  std::string attack_3 = "E - Up";
  std::string attack_4 = "R - Down";

  // :mobs
  for (int i = 0; i < 10; i++) {
    Entity *en = create_entity();
    setup_goblin(en);
    en->position = {(float)GetRandomValue(-200, 200),
                    (float)GetRandomValue(-200, 200)};
    en->position = round_vector2_world_position_to_tile_position(en->position);
  }

  Entity *player_en = create_entity();
  setup_player(player_en);
  player_en->position = {0, 0};

  // :init :camera
  Camera2D camera{};
  camera.offset = {screenWidth / 2.0f, screenHeight / 2.0f};
  camera.zoom = 2.0;
  camera.target = player_en->position;
  // updatin position
  //
  //

  SetTargetFPS(60); // Set our game to run at 60 frames-per-second
  //--------------------------------------------------------------------------------------

  // Main game loop
  while (!WindowShouldClose()) // Detect window close button or ESC key
  {

    // :reset values
    float delta_time = GetFrameTime();
    for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
      Entity *entity = &world->entities[i];
      if (entity->is_valid) {

        if (!entity->is_attacking()) {
          if (entity->end_movement_position != nullptr) {

            if (almost_equals(entity->position.x,
                              entity->end_movement_position->x, 5) &&
                almost_equals(entity->position.y,
                              entity->end_movement_position->y, 5)) {
              entity->input_axis = {0, 0};
              entity->end_movement_position = nullptr;
            }
          }
        } else {

          if (entity->end_attack_position != nullptr) {
            if (almost_equals(entity->position.x,
                              entity->end_attack_position->x, 5) &&
                almost_equals(entity->position.y,
                              entity->end_attack_position->y, 5)) {
              entity->attack_input_axis = {0, 0};
              entity->end_attack_position = nullptr;
              entity->position = *entity->before_attack_position;
              entity->before_attack_position = nullptr;
              entity->attack = AttackArcheType::NIL;
              entity->angle = entity->angle_before_attack;
            }
          }
        }
      }
    }

    // :mouse :values
    Vector2 mouse_position_camera = GetMousePosition();
    Vector2 mouse_position_world =
        GetScreenToWorld2D(mouse_position_camera, camera);
    Vector2 mouse_position_tile =
        world_position_to_tile_position(mouse_position_world);

    // :capture :inptus :mouse
    // :cpature :input :player
    {

      if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) &&
          !player_en->is_attacking()) {
        Vector2 mouse_pos = {mouse_position_world.x, mouse_position_world.y};
        player_en->end_movement_position = &mouse_pos;

        player_en->input_axis = Vector2Normalize(
            Vector2Subtract(mouse_position_world, player_en->position));

        player_en->angle = (atan2((mouse_pos.y - player_en->position.y),
                                  (mouse_pos.x - player_en->position.x)) *
                            RAD2DEG) +
                           90;

        /* player_en->angle = */
        /*     Vector2LineAngle(player_en->position, mouse_pos) * RAD2DEG; */

        // std::cout << "this is the angle " << player_en->angle;
      }

      // :input :keyboard :attack

      {

        if (IsKeyPressed(KEY_Q)) {

          Vector2 player_current_position = player_en->position;
          player_en->angle_before_attack = player_en->angle;

          player_en->angle =
              (atan2((mouse_position_world.y - player_current_position.y),
                     (mouse_position_world.x - player_current_position.x)) *
               RAD2DEG) +
              90 + 90;
          player_en->before_attack_position = &player_current_position;
          player_en->attack = AttackArcheType::SWIPE_LEFT;
          Vector2 move_to =
              Vector2Subtract(player_current_position, mouse_position_world);
          move_to = Vector2Scale(move_to, -1);
          // rotate 90 clockwise
          move_to = {-move_to.y, move_to.x};
          move_to = Vector2Normalize(move_to);
          Vector2 begin_position = Vector2Rotate(player_en->position, 90);
          Vector2 end_position =
              Vector2Add(player_en->position,
                         Vector2Scale(move_to, -player_en->attack_range));

          player_en->position =
              Vector2Add(player_en->position,
                         Vector2Scale(move_to, player_en->attack_range));
          Vector2 input_axis = Vector2Normalize(
              Vector2Subtract(end_position, player_en->position));

          player_en->attack_input_axis = input_axis;
          player_en->end_attack_position = &end_position;
          float distance = Vector2Distance(player_en->position, end_position);
          float time = distance / (player_en->attack_speed * delta_time);
          float omega = -180 / time;
          player_en->attack_omega = omega;

          /**/
          /* Vector2 begin_position = Vector2MoveTowards( */
          /*     player_en->position, */
          /*     {-mouse_position_world.y, mouse_position_world.x}, 40); */
          /* player_en->position = begin_position; */
          /* player_en->end_movement_position = &end_position; */
          /* player_en->input_axis = Vector2Normalize( */
          /*     Vector2Subtract(player_en->position, end_position)); */
        }
      }
    }

    //----------------------------------------------------------------------------------
    // TODO: Update your variables here
    //----------------------------------------------------------------------------------

    // :update
    {
      // :update :camera
      {
        if (!player_en->is_attacking()) {
          animate_to_target(&camera.target, player_en->position, delta_time,
                            15.0f);
        }
      }

      // :update :position

      // update draw positions

      for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
        Entity *entity = &world->entities[i];
        if (entity->is_valid) {

          if (!entity->is_attacking()) {
            entity->position =
                Vector2Add(entity->position,
                           Vector2Scale(entity->input_axis,
                                        entity->movement_speed * delta_time));
          } else {
            entity->position =
                Vector2Add(entity->position,
                           Vector2Scale(entity->attack_input_axis,
                                        entity->attack_speed * delta_time));
            entity->angle += entity->attack_omega;
          }
        }
      }
    }

    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();
    ClearBackground(RAYWHITE);
    BeginMode2D(camera);

    // :2d :world :camera
    {

      // :tile :renderign
      {

        int player_tile_x =
            world_position_to_tile_position(player_en->position.x);
        int player_tile_y =
            world_position_to_tile_position(player_en->position.y);
        const int tile_radius_x = 40;
        const int tile_radius_y = 30;

        for (int x = player_tile_x - tile_radius_x;
             x < player_tile_x + tile_radius_x; x++) {
          for (int y = player_tile_y - tile_radius_y;
               y < player_tile_y + tile_radius_y; y++) {
            if ((x + (y % 2 == 0)) % 2 == 0) {
              float x_pos = x * TILE_WIDTH;
              float y_pos = y * TILE_WIDTH;
              Color color = GRAY;
              /* if (x == mouse_position_tile.x && y ==
               * mouse_position_tile.y) {
               */
              /*   color = RED; */
              /* } */
              DrawRectangleV(
                  {
                      x_pos + (float)(TILE_WIDTH * -0.5),
                      (y_pos + (float)(TILE_WIDTH * -0.5)),
                  },
                  {TILE_WIDTH, TILE_WIDTH}, color);
            }
          }
        }
        Vector2 mouse_tile_to_world =
            tile_position_to_world_position(mouse_position_tile);

        mouse_tile_to_world.x += TILE_WIDTH * -0.5;
        mouse_tile_to_world.y += TILE_WIDTH * -0.5;
        DrawRectangleV(mouse_tile_to_world, {TILE_WIDTH, TILE_WIDTH}, RED);
      }

      // :render
      for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
        Entity *entity = &world->entities[i];
        if (entity->is_valid) {
          Texture2D *texture = entity->get_texture();
          Rectangle source_rectangle = {0.0f, 0.0f, (float)texture->width,
                                        (float)texture->height};
          Rectangle destination_rectangle = {
              entity->position.x, entity->position.y, (float)texture->width,
              (float)texture->height};
          Vector2 origin = {(float)texture->width / 2,
                            (float)texture->height / 2};
          switch (entity->type) {
          default: {
            DrawTexturePro(*texture, source_rectangle, destination_rectangle,
                           origin, entity->angle, WHITE);
            entity->rectangle();

            break;
          }
          }
        }
      }
      {

        // :mouse :selection
        //
        {
          Vector2 mosue_tile_position =
              world_position_to_tile_position(mouse_position_world);
          for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
            Entity *entity = &world->entities[i];
            if (entity->is_valid) {
              Texture2D *texture = entity->get_texture();
              Range2f bounds = range2f_make_bottom_center(
                  Vector2{(float)(texture->width), (float)texture->height});
              bounds = range2f_shift(bounds, entity->position);
              Color color = WHITE; // TODO
              color.a = 100;
              if (range2f_contains(bounds, mouse_position_world)) {
                color.a = 200;
              }

              // TODO
              /* DrawRectangleV(bounds.min, range2f_size(bounds), color); */
              Vector2 size = range2f_size(bounds);
              /* DrawRectanglePro({bounds.min.x, bounds.min.y, size.x, size.y},
               */
              /*                  {size.x / 2, size.y / 2}, entity->angle,
               * color); */
            }
          }
        }
      }
    }

    EndMode2D();

    // :ui
    {
      DrawText(attack_1.c_str(), screenWidth / 2 - (20 * attack_1.length() * 2),
               screenHeight - 20, 20, BLACK);

      DrawText(attack_2.c_str(), screenWidth / 2 - (20 * attack_1.length() * 1),
               screenHeight - 20, 20, BLACK);

      DrawText(attack_3.c_str(), screenWidth / 2 + (20 * attack_1.length() * 1),
               screenHeight - 20, 20, BLACK);

      DrawText(attack_4.c_str(), screenWidth / 2 + (20 * attack_1.length() * 2),
               screenHeight - 20, 20, BLACK);
    }
    /* std::cout << " x: " << mouse_position_world.x */
    /*           << " y: " << mouse_position_world.y << std::endl; */

    EndDrawing();
    //----------------------------------------------------------------------------------
  }

  for (int i = 0; i < (int)TextureId::MAX; i++) {
    UnloadTexture(textures[i]);
  }

  // De-Initialization
  //--------------------------------------------------------------------------------------
  CloseWindow(); // Close window and OpenGL context
  //--------------------------------------------------------------------------------------

  return 0;
}
