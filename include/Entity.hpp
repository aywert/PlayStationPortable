#ifndef DRAWABLE_H
#define DRAWABLE_H

#include <TFT_eSPI.h>
#include "Map.hpp"

struct Rect { 
  int x, y, w, h; 
};

enum Direction {
  DIR_UP   ,
  DIR_DOWN ,
  DIR_LEFT , 
  DIR_RIGHT,
};

enum class CollidableType {
  BOT,
  TANK,
  BULLET,
  WALL,
  SHIELD, 
  HARPOON,
  NONE
};

constexpr uint16_t TRANSPARENT_COLOR = 0x0000; // white light considers transparent

class Entity {
  protected:
    int pos_x, pos_y, old_x, old_y;
    std::unique_ptr<uint16_t[]> background_buffer = nullptr;  // buffer to store the background pixels before drawing the tank
    int width, height;
    
    bool active;        // активен ли объект (true - рисовать/обновлять)
    bool visible;       // видим ли объект

    int orientation; // start from up and increases by turning to the left in degrees
    
  public:
     Entity(int startX = 0, int startY = 0, int w = 10, int h = 10) : 
      pos_x(startX), 
      pos_y(startY), 
      old_x(startX), 
      old_y(startY),
      width(w), 
      height(h), 
      active(true), 
      visible(true),
      orientation(0) {
        // if you need to use background buffer, uncomment this part in order to allocate mempry
        // if (width > 0 && height > 0) {
        //   background_buffer = std::make_unique<uint16_t[]>(width * height);
        // }
      }

    virtual ~Entity() {}
    
    //inheritor have to realize this functions 
    virtual void draw()   = 0;

    void restore_background(TFT_eSPI& tft) {
      tft.setSwapBytes(false); 
      tft.pushImage(old_x, old_y,width, height, background_buffer.get());
    }

    void save_background(TFT_eSPI& tft) {
      tft.setSwapBytes(false); 
      tft.readRect(pos_x, pos_y, width, height, background_buffer.get());
    }
    
    int getX()      { return pos_x; }
    int getY()      { return pos_y; }
    int getWidth()  { return width; }
    int getHeight() { return height; }
    int getOrientation() { return orientation; }
    
    void setPosition(int newX, int newY) {
      pos_x = newX;
      pos_y = newY;
    }

    void setOrientation(int newOrientation) {
      orientation = newOrientation;
    }
    
    void move(int dx, int dy) {
      pos_x += dx;
      pos_y += dy;
    }
    
    bool isActive() { return active; }
    void setActive(bool state) { active = state; }
    
    bool isVisible() { return visible; }
    void setVisible(bool state) { visible = state; }
    
    // Проверка столкновения с другим объектом
    bool collidesWith(int dx, int dy,  Entity* other) {
      return ((pos_x + dx + width  > other->pos_x  &&
               pos_y + dy + height > other->pos_y) &&
              (pos_x + dx < other->pos_x + other->width &&
               pos_y + dy < other->pos_y + other->height)
            );
    }
    
    // Проверка, находится ли точка внутри объекта
    bool containsPoint(int px, int py) {
      return (px >= pos_x && px <= pos_x + width &&
        py >= pos_y && py <= pos_y + height);
    }

  virtual std::shared_ptr<Entity> get_owner() const { return std::shared_ptr<Entity>(); }
  virtual Rect get_collision_rect() const = 0;
  virtual bool on_collision(std::shared_ptr<Entity> other) = 0; 
  virtual bool is_active() const = 0; 
  virtual void update() = 0;
  virtual CollidableType get_type() const = 0;
};

class MapWallEntity : public Entity {
public:
    CollidableType type_ = CollidableType::WALL;
    MapWallEntity(CollidableType type) : Entity(0, 0, TILE_SIZE, TILE_SIZE), type_(type) {}
    void draw() override {} 
    void update() override {}
    bool is_active() const override { return true; }
    CollidableType get_type() const override { return type_; }
    Rect get_collision_rect() const override { return {0,0,0,0}; } // Не используется
    bool on_collision(std::shared_ptr<Entity> other) override {return false;} 
};

#endif