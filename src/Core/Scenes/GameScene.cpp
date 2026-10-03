#include "GameScene.h"
#include "CameraManager.h"
#include "../Scenes/SceneManager.h"
#include <iostream>
#include "../Application/Application.h"

#include "../ECS/Components.h"
#include "../ECS/HierarchySystem.h"
#include "../ECS/PhysicsSystem.h"
#include "../Scripting/EventManager.h"
#include "../UI/UIManager.h"
#include "../Scripting/ScriptComponent.h"
#include "../Scripting/TimerManager.h"
#include "../Scripting/TweenManager.h"
#include "../Audio/AudioManager.h"
#include "../Input/InputManager.h"
#include "../Scripting/LuaState.h"
#include "../Application/EngineVersion.h"
#include "SFML/Graphics/RectangleShape.hpp"
#include "SFML/Graphics/CircleShape.hpp"
#include "SFML/Window/Event.hpp"
#include <unordered_set>

GameScene::GameScene(SceneManager &manager, sf::RenderWindow &window, Registry &registry)
    : Scene(manager), m_Window(window), m_Registry(registry)
{
    m_Font = ResourceManager::Get().GetFont(ENGINE_ASSET_PATH "/fonts/Merriweather.ttf");

    m_DebugText.setFont(*m_Font);
    m_DebugText.setCharacterSize(12);
    m_DebugText.setFillColor(sf::Color(180, 180, 180));
    m_DebugText.setPosition(8.f, 8.f);
}

static bool IsInputEvent(const sf::Event &event)
{
    switch (event.type)
    {
        case sf::Event::KeyPressed:
        case sf::Event::KeyReleased:
        case sf::Event::TextEntered:
        case sf::Event::MouseButtonPressed:
        case sf::Event::MouseButtonReleased:
        case sf::Event::MouseMoved:
        case sf::Event::MouseWheelScrolled:
        case sf::Event::JoystickButtonPressed:
        case sf::Event::JoystickButtonReleased:
        case sf::Event::JoystickMoved:
        case sf::Event::JoystickConnected:
        case sf::Event::JoystickDisconnected:
        case sf::Event::TouchBegan:
        case sf::Event::TouchMoved:
        case sf::Event::TouchEnded:
        case sf::Event::SensorChanged:
            return true;
        default:
            return false;
    }
}

static sol::table CreateInputEventTable(sol::state &lua, const sf::Event &event, const sf::RenderWindow &window, const sf::View &camera)
{
    sol::table t = lua.create_table();
    t["isInput"] = true;
    t["isPaused"] = (g_App && g_App->IsPaused());

    switch (event.type)
    {
        case sf::Event::KeyPressed:
        {
            t["type"] = static_cast<int>(InputEventType::KeyDown);
            t["typeName"] = "KeyDown";
            t["typeStr"] = "key_pressed";
            t["key"] = static_cast<int>(event.key.code);
            t["keyCode"] = static_cast<int>(event.key.code);
            t["keyName"] = InputManager::KeyToString(event.key.code);
            t["alt"] = event.key.alt;
            t["control"] = event.key.control;
            t["shift"] = event.key.shift;
            t["system"] = event.key.system;
            break;
        }
        case sf::Event::KeyReleased:
        {
            t["type"] = static_cast<int>(InputEventType::KeyUp);
            t["typeName"] = "KeyUp";
            t["typeStr"] = "key_released";
            t["key"] = static_cast<int>(event.key.code);
            t["keyCode"] = static_cast<int>(event.key.code);
            t["keyName"] = InputManager::KeyToString(event.key.code);
            t["alt"] = event.key.alt;
            t["control"] = event.key.control;
            t["shift"] = event.key.shift;
            t["system"] = event.key.system;
            break;
        }
        case sf::Event::TextEntered:
        {
            t["type"] = static_cast<int>(InputEventType::TextEntered);
            t["typeName"] = "TextEntered";
            t["typeStr"] = "text_entered";
            t["unicode"] = static_cast<uint32_t>(event.text.unicode);
            if (event.text.unicode < 128)
                t["text"] = std::string(1, static_cast<char>(event.text.unicode));
            else
                t["text"] = "";
            break;
        }
        case sf::Event::MouseButtonPressed:
        {
            t["type"] = static_cast<int>(InputEventType::MouseDown);
            t["typeName"] = "MouseDown";
            t["typeStr"] = "mouse_pressed";
            t["button"] = static_cast<int>(event.mouseButton.button);
            t["buttonCode"] = static_cast<int>(event.mouseButton.button);
            t["buttonName"] = InputManager::MouseButtonToString(event.mouseButton.button);
            t["x"] = event.mouseButton.x;
            t["y"] = event.mouseButton.y;
            sf::Vector2f world = window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y}, camera);
            t["worldX"] = world.x;
            t["worldY"] = world.y;
            break;
        }
        case sf::Event::MouseButtonReleased:
        {
            t["type"] = static_cast<int>(InputEventType::MouseUp);
            t["typeName"] = "MouseUp";
            t["typeStr"] = "mouse_released";
            t["button"] = static_cast<int>(event.mouseButton.button);
            t["buttonCode"] = static_cast<int>(event.mouseButton.button);
            t["buttonName"] = InputManager::MouseButtonToString(event.mouseButton.button);
            t["x"] = event.mouseButton.x;
            t["y"] = event.mouseButton.y;
            sf::Vector2f world = window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y}, camera);
            t["worldX"] = world.x;
            t["worldY"] = world.y;
            break;
        }
        case sf::Event::MouseMoved:
        {
            t["type"] = static_cast<int>(InputEventType::MouseMove);
            t["typeName"] = "MouseMove";
            t["typeStr"] = "mouse_moved";
            t["x"] = event.mouseMove.x;
            t["y"] = event.mouseMove.y;
            sf::Vector2f world = window.mapPixelToCoords({event.mouseMove.x, event.mouseMove.y}, camera);
            t["worldX"] = world.x;
            t["worldY"] = world.y;
            break;
        }
        case sf::Event::MouseWheelScrolled:
        {
            t["type"] = static_cast<int>(InputEventType::MouseWheel);
            t["typeName"] = "MouseWheel";
            t["typeStr"] = "mouse_wheel";
            t["delta"] = event.mouseWheelScroll.delta;
            t["x"] = event.mouseWheelScroll.x;
            t["y"] = event.mouseWheelScroll.y;
            t["wheel"] = (event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) ? "vertical" : "horizontal";
            sf::Vector2f world = window.mapPixelToCoords({event.mouseWheelScroll.x, event.mouseWheelScroll.y}, camera);
            t["worldX"] = world.x;
            t["worldY"] = world.y;
            break;
        }
        case sf::Event::JoystickButtonPressed:
        {
            t["type"] = static_cast<int>(InputEventType::JoystickPressed);
            t["typeName"] = "JoystickPressed";
            t["typeStr"] = "joystick_pressed";
            t["joystickId"] = event.joystickButton.joystickId;
            t["button"] = event.joystickButton.button;
            t["buttonCode"] = static_cast<int>(event.joystickButton.button);
            break;
        }
        case sf::Event::JoystickButtonReleased:
        {
            t["type"] = static_cast<int>(InputEventType::JoystickReleased);
            t["typeName"] = "JoystickReleased";
            t["typeStr"] = "joystick_released";
            t["joystickId"] = event.joystickButton.joystickId;
            t["button"] = event.joystickButton.button;
            t["buttonCode"] = static_cast<int>(event.joystickButton.button);
            break;
        }
        case sf::Event::JoystickMoved:
        {
            t["type"] = static_cast<int>(InputEventType::JoystickMoved);
            t["typeName"] = "JoystickMoved";
            t["typeStr"] = "joystick_moved";
            t["joystickId"] = event.joystickMove.joystickId;
            t["axis"] = static_cast<int>(event.joystickMove.axis);
            t["position"] = event.joystickMove.position;
            break;
        }
        default:
        {
            t["type"] = static_cast<int>(InputEventType::Unknown);
            t["typeName"] = "Unknown";
            t["typeStr"] = "unknown";
            break;
        }
    }

    return t;
}

void GameScene::OnEnter()
{
    std::cout << "[INFO] [GameScene] Starting simulation...\n";

#ifndef RAYNE_STANDALONE
    m_Window.setTitle(
        (g_App ? g_App->GetProjectName() : std::string(Rayne::DEFAULT_PROJECT_NAME))
        + ": Play Mode"
        + " (" + Rayne::PlatformString() + ")"
        + " - RayneEngine " + Rayne::VersionString());
#endif

    CameraManager::Get().Init(m_Window.getDefaultView(), &m_Window);
    m_Camera = CameraManager::Get().GetView();
    m_LastCollisions.clear();
    PhysicsSystem::Reset();

    EventManager::Get().SubscribeCollision([this](CollisionEvent e) {
        if (m_Registry.HasComponent<ScriptComponent>(e.a))
            m_Registry.GetComponent<ScriptComponent>(e.a).OnCollision(e.b);

        if (m_Registry.HasComponent<ScriptComponent>(e.b))
            m_Registry.GetComponent<ScriptComponent>(e.b).OnCollision(e.a);
    });

    EventManager::Get().SubscribeButtonClick([this](const std::string &buttonId) {
        m_Registry.ForEach<ScriptComponent>([&buttonId](Entity, ScriptComponent &sc) {
            sc.OnButtonClicked(buttonId);
        });
    });

    EventManager::Get().SubscribeButtonHover([this](const std::string &buttonId) {
        m_Registry.ForEach<ScriptComponent>([&buttonId](Entity, ScriptComponent &sc) {
            sc.OnButtonHovered(buttonId);
        });
    });

    EventManager::Get().SubscribeSliderChange([this](const std::string &sliderId, float val) {
        m_Registry.ForEach<ScriptComponent>([&sliderId, val](Entity, ScriptComponent &sc) {
            sc.OnSliderChanged(sliderId, val);
        });
    });

    EventManager::Get().SubscribeCheckboxChange([this](const std::string &checkboxId, bool checked) {
        m_Registry.ForEach<ScriptComponent>([&checkboxId, checked](Entity, ScriptComponent &sc) {
            sc.OnCheckboxChanged(checkboxId, checked);
        });
    });

    m_Registry.ForEach<TransformComponent, AudioSourceComponent>([](Entity, TransformComponent &, AudioSourceComponent &ac) {
        if (ac.playOnStart && !ac.soundPath.empty()) {
            AudioManager::Get().PlaySound(ac.soundPath, ac.volume, ac.pitch, ac.loop);
        }
    });

    EventManager::Get().SubscribeTextInputChange([this](const std::string &inputId, const std::string &text) {
        m_Registry.ForEach<ScriptComponent>([&inputId, &text](Entity, ScriptComponent &sc) {
            sc.OnTextInputChanged(inputId, text);
        });
    });

    EventManager::Get().SubscribeTextInputSubmit([this](const std::string &inputId, const std::string &text) {
        m_Registry.ForEach<ScriptComponent>([&inputId, &text](Entity, ScriptComponent &sc) {
            sc.OnTextInputSubmitted(inputId, text);
        });
    });

    EventManager::Get().SubscribeUIHover([this](const std::string &id, bool hovered) {
        m_Registry.ForEach<ScriptComponent>([&id, hovered](Entity, ScriptComponent &sc) {
            sc.OnUIHover(id, hovered);
        });
    });

    EventManager::Get().SubscribeUIFocus([this](const std::string &id, bool focused) {
        m_Registry.ForEach<ScriptComponent>([&id, focused](Entity, ScriptComponent &sc) {
            sc.OnUIFocus(id, focused);
        });
    });

    EventManager::Get().SubscribeInput([this](const sf::Event &event) {
        sol::state &lua = LuaState::GetLua();
        sol::table eventTable = CreateInputEventTable(lua, event, m_Window, CameraManager::Get().GetView());
        lua["__last_input_event"] = eventTable;

        m_Registry.ForEach<ScriptComponent>([&eventTable](Entity, ScriptComponent &sc) {
            sc.OnInputReceived(eventTable);
        });

        sol::object globalCb = lua["OnInputReceived"];
        if (!globalCb.is<sol::protected_function>())
            globalCb = lua["OnInputReceiced"];
        if (globalCb.is<sol::protected_function>())
        {
            sol::protected_function pfn = globalCb.as<sol::protected_function>();
            auto res = pfn(eventTable);
            if (!res.valid())
            {
                sol::error err = res;
                std::cerr << "[ERROR] [Script] Global OnInputReceived error: " << err.what() << "\n";
            }
        }
    });
    for (const auto &el : UIManager::Get().GetElements())
    {
        if (!el.scriptPath.empty())
        {
            std::string resolved = ResourceManager::ResolveAssetPath(el.scriptPath);
            std::error_code ec;
            if (std::filesystem::exists(resolved, ec))
            {
                auto res = LuaState::GetLua().safe_script_file(resolved);
                if (!res.valid())
                {
                    sol::error err = res;
                    std::cerr << "[ERROR] [GameScene] Failed to load UI script (" << resolved << "): " << err.what() << "\n";
                }
                else
                {
                    std::cout << "[INFO] [GameScene] Loaded UI script: " << resolved << "\n";
                }
            }
        }
    }

    std::cout << "[INFO] [GameScene] Firing OnCreate() for all active scripts...\n";
    m_Registry.ForEach<ScriptComponent>([](Entity, ScriptComponent &sc) { sc.OnCreate(); });
    std::cout << "[INFO] [GameScene] Simulation initialized and running.\n";
}

void GameScene::OnExit()
{
    std::cout << "[INFO] [GameScene] Stopping simulation, stopping audio, clearing collision state...\n";
    m_Registry.ForEach<ScriptComponent>([](Entity, ScriptComponent &sc) { sc.OnDestroy(); });
    m_LastCollisions.clear();
    PhysicsSystem::Reset();
    TimerManager::Get().Clear();
    TweenManager::Get().Clear();
    EventManager::Get().Clear();
    CameraManager::Get().Reset();

    AudioManager::Get().StopAllSounds();
    AudioManager::Get().StopMusic();

    if (g_App)
    {
        g_App->SetPaused(false);
        g_App->SetTimeScale(1.0f);
        g_App->SetCursorVisible(true);
    }
    UIManager::Get().ClearClickedButton();

    std::cout << "[INFO] [GameScene] Simulation stopped cleanly.\n";
}

void GameScene::CheckCollisions()
{
    struct CollidableEntity
    {
        Entity id;
        float x, y, w, h;
        int channel;
        CollisionType type;
    };
    std::vector<CollidableEntity> collidables;

    m_Registry.ForEach<TransformComponent, RenderComponent>(
        [&collidables, this](Entity e, TransformComponent &t, RenderComponent &r) {
            if (m_Registry.HasComponent<CollisionComponent>(e))
            {
                auto &col = m_Registry.GetComponent<CollisionComponent>(e);
                collidables.push_back({e, t.worldX, t.worldY, r.size.x, r.size.y, col.channel, col.type});
            }
        });

    std::unordered_set<std::pair<Entity, Entity>, PairHash> currentCollisions;

    for (size_t i = 0; i < collidables.size(); i++)
    {
        for (size_t j = i + 1; j < collidables.size(); j++)
        {
            auto &a = collidables[i];
            auto &b = collidables[j];

            if (a.channel != b.channel) continue;

            const bool overlapping =
                    a.x < b.x + b.w && a.x + a.w > b.x &&
                    a.y < b.y + b.h && a.y + a.h > b.y;

            if (!overlapping) continue;

            Entity e1 = std::min(a.id, b.id);
            Entity e2 = std::max(a.id, b.id);

            currentCollisions.emplace(e1, e2);

            const bool wasColliding = m_LastCollisions.count({e1, e2}) > 0;

            if (!wasColliding)
                EventManager::Get().FireCollision(a.id, b.id);
                
            if (a.type == CollisionType::Solid && b.type == CollisionType::Solid)
            {
                float aCenterX = a.x + a.w / 2.0f;
                float aCenterY = a.y + a.h / 2.0f;
                float bCenterX = b.x + b.w / 2.0f;
                float bCenterY = b.y + b.h / 2.0f;

                float dx = aCenterX - bCenterX;
                float dy = aCenterY - bCenterY;
                
                float overlapX = (a.w / 2.0f + b.w / 2.0f) - std::abs(dx);
                float overlapY = (a.h / 2.0f + b.h / 2.0f) - std::abs(dy);

                if (overlapX > 0 && overlapY > 0)
                {
                    if (!m_Registry.HasComponent<TransformComponent>(a.id) || !m_Registry.HasComponent<TransformComponent>(b.id))
                        continue; 

                    bool aMovable = m_Registry.HasComponent<VelocityComponent>(a.id);
                    bool bMovable = m_Registry.HasComponent<VelocityComponent>(b.id);
                    
                    if (overlapX < overlapY)
                    {
                        float pushX = (dx > 0) ? overlapX : -overlapX;
                        if (aMovable && !bMovable) {
                            m_Registry.GetComponent<TransformComponent>(a.id).x += pushX;
                            a.x += pushX;
                        } else if (!aMovable && bMovable) {
                            m_Registry.GetComponent<TransformComponent>(b.id).x -= pushX;
                            b.x -= pushX;
                        } else {
                            m_Registry.GetComponent<TransformComponent>(a.id).x += pushX / 2.0f;
                            a.x += pushX / 2.0f;
                            m_Registry.GetComponent<TransformComponent>(b.id).x -= pushX / 2.0f;
                            b.x -= pushX / 2.0f;
                        }
                    }
                    else
                    {
                        float pushY = (dy > 0) ? overlapY : -overlapY;
                        if (aMovable && !bMovable) {
                            m_Registry.GetComponent<TransformComponent>(a.id).y += pushY;
                            a.y += pushY;
                        } else if (!aMovable && bMovable) {
                            m_Registry.GetComponent<TransformComponent>(b.id).y -= pushY;
                            b.y -= pushY;
                        } else {
                            m_Registry.GetComponent<TransformComponent>(a.id).y += pushY / 2.0f;
                            a.y += pushY / 2.0f;
                            m_Registry.GetComponent<TransformComponent>(b.id).y -= pushY / 2.0f;
                            b.y -= pushY / 2.0f;
                        }
                    }
                }
            }
        }
    }

    m_LastCollisions = std::move(currentCollisions);
}

void GameScene::HandleEvent(const sf::Event &event)
{
#ifndef RAYNE_STANDALONE
    if (event.type == sf::Event::KeyPressed &&
        event.key.code == sf::Keyboard::Escape) { m_manager.SwitchSceneTo("editor"); }
#else
    if (event.type == sf::Event::KeyPressed &&
        event.key.code == sf::Keyboard::Escape) { m_Window.close(); }
#endif

    if (IsInputEvent(event))
    {
        EventManager::Get().FireInput(event);
    }
}

void GameScene::Update(float deltaTime)
{
    bool isPaused = (g_App && g_App->IsPaused());
    float timeScale = (g_App ? g_App->GetTimeScale() : 1.0f);
    float effectiveDt = isPaused ? 0.f : (deltaTime * timeScale);

    HierarchySystem::UpdateWorldTransforms(m_Registry);

#ifndef RAYNE_STANDALONE
    m_HotReloadTimer += deltaTime;
    if (m_HotReloadTimer >= 0.5f)
    {
        m_HotReloadTimer = 0.f;
        m_Registry.ForEach<ScriptComponent>([](Entity, ScriptComponent &sc) { sc.ReloadIfNeeded(); });
    }
#endif

    sf::View uiView(sf::FloatRect(0.f, 0.f, 1920.f, 1080.f));
    uiView.setViewport(sf::FloatRect(0.f, 0.f, 1.f, 1.f));

    sf::Vector2i pixelPos = sf::Mouse::getPosition(m_Window);
    sf::Vector2f mousePos = m_Window.mapPixelToCoords(pixelPos, uiView);

    bool mouseClicked = sf::Mouse::isButtonPressed(sf::Mouse::Left);
    static bool wasClicked = false;
    bool justClicked = mouseClicked && !wasClicked;
    bool justReleased = !mouseClicked && wasClicked;
    wasClicked = mouseClicked;

    UIManager::Get().Update(deltaTime, mousePos, justClicked, justReleased);

    m_Registry.ForEach<ScriptComponent>([effectiveDt](Entity, ScriptComponent &sc) { sc.OnUpdate(effectiveDt); });

    if (!isPaused)
    {
        TimerManager::Get().Update(effectiveDt);
        TweenManager::Get().Update(effectiveDt);
        PhysicsSystem::Step(m_Registry, effectiveDt);
        HierarchySystem::UpdateWorldTransforms(m_Registry);

        m_Registry.ForEach<TransformComponent, ParticleEmitterComponent>([effectiveDt](Entity, TransformComponent &t, ParticleEmitterComponent &pec) {
            if (!pec.emitting) return;
            pec.spawnAccumulator += effectiveDt;
            float spawnInterval = (pec.emissionRate > 0.001f) ? (1.0f / pec.emissionRate) : 1.0f;
            while (pec.spawnAccumulator >= spawnInterval && (int)pec.particles.size() < pec.maxParticles)
            {
                pec.spawnAccumulator -= spawnInterval;
                Particle p;
                p.position = sf::Vector2f(t.worldX, t.worldY);
                p.lifetime = 0.0f;
                p.maxLifetime = std::max(0.1f, pec.lifetime);
                p.size = pec.startSize;
                p.color = pec.startColor;

                float randSpread = ((float)(rand() % 1000) / 1000.f - 0.5f) * pec.spreadAngle;
                float finalAngleDeg = pec.angle + randSpread;
                float rad = finalAngleDeg * 3.14159265f / 180.f;
                float spd = pec.speed + ((float)(rand() % 1000) / 1000.f - 0.5f) * pec.speedVariance;
                p.velocity = sf::Vector2f(std::cos(rad) * spd, std::sin(rad) * spd);

                pec.particles.push_back(p);
            }

            for (auto &p : pec.particles)
            {
                p.lifetime += effectiveDt;
                p.velocity.x += pec.gravityX * effectiveDt;
                p.velocity.y += pec.gravityY * effectiveDt;
                p.position += p.velocity * effectiveDt;

                float ratio = std::min(1.0f, p.lifetime / p.maxLifetime);
                p.size = pec.startSize + ratio * (pec.endSize - pec.startSize);
                p.color.r = static_cast<sf::Uint8>(pec.startColor.r + ratio * (pec.endColor.r - pec.startColor.r));
                p.color.g = static_cast<sf::Uint8>(pec.startColor.g + ratio * (pec.endColor.g - pec.startColor.g));
                p.color.b = static_cast<sf::Uint8>(pec.startColor.b + ratio * (pec.endColor.b - pec.startColor.b));
                p.color.a = static_cast<sf::Uint8>(pec.startColor.a + ratio * (pec.endColor.a - pec.startColor.a));
            }

            std::erase_if(pec.particles, [](const Particle &p) {
                return p.lifetime >= p.maxLifetime;
            });
        });
    }

    CameraManager::Get().Update(effectiveDt, m_Registry);
    m_Camera = CameraManager::Get().GetView();

    UIManager::Get().ClearClickedButton();
}

void GameScene::Render(sf::RenderWindow &window)
{
    sf::View uiView(sf::FloatRect(0.f, 0.f, 1920.f, 1080.f));
    uiView.setViewport(sf::FloatRect(0.f, 0.f, 1.f, 1.f));

    m_Camera = CameraManager::Get().GetView();
    window.setView(m_Camera);

    struct RenderEntry {
        Entity e;
        TransformComponent t;
        RenderComponent r;
    };
    std::vector<RenderEntry> renderEntries;
    m_Registry.ForEach<TransformComponent, RenderComponent>(
        [&](Entity e, TransformComponent &t, RenderComponent &r) {
            renderEntries.push_back({e, t, r});
        });
    std::stable_sort(renderEntries.begin(), renderEntries.end(), [](const RenderEntry &a, const RenderEntry &b) {
        return a.r.zIndex < b.r.zIndex;
    });

    for (const auto &entry : renderEntries)
    {
        Entity e = entry.e;
        const auto &t = entry.t;
        const auto &r = entry.r;

        if (m_Registry.HasComponent<SpriteComponent>(e))
        {
            auto &sc = m_Registry.GetComponent<SpriteComponent>(e);
            sc.sprite.setPosition(t.worldX, t.worldY);
            sc.sprite.setRotation(t.worldRotation);
            
            float baseScaleX = 1.f, baseScaleY = 1.f;
            if (sc.texture) {
                auto texSize = sc.texture->getSize();
                if (texSize.x > 0 && texSize.y > 0) {
                    baseScaleX = sc.size.x / static_cast<float>(texSize.x);
                    baseScaleY = sc.size.y / static_cast<float>(texSize.y);
                }
            }
            sc.sprite.setScale(baseScaleX * t.worldScaleX, baseScaleY * t.worldScaleY);
            
            window.draw(sc.sprite);
        } else
        {
            if (r.shapeType == ShapeType::Rectangle)
            {
                sf::RectangleShape shape(r.size);
                shape.setPosition(t.worldX, t.worldY);
                shape.setRotation(t.worldRotation);
                shape.setScale(t.worldScaleX, t.worldScaleY);
                shape.setFillColor(r.color);
                window.draw(shape);
            } else
            {
                sf::CircleShape circle;
                switch (r.shapeType)
                {
                    case ShapeType::Triangle: circle.setPointCount(3);
                        break;
                    case ShapeType::Pentagon: circle.setPointCount(5);
                        break;
                    case ShapeType::Hexagon: circle.setPointCount(6);
                        break;
                    case ShapeType::Circle: default: circle.setPointCount(30);
                        break;
                }
                float rx = r.size.x * 0.5f;
                float ry = r.size.y * 0.5f;
                if (rx > 0.001f && ry > 0.001f)
                {
                    circle.setRadius(rx);
                    circle.setScale(t.worldScaleX, t.worldScaleY * (ry / rx));
                }
                else
                {
                    circle.setRadius(r.size.x / 2.f);
                    circle.setScale(t.worldScaleX, t.worldScaleY);
                }
                circle.setPosition(t.worldX, t.worldY);
                circle.setRotation(t.worldRotation);
                circle.setFillColor(r.color);
                window.draw(circle);
            }
        }
    }

    m_Registry.ForEach<TransformComponent, TextComponent>([&](Entity, TransformComponent &t, TextComponent &tc) {
        sf::Text txt;
        auto font = ResourceManager::Get().GetFont(tc.fontPath.empty() ? (ENGINE_ASSET_PATH "/fonts/Merriweather.ttf") : tc.fontPath);
        if (font) txt.setFont(*font);
        txt.setString(tc.text);
        txt.setCharacterSize(tc.characterSize);
        txt.setFillColor(tc.color);
        txt.setPosition(t.worldX, t.worldY);
        txt.setRotation(t.worldRotation);
        txt.setScale(t.worldScaleX, t.worldScaleY);
        if (tc.outlineThickness > 0.f) {
            txt.setOutlineColor(tc.outlineColor);
            txt.setOutlineThickness(tc.outlineThickness);
        }
        window.draw(txt);
    });

    m_Registry.ForEach<TransformComponent, ParticleEmitterComponent>([&](Entity, TransformComponent &, ParticleEmitterComponent &pec) {
        for (const auto &p : pec.particles) {
            sf::CircleShape pShape(p.size * 0.5f);
            pShape.setOrigin(p.size * 0.5f, p.size * 0.5f);
            pShape.setPosition(p.position);
            pShape.setFillColor(p.color);
            window.draw(pShape);
        }
    });

    window.setView(uiView);
    UIManager::Get().Render(window);

    window.setView(window.getDefaultView());
#ifndef RAYNE_STANDALONE
    m_DebugText.setString("GAME  |  Esc: back to editor");
    window.draw(m_DebugText);
#endif
}
