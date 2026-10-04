#include "CameraManager.h"
#include "../Application/Application.h"
#include <iostream>

CameraManager::CameraManager()
    : m_Rng(std::random_device{}()) {}

void CameraManager::Init(const sf::View &defaultView, sf::RenderWindow *window)
{
    m_Window = window;
    float baseW = 1920.f;
    float baseH = 1080.f;
    if (g_App && g_App->GetWindowWidth() > 0 && g_App->GetWindowHeight() > 0)
    {
        baseW = static_cast<float>(g_App->GetWindowWidth());
        baseH = static_cast<float>(g_App->GetWindowHeight());
    }
    else if (window && window->getSize().x > 0 && window->getSize().y > 0)
    {
        baseW = static_cast<float>(window->getSize().x);
        baseH = static_cast<float>(window->getSize().y);
    }
    else if (defaultView.getSize().x > 0 && defaultView.getSize().y > 0)
    {
        baseW = defaultView.getSize().x;
        baseH = defaultView.getSize().y;
    }
    m_BaseSize = {baseW, baseH};
    m_DefaultCenter = {baseW * 0.5f, baseH * 0.5f};
    m_Position = m_DefaultCenter;
    m_Zoom = 1.0f;
    m_Rotation = 0.0f;
    m_FollowTarget = 0;
    m_FollowTargets.clear();
    m_PrimaryCamera = 0;
    m_MultiFollowMode = CameraMultiFollowMode::Average;
    m_FollowSpeed = 0.0f;
    m_FollowOffset = {0.f, 0.f};
    m_ManualFollowDisabled = false;
    m_HasBounds = false;
    m_FirstUpdate = true;
    StopShake();
    std::cout << "[INFO] [Camera] CameraManager initialized (" << m_BaseSize.x << "x" << m_BaseSize.y << ").\n";
}

void CameraManager::Reset()
{
    m_Position = m_DefaultCenter;
    m_Zoom = 1.0f;
    m_Rotation = 0.0f;
    m_FollowTarget = 0;
    m_FollowTargets.clear();
    m_PrimaryCamera = 0;
    m_MultiFollowMode = CameraMultiFollowMode::Average;
    m_FollowSpeed = 0.0f;
    m_FollowOffset = {0.f, 0.f};
    m_ManualFollowDisabled = false;
    m_HasBounds = false;
    m_FirstUpdate = true;
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

sf::Vector2f CameraManager::GetPosition() const { return m_Position; }

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

void CameraManager::SetRotation(float degrees) { m_Rotation = degrees; }

void CameraManager::Rotate(float deltaDegrees) { m_Rotation += deltaDegrees; }

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
    std::cout << "[INFO] [Camera] Camera started following entity " << entity << ".\n";
}

void CameraManager::StopFollow()
{
    m_FollowTarget = 0;
    m_FollowTargets.clear();
    m_ManualFollowDisabled = true;
    std::cout << "[INFO] [Camera] Camera stopped following target.\n";
}

void CameraManager::ResumeFollow()
{
    m_ManualFollowDisabled = false;
    std::cout << "[INFO] [Camera] Camera resumed follow behavior.\n";
}

bool CameraManager::IsFollowing() const
{
    return m_FollowTarget != 0 || !m_FollowTargets.empty() || !m_ManualFollowDisabled;
}

void CameraManager::AddFollowTarget(Entity entity)
{
    if (entity != 0 && std::find(m_FollowTargets.begin(), m_FollowTargets.end(), entity) == m_FollowTargets.end())
    {
        m_FollowTargets.push_back(entity);
        std::cout << "[INFO] [Camera] Added follow target entity " << entity << ".\n";
    }
    m_ManualFollowDisabled = false;
}

void CameraManager::RemoveFollowTarget(Entity entity)
{
    m_FollowTargets.erase(std::remove(m_FollowTargets.begin(), m_FollowTargets.end(), entity), m_FollowTargets.end());
    std::cout << "[INFO] [Camera] Removed follow target entity " << entity << ".\n";
}

void CameraManager::ClearFollowTargets()
{
    m_FollowTargets.clear();
    std::cout << "[INFO] [Camera] Cleared all follow targets.\n";
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
    std::cout << "[INFO] [Camera] Set camera bounds to (" << left << ", " << top << ", " << width << ", " << height <<
            ").\n";
}

void CameraManager::Shake(float intensity, float duration, bool decay)
{
    m_ShakeIntensity = intensity;
    m_ShakeDuration = std::max(0.001f, duration);
    m_ShakeTimeRemaining = m_ShakeDuration;
    m_ShakeDecay = decay;
    std::cout << "[INFO] [Camera] Camera shake triggered (intensity: " << intensity << ", duration: " << duration <<
            "s).\n";
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
    } else
    {
        m_Position.x = std::clamp(m_Position.x, m_Bounds.left, m_Bounds.left + m_Bounds.width);
        m_Position.y = std::clamp(m_Position.y, m_Bounds.top, m_Bounds.top + m_Bounds.height);
    }
}

void CameraManager::Update(float dt, Registry &registry)
{
    std::vector<std::pair<Entity, CameraComponent *> > activeTargets;

    if (!m_FollowTargets.empty())
    {
        for (Entity e: m_FollowTargets)
        {
            if (e != 0 && registry.HasComponent<TransformComponent>(e))
            {
                CameraComponent *cam = registry.HasComponent<CameraComponent>(e)
                                           ? &registry.GetComponent<CameraComponent>(e)
                                           : nullptr;
                activeTargets.push_back({e, cam});
            }
        }
    } else if (m_FollowTarget != 0)
    {
        if (registry.HasComponent<TransformComponent>(m_FollowTarget))
        {
            CameraComponent *cam = registry.HasComponent<CameraComponent>(m_FollowTarget)
                                       ? &registry.GetComponent<CameraComponent>(m_FollowTarget)
                                       : nullptr;
            activeTargets.push_back({m_FollowTarget, cam});
        }
    } else if (!m_ManualFollowDisabled)
    {
        registry.ForEach<TransformComponent, CameraComponent>(
            [&](Entity e, TransformComponent &, CameraComponent &c) {
                if (c.active) { activeTargets.push_back({e, &c}); }
            });
    }

    auto GetTargetCenter = [&](Entity e, const TransformComponent &t) -> sf::Vector2f {
        sf::Vector2f center(t.worldX, t.worldY);
        if (registry.HasComponent<RenderComponent>(e))
        {
            const auto &r = registry.GetComponent<RenderComponent>(e);
            sf::Transform tf;
            tf.translate(t.worldX, t.worldY);
            tf.rotate(t.worldRotation);
            tf.scale(t.worldScaleX, t.worldScaleY);
            center = tf.transformPoint(r.size.x * 0.5f, r.size.y * 0.5f);
        }
        return center;
    };

    if (activeTargets.size() == 1)
    {
        Entity target = activeTargets[0].first;
        CameraComponent *cam = activeTargets[0].second;
        float speed = cam ? cam->smoothSpeed : m_FollowSpeed;
        sf::Vector2f offset = cam ? sf::Vector2f(cam->offsetX, cam->offsetY) : m_FollowOffset;

        if (cam && cam->zoom > 0.01f) { m_Zoom = cam->zoom; }

        auto &t = registry.GetComponent<TransformComponent>(target);
        m_Rotation = t.worldRotation;
        sf::Vector2f desiredPos = GetTargetCenter(target, t) + offset;

        if (m_FirstUpdate)
        {
            m_Position = desiredPos;
            m_FirstUpdate = false;
        }
        else if (speed > 0.0f && dt > 0.0f)
        {
            float alpha = 1.0f - std::exp(-speed * dt);
            m_Position += (desiredPos - m_Position) * alpha;
        } else { m_Position = desiredPos; }
    } else if (activeTargets.size() >= 2)
    {
        CameraMultiFollowMode mode = m_MultiFollowMode;
        if (activeTargets[0].second) { mode = activeTargets[0].second->multiFollowMode; }

        if (mode == CameraMultiFollowMode::Priority)
        {
            Entity bestEntity = activeTargets[0].first;
            CameraComponent *bestCam = activeTargets[0].second;
            int bestPriority = bestCam ? bestCam->priority : -99999;

            for (size_t i = 1; i < activeTargets.size(); ++i)
            {
                CameraComponent *cam = activeTargets[i].second;
                int prio = cam ? cam->priority : 0;
                if (prio > bestPriority || (activeTargets[i].first == m_PrimaryCamera))
                {
                    bestEntity = activeTargets[i].first;
                    bestCam = cam;
                    bestPriority = prio;
                }
            }

            float speed = bestCam ? bestCam->smoothSpeed : m_FollowSpeed;
            sf::Vector2f offset = bestCam ? sf::Vector2f(bestCam->offsetX, bestCam->offsetY) : m_FollowOffset;
            if (bestCam && bestCam->zoom > 0.01f) { m_Zoom = bestCam->zoom; }
            auto &t = registry.GetComponent<TransformComponent>(bestEntity);
            m_Rotation = t.worldRotation;
            sf::Vector2f desiredPos = GetTargetCenter(bestEntity, t) + offset;

            if (m_FirstUpdate)
            {
                m_Position = desiredPos;
                m_FirstUpdate = false;
            }
            else if (speed > 0.0f && dt > 0.0f)
            {
                float alpha = 1.0f - std::exp(-speed * dt);
                m_Position += (desiredPos - m_Position) * alpha;
            } else { m_Position = desiredPos; }
        } else if (mode == CameraMultiFollowMode::Average)
        {
            sf::Vector2f sumPos(0.f, 0.f);
            float sumSpeed = 0.f;
            float count = static_cast<float>(activeTargets.size());

            for (auto &pair: activeTargets)
            {
                auto &t = registry.GetComponent<TransformComponent>(pair.first);
                sf::Vector2f off = pair.second
                                       ? sf::Vector2f(pair.second->offsetX, pair.second->offsetY)
                                       : m_FollowOffset;
                sumPos += (GetTargetCenter(pair.first, t) + off);
                sumSpeed += (pair.second ? pair.second->smoothSpeed : m_FollowSpeed);
            }

            sf::Vector2f desiredPos = sumPos / count;
            float speed = sumSpeed / count;

            if (m_FirstUpdate)
            {
                m_Position = desiredPos;
                m_FirstUpdate = false;
            }
            else if (speed > 0.0f && dt > 0.0f)
            {
                float alpha = 1.0f - std::exp(-speed * dt);
                m_Position += (desiredPos - m_Position) * alpha;
            } else { m_Position = desiredPos; }
        } else if (mode == CameraMultiFollowMode::AutoFrame)
        {
            float minX = 1e9f, maxX = -1e9f;
            float minY = 1e9f, maxY = -1e9f;
            float sumSpeed = 0.f;
            float padding = m_AutoFramePadding;
            float minZoom = m_MinAutoZoom;
            float maxZoom = m_MaxAutoZoom;

            for (auto &pair: activeTargets)
            {
                auto &t = registry.GetComponent<TransformComponent>(pair.first);
                sf::Vector2f off = pair.second
                                       ? sf::Vector2f(pair.second->offsetX, pair.second->offsetY)
                                       : m_FollowOffset;
                sf::Vector2f center = GetTargetCenter(pair.first, t) + off;
                float px = center.x;
                float py = center.y;
                minX = std::min(minX, px);
                maxX = std::max(maxX, px);
                minY = std::min(minY, py);
                maxY = std::max(maxY, py);
                sumSpeed += (pair.second ? pair.second->smoothSpeed : m_FollowSpeed);
                if (pair.second)
                {
                    padding = pair.second->autoFramePadding;
                    minZoom = pair.second->minZoom;
                    maxZoom = pair.second->maxZoom;
                }
            }

            sf::Vector2f desiredPos((minX + maxX) * 0.5f, (minY + maxY) * 0.5f);
            float speed = sumSpeed / static_cast<float>(activeTargets.size());

            float reqW = std::max(200.f, (maxX - minX) + padding * 2.f);
            float reqH = std::max(150.f, (maxY - minY) + padding * 2.f);
            float desiredZoom = std::clamp(std::min(m_BaseSize.x / reqW, m_BaseSize.y / reqH), minZoom, maxZoom);

            if (m_FirstUpdate)
            {
                m_Position = desiredPos;
                m_Zoom = desiredZoom;
                m_FirstUpdate = false;
            }
            else if (speed > 0.0f && dt > 0.0f)
            {
                float alpha = 1.0f - std::exp(-speed * dt);
                m_Position += (desiredPos - m_Position) * alpha;
                m_Zoom += (desiredZoom - m_Zoom) * alpha;
            } else
            {
                m_Position = desiredPos;
                m_Zoom = desiredZoom;
            }
        }
    }
    else
    {
        m_FirstUpdate = false;
    }


    if (m_ShakeTimeRemaining > 0.0f)
    {
        m_ShakeTimeRemaining = std::max(0.0f, m_ShakeTimeRemaining - dt);
        float currentIntensity = m_ShakeIntensity;
        if (m_ShakeDecay &&m_ShakeDuration > 0.0f) {
            currentIntensity *= (m_ShakeTimeRemaining / m_ShakeDuration);
        }
        std::uniform_real_distribution<float> dist(-currentIntensity, currentIntensity);
        m_ShakeOffset = {dist(m_Rng), dist(m_Rng)};
    } else { m_ShakeOffset = {0.f, 0.f}; }

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
                                      "zoom", &CameraComponent::zoom,
                                      "priority", &CameraComponent::priority,
                                      "multiFollowMode", sol::property(
                                          [](CameraComponent &c) -> int { return static_cast<int>(c.multiFollowMode); },
                                          [](CameraComponent &c, int m) {
                                              c.multiFollowMode = static_cast<CameraMultiFollowMode>(m);
                                          }
                                      ),
                                      "minZoom", &CameraComponent::minZoom,
                                      "maxZoom", &CameraComponent::maxZoom,
                                      "autoFramePadding", &CameraComponent::autoFramePadding
    );

    auto multiMode = lua.create_named_table("CameraMultiFollowMode");
    multiMode["Priority"] = static_cast<int>(CameraMultiFollowMode::Priority);
    multiMode["Average"] = static_cast<int>(CameraMultiFollowMode::Average);
    multiMode["AutoFrame"] = static_cast<int>(CameraMultiFollowMode::AutoFrame);

    auto camera = lua.create_named_table("Camera");

    camera.set_function("SetPosition", [](float x, float y) { CameraManager::Get().SetPosition(x, y); });

    camera.set_function("GetPosition", []() -> std::tuple<float, float> {
        auto pos = CameraManager::Get().GetPosition();
        return {pos.x, pos.y};
    });

    camera.set_function("GetX", []() -> float { return CameraManager::Get().GetX(); });

    camera.set_function("GetY", []() -> float { return CameraManager::Get().GetY(); });

    camera.set_function("Move", [](float dx, float dy) { CameraManager::Get().Move(dx, dy); });

    camera.set_function("SetZoom", [](float zoom) { CameraManager::Get().SetZoom(zoom); });

    camera.set_function("GetZoom", []() -> float { return CameraManager::Get().GetZoom(); });

    camera.set_function("Zoom", [](float factor) { CameraManager::Get().Zoom(factor); });

    camera.set_function("SetRotation", [](float deg) { CameraManager::Get().SetRotation(deg); });

    camera.set_function("GetRotation", []() -> float { return CameraManager::Get().GetRotation(); });

    camera.set_function("Rotate", [](float deltaDeg) { CameraManager::Get().Rotate(deltaDeg); });

    camera.set_function("SetSize", [](float w, float h) { CameraManager::Get().SetSize(w, h); });

    camera.set_function("GetSize", []() -> std::tuple<float, float> {
        auto s = CameraManager::Get().GetSize();
        return {s.x, s.y};
    });

    camera.set_function("Reset", []() { CameraManager::Get().Reset(); });

    camera.set_function(
        "Follow",
        [](Entity entity, sol::optional<float> smoothSpeed, sol::optional<float> offsetX,
           sol::optional<float> offsetY) {
            CameraManager::Get().Follow(entity, smoothSpeed.value_or(0.0f), offsetX.value_or(0.0f),
                                        offsetY.value_or(0.0f));
        });

    camera.set_function("StopFollow", []() { CameraManager::Get().StopFollow(); });

    camera.set_function("ResumeFollow", []() { CameraManager::Get().ResumeFollow(); });

    camera.set_function("IsFollowing", []() -> bool { return CameraManager::Get().IsFollowing(); });

    camera.set_function("GetFollowTarget", []() -> Entity { return CameraManager::Get().GetFollowTarget(); });

    camera.set_function("SetFollowSpeed", [](float speed) { CameraManager::Get().SetFollowSpeed(speed); });

    camera.set_function("GetFollowSpeed", []() -> float { return CameraManager::Get().GetFollowSpeed(); });

    camera.set_function("SetFollowOffset", [](float ox, float oy) { CameraManager::Get().SetFollowOffset(ox, oy); });

    camera.set_function("GetFollowOffset", []() -> std::tuple<float, float> {
        auto off = CameraManager::Get().GetFollowOffset();
        return {off.x, off.y};
    });

    camera.set_function("SetBounds",
                        [](float minX, float minY, float maxX, float maxY, sol::optional<bool> clampEdges) {
                            CameraManager::Get().SetBounds(minX, minY, maxX, maxY, clampEdges.value_or(true));
                        });

    camera.set_function("ClearBounds", []() { CameraManager::Get().ClearBounds(); });

    camera.set_function("HasBounds", []() -> bool { return CameraManager::Get().HasBounds(); });

    camera.set_function("GetBounds", []() -> std::tuple<float, float, float, float> {
        auto b = CameraManager::Get().GetBounds();
        return {b.left, b.top, b.left + b.width, b.top + b.height};
    });

    camera.set_function("Shake", [](float intensity, float duration, sol::optional<bool> decay) {
        CameraManager::Get().Shake(intensity, duration, decay.value_or(true));
    });

    camera.set_function("StopShake", []() { CameraManager::Get().StopShake(); });

    camera.set_function("IsShaking", []() -> bool { return CameraManager::Get().IsShaking(); });

    camera.set_function("ScreenToWorld", [](float sx, float sy) -> std::tuple<float, float> {
        auto w = CameraManager::Get().ScreenToWorld(sx, sy);
        return {w.x, w.y};
    });

    camera.set_function("WorldToScreen", [](float wx, float wy) -> std::tuple<float, float> {
        auto s = CameraManager::Get().WorldToScreen(wx, wy);
        return {s.x, s.y};
    });

    camera.set_function("SetMultiFollowMode", [](sol::object modeObj) {
        if (modeObj.is<int>())
        {
            CameraManager::Get().SetMultiFollowMode(
                static_cast<CameraMultiFollowMode>(modeObj.as<int>()));
        } else if (modeObj.is<std::string>())
        {
            std::string s = modeObj.as<std::string>();
            if (s == "priority" || s == "Priority") CameraManager::Get().SetMultiFollowMode(
                CameraMultiFollowMode::Priority);
            else if (s == "auto_frame" || s == "AutoFrame" || s == "autoframe") CameraManager::Get()
                    .SetMultiFollowMode(CameraMultiFollowMode::AutoFrame);
            else CameraManager::Get().SetMultiFollowMode(CameraMultiFollowMode::Average);
        }
    });

    camera.set_function("GetMultiFollowMode", []() -> std::string {
        auto m = CameraManager::Get().GetMultiFollowMode();
        if (m == CameraMultiFollowMode::Priority) return "priority";
        if (m == CameraMultiFollowMode::AutoFrame) return "auto_frame";
        return "average";
    });

    camera.set_function("AddFollowTarget", [](Entity e) { CameraManager::Get().AddFollowTarget(e); });

    camera.set_function("RemoveFollowTarget", [](Entity e) { CameraManager::Get().RemoveFollowTarget(e); });

    camera.set_function("ClearFollowTargets", []() { CameraManager::Get().ClearFollowTargets(); });

    camera.set_function("FollowGroup", [](sol::table targets) {
        CameraManager::Get().ClearFollowTargets();
        for (size_t i = 1; i <= targets.size(); ++i)
        {
            sol::object obj = targets[i];
            if (obj.is<Entity>()) { CameraManager::Get().AddFollowTarget(obj.as<Entity>()); }
        }
    });

    camera.set_function("GetFollowTargets", [](sol::this_state s) -> sol::table {
        sol::state_view l(s);
        sol::table t = l.create_table();
        const auto &targets = CameraManager::Get().GetFollowTargets();
        for (size_t i = 0; i < targets.size(); ++i) { t[i + 1] = targets[i]; }
        return t;
    });

    camera.set_function("SetAutoFramePadding", [](float pad) { CameraManager::Get().SetAutoFramePadding(pad); });

    camera.set_function("GetAutoFramePadding", []() -> float { return CameraManager::Get().GetAutoFramePadding(); });

    camera.set_function("SetAutoFrameZoomLimits", [](float minZ, float maxZ) {
        CameraManager::Get().SetAutoFrameZoomLimits(minZ, maxZ);
    });

    camera.set_function("GetAutoFrameZoomLimits", []() -> std::tuple<float, float> {
        return {CameraManager::Get().GetMinAutoZoom(), CameraManager::Get().GetMaxAutoZoom()};
    });

    camera.set_function("SetPrimary", [](Entity e) { CameraManager::Get().SetPrimaryCamera(e); });

    camera.set_function("GetPrimary", []() -> Entity { return CameraManager::Get().GetPrimaryCamera(); });

    lua.set_function("AddCamera",
                     [&registry](Entity e, sol::optional<float> smoothSpeed, sol::optional<float> offsetX,
                                 sol::optional<float> offsetY, sol::optional<float> zoom,
                                 sol::optional<int> priority) -> CameraComponent & {
                         CameraComponent cam{
                             true, smoothSpeed.value_or(0.0f), offsetX.value_or(0.0f), offsetY.value_or(0.0f),
                             zoom.value_or(1.0f), priority.value_or(0)
                         };
                         return registry.AddComponent(e, cam);
                     });

    lua.set_function("GetCamera", [&registry](Entity e) -> CameraComponent * {
        if (!registry.HasComponent<CameraComponent>(e)) return nullptr;
        return &registry.GetComponent<CameraComponent>(e);
    });

    lua.set_function("RemoveCamera", [&registry](Entity e) {
        if (registry.HasComponent<CameraComponent>(e))
            registry.RemoveComponent<CameraComponent>(e);
    });

    lua.set_function("HasCamera", [&registry](Entity e) -> bool { return registry.HasComponent<CameraComponent>(e); });
}
