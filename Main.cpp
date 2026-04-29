#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include <random>
#include <algorithm>
#include <iostream>
#include <string>
#include <sstream>
#include <cmath>
#include <iomanip>

// ========== CONSTANTS & GLOBAL SETTINGS ==========
const int WINDOW_WIDTH = 1024;
const int WINDOW_HEIGHT = 768;
const std::string GAME_TITLE = "Sky Defender Pro";

// Color Palette
namespace Colors {
    const sf::Color DAY_SKY(135, 206, 235, 255);      // Light blue
    const sf::Color DUSK_SKY(255, 140, 0, 255);       // Orange
    const sf::Color NIGHT_SKY(10, 10, 40, 255);       // Dark blue
    const sf::Color DAWN_SKY(255, 200, 100, 255);     // Yellow-orange
    
    const sf::Color BULLET_YELLOW(255, 215, 0, 255);
    const sf::Color BULLET_ORANGE(255, 140, 0, 255);
    const sf::Color BULLET_BLUE(0, 191, 255, 255);
    const sf::Color BULLET_PURPLE(138, 43, 226, 255);
    
    const sf::Color UI_WHITE(255, 255, 255, 255);
    const sf::Color UI_GOLD(255, 215, 0, 255);
    const sf::Color UI_GREEN(50, 205, 50, 255);
    const sf::Color UI_RED(220, 20, 60, 255);
    const sf::Color UI_BLUE(30, 144, 255, 255);
    const sf::Color UI_SHADOW(0, 0, 0, 180);
}

// ========== ENUM DECLARATIONS ==========
enum PowerUpType {
    POWER_NONE,
    POWER_MULTI_SHOT,
    POWER_RAPID_FIRE,
    POWER_BULLET_SPEED,
    POWER_SHIELD,
    POWER_BURST
};

enum ObstacleType {
    CLOUD,
    BIRD,
    ENEMY_PLANE
};

enum BulletType {
    NORMAL,
    RAPID,
    BURST,
    POWERED
};

// ========== UTILITY CLASSES ==========
class Random {
public:
    static int getInt(int min, int max) {
        static std::mt19937 gen(std::random_device{}());
        std::uniform_int_distribution<> dist(min, max);
        return dist(gen);
    }
    
    static float getFloat(float min, float max) {
        static std::mt19937 gen(std::random_device{}());
        std::uniform_real_distribution<float> dist(min, max);
        return dist(gen);
    }
    
    static bool chance(int percent) {
        return getInt(1, 100) <= percent;
    }
};

// ========== PARTICLE SYSTEM ==========
class Particle {
public:
    Particle(sf::Vector2f position, sf::Color color, float speed, float lifetime)
        : position(position), velocity(Random::getFloat(-speed, speed), Random::getFloat(-speed, speed)),
          color(color), lifetime(lifetime), maxLifetime(lifetime) {
        shape.setRadius(Random::getFloat(2.f, 5.f));
        shape.setFillColor(color);
        shape.setOrigin(shape.getRadius(), shape.getRadius());
        shape.setPosition(position);
    }
    
    bool update(float dt) {
        lifetime -= dt;
        if (lifetime <= 0) return false;
        
        position += velocity * dt;
        velocity *= 0.95f; // Damping
        shape.setPosition(position);
        
        // Fade out
        float alpha = (lifetime / maxLifetime) * 255.f;
        color.a = static_cast<sf::Uint8>(alpha);
        shape.setFillColor(color);
        
        return true;
    }
    
    void draw(sf::RenderWindow& window) const {
        window.draw(shape);
    }
    
private:
    sf::Vector2f position;
    sf::Vector2f velocity;
    sf::CircleShape shape;
    sf::Color color;
    float lifetime;
    float maxLifetime;
};

class ParticleSystem {
public:
    void addParticle(const Particle& particle) {
        particles.push_back(particle);
    }
    
    void addExplosion(sf::Vector2f position, sf::Color color, int count = 20) {
        for (int i = 0; i < count; i++) {
            particles.emplace_back(position, color, 
                                  Random::getFloat(50.f, 200.f), 
                                  Random::getFloat(0.5f, 1.5f));
        }
    }
    
    void update(float dt) {
        for (auto it = particles.begin(); it != particles.end();) {
            if (!it->update(dt)) {
                it = particles.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    void draw(sf::RenderWindow& window) const {
        for (const auto& particle : particles) {
            particle.draw(window);
        }
    }
    
    void clear() {
        particles.clear();
    }
    
private:
    std::vector<Particle> particles;
};

// ========== ENVIRONMENT ==========
class Environment {
public:
    Environment() : timeOfDay(0.f), dayDuration(120.f), isNight(false) {
        // Create hills
        createHills();
        
        // Create stars
        createStars();
        
        // Create sun/moon
        sun.setRadius(40.f);
        sun.setFillColor(sf::Color(255, 255, 200));
        sun.setOrigin(40.f, 40.f);
        
        moon.setRadius(30.f);
        moon.setFillColor(sf::Color(200, 200, 255));
        moon.setOrigin(30.f, 30.f);
    }
    
    void update(float dt) {
        timeOfDay += dt;
        if (timeOfDay > dayDuration) {
            timeOfDay = 0.f;
            isNight = !isNight;
        }
        
        // Update sun/moon position
        float progress = timeOfDay / dayDuration;
        float x = WINDOW_WIDTH * progress;
        float y = std::sin(progress * 3.14159f) * 300.f + 100.f;
        
        if (!isNight) {
            sun.setPosition(x, y);
        } else {
            moon.setPosition(x, y);
        }
        
        // Twinkle stars at night
        if (isNight) {
            for (auto& star : stars) {
                float alpha = 150 + 100 * std::sin(timeOfDay * 3.f + star.getPosition().x * 0.01f);
                star.setFillColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(alpha)));
            }
        }
    }
    
    void draw(sf::RenderWindow& window) {
        // Draw sky with day/night transition
        sf::Color skyColor = getSkyColor();
        window.clear(skyColor);
        
        // Draw hills
        for (const auto& hill : hills) {
            window.draw(hill);
        }
        
        // Draw stars (only at night)
        if (isNight) {
            for (const auto& star : stars) {
                window.draw(star);
            }
        }
        
        // Draw sun or moon
        if (!isNight) {
            window.draw(sun);
        } else {
            window.draw(moon);
        }
    }
    
    sf::Color getSkyColor() const {
        float progress = timeOfDay / dayDuration;
        
        if (!isNight) {
            // Day to dusk transition
            if (progress < 0.25f) {
                return lerpColor(Colors::DAY_SKY, Colors::DAWN_SKY, progress * 4.f);
            } else if (progress < 0.5f) {
                return lerpColor(Colors::DAWN_SKY, Colors::DAY_SKY, (progress - 0.25f) * 4.f);
            } else if (progress < 0.75f) {
                return lerpColor(Colors::DAY_SKY, Colors::DUSK_SKY, (progress - 0.5f) * 4.f);
            } else {
                return lerpColor(Colors::DUSK_SKY, Colors::NIGHT_SKY, (progress - 0.75f) * 4.f);
            }
        } else {
            // Night to dawn transition
            return Colors::NIGHT_SKY;
        }
    }
    
private:
    void createHills() {
        for (int i = 0; i < 3; i++) {
            sf::ConvexShape hill;
            hill.setPointCount(6);
            
            float yBase = WINDOW_HEIGHT - 100.f * (i + 1);
            float xStep = WINDOW_WIDTH / 5.f;
            
            hill.setPoint(0, sf::Vector2f(0, WINDOW_HEIGHT));
            hill.setPoint(1, sf::Vector2f(0, yBase + 50));
            hill.setPoint(2, sf::Vector2f(xStep * 1, yBase));
            hill.setPoint(3, sf::Vector2f(xStep * 2, yBase + 30));
            hill.setPoint(4, sf::Vector2f(xStep * 3, yBase - 20));
            hill.setPoint(5, sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
            
            // Different shades of green for depth
            int green = 100 + i * 30;
            hill.setFillColor(sf::Color(0, green, 0));
            hill.setOutlineThickness(2.f);
            hill.setOutlineColor(sf::Color(0, green + 20, 0));
            
            hills.push_back(hill);
        }
    }
    
    void createStars() {
        for (int i = 0; i < 200; i++) {
            sf::CircleShape star(Random::getFloat(1.f, 3.f));
            star.setFillColor(sf::Color::White);
            star.setPosition(
                Random::getFloat(0.f, WINDOW_WIDTH),
                Random::getFloat(0.f, WINDOW_HEIGHT * 0.7f)
            );
            stars.push_back(star);
        }
    }
    
    sf::Color lerpColor(sf::Color a, sf::Color b, float t) const {
        t = std::max(0.f, std::min(1.f, t));
        return sf::Color(
            static_cast<sf::Uint8>(a.r + (b.r - a.r) * t),
            static_cast<sf::Uint8>(a.g + (b.g - a.g) * t),
            static_cast<sf::Uint8>(a.b + (b.b - a.b) * t)
        );
    }
    
    std::vector<sf::ConvexShape> hills;
    std::vector<sf::CircleShape> stars;
    sf::CircleShape sun;
    sf::CircleShape moon;
    float timeOfDay;
    float dayDuration;
    bool isNight;
};

// ========== LEVEL MANAGER ==========
class LevelManager {
public:
    LevelManager() : currentLevel(1), score(0), scoreToNextLevel(1000), spawnMultiplier(1.0f) {}
    
    void update(int newScore) {
        score = newScore;
        
        // Check for level up
        if (score >= scoreToNextLevel) {
            currentLevel++;
            scoreToNextLevel = currentLevel * 1500;
            spawnMultiplier = 1.0f + (currentLevel - 1) * 0.2f;
        }
    }
    
    int getCurrentLevel() const { return currentLevel; }
    float getSpawnMultiplier() const { return spawnMultiplier; }
    float getSpeedMultiplier() const { return 1.0f + (currentLevel - 1) * 0.15f; }
    
    bool shouldSpawnEnemy() const {
        return Random::chance(5 + currentLevel * 2);
    }
    
    bool shouldSpawnBird() const {
        return currentLevel >= 2 && Random::chance(10 + (currentLevel - 2) * 3);
    }
    
    float getPowerUpSpawnInterval() const {
        if (currentLevel <= 3) {
            return Random::getFloat(2.0f, 3.0f);
        } else if (currentLevel <= 6) {
            return Random::getFloat(3.0f, 4.0f);
        } else {
            return Random::getFloat(4.0f, 5.0f);
        }
    }
    
    PowerUpType getRandomPowerUpType() const {
        int roll = Random::getInt(1, 100);
        
        if (roll <= 40) {
            return POWER_SHIELD;
        } else if (roll <= 70) {
            return POWER_RAPID_FIRE;
        } else if (roll <= 85) {
            return POWER_BULLET_SPEED;
        } else if (roll <= 95) {
            return POWER_MULTI_SHOT;
        } else {
            return POWER_BURST;
        }
    }
    
private:
    int currentLevel;
    int score;
    int scoreToNextLevel;
    float spawnMultiplier;
};

// ========== POWER-UP SYSTEM ==========
class PowerUp {
public:
    PowerUp(sf::Vector2f position, PowerUpType type)
        : position(position), type(type), active(true) {
        
        shape.setRadius(20.f);
        shape.setOrigin(20.f, 20.f);
        shape.setPosition(position);
        
        switch(type) {
            case POWER_MULTI_SHOT:
                shape.setFillColor(sf::Color(138, 43, 226, 220));
                break;
            case POWER_RAPID_FIRE:
                shape.setFillColor(sf::Color(255, 69, 0, 220));
                break;
            case POWER_BULLET_SPEED:
                shape.setFillColor(sf::Color(0, 191, 255, 220));
                break;
            case POWER_SHIELD:
                shape.setFillColor(sf::Color(50, 205, 50, 220));
                break;
            case POWER_BURST:
                shape.setFillColor(sf::Color(255, 215, 0, 220));
                break;
            default:
                shape.setFillColor(sf::Color::White);
        }
        
        shape.setOutlineThickness(3.f);
        shape.setOutlineColor(sf::Color::White);
        originalRadius = 20.f;
    }
    
    void update(float dt) {
        position.x -= 100.f * dt;
        shape.setPosition(position);
        
        float offset = std::sin(animationTime * 3.f) * 5.f;
        shape.move(0.f, offset * dt);
        
        float pulse = std::sin(animationTime * 5.f) * 4.f;
        shape.setRadius(originalRadius + pulse);
        shape.setOrigin(originalRadius + pulse, originalRadius + pulse);
        
        animationTime += dt;
        
        if (position.x < -30.f) {
            active = false;
        }
    }
    
    void draw(sf::RenderWindow& window) const {
        window.draw(shape);
        
        sf::CircleShape icon(8.f);
        icon.setFillColor(sf::Color::White);
        icon.setOrigin(8.f, 8.f);
        icon.setPosition(position);
        window.draw(icon);
    }
    
    bool isActive() const { return active; }
    sf::FloatRect getBounds() const { return shape.getGlobalBounds(); }
    PowerUpType getType() const { return type; }
    
private:
    sf::Vector2f position;
    sf::CircleShape shape;
    PowerUpType type;
    bool active;
    float animationTime = 0.f;
    float originalRadius;
};

// ========== BULLET SYSTEM ==========
class Bullet {
public:
    Bullet(sf::Vector2f position, float angle = 0.f, BulletType type = NORMAL, float speed = 600.f)
        : position(position), velocity(std::cos(angle) * speed, std::sin(angle) * speed),
          type(type), active(true) {
        
        shape.setSize(sf::Vector2f(20.f, 6.f));
        shape.setOrigin(10.f, 3.f);
        shape.setPosition(position);
        shape.setRotation(angle * 180.f / 3.14159f);
        
        switch(type) {
            case NORMAL:
                shape.setFillColor(Colors::BULLET_YELLOW);
                break;
            case RAPID:
                shape.setFillColor(Colors::BULLET_ORANGE);
                break;
            case BURST:
                shape.setFillColor(Colors::BULLET_BLUE);
                break;
            case POWERED:
                shape.setFillColor(Colors::BULLET_PURPLE);
                break;
        }
        
        glow.setRadius(8.f);
        glow.setOrigin(8.f, 8.f);
        glow.setPosition(position);
        
        sf::Color glowColor = shape.getFillColor();
        glowColor.a = 100;
        glow.setFillColor(glowColor);
    }
    
    void update(float dt) {
        position += velocity * dt;
        shape.setPosition(position);
        glow.setPosition(position);
        
        if (position.x < -50.f || position.x > WINDOW_WIDTH + 50.f ||
            position.y < -50.f || position.y > WINDOW_HEIGHT + 50.f) {
            active = false;
        }
    }
    
    void draw(sf::RenderWindow& window) const {
        window.draw(glow);
        window.draw(shape);
    }
    
    bool isActive() const { return active; }
    sf::FloatRect getBounds() const { return shape.getGlobalBounds(); }
    BulletType getType() const { return type; }
    
private:
    sf::Vector2f position;
    sf::Vector2f velocity;
    sf::RectangleShape shape;
    sf::CircleShape glow;
    BulletType type;
    bool active;
};

// ========== OBSTACLE BASE CLASS ==========
class Obstacle {
public:
    Obstacle(sf::Vector2f position, ObstacleType type, float speed)
        : position(position), type(type), speed(speed), active(true) {}
    
    virtual ~Obstacle() = default;
    
    virtual void update(float dt) {
        position.x -= speed * dt;
        if (position.x < -100.f) {
            active = false;
        }
    }
    
    virtual void draw(sf::RenderWindow& window) = 0;
    virtual sf::FloatRect getBounds() const = 0;
    
    bool isActive() const { return active; }
    ObstacleType getType() const { return type; }
    sf::Vector2f getPosition() const { return position; }
    
    int getScoreValue() const {
        switch(type) {
            case CLOUD: return 10;
            case BIRD: return 25;
            case ENEMY_PLANE: return 50;
            default: return 0;
        }
    }
    
protected:
    sf::Vector2f position;
    ObstacleType type;
    float speed;
    bool active;
};

// ========== CLOUD OBSTACLE ==========
class CloudObstacle : public Obstacle {
public:
    CloudObstacle(sf::Vector2f position, float speed)
        : Obstacle(position, CLOUD, speed) {
        
        int circles = Random::getInt(3, 5);
        for (int i = 0; i < circles; i++) {
            sf::CircleShape circle(Random::getFloat(15.f, 25.f));
            circle.setFillColor(sf::Color(255, 255, 255, 180));
            circle.setPosition(
                position.x + Random::getFloat(-20.f, 20.f),
                position.y + Random::getFloat(-10.f, 10.f)
            );
            cloudParts.push_back(circle);
        }
    }
    
    void update(float dt) override {
        Obstacle::update(dt);
        for (auto& circle : cloudParts) {
            circle.move(-speed * dt, 0.f);
        }
    }
    
    void draw(sf::RenderWindow& window) override {
        for (const auto& circle : cloudParts) {
            window.draw(circle);
        }
    }
    
    sf::FloatRect getBounds() const override {
        if (cloudParts.empty()) return sf::FloatRect();
        sf::FloatRect bounds = cloudParts[0].getGlobalBounds();
        for (size_t i = 1; i < cloudParts.size(); i++) {
            sf::FloatRect partBounds = cloudParts[i].getGlobalBounds();
            bounds.left = std::min(bounds.left, partBounds.left);
            bounds.top = std::min(bounds.top, partBounds.top);
            bounds.width = std::max(bounds.width, partBounds.left + partBounds.width - bounds.left);
            bounds.height = std::max(bounds.height, partBounds.top + partBounds.height - bounds.top);
        }
        return bounds;
    }
    
private:
    std::vector<sf::CircleShape> cloudParts;
};

// ========== BIRD OBSTACLE ==========
class BirdObstacle : public Obstacle {
public:
    BirdObstacle(sf::Vector2f position, float speed)
        : Obstacle(position, BIRD, speed * 1.5f), animationTime(0.f) {
        
        shape.setPointCount(3);
        shape.setPoint(0, sf::Vector2f(0, 0));
        shape.setPoint(1, sf::Vector2f(-20, -10));
        shape.setPoint(2, sf::Vector2f(-20, 10));
        
        shape.setFillColor(sf::Color(160, 82, 45));
        shape.setOutlineThickness(2.f);
        shape.setOutlineColor(sf::Color(139, 69, 19));
        
        shape.setPosition(position);
        shape.setRotation(180.f);
    }
    
    void update(float dt) override {
        Obstacle::update(dt);
        
        animationTime += dt * 10.f;
        float flap = std::sin(animationTime) * 5.f;
        shape.setPosition(position.x, position.y + flap);
        
        position.y += std::sin(animationTime * 0.5f) * 20.f * dt;
    }
    
    void draw(sf::RenderWindow& window) override {
        window.draw(shape);
    }
    
    sf::FloatRect getBounds() const override {
        return shape.getGlobalBounds();
    }
    
private:
    sf::ConvexShape shape;
    float animationTime;
};

// ========== ENEMY PLANE ==========
class EnemyPlane : public Obstacle {
public:
    EnemyPlane(sf::Vector2f position, float speed)
        : Obstacle(position, ENEMY_PLANE, speed * 0.8f),
          shootCooldown(Random::getFloat(1.f, 3.f)),
          shootTimer(0.f) {
        
        shape.setPointCount(7);
        shape.setPoint(0, sf::Vector2f(0, 0));
        shape.setPoint(1, sf::Vector2f(-20, -10));
        shape.setPoint(2, sf::Vector2f(-40, -10));
        shape.setPoint(3, sf::Vector2f(-50, 0));
        shape.setPoint(4, sf::Vector2f(-40, 10));
        shape.setPoint(5, sf::Vector2f(-20, 10));
        shape.setPoint(6, sf::Vector2f(0, 0));
        
        shape.setFillColor(sf::Color(220, 20, 60));
        shape.setOutlineThickness(2.f);
        shape.setOutlineColor(sf::Color::White);
        
        shape.setPosition(position);
        shape.setRotation(180.f);
    }
    
    void update(float dt) override {
        Obstacle::update(dt);
        shape.setPosition(position);
        
        position.y += std::sin(animationTime) * 50.f * dt;
        animationTime += dt;
        
        shootTimer += dt;
    }
    
    void draw(sf::RenderWindow& window) override {
        window.draw(shape);
        
        sf::CircleShape engine(5.f);
        engine.setFillColor(sf::Color(255, 100, 0, 150));
        engine.setOrigin(5.f, 5.f);
        engine.setPosition(position + sf::Vector2f(15.f, 0.f));
        window.draw(engine);
    }
    
    sf::FloatRect getBounds() const override {
        return shape.getGlobalBounds();
    }
    
    bool canShoot() const {
        return shootTimer >= shootCooldown;
    }
    
    void resetShootTimer() {
        shootTimer = 0.f;
        shootCooldown = Random::getFloat(2.f, 4.f);
    }
    
    Bullet shoot() {
        return Bullet(position + sf::Vector2f(-20.f, 0.f), 3.14159f, NORMAL, 300.f);
    }
    
private:
    sf::ConvexShape shape;
    float shootCooldown;
    float shootTimer;
    float animationTime = 0.f;
};

// ========== PLAYER PLANE ==========
class PlayerPlane {
public:
    PlayerPlane() 
        : position(100.f, WINDOW_HEIGHT / 2.f),
          velocity(0.f, 0.f),
          acceleration(800.f),
          maxSpeed(400.f),
          friction(0.9f),
          shootCooldown(0.2f),
          shootTimer(0.f),
          powerUpTimer(0.f),
          currentPower(POWER_NONE),
          shieldActive(false),
          shieldTime(0.f),
          burstCount(0),
          burstDelay(0.f) {
        
        shape.setPointCount(7);
        shape.setPoint(0, sf::Vector2f(40.f, 0.f));
        shape.setPoint(1, sf::Vector2f(20.f, -15.f));
        shape.setPoint(2, sf::Vector2f(-20.f, -15.f));
        shape.setPoint(3, sf::Vector2f(-30.f, 0.f));
        shape.setPoint(4, sf::Vector2f(-20.f, 15.f));
        shape.setPoint(5, sf::Vector2f(20.f, 15.f));
        shape.setPoint(6, sf::Vector2f(40.f, 0.f));
        
        shape.setFillColor(sf::Color(30, 30, 40));
        shape.setOutlineThickness(3.f);
        shape.setOutlineColor(sf::Color(100, 100, 150));
        shape.setPosition(position);
        shape.setOrigin(5.f, 0.f);
        
        cockpit.setRadius(10.f);
        cockpit.setFillColor(sf::Color(173, 216, 230));
        cockpit.setOrigin(10.f, 10.f);
        
        shield.setRadius(35.f);
        shield.setFillColor(sf::Color(0, 191, 255, 100));
        shield.setOutlineThickness(2.f);
        shield.setOutlineColor(sf::Color(0, 191, 255, 150));
        shield.setOrigin(35.f, 35.f);
    }
    
    void update(float dt) {
        sf::Vector2f input(0.f, 0.f);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) || 
            sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
            input.y -= 1.f;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) || 
            sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
            input.y += 1.f;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || 
            sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
            input.x -= 1.f;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) || 
            sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
            input.x += 1.f;
        }
        
        if (input.x != 0.f && input.y != 0.f) {
            input *= 0.7071f;
        }
        
        velocity += input * acceleration * dt;
        velocity *= friction;
        
        float speed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
        if (speed > maxSpeed) {
            velocity = velocity * (maxSpeed / speed);
        }
        
        position += velocity * dt;
        
        position.x = std::max(20.f, std::min(position.x, static_cast<float>(WINDOW_WIDTH - 20)));
        position.y = std::max(30.f, std::min(position.y, static_cast<float>(WINDOW_HEIGHT - 30)));
        
        shape.setPosition(position);
        cockpit.setPosition(position + sf::Vector2f(15.f, 0.f));
        if (shieldActive) {
            shield.setPosition(position);
        }
        
        shootTimer += dt;
        if (powerUpTimer > 0.f) {
            powerUpTimer -= dt;
            if (powerUpTimer <= 0.f) {
                currentPower = POWER_NONE;
                burstCount = 0;
            }
        }
        
        if (shieldActive) {
            shieldTime -= dt;
            if (shieldTime <= 0.f) {
                shieldActive = false;
            }
        }
        
        if (burstCount > 0 && burstDelay > 0.f) {
            burstDelay -= dt;
        }
    }
    
    std::vector<Bullet> shoot() {
        std::vector<Bullet> bullets;
        
        float currentCooldown = shootCooldown;
        if (currentPower == POWER_RAPID_FIRE) {
            currentCooldown *= 0.3f;
        }
        
        if (shootTimer >= currentCooldown || (burstCount > 0 && burstDelay <= 0.f)) {
            if (burstCount > 0) {
                burstCount--;
                burstDelay = 0.05f;
            } else {
                shootTimer = 0.f;
            }
            
            float bulletSpeed = 600.f;
            if (currentPower == POWER_BULLET_SPEED) {
                bulletSpeed *= 1.5f;
            }
            
            switch(currentPower) {
                case POWER_MULTI_SHOT:
                    bullets.push_back(Bullet(position + sf::Vector2f(40.f, 0.f), 0.f, 
                                           POWERED, bulletSpeed));
                    bullets.push_back(Bullet(position + sf::Vector2f(40.f, 0.f), -0.2f, 
                                           POWERED, bulletSpeed));
                    bullets.push_back(Bullet(position + sf::Vector2f(40.f, 0.f), 0.2f, 
                                           POWERED, bulletSpeed));
                    break;
                    
                case POWER_BURST:
                    if (burstCount == 0) {
                        burstCount = 5;
                        burstDelay = 0.f;
                    }
                    bullets.push_back(Bullet(position + sf::Vector2f(40.f, 0.f), 0.f,
                                           BURST, bulletSpeed));
                    break;
                    
                default:
                    BulletType bulletType = (currentPower == POWER_RAPID_FIRE) ? 
                                                    RAPID : NORMAL;
                    bullets.push_back(Bullet(position + sf::Vector2f(40.f, 0.f), 0.f,
                                           bulletType, bulletSpeed));
                    break;
            }
        }
        
        return bullets;
    }
    
    void activatePowerUp(PowerUpType type) {
        currentPower = type;
        
        switch(type) {
            case POWER_MULTI_SHOT:
            case POWER_RAPID_FIRE:
            case POWER_BULLET_SPEED:
                powerUpTimer = 10.f;
                break;
            case POWER_SHIELD:
                shieldActive = true;
                shieldTime = 8.f;
                break;
            case POWER_BURST:
                powerUpTimer = 5.f;
                break;
            default:
                break;
        }
    }
    
    void draw(sf::RenderWindow& window) {
        float speed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
        if (speed > 50.f) {
            sf::ConvexShape trail;
            trail.setPointCount(4);
            trail.setPoint(0, sf::Vector2f(-25.f, -5.f));
            trail.setPoint(1, sf::Vector2f(-40.f, 0.f));
            trail.setPoint(2, sf::Vector2f(-25.f, 5.f));
            trail.setPoint(3, sf::Vector2f(-25.f, -5.f));
            
            int alpha = static_cast<int>(150 * (speed / maxSpeed));
            trail.setFillColor(sf::Color(255, 100, 0, alpha));
            trail.setPosition(position);
            window.draw(trail);
        }
        
        window.draw(shape);
        window.draw(cockpit);
        
        if (shieldActive) {
            window.draw(shield);
        }
        
        sf::CircleShape engine(6.f);
        engine.setFillColor(sf::Color(255, 100, 0, 200));
        engine.setOrigin(6.f, 6.f);
        engine.setPosition(position + sf::Vector2f(-25.f, 0.f));
        window.draw(engine);
    }
    
    sf::FloatRect getBounds() const {
        return shape.getGlobalBounds();
    }
    
    sf::Vector2f getPosition() const { return position; }
    bool hasShield() const { return shieldActive; }
    PowerUpType getCurrentPower() const { return currentPower; }
    float getPowerUpTime() const { return powerUpTimer; }
    float getShieldTime() const { return shieldTime; }
    
private:
    sf::Vector2f position;
    sf::Vector2f velocity;
    sf::ConvexShape shape;
    sf::CircleShape cockpit;
    sf::CircleShape shield;
    
    float acceleration;
    float maxSpeed;
    float friction;
    
    float shootCooldown;
    float shootTimer;
    
    float powerUpTimer;
    PowerUpType currentPower;
    
    bool shieldActive;
    float shieldTime;
    
    int burstCount;
    float burstDelay;
};

// ========== UI MANAGER WITH RELIABLE FONT ==========
class UIManager {
public:
    UIManager() {
        // Try multiple font paths
        if (!font.loadFromFile("arial.ttf")) {
            if (!font.loadFromFile("/System/Library/Fonts/Supplemental/Arial.ttf")) {
                if (!font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")) {
                    std::cerr << "Warning: Could not load font. Text may not display.\n";
                    // Font will use default (which should work in SFML)
                }
            }
        }
        
        setupTexts();
    }
    
    void update(int score, int level, int lives, const PlayerPlane& player) {
        scoreText.setString("SCORE: " + std::to_string(score));
        levelText.setString("LEVEL: " + std::to_string(level));
        livesText.setString("LIVES: " + std::to_string(lives));
        
        updatePowerUpText(player);
    }
    
    void drawGameUI(sf::RenderWindow& window) {
        window.draw(scoreText);
        window.draw(levelText);
        window.draw(livesText);
        window.draw(powerUpText);
    }
    
    void drawMenu(sf::RenderWindow& window) {
        // Draw title with shadow
        sf::Text shadowTitle = titleText;
        shadowTitle.setFillColor(Colors::UI_SHADOW);
        shadowTitle.move(4, 4);
        window.draw(shadowTitle);
        window.draw(titleText);
        
        // Draw subtitle with shadow
        sf::Text shadowSubtitle = subtitleText;
        shadowSubtitle.setFillColor(Colors::UI_SHADOW);
        shadowSubtitle.move(4, 4);
        window.draw(shadowSubtitle);
        window.draw(subtitleText);
        
        // Draw instructions
        window.draw(controlsTitle);
        window.draw(control1);
        window.draw(control2);
        window.draw(control3);
        
        // Draw start instruction
        window.draw(startInstruction);
        window.draw(powerupInfo);
    }
    
    void drawGameOver(sf::RenderWindow& window, int score, int level) {
        // Draw game over with shadow
        sf::Text shadowGameOver = gameOverText;
        shadowGameOver.setFillColor(Colors::UI_SHADOW);
        shadowGameOver.move(4, 4);
        window.draw(shadowGameOver);
        window.draw(gameOverText);
        
        finalScoreText.setString("Final Score: " + std::to_string(score));
        finalLevelText.setString("Level Reached: " + std::to_string(level));
        
        centerText(finalScoreText, WINDOW_HEIGHT / 2.f - 20.f);
        centerText(finalLevelText, WINDOW_HEIGHT / 2.f + 30.f);
        
        window.draw(finalScoreText);
        window.draw(finalLevelText);
        window.draw(restartText);
        window.draw(menuText);
    }
    
private:
    sf::Font font;
    
    // Menu texts
    sf::Text titleText;
    sf::Text subtitleText;
    sf::Text controlsTitle;
    sf::Text control1;
    sf::Text control2;
    sf::Text control3;
    sf::Text startInstruction;
    sf::Text powerupInfo;
    
    // Game UI texts
    sf::Text scoreText;
    sf::Text levelText;
    sf::Text livesText;
    sf::Text powerUpText;
    
    // Game over texts
    sf::Text gameOverText;
    sf::Text finalScoreText;
    sf::Text finalLevelText;
    sf::Text restartText;
    sf::Text menuText;
    
    void setupTexts() {
        // Menu setup
        titleText.setFont(font);
        titleText.setString("SKY DEFENDER");
        titleText.setCharacterSize(72);
        titleText.setFillColor(Colors::UI_GOLD);
        centerText(titleText, 150.f);
        
        subtitleText.setFont(font);
        subtitleText.setString("Arcade Shooter");
        subtitleText.setCharacterSize(36);
        subtitleText.setFillColor(Colors::UI_WHITE);
        centerText(subtitleText, 230.f);
        
        controlsTitle.setFont(font);
        controlsTitle.setString("CONTROLS:");
        controlsTitle.setCharacterSize(28);
        controlsTitle.setFillColor(Colors::UI_GREEN);
        controlsTitle.setPosition(WINDOW_WIDTH / 2.f - 200.f, 300.f);
        
        control1.setFont(font);
        control1.setString("WASD or Arrow Keys - Move Plane");
        control1.setCharacterSize(22);
        control1.setFillColor(Colors::UI_WHITE);
        control1.setPosition(WINDOW_WIDTH / 2.f - 200.f, 340.f);
        
        control2.setFont(font);
        control2.setString("SPACE - Shoot Bullets");
        control2.setCharacterSize(22);
        control2.setFillColor(Colors::UI_WHITE);
        control2.setPosition(WINDOW_WIDTH / 2.f - 200.f, 370.f);
        
        control3.setFont(font);
        control3.setString("ESC - Pause/Return to Menu");
        control3.setCharacterSize(22);
        control3.setFillColor(Colors::UI_WHITE);
        control3.setPosition(WINDOW_WIDTH / 2.f - 200.f, 400.f);
        
        startInstruction.setFont(font);
        startInstruction.setString("PRESS ENTER TO START");
        startInstruction.setCharacterSize(32);
        startInstruction.setFillColor(Colors::UI_BLUE);
        centerText(startInstruction, 480.f);
        
        powerupInfo.setFont(font);
        powerupInfo.setString("Power-ups spawn every 2-3 seconds!");
        powerupInfo.setCharacterSize(20);
        powerupInfo.setFillColor(Colors::UI_GOLD);
        centerText(powerupInfo, 530.f);
        
        // Game UI setup
        scoreText.setFont(font);
        scoreText.setCharacterSize(24);
        scoreText.setFillColor(Colors::UI_WHITE);
        scoreText.setPosition(20.f, 20.f);
        
        levelText.setFont(font);
        levelText.setCharacterSize(24);
        levelText.setFillColor(Colors::UI_GOLD);
        levelText.setPosition(WINDOW_WIDTH - 150.f, 20.f);
        
        livesText.setFont(font);
        livesText.setCharacterSize(24);
        livesText.setFillColor(Colors::UI_GREEN);
        livesText.setPosition(WINDOW_WIDTH / 2.f - 50.f, 20.f);
        
        powerUpText.setFont(font);
        powerUpText.setCharacterSize(18);
        powerUpText.setFillColor(Colors::UI_BLUE);
        powerUpText.setPosition(20.f, 60.f);
        
        // Game over setup
        gameOverText.setFont(font);
        gameOverText.setString("GAME OVER");
        gameOverText.setCharacterSize(72);
        gameOverText.setFillColor(Colors::UI_RED);
        centerText(gameOverText, 200.f);
        
        finalScoreText.setFont(font);
        finalScoreText.setCharacterSize(42);
        finalScoreText.setFillColor(Colors::UI_GOLD);
        
        finalLevelText.setFont(font);
        finalLevelText.setCharacterSize(36);
        finalLevelText.setFillColor(Colors::UI_WHITE);
        
        restartText.setFont(font);
        restartText.setString("Press R to Restart");
        restartText.setCharacterSize(28);
        restartText.setFillColor(Colors::UI_GREEN);
        centerText(restartText, 500.f);
        
        menuText.setFont(font);
        menuText.setString("Press ESC for Main Menu");
        menuText.setCharacterSize(24);
        menuText.setFillColor(Colors::UI_WHITE);
        centerText(menuText, 550.f);
    }
    
    void centerText(sf::Text& text, float y) {
        sf::FloatRect bounds = text.getLocalBounds();
        text.setOrigin(bounds.width / 2.f, bounds.height / 2.f);
        text.setPosition(WINDOW_WIDTH / 2.f, y);
    }
    
    void updatePowerUpText(const PlayerPlane& player) {
        PowerUpType power = player.getCurrentPower();
        float time = player.getPowerUpTime();
        float shieldTime = player.getShieldTime();
        
        std::string text = "Power: ";
        
        if (power != POWER_NONE && time > 0) {
            switch(power) {
                case POWER_MULTI_SHOT:
                    text += "Multi-shot ";
                    break;
                case POWER_RAPID_FIRE:
                    text += "Rapid Fire ";
                    break;
                case POWER_BULLET_SPEED:
                    text += "Bullet Speed ";
                    break;
                case POWER_BURST:
                    text += "Burst Fire ";
                    break;
                default:
                    text += "None ";
            }
            text += "(" + std::to_string(static_cast<int>(time)) + "s)";
        } else if (player.hasShield()) {
            text += "Shield (" + std::to_string(static_cast<int>(shieldTime)) + "s)";
        } else {
            text += "None";
        }
        
        powerUpText.setString(text);
    }
};

// ========== MAIN GAME CLASS ==========
class Game {
public:
    Game() : window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), GAME_TITLE),
             state(MENU),
             score(0),
             lives(3),
             spawnTimer(0.f),
             enemyBulletTimer(0.f),
             powerUpTimer(0.f) {
        
        window.setFramerateLimit(60);
        window.setVerticalSyncEnabled(true);
        
        // Set initial power-up timer
        powerUpTimer = levelManager.getPowerUpSpawnInterval();
    }
    
    void run() {
        sf::Clock clock;
        
        while (window.isOpen()) {
            float dt = clock.restart().asSeconds();
            
            handleEvents();
            
            switch(state) {
                case MENU:
                    updateMenu(dt);
                    break;
                case PLAYING:
                    updateGame(dt);
                    break;
                case GAME_OVER:
                    updateGameOver(dt);
                    break;
            }
            
            render();
        }
    }
    
private:
    enum GameState { MENU, PLAYING, GAME_OVER };
    
    sf::RenderWindow window;
    GameState state;
    
    Environment environment;
    LevelManager levelManager;
    UIManager uiManager;
    ParticleSystem particles;
    
    PlayerPlane player;
    std::vector<std::unique_ptr<Obstacle>> obstacles;
    std::vector<Bullet> playerBullets;
    std::vector<Bullet> enemyBullets;
    std::vector<std::unique_ptr<PowerUp>> powerUps;
    
    int score;
    int lives;
    float spawnTimer;
    float enemyBulletTimer;
    float powerUpTimer;
    
    void handleEvents() {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            
            if (event.type == sf::Event::KeyPressed) {
                handleKeyPress(event.key.code);
            }
        }
    }
    
    void handleKeyPress(sf::Keyboard::Key key) {
        switch(state) {
            case MENU:
                if (key == sf::Keyboard::Enter) {
                    startGame();
                }
                else if (key == sf::Keyboard::Escape) {
                    window.close();
                }
                break;
                
            case PLAYING:
                if (key == sf::Keyboard::Space) {
                    auto newBullets = player.shoot();
                    playerBullets.insert(playerBullets.end(), newBullets.begin(), newBullets.end());
                }
                else if (key == sf::Keyboard::Escape) {
                    state = MENU;
                }
                break;
                
            case GAME_OVER:
                if (key == sf::Keyboard::R) {
                    startGame();
                }
                else if (key == sf::Keyboard::Escape) {
                    state = MENU;
                }
                break;
        }
    }
    
    void startGame() {
        state = PLAYING;
        score = 0;
        lives = 3;
        
        player = PlayerPlane();
        obstacles.clear();
        playerBullets.clear();
        enemyBullets.clear();
        powerUps.clear();
        particles.clear();
        
        spawnTimer = 0.f;
        enemyBulletTimer = 0.f;
        
        levelManager = LevelManager();
        environment = Environment();
        
        powerUpTimer = levelManager.getPowerUpSpawnInterval();
        
        uiManager.update(score, levelManager.getCurrentLevel(), lives, player);
    }
    
    void updateMenu(float dt) {
        environment.update(dt);
    }
    
    void updateGame(float dt) {
        environment.update(dt);
        player.update(dt);
        
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
            auto newBullets = player.shoot();
            playerBullets.insert(playerBullets.end(), newBullets.begin(), newBullets.end());
        }
        
        levelManager.update(score);
        
        spawnTimer += dt;
        float spawnRate = 1.0f / levelManager.getSpawnMultiplier();
        if (spawnTimer >= spawnRate) {
            spawnObstacle();
            spawnTimer = 0.f;
        }
        
        powerUpTimer -= dt;
        if (powerUpTimer <= 0.f) {
            spawnPowerUp();
            powerUpTimer = levelManager.getPowerUpSpawnInterval();
        }
        
        updateBullets(dt);
        updateObstacles(dt);
        updatePowerUps(dt);
        particles.update(dt);
        
        checkCollisions();
        
        uiManager.update(score, levelManager.getCurrentLevel(), lives, player);
        
        if (lives <= 0) {
            state = GAME_OVER;
        }
    }
    
    void updateGameOver(float dt) {
        environment.update(dt);
        particles.update(dt);
    }
    
    void spawnObstacle() {
        float y = Random::getFloat(50.f, WINDOW_HEIGHT - 150.f);
        float speed = 100.f * levelManager.getSpeedMultiplier();
        
        if (levelManager.shouldSpawnEnemy()) {
            obstacles.push_back(std::make_unique<EnemyPlane>(
                sf::Vector2f(WINDOW_WIDTH + 50.f, y), speed));
        } else if (levelManager.shouldSpawnBird()) {
            obstacles.push_back(std::make_unique<BirdObstacle>(
                sf::Vector2f(WINDOW_WIDTH + 50.f, y), speed));
        } else {
            obstacles.push_back(std::make_unique<CloudObstacle>(
                sf::Vector2f(WINDOW_WIDTH + 50.f, y), speed * 0.7f));
        }
    }
    
    void spawnPowerUp() {
        float y = Random::getFloat(100.f, WINDOW_HEIGHT - 100.f);
        PowerUpType type = levelManager.getRandomPowerUpType();
        powerUps.push_back(std::make_unique<PowerUp>(
            sf::Vector2f(WINDOW_WIDTH + 50.f, y), type));
    }
    
    void updateBullets(float dt) {
        for (auto& bullet : playerBullets) {
            bullet.update(dt);
        }
        
        playerBullets.erase(std::remove_if(playerBullets.begin(), playerBullets.end(),
            [](const Bullet& b) { return !b.isActive(); }), playerBullets.end());
        
        for (auto& bullet : enemyBullets) {
            bullet.update(dt);
        }
        
        enemyBullets.erase(std::remove_if(enemyBullets.begin(), enemyBullets.end(),
            [](const Bullet& b) { return !b.isActive(); }), enemyBullets.end());
        
        enemyBulletTimer += dt;
        if (enemyBulletTimer >= 0.5f) {
            for (auto& obstacle : obstacles) {
                if (obstacle->getType() == ENEMY_PLANE) {
                    auto* enemy = dynamic_cast<EnemyPlane*>(obstacle.get());
                    if (enemy && enemy->canShoot()) {
                        enemyBullets.push_back(enemy->shoot());
                        enemy->resetShootTimer();
                    }
                }
            }
            enemyBulletTimer = 0.f;
        }
    }
    
    void updateObstacles(float dt) {
        for (auto& obstacle : obstacles) {
            obstacle->update(dt);
        }
        
        obstacles.erase(std::remove_if(obstacles.begin(), obstacles.end(),
            [](const std::unique_ptr<Obstacle>& o) { return !o->isActive(); }), obstacles.end());
    }
    
    void updatePowerUps(float dt) {
        for (auto& powerUp : powerUps) {
            powerUp->update(dt);
        }
        
        powerUps.erase(std::remove_if(powerUps.begin(), powerUps.end(),
            [](const std::unique_ptr<PowerUp>& p) { return !p->isActive(); }), powerUps.end());
    }
    
    void checkCollisions() {
        for (auto bulletIt = playerBullets.begin(); bulletIt != playerBullets.end();) {
            bool bulletHit = false;
            
            for (auto obstacleIt = obstacles.begin(); obstacleIt != obstacles.end();) {
                if (bulletIt->getBounds().intersects((*obstacleIt)->getBounds())) {
                    score += (*obstacleIt)->getScoreValue();
                    
                    particles.addExplosion((*obstacleIt)->getPosition(),
                                          sf::Color::White, 30);
                    
                    obstacleIt = obstacles.erase(obstacleIt);
                    bulletIt = playerBullets.erase(bulletIt);
                    bulletHit = true;
                    break;
                } else {
                    ++obstacleIt;
                }
            }
            
            if (!bulletHit) {
                ++bulletIt;
            }
        }
        
        // Enemy bullet vs player collisions
        for (auto bulletIt = enemyBullets.begin(); bulletIt != enemyBullets.end();) {
            if (bulletIt->getBounds().intersects(player.getBounds())) {
                if (!player.hasShield()) {
                    lives--;
                    particles.addExplosion(player.getPosition(),
                                          sf::Color::Red, 50);
                }
                bulletIt = enemyBullets.erase(bulletIt);
            } else {
                ++bulletIt;
            }
        }
        
        // Player vs obstacle collisions
        for (auto obstacleIt = obstacles.begin(); obstacleIt != obstacles.end();) {
            if (player.getBounds().intersects((*obstacleIt)->getBounds())) {
                if (!player.hasShield()) {
                    lives--;
                    particles.addExplosion(player.getPosition(),
                                          sf::Color::Red, 50);
                }
                
                particles.addExplosion((*obstacleIt)->getPosition(),
                                      sf::Color::White, 30);
                
                obstacleIt = obstacles.erase(obstacleIt);
            } else {
                ++obstacleIt;
            }
        }
        
        // Player vs power-up collisions
        for (auto powerUpIt = powerUps.begin(); powerUpIt != powerUps.end();) {
            if (player.getBounds().intersects((*powerUpIt)->getBounds())) {
                player.activatePowerUp((*powerUpIt)->getType());
                particles.addExplosion((*powerUpIt)->getBounds().getPosition(),
                                      (*powerUpIt)->getType() == POWER_SHIELD ? 
                                      sf::Color::Blue : sf::Color::Green, 20);
                powerUpIt = powerUps.erase(powerUpIt);
            } else {
                ++powerUpIt;
            }
        }
    }
    
    void render() {
        window.clear();
        
        // Draw environment
        environment.draw(window);
        
        switch(state) {
            case MENU:
                uiManager.drawMenu(window);
                break;
                
            case PLAYING:
                for (const auto& obstacle : obstacles) {
                    obstacle->draw(window);
                }
                
                for (const auto& bullet : enemyBullets) {
                    bullet.draw(window);
                }
                
                for (const auto& bullet : playerBullets) {
                    bullet.draw(window);
                }
                
                for (const auto& powerUp : powerUps) {
                    powerUp->draw(window);
                }
                
                particles.draw(window);
                player.draw(window);
                uiManager.drawGameUI(window);
                break;
                
            case GAME_OVER:
                for (const auto& obstacle : obstacles) {
                    obstacle->draw(window);
                }
                
                particles.draw(window);
                player.draw(window);
                uiManager.drawGameOver(window, score, levelManager.getCurrentLevel());
                break;
        }
        
        window.display();
    }
};

// ========== MAIN FUNCTION ==========
int main() {
    std::cout << "==========================================" << std::endl;
    std::cout << "          SKY DEFENDER PRO" << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << "Starting game..." << std::endl;
    std::cout << "If text doesn't display, install arial.ttf" << std::endl;
    std::cout << "==========================================" << std::endl;
    
    try {
        Game game;
        game.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}