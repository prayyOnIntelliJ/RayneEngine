#include "CameraManager.h"

CameraManager::CameraManager()
    : m_Rng(std::random_device{}())
{
}

void CameraManager::Init(const sf::View &defaultView, sf::RenderWindow *window)
{
    m_Window = window;
    m_BaseSize = defaultView.getSize();
    m_DefaultCenter = defaultView.getCenter();
    m_Position = m_DefaultCenter;
    m_Zoom = 1.0f;
    m_Rotation = 0.0f;
    m_FollowTarget = 0;
    m_FollowSpeed = 0.0f;
    m_FollowOffset = {0.f, 0.f};
    m_ManualFollowDisabled = false;
    m_HasBounds = false;
    StopShake();
}

void CameraManager::Reset()
{
    m_Position = m_DefaultCenter;
    m_Zoom = 1.0f;
    m_Rotation = 0.0f;
    m_FollowTarget = 0;
    m_FollowSpeed = 0.0f;
    m_FollowOffset = {0.f, 0.f};
    m_ManualFollowDisabled = false;
    m_HasBounds = false;
    StopShake();
}

void CameraManager::SetPosition(float x, float y)
{
    m_Position = {x, y};
    ClampToBounds();
}

void CameraManager::SetPosition(const sf::Vector2f &pos)
{
    m_Position = pos;
    ClampToBounds();
}

sf::Vector2f CameraManager::GetPosition() const
{
    return m_Position;
}

void CameraManager::Move(float dx, float dy)
{
    m_Position.x += dx;
    m_Position.y += dy;
    ClampToBounds();
}

void CameraManager::SetZoom(float zoomFactor)
{
    m_Zoom = std::max(0.01f, zoomFactor);
    ClampToBounds();
}

void CameraManager::Zoom(float factor)
{
    m_Zoom = std::max(0.01f, m_Zoom * factor);
    ClampToBounds();
}

void CameraManager::SetRotation(float degrees)
{
    m_Rotation = degrees;
}

void CameraManager::Rotate(float deltaDegrees)
{
    m_Rotation += deltaDegrees;
}

void CameraManager::SetSize(float width, float height)
{
    m_BaseSize = {std::max(1.f, width), std::max(1.f, height)};
    ClampToBounds();
}

void CameraManager::Follow(Entity entity, float smoothSpeed, float offsetX, float offsetY)
{
    m_FollowTarget = entity;
    m_FollowSpeed = std::max(0.0f, smoothSpeed);
    m_FollowOffset = {offsetX, offsetY};
    m_ManualFollowDisabled = false;
}

void CameraManager::StopFollow()
{
    m_FollowTarget = 0;
    m_ManualFollowDisabled = true;
}

void CameraManager::ResumeFollow()
{
    m_ManualFollowDisabled = false;
}

bool CameraManager::IsFollowing() const
{
    return m_FollowTarget != 0 || !m_ManualFollowDisabled;
}

void CameraManager::SetBounds(float minX, float minY, float maxX, float maxY, bool clampEdges)
{
    float left = std::min(minX, maxX);
    float top = std::min(minY, maxY);
    float width = std::abs(maxX - minX);
    float height = std::abs(maxY - minY);
    m_Bounds = sf::FloatRect(left, top, width, height);
    m_ClampEdges = clampEdges;
    m_HasBounds = true;
    ClampToBounds();
}

void CameraManager::Shake(float intensity, float duration, bool decay)
{
    m_ShakeIntensity = intensity;
    m_ShakeDuration = std::max(0.001f, duration);
    m_ShakeTimeRemaining = m_ShakeDuration;
    m_ShakeDecay = decay;
}

void CameraManager::StopShake()
{
    m_ShakeTimeRemaining = 0.0f;
    m_ShakeOffset = {0.f, 0.f};
}

void CameraManager::ClampToBounds()
{
    if (!m_HasBounds) return;

    if (m_ClampEdges)
    {
        float halfW = (m_BaseSize.x / std::max(0.001f, m_Zoom)) * 0.5f;
        float halfH = (m_BaseSize.y / std::max(0.001f, m_Zoom)) * 0.5f;

        if (m_Bounds.width >= halfW * 2.f)
            m_Position.x = std::clamp(m_Position.x, m_Bounds.left + halfW, m_Bounds.left + m_Bounds.width - halfW);
        else
            m_Position.x = m_Bounds.left + m_Bounds.width * 0.5f;

        if (m_Bounds.height >= halfH * 2.f)
            m_Position.y = std::clamp(m_Position.y, m_Bounds.top + halfH, m_Bounds.top + m_Bounds.height - halfH);
        else
            m_Position.y = m_Bounds.top + m_Bounds.height * 0.5f;
    }
    else
    {
        m_Position.x = std::clamp(m_Position.x, m_Bounds.left, m_Bounds.left + m_Bounds.width);
        m_Position.y = std::clamp(m_Position.y, m_Bounds.top, m_Bounds.top + m_Bounds.height);
    }
}

void CameraManager::Update(float dt, Registry &registry)
{
    Entity target = m_FollowTarget;
    float speed = m_FollowSpeed;
    sf::Vector2f offset = m_FollowOffset;

    if (target == 0 && !m_ManualFollowDisabled)
    {
        registry.ForEach<TransformComponent, CameraComponent>(
            [&](Entity e, TransformComponent &, CameraComponent &c) {
                if (c.active && target == 0)
                {
                    target = e;
                    speed = c.smoothSpeed;
                    offset = {c.offsetX, c.offsetY};
                    if (c.zoom > 0.01f && m_Zoom == 1.0f)
                    {
                        m_Zoom = c.zoom;
                    }
                }
            });
    }

    if (target != 0 && registry.HasComponent<TransformComponent>(target))
    {
        auto &t = registry.GetComponent<TransformComponent>(target);
        sf::Vector2f desiredPos(t.worldX + offset.x, t.worldY + offset.y);

        if (speed > 0.0f && dt > 0.0f)
        {
            float alpha = 1.0f - std::exp(-speed * dt);
            m_Position += (desiredPos - m_Position) * alpha;
        }
        else
        {
            m_Position = desiredPos;
        }
    }

    if (m_ShakeTimeRemaining > 0.0f)
    {
        m_ShakeTimeRemaining = std::max(0.0f, m_ShakeTimeRemaining - dt);
        float currentIntensity = m_ShakeIntensity;
        if (m_ShakeDecay && m_ShakeDuration > 0.0f)
        {
            currentIntensity *= (m_ShakeTimeRemaining / m_ShakeDuration);
        }
        std::uniform_real_distribution<float> dist(-currentIntensity, currentIntensity);
        m_ShakeOffset = {dist(m_Rng), dist(m_Rng)};
    }
    else
    {
        m_ShakeOffset = {0.f, 0.f};
    }

    ClampToBounds();
}

sf::View CameraManager::GetView() const
{
    sf::View v;
    float w = m_BaseSize.x / std::max(0.001f, m_Zoom);
    float h = m_BaseSize.y / std::max(0.001f, m_Zoom);
    v.setSize(w, h);
    v.setCenter(m_Position + m_ShakeOffset);
    v.setRotation(m_Rotation);
    return v;
}

sf::Vector2f CameraManager::ScreenToWorld(float screenX, float screenY, const sf::RenderWindow *window) const
{
    const sf::RenderWindow *win = window ? window : m_Window;
    if (win)
    {
        return win->mapPixelToCoords(sf::Vector2i(static_cast<int>(screenX), static_cast<int>(screenY)), GetView());
    }
    return GetView().getInverseTransform().transformPoint(screenX, screenY);
}

sf::Vector2f CameraManager::WorldToScreen(float worldX, float worldY, const sf::RenderWindow *window) const
{
    const sf::RenderWindow *win = window ? window : m_Window;
    if (win)
    {
        sf::Vector2i px = win->mapCoordsToPixel(sf::Vector2f(worldX, worldY), GetView());
        return sf::Vector2f(static_cast<float>(px.x), static_cast<float>(px.y));
    }
    return GetView().getTransform().transformPoint(worldX, worldY);
}

void CameraManager::RegisterLua(sol::state &lua, Registry &registry)
{
    lua.new_usertype<CameraComponent>("CameraComponent",
        "active", &CameraComponent::active,
        "smoothSpeed", &CameraComponent::smoothSpeed,
        "offsetX", &CameraComponent::offsetX,
        "offsetY", &CameraComponent::offsetY,
        "zoom", &CameraComponent::zoom
    );

    auto camera = lua.create_named_table("Camera");

    camera.set_function("SetPosition", [](float x, float y) {
        CameraManager::Get().SetPosition(x, y);
    });

    camera.set_function("GetPosition", []() -> std::tuple<float, float> {
        auto pos = CameraManager::Get().GetPosition();
        return {pos.x, pos.y};
    });

    camera.set_function("GetX", []() -> float {
        return CameraManager::Get().GetX();
    });

    camera.set_function("GetY", []() -> float {
        return CameraManager::Get().GetY();
    });

    camera.set_function("Move", [](float dx, float dy) {
        CameraManager::Get().Move(dx, dy);
    });

    camera.set_function("SetZoom", [](float zoom) {
        CameraManager::Get().SetZoom(zoom);
    });

    camera.set_function("GetZoom", []() -> float {
        return CameraManager::Get().GetZoom();
    });

    camera.set_function("Zoom", [](float factor) {
        CameraManager::Get().Zoom(factor);
    });

    camera.set_function("SetRotation", [](float deg) {
        CameraManager::Get().SetRotation(deg);
    });

    camera.set_function("GetRotation", []() -> float {
        return CameraManager::Get().GetRotation();
    });

    camera.set_function("Rotate", [](float deltaDeg) {
        CameraManager::Get().Rotate(deltaDeg);
    });

    camera.set_function("SetSize", [](float w, float h) {
        CameraManager::Get().SetSize(w, h);
    });

    camera.set_function("GetSize", []() -> std::tuple<float, float> {
        auto s = CameraManager::Get().GetSize();
        return {s.x, s.y};
    });

    camera.set_function("Reset", []() {
        CameraManager::Get().Reset();
    });

    camera.set_function("Follow", [](Entity entity, sol::optional<float> smoothSpeed, sol::optional<float> offsetX, sol::optional<float> offsetY) {
        CameraManager::Get().Follow(entity, smoothSpeed.value_or(0.0f), offsetX.value_or(0.0f), offsetY.value_or(0.0f));
    });

    camera.set_function("StopFollow", []() {
        CameraManager::Get().StopFollow();
    });

    camera.set_function("ResumeFollow", []() {
        CameraManager::Get().ResumeFollow();
    });

    camera.set_function("IsFollowing", []() -> bool {
        return CameraManager::Get().IsFollowing();
    });

    camera.set_function("GetFollowTarget", []() -> Entity {
        return CameraManager::Get().GetFollowTarget();
    });

    camera.set_function("SetFollowSpeed", [](float speed) {
        CameraManager::Get().SetFollowSpeed(speed);
    });

    camera.set_function("GetFollowSpeed", []() -> float {
        return CameraManager::Get().GetFollowSpeed();
    });

    camera.set_function("SetFollowOffset", [](float ox, float oy) {
        CameraManager::Get().SetFollowOffset(ox, oy);
    });

    camera.set_function("GetFollowOffset", []() -> std::tuple<float, float> {
        auto off = CameraManager::Get().GetFollowOffset();
        return {off.x, off.y};
    });

    camera.set_function("SetBounds", [](float minX, float minY, float maxX, float maxY, sol::optional<bool> clampEdges) {
        CameraManager::Get().SetBounds(minX, minY, maxX, maxY, clampEdges.value_or(true));
    });

    camera.set_function("ClearBounds", []() {
        CameraManager::Get().ClearBounds();
    });

    camera.set_function("HasBounds", []() -> bool {
        return CameraManager::Get().HasBounds();
    });

    camera.set_function("GetBounds", []() -> std::tuple<float, float, float, float> {
        auto b = CameraManager::Get().GetBounds();
        return {b.left, b.top, b.left + b.width, b.top + b.height};
    });

    camera.set_function("Shake", [](float intensity, float duration, sol::optional<bool> decay) {
        CameraManager::Get().Shake(intensity, duration, decay.value_or(true));
    });

    camera.set_function("StopShake", []() {
        CameraManager::Get().StopShake();
    });

    camera.set_function("IsShaking", []() -> bool {
        return CameraManager::Get().IsShaking();
    });

    camera.set_function("ScreenToWorld", [](float sx, float sy) -> std::tuple<float, float> {
        auto w = CameraManager::Get().ScreenToWorld(sx, sy);
        return {w.x, w.y};
    });

    camera.set_function("WorldToScreen", [](float wx, float wy) -> std::tuple<float, float> {
        auto s = CameraManager::Get().WorldToScreen(wx, wy);
        return {s.x, s.y};
    });

    lua.set_function("AddCamera", [&registry](Entity e, sol::optional<float> smoothSpeed, sol::optional<float> offsetX, sol::optional<float> offsetY, sol::optional<float> zoom) -> CameraComponent& {
        CameraComponent cam{true, smoothSpeed.value_or(0.0f), offsetX.value_or(0.0f), offsetY.value_or(0.0f), zoom.value_or(1.0f)};
        return registry.AddComponent(e, cam);
    });

    lua.set_function("GetCamera", [&registry](Entity e) -> CameraComponent* {
        if (!registry.HasComponent<CameraComponent>(e)) return nullptr;
        return &registry.GetComponent<CameraComponent>(e);
    });

    lua.set_function("RemoveCamera", [&registry](Entity e) {
        if (registry.HasComponent<CameraComponent>(e))
            registry.RemoveComponent<CameraComponent>(e);
    });

    lua.set_function("HasCamera", [&registry](Entity e) -> bool {
        return registry.HasComponent<CameraComponent>(e);
    });
}
