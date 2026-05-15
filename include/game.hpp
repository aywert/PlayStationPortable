#ifndef GAME_CLASS
#define GAME_CLASS

#include <SPI.h>
#include <TFT_eSPI.h> 
#include <vector>
#include <memory>
#include <optional>
#include "CollisionManager.hpp"
#include "LevelManager.hpp"
#include "Tank.hpp"
#include "BotTank.hpp"
#include "Button.hpp"
#include "Harpoon.hpp"
#include "Map.hpp"
#include "./helpers/helper_for_std_unordered_set.hpp"
#include "./helpers/RectCompare.hpp"

constexpr size_t COUNT_DOWN = 5;
extern Level levels[];

enum class GameStatus {
    IN_PROGRESS,
    ON_HOLD,
    OVER,
};

class Game final {
    TFT_eSPI& tft_;
    GameStatus status_ = GameStatus::IN_PROGRESS;

    CollisionManager collision_mgr_;
    LevelManager     level_mgr_;

    uint8_t game_map_[MAP_HEIGHT][MAP_WIDTH];

    std::vector<std::shared_ptr<Tank>>      tanks_;
    std::vector<std::shared_ptr<Bullet>>  bullets_;
    std::vector<std::shared_ptr<Harpoon>> harpoons_;

    std::unique_ptr<uint16_t[]> full_screen_buffer_;

    Button buttons_[BTN_COUNT];

    bool is_running_ = true;

    int last_health_ = -1;
    int last_ammo_ = -1;
    int last_x_ = -1;
    int last_y_ = -1;

    void move_player(DirtyRectsSet& dirty_rects);
    void move_bots(DirtyRectsSet& dirty_rects);
    uint16_t getBlockColor(int row, int col);

    public:
        Game(TFT_eSPI& tft) : tft_(tft), level_mgr_(levels, 3), collision_mgr_(nullptr), 
        buttons_{
            Button(BTN_UP_PIN),     // BTN_UP
            Button(BTN_DOWN_PIN),   // BTN_DOWN
            Button(BTN_LEFT_PIN),   // BTN_LEFT
            Button(BTN_RIGHT_PIN),  // BTN_RIGHT
            Button(BTN_X_PIN),      // BTN_X
            Button(BTN_Y_PIN),      // BTN_Y
            Button(BTN_A_PIN),      // BTN_A
            Button(BTN_B_PIN),      // BTN_B
            Button(BTN_PAUSA_PIN)}  // BTN_PAUSA 
        {
            memcpy(game_map_, level_mgr_.get_current_level()->map, sizeof(game_map_));

            level_mgr_.set_is_field_empty_callback(
                [this]() { return tanks_.size() == 1;}
            );

            collision_mgr_.set_game_map(game_map_);

            collision_mgr_.set_wall_destroyed_callback(
                [&tft](int x, int y) {
                    tft.fillRect(x, y, TILE_SIZE, TILE_SIZE, TFT_BLACK);
                }
            );

            collision_mgr_.set_base_destroyed_callback(
                [this]() { 
                    status_ = GameStatus::OVER;  
                }
            );
        };

        void restart() {
            tanks_.clear();
            bullets_.clear();
            harpoons_.clear();
            
            status_ = GameStatus::IN_PROGRESS;
            is_running_ = true;
        
            level_mgr_.restart();
            memcpy(game_map_, level_mgr_.get_current_level()->map, sizeof(game_map_));

            //updating static variables in order to let info table work correctly
            last_health_ = -1;
            last_ammo_ = -1;
            last_x_ = -1;
            last_y_ = -1;

            start();
        }

        void start(void);

        // function to update the status of buttons, can be used in the main loop to check for button presses
        std::vector<Rect> draw_map();
        void draw_map_part(Rect r);
        void draw_pause_screen();
        void draw_info_table();
        void print_tank_data_to_info_table(const Tank& tank, bool force_update = false);

        void check_updates_buttons(void); //input_manager
        void execute_updates();
        void advance_to_next_level();

        void register_collidables();

        void create_tank(size_t x_pos, size_t y_pos, size_t health, size_t ammunition, size_t speed); 
        void create_bot( size_t x_pos, size_t y_pos, BotType type);

        //check whether or not coordinates lies within a tank which already exists
        bool is_block_free(size_t x_pos, size_t y_pos);
        void delete_tank(size_t index);
        void delete_enemy_tanks(void); 
        void create_flying_bullet (std::shared_ptr<Tank> tank);
        void create_flying_harpoon(std::shared_ptr<Tank> tank);
        bool is_out_of_bounds (Rect next);
        void cleanup_dead_objects();

        GameStatus get_status() const noexcept {return status_;}
        bool   is_in_progress() const noexcept {return status_ == GameStatus::IN_PROGRESS;}

        Game& operator=(Game& other) = delete; // delete copy assignment operator
        Game(Game& other)  = delete; // delete copy constructor
        Game(Game&& other) = delete; // delete move constructor

        void save_full_background() {
            tft_.readRect(0, 0, X_MAX, Y_MAX, full_screen_buffer_.get());
        }
        
        void restore_full_background() {
            tft_.pushImage(0, 0, X_MAX, Y_MAX, full_screen_buffer_.get());
        }
};

void CountDown(TFT_eSPI& tft);

#endif