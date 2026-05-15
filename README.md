# Проект: Игра в танки на ESP32 и TFT дисплее (480x320)

#### Техническое задание (ТЗ)

#### 1. Цель проекта
Разработать игру «Танки» (аналог классической Battle City) для микроконтроллера ESP32 с выводом графики на TFT дисплей 480x320.

#### 2. Целевая платформа

| Компонент | Модель | Характеристики |
|-----------|--------|----------------|
| Микроконтроллер | ESP32 | ESP-WROOM-32, 240MHz, 320KB RAM, 4МБ Flash|
| Дисплей | TFT 3.5" / 3.95" | Разрешение 480x320, контроллер **ILI9488**, SPI интерфейс |
| Ввод | Физические кнопки | 9 кнопок (Вверх, Вниз, Влево, Вправо, Стрельба, Старт, доплнительный кнопки) |

#### 3. Функциональные требования

#### 3.1. Механика
- [x] Управление танком игрока (вперед/назад/повороты)
- [x] Стрельба снарядами
- [x] Вражеские танки(боты)
- [x] Разрушаемые/неразрушаемые стены (кирпичные)
- [x] База игрока (орел)

### 3.2. Дополнительная механика
- [x] Гарпун

#### 3.3. Графика и отображение
- [x] Рендеринг игрового поля 24x16 клеток (размер клетки 20x20 пикселя)
- [x] Спрайты: игрок, враги, снаряды, стены
- [x] Счет игрока и количество жизней
- [x] Текущий уровень


### 4 Технологический стек

В качестве среды разработки была выбрана **PlatformIO**, которая облегчает работы с библиотеками и конфигурацией проекта

В проекте используется библиотеки: SPI.h TFT_eSPI.h для работы с дисплеем

<details>
<summary><b><span style="color: #d86812;">platformio.ini</span> <span style="color: #164542;">(раскрыть код)</span></b></summary>


```cpp
[env]
framework = arduino
monitor_speed = 115200
lib_deps = 

[env:esp32doit-devkit-v1]
platform = espressif32
board = esp32doit-devkit-v1
upload_speed = 115200
upload_port = COM9
lib_deps = bodmer/TFT_eSPI@^2.5.43
board_build.partitions = huge_app.csv 
build_unflags = 
	-std=gnu++11
	-std=c++11
	-std=gnu++0x
	-std=c++0x
build_flags = 
	-std=gnu++14

```
</details>


Проект пишется на C++, что удобно для описания сущностей таких как танк, пуля, стена, как классы с наследованием. Помимо этого используемые библиотеки(и большинство библиотек для esp32) написаны на C++, что позволит местами упростить работу над проектом и не писать "велосипед".


# Этапы разработки
* Реализовать класс Tank, описывающий базовую механику танков по полю.
<details>
<summary><b><span style="color: #475614;">class Tank</span> <span style="color: #164542;">(раскрыть код)</span></b></summary>


```cpp
class Tank : public Entity {
  const size_t max_health_, max_ammunition_; 
  int health_, ammunition_, speed_; 

  bool is_valid_;
  bool active_ = true;
  TankState state_ = TankState::Active;

  inline static const uint16_t* skins[2][4] = {
    {default_tank_up, 
    default_tank_down, 
    default_tank_left, 
    default_tank_right}, 
    {boom, boom, boom, boom}
  };

  int tank_type = 0; // Индекс скина для конкретного экземпляра

  int explosion_timer_ = 0;
  int EXPLOSION_DURATION = 10;

  //shot stand for a bullet shot
  unsigned long lastShotTime = 0;
  unsigned long shootCooldownMs = 400;

  // launch stands for harpoon shot
  unsigned long lastLaunchTime = 0;
  unsigned long launchCooldownMs = 1000;

  int reloadCounter_ = 0;
  
  TFT_eSPI& tft_;

  public:
    Tank(size_t x_pos, size_t y_pos, size_t health, size_t ammunition, size_t speed, TFT_eSPI& tft) : 
    Entity(x_pos, y_pos, DEFAULT_TANK_WIDTH, DEFAULT_TANK_HEIGHT),
    tft_(tft), speed_(speed), max_health_(health), health_(health),

    max_ammunition_(ammunition), ammunition_(ammunition), is_valid_(false) { 
      is_valid_ = true;
    };  

    void draw() override;
    bool is_valid() {return is_valid_;}  
    void update() override { 
      explosion_timer_++; 
      if (ammunition_ < max_ammunition_) {
        reloadCounter_++;
        if (reloadCounter_ >= 30) { 
            reloadCounter_ = 0;
            ammunition_++;
        }
      }
    } 

    std::shared_ptr<Entity> get_owner() const override { return std::shared_ptr<Entity>(); }

    Rect get_collision_rect() const override {
      return {pos_x, pos_y, width, height};
    }

    Rect get_next_position_rect(Direction dir) const {
      Rect current_rect = get_collision_rect();
      Rect next_rect = current_rect;
      int speed = get_speed();
      
      switch(dir) {
        case Direction::DIR_UP:    
          next_rect.y -= speed; 
          break;
        case Direction::DIR_DOWN:  
          next_rect.y += speed; 
          break;
        case Direction::DIR_LEFT:  
          next_rect.x -= speed; 
          break;
        case Direction::DIR_RIGHT: 
          next_rect.x += speed; 
          break;
      }
      
      return next_rect;
    }

    void on_collision(std::shared_ptr<Entity> other) override {
      auto type = other->get_type();
      
      switch (type) {
        case CollidableType::BULLET:
          health_ -= 10;
          if (health_ <= 0) { mark_exploding();}
          break;
            
        case CollidableType::WALL:
          break;

        case CollidableType::TANK:
          break;
            
        default:  
          break;
      }
    }

    CollidableType get_type() const override {
      return CollidableType::TANK;
    }

    void shoot() {ammunition_--;}

    bool canShoot() {
      unsigned long tmp = millis(); //timer
      if (tmp-lastShotTime >= shootCooldownMs && ammunition_ > 0) {
        lastShotTime = tmp;
        return true; 
      }

      return false;
    }

    bool canLaunchHarpoon() {
      unsigned long tmp = millis();
      if (tmp-lastLaunchTime >= launchCooldownMs) {
        lastLaunchTime = tmp;
        return true; 
      }

      return false;
    }

    bool animation_finished() const {return explosion_timer_ >= EXPLOSION_DURATION;}
    void mark_dead() {state_ = TankState::Dead;}
    void mark_exploding() {tank_type = 1; explosion_timer_ = 0; state_ = TankState::Exploding;}

    bool is_active()    const override {return state_ == TankState::Active;}
    bool is_exploding() const {return state_ == TankState::Exploding;}
    bool is_dead()      const {return state_ == TankState::Dead;}

    void kill() {active_ = false;} 

    void update_orientation(int dx, int dy);

    std::pair<int, int> count_nose_of_the_tank(int bullet_width, int bullet_length) const {
      switch(orientation) {
        case DIR_UP:    return std::pair<int, int>(pos_x + width/2 - bullet_width/2, pos_y - bullet_length);  break;
        case DIR_DOWN:  return std::pair<int, int>(pos_x + width/2 - bullet_width/2, pos_y + height);         break;
        case DIR_LEFT:  return std::pair<int, int>(pos_x - bullet_length, pos_y + height/2 - bullet_width/2); break;
        case DIR_RIGHT: return std::pair<int, int>(pos_x + width, pos_y + height/2 - bullet_width/2);         break;
      }

      return std::pair{0, 0};
    }    

    void set_speed(size_t speed) {speed_ = speed;}
    void set_health(size_t health) {health_ = health;}
    void set_ammunition(size_t ammo) {ammunition_ = ammo;}
    void set_reload_counter(size_t counter) {reloadCounter_ = counter;}
    unsigned long get_last_shot_time()  const noexcept { return lastShotTime; }
    unsigned long get_shoot_cooldown()  const noexcept { return shootCooldownMs; }
    unsigned long get_explosion_timer() const noexcept { return explosion_timer_; }
    unsigned long get_reload_counter()  const noexcept { return reloadCounter_; }
    void inc_explosion_timer()  noexcept { explosion_timer_++; } 
    void inc_reload_counter()   noexcept { reloadCounter_++; }
    int    get_speed()          const noexcept {return speed_;}
    int    get_health()         const noexcept {return health_;}
    int    get_ammunition()     const noexcept {return ammunition_;}
    size_t get_max_health()     const noexcept {return max_health_;}
    size_t get_max_ammunition() const noexcept {return max_ammunition_;}

    TFT_eSPI& get_tft() const noexcept {return tft_;};
}; 
```
</details>

* Добавление окружения и взаимодействия объектов с ним
* Реализация физических механик, которых нет в оригинале игры Battle City


Текущий прогресс
![alt text](img/result.png)