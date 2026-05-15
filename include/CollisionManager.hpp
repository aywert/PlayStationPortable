#pragma once
#include <vector>
#include <algorithm>
#include "Entity.hpp"

struct Tile {
    CollidableType type; 
    int x; 
    int y;
    Tile(CollidableType t, int px, int py) : type(t), x(px), y(py) {}
};

class CollisionManager {
    uint8_t (*game_map)[MAP_WIDTH];
    std::vector<std::weak_ptr<Entity>> objects_;
    std::function<void(int tileX, int tileY)> wall_destroyed_callback_;
    std::function<void(void)> base_destroyed_callback_;
public:
    CollisionManager(uint8_t (*map)[MAP_WIDTH]) 
        : game_map(map)
    {}
    
    void register_object(std::shared_ptr<Entity> obj) { objects_.push_back(obj); }
    void clear() { objects_.clear(); }

    void set_wall_destroyed_callback(std::function<void(int, int)> callback) {
        wall_destroyed_callback_ = std::move(callback);
    }

    void set_base_destroyed_callback(std::function<void()> callback) {
        base_destroyed_callback_ = std::move(callback);
    }

    void set_game_map(uint8_t (*map)[MAP_WIDTH]) {
        game_map = map;
    }
    
    bool handle_collisions(std::shared_ptr<Entity> mover, Rect future_rect) {
        bool has_collision = false;
        for (auto& weak_target : objects_) {
            auto target = weak_target.lock();
            if (target == mover || !target->is_active() || target->get_owner() == mover) continue;

            Rect r = target->get_collision_rect();
            if (future_rect.x < r.x + r.w && future_rect.x + future_rect.w > r.x &&
                future_rect.y < r.y + r.h && future_rect.y + future_rect.h > r.y) {
                
                mover->on_collision(target);
                target->on_collision(mover);
                has_collision = true;
            }
        }

        auto collision = check_map_collision(future_rect, mover->get_type());
        if (collision.has_value()) {
            auto tmp = std::make_shared<MapWallEntity>(collision->type);
            if (mover->on_collision(tmp)) {
                return false;
            }

            else {has_collision = true;}            
        }

        return has_collision;
    }
    
    void cleanup() {
        objects_.erase(
            std::remove_if(objects_.begin(), objects_.end(),
                [](const auto& weak_obj) { 
                    auto shared_obj = weak_obj.lock();
                    return !shared_obj || !shared_obj->is_active();
                }),
            objects_.end()
        );
    }

    std::optional<Tile> check_map_collision(Rect rect, CollidableType type) {
        int start_col = rect.x / TILE_SIZE;
        int end_col   = (rect.x + rect.w - 1) / TILE_SIZE;
        int start_row = rect.y / TILE_SIZE;
        int end_row   = (rect.y + rect.h - 1) / TILE_SIZE;

        if (start_col < 0 || end_col >= MAP_WIDTH || start_row < 0 || end_row >= MAP_HEIGHT) {
           return std::optional<Tile>(Tile(CollidableType::NONE, 0, 0));
        }

        for (int i = start_row; i <= end_row; i++) {
            for (int j = start_col; j <= end_col; j++) {
                switch(game_map[i][j]) {
                    case SHIELD:
                    case SHIELD_ON_BEDROCK:
                    case SHIELD_ON_GRASS:
                    case SHIELD_ON_BRICK:
                        return std::optional<Tile>(Tile{CollidableType::SHIELD, i, j}); 
                    case BRICKS_WALL: {
                        if (type == CollidableType::BULLET) {
                            game_map[i][j] = BLACK;
                            if (wall_destroyed_callback_) {
                                wall_destroyed_callback_(j * TILE_SIZE, i * TILE_SIZE);
                            }
                        }
                        
                        return std::optional<Tile>(Tile{CollidableType::WALL, i, j}); 
                    }

                    case BEDROCK: return std::optional<Tile>(Tile{CollidableType::WALL, i, j}); ;
                    case SPECIAL: {
                        if (type == CollidableType::BULLET) {
                            game_map[i][j] = BLACK;
                            if (base_destroyed_callback_) {
                                wall_destroyed_callback_(j * TILE_SIZE, i * TILE_SIZE);
                                base_destroyed_callback_();
                            }
                        }
                        
                        return std::optional<Tile>(Tile{CollidableType::WALL, i, j}); 
                    }
                }
            }
        }
        return std::nullopt;
    }

    std::vector<Direction> get_valid_directions(int speed, Rect current_rect) {
        std::vector<Direction> valid_directions;
        
        // Проверка движения вверх
        Rect up_rect = current_rect;
        up_rect.y -= speed;
        if (!is_colliding_with_map(up_rect)) {
            valid_directions.push_back(DIR_UP);
        }
        
        // Проверка движения вниз
        Rect down_rect = current_rect;
        down_rect.y += speed;
        if (!is_colliding_with_map(down_rect)) {
            valid_directions.push_back(DIR_DOWN);
        }
        
        // Проверка движения влево
        Rect left_rect = current_rect;
        left_rect.x -= speed;
        if (!is_colliding_with_map(left_rect)) {
            valid_directions.push_back(DIR_LEFT);
        }
        
        // Проверка движения вправо
        Rect right_rect = current_rect;
        right_rect.x += speed;
        if (!is_colliding_with_map(right_rect)) {
            valid_directions.push_back(DIR_RIGHT);
        }
        
        return valid_directions;
    }

    bool is_colliding_with_map(const Rect& rect) {
        if (!game_map) return true;
        
        // Проверка границ
        if (rect.x < 0 || rect.x + rect.w > X_MAX ||
            rect.y < 0 || rect.y + rect.h > Y_MAX) {
            return true;
        }
        
        // Получаем центральную клетку танка
        int center_x = (rect.x + rect.w / 2) / TILE_SIZE;
        int center_y = (rect.y + rect.h / 2) / TILE_SIZE;
        
        // Проверяем только центральную клетку и клетки по краям
        int left_tile   = rect.x / TILE_SIZE;
        int right_tile  = (rect.x + rect.w - 1) / TILE_SIZE;
        int top_tile    = rect.y / TILE_SIZE;
        int bottom_tile = (rect.y + rect.h - 1) / TILE_SIZE;
        
        // Проверяем только угловые точки (4 клетки максимум)
        int tiles_x[] = {left_tile, right_tile};
        int tiles_y[] = {top_tile, bottom_tile};
        
        for (int ty : tiles_y) {
            for (int tx : tiles_x) {
                if (tx >= 0 && tx < MAP_WIDTH && ty >= 0 && ty < MAP_HEIGHT) {
                    if (
                        game_map[ty][tx] == BRICKS_WALL || 
                        game_map[ty][tx] == BEDROCK) {
                        return true;
                    }
                }
            }   
        }
        
        return false;
    }
};
