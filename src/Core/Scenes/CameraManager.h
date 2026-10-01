#ifndef RAYNEENGINE_CAMERAMANAGER_H
#define RAYNEENGINE_CAMERAMANAGER_H

#include <SFML/Graphics/View.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Vector2.hpp>
#include <sol/sol.hpp>
#include <random>
#include <cmath>
#include <algorithm>
#include <iostream>
#include "../ECS/Registry.h"
#include "../ECS/Components.h"

class CameraManager {
public:
    static CameraManager& Get() {
        static CameraManager instance;
        return instance;
    }

    void Init(const sf::View &defaultView, sf::RenderWindow *window = nullptr);
    void Reset();

    // Position & Translation
    void SetPosition(float x, float y);
    void SetPosition(const sf::Vector2f &pos);
    sf::Vector2f GetPosition() const;
    float GetX() const { return m_Position.x; }
    float GetY() const { return m_Position.y; }
    void Move(float dx, float dy);

    // Zoom
    void SetZoom(float zoomFactor); // 1.0 = normal, >1 zoom in, <1 zoom out
    float GetZoom() const { return m_Zoom; }
    void Zoom(float factor);

    // Rotation
    void SetRotation(float degrees);
    float GetRotation() const { return m_Rotation; }
    void Rotate(float deltaDegrees);

    // View Size
    void SetSize(float width, float height);
    sf::Vector2f GetSize() const { return m_BaseSize; }

    // Follow Target
    void Follow(Entity entity, float smoothSpeed = 0.0f, float offsetX = 0.0f, float offsetY = 0.0f);
    void StopFollow();
    void ResumeFollow();
    bool IsFollowing() const;
    Entity GetFollowTarget() const { return m_FollowTarget; }
    void SetFollowSpeed(float speed) { m_FollowSpeed = std::max(0.0f, speed); }
    float GetFollowSpeed() const { return m_FollowSpeed; }
    void SetFollowOffset(float offsetX, float offsetY) { m_FollowOffset = {offsetX, offsetY}; }
    sf::Vector2f GetFollowOffset() const { return m_FollowOffset; }

    // Multi-Target / Multi-Camera Follow
    void SetMultiFollowMode(CameraMultiFollowMode mode) { m_MultiFollowMode = mode; }
    CameraMultiFollowMode GetMultiFollowMode() const { return m_MultiFollowMode; }
    void SetAutoFramePadding(float padding) { m_AutoFramePadding = padding; }
    float GetAutoFramePadding() const { return m_AutoFramePadding; }
    void SetAutoFrameZoomLimits(float minZoom, float maxZoom) { m_MinAutoZoom = minZoom; m_MaxAutoZoom = maxZoom; }
    float GetMinAutoZoom() const { return m_MinAutoZoom; }
    float GetMaxAutoZoom() const { return m_MaxAutoZoom; }

    void AddFollowTarget(Entity entity);
    void RemoveFollowTarget(Entity entity);
    void ClearFollowTargets();
    const std::vector<Entity>& GetFollowTargets() const { return m_FollowTargets; }

    void SetPrimaryCamera(Entity entity) { m_PrimaryCamera = entity; }
    Entity GetPrimaryCamera() const { return m_PrimaryCamera; }

    // Bounds Constraint
    void SetBounds(float minX, float minY, float maxX, float maxY, bool clampEdges = true);
    void ClearBounds() { m_HasBounds = false; }
    bool HasBounds() const { return m_HasBounds; }
    sf::FloatRect GetBounds() const { return m_Bounds; }

    // Screen Shake
    void Shake(float intensity, float duration, bool decay = true);
    void StopShake();
    bool IsShaking() const { return m_ShakeTimeRemaining > 0.0f; }

    // Coordinate Conversion
    sf::Vector2f ScreenToWorld(float screenX, float screenY, const sf::RenderWindow *window = nullptr) const;
    sf::Vector2f WorldToScreen(float worldX, float worldY, const sf::RenderWindow *window = nullptr) const;

    // View retrieval
    sf::View GetView() const;

    // Update
    void Update(float dt, Registry &registry);

    // Lua Bindings
    static void RegisterLua(sol::state &lua, Registry &registry);

private:
    CameraManager();

    void ClampToBounds();

    sf::RenderWindow *m_Window = nullptr;
    sf::Vector2f m_Position = {960.f, 540.f};
    sf::Vector2f m_BaseSize = {1920.f, 1080.f};
    sf::Vector2f m_DefaultCenter = {960.f, 540.f};
    float m_Zoom = 1.0f;
    float m_Rotation = 0.0f;

    // Follow state
    Entity m_FollowTarget = 0;
    float m_FollowSpeed = 0.0f;
    sf::Vector2f m_FollowOffset = {0.f, 0.f};
    bool m_ManualFollowDisabled = false;

    CameraMultiFollowMode m_MultiFollowMode = CameraMultiFollowMode::Average;
    float m_AutoFramePadding = 200.0f;
    float m_MinAutoZoom = 0.3f;
    float m_MaxAutoZoom = 3.0f;
    std::vector<Entity> m_FollowTargets;
    Entity m_PrimaryCamera = 0;

    // Bounds
    bool m_HasBounds = false;
    bool m_ClampEdges = true;
    sf::FloatRect m_Bounds = {0.f, 0.f, 0.f, 0.f};

    // Shake
    float m_ShakeIntensity = 0.0f;
    float m_ShakeDuration = 0.0f;
    float m_ShakeTimeRemaining = 0.0f;
    bool m_ShakeDecay = true;
    sf::Vector2f m_ShakeOffset = {0.f, 0.f};

    std::mt19937 m_Rng;
};

#endif
