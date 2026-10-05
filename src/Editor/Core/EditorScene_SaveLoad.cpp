#include "EditorScene_Common.h"
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Window/Clipboard.hpp>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <map>
#include <unordered_set>


void EditorScene::SaveToJson(const std::string &path)
{
    std::cout << "[INFO] [EditorScene] Saving scene to " << path << " (" << m_Objects.size() << " objects)...\n";
    CommitActiveField();
    SyncToRegistry();

    std::string uiPath = path.substr(0, path.find_last_of('.')) + "_ui.json";
    UIManager::Get().SetCurrentUIPath(uiPath);
    if (!std::filesystem::exists(uiPath))
    {
        std::error_code ec;
        std::filesystem::path uip(uiPath);
        if (uip.has_parent_path()) std::filesystem::create_directories(uip.parent_path(), ec);
        std::ofstream uiFile(uiPath);
        if (uiFile.is_open())
        {
            uiFile << "{\n    \"ui_elements\": []\n}\n";
            uiFile.close();
        }
    }

    json data;
    data["name"] = "game";
    data["objects"] = json::array();

    for (auto &obj: m_Objects) { data["objects"].push_back(SerializeObject(obj)); }

    std::ofstream file(path);
    if (!file.is_open())
    {
        std::cerr << "[ERROR] [EditorScene] Failed to open file for saving: " << path << "\n";
        return;
    }
    file << data.dump(4);
    SetDirty(false);
    UpdateStatusText();
    std::cout << "[INFO] [EditorScene] Scene saved successfully to " << path << ".\n";
}


void EditorScene::LoadFromJson(const std::string &path)
{
    std::cout << "[INFO] [EditorScene] Loading scene from " << path << "...\n";
    std::string uiPath = path.substr(0, path.find_last_of('.')) + "_ui.json";
    UIManager::Get().SetCurrentUIPath(uiPath);
    if (!std::filesystem::exists(uiPath))
    {
        std::error_code ec;
        std::filesystem::path uip(uiPath);
        if (uip.has_parent_path()) std::filesystem::create_directories(uip.parent_path(), ec);
        std::ofstream uiFile(uiPath);
        if (uiFile.is_open())
        {
            uiFile << "{\n    \"ui_elements\": []\n}\n";
            uiFile.close();
        }
    }
    UIManager::Get().Load(uiPath);

    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "[ERROR] [EditorScene] Scene file not found or unreadable: " << path << "\n";
        return;
    }

    for (auto &obj: m_Objects)
        if (obj.entity != 0)
            m_Registry.DestroyEntity(obj.entity);
    m_Registry.Clear();
    m_Registry.SetEntityCounter(1);

    m_Objects.clear();
    ClearSelection();
    m_UndoStack.clear();
    m_RedoStack.clear();

    json data;
    try { data = json::parse(file); } catch (const std::exception &e)
    {
        std::cerr << "[ERROR] [EditorScene] Failed to parse scene JSON: " << e.what() << "\n";
        return;
    }

    if (!data.contains("objects") || !data["objects"].is_array())
    {
        std::cerr << "[WARN] [EditorScene] Scene file has no 'objects' array: " << path << "\n";
        return;
    }

    for (auto &j: data["objects"])
    {
        EditorObject obj;
        obj.id = j["id"];
        if (j.contains("tag")) obj.tag = j["tag"];
        obj.parentId = j.value("parent", "");
        obj.color = sf::Color(j["color"][0], j["color"][1], j["color"][2]);
        obj.shape.setSize({j["width"], j["height"]});
        obj.localPosition = {j.value("x", 0.f), j.value("y", 0.f)};
        obj.shape.setPosition(obj.localPosition);
        if (j.contains("rotation")) obj.rotation = j["rotation"];
        if (j.contains("scaleX")) obj.scaleX = j["scaleX"];
        if (j.contains("scaleY")) obj.scaleY = j["scaleY"];
        obj.shape.setRotation(obj.rotation);
        obj.shape.setScale(obj.scaleX, obj.scaleY);
        obj.shape.setFillColor(obj.color);

        const std::string typeStr = j.value("type", "rectangle");
        if (typeStr == "circle") obj.objectType = ObjectType::Circle;
        else if (typeStr == "triangle") obj.objectType = ObjectType::Triangle;
        else if (typeStr == "pentagon") obj.objectType = ObjectType::Pentagon;
        else if (typeStr == "hexagon") obj.objectType = ObjectType::Hexagon;
        else if (typeStr == "sprite") obj.objectType = ObjectType::Sprite;
        else if (typeStr == "camera") obj.objectType = ObjectType::Camera;
        else if (typeStr == "empty") obj.objectType = ObjectType::Empty;
        else if (typeStr == "spawn_point") obj.objectType = ObjectType::SpawnPoint;
        else if (typeStr == "trigger_zone") obj.objectType = ObjectType::TriggerZone;
        else if (typeStr == "physics_box") obj.objectType = ObjectType::PhysicsBox;
        else if (typeStr == "physics_ball") obj.objectType = ObjectType::PhysicsBall;
        else if (typeStr == "static_platform") obj.objectType = ObjectType::StaticPlatform;
        else if (typeStr == "world_text") obj.objectType = ObjectType::WorldText;
        else if (typeStr == "audio_source") obj.objectType = ObjectType::AudioSource;
        else if (typeStr == "particle_emitter") obj.objectType = ObjectType::ParticleEmitter;
        else obj.objectType = ObjectType::Rectangle;

        bool defVisible = !(obj.objectType == ObjectType::SpawnPoint || obj.objectType == ObjectType::AudioSource || obj
                            .objectType == ObjectType::ParticleEmitter || obj.objectType == ObjectType::Camera || obj.
                            objectType == ObjectType::Empty || obj.objectType == ObjectType::TriggerZone);
        obj.visibleInGame = j.value("visibleInGame", defVisible);

        if (IsPolygonType(obj.objectType))
        {
            obj.circleShape.setPointCount(GetPolygonPointCount(obj.objectType));
            float rx = obj.shape.getSize().x * 0.5f;
            float ry = obj.shape.getSize().y * 0.5f;
            if (rx > 0.001f && ry > 0.001f)
            {
                obj.circleShape.setRadius(rx);
                obj.circleShape.setScale(obj.scaleX, obj.scaleY * (ry / rx));
            }
            obj.circleShape.setPosition(obj.shape.getPosition());
            obj.circleShape.setRotation(obj.rotation);
            obj.circleShape.setFillColor(obj.color);
        }

        Entity desiredId = j.value("entity", static_cast<Entity>(0));
        obj.entity = m_Registry.CreateEntity(desiredId);

        TransformComponent t;
        t.x = j["x"];
        t.y = j["y"];
        t.rotation = obj.rotation;
        t.scaleX = obj.scaleX;
        t.scaleY = obj.scaleY;
        m_Registry.AddComponent(obj.entity, t);
        m_Registry.AddComponent(obj.entity, RenderComponent{
                                    obj.color, obj.shape.getSize(), MapToShapeType(obj.objectType), obj.zIndex,
                                    obj.visibleInGame
                                });
        if (!obj.id.empty()) { m_Registry.AddComponent(obj.entity, NameComponent{obj.id}); }
        if (!obj.tag.empty()) { m_Registry.AddComponent(obj.entity, TagComponent{obj.tag}); }

        if (j.contains("sprite"))
        {
            std::string sp = j["sprite"].get<std::string>();
            std::filesystem::path p(sp);
            if (!p.is_absolute()) { sp = (std::filesystem::path(ASSET_PATH) / p).string(); }
            ApplySpriteToObject(obj, sp);
        }

        if (j.contains("template"))
        {
            std::string tp = j["template"].get<std::string>();
            std::filesystem::path p(tp);
            if (!p.is_absolute()) { tp = (FindProjectRoot() / p).string(); }
            obj.templatePath = tp;
        }

        if (j.contains("velocity") && j["velocity"].is_object())
        {
            float dx = j["velocity"].value("dx", 0.f);
            float dy = j["velocity"].value("dy", 0.f);
            m_Registry.AddComponent(obj.entity, VelocityComponent{dx, dy});
        }

        if (j.contains("script"))
        {
            std::string sp = j["script"].get<std::string>();
            std::filesystem::path p(sp);
            if (!p.is_absolute()) { sp = (std::filesystem::path(ASSET_PATH) / p).string(); }
            auto &sc = m_Registry.AddComponent(obj.entity, ScriptComponent(LuaState::GetLua(), sp, obj.entity));
            sc.SetEntity(obj.entity);
            obj.scriptPath = sp;
            for (const auto &prop: sc.GetExportedProperties()) { obj.scriptProperties[prop.name] = prop; }
            if (j.contains("scriptProperties"))
            {
                for (auto it = j["scriptProperties"].begin(); it != j["scriptProperties"].end(); ++it)
                {
                    ScriptComponent::Property prop;
                    prop.name = it.key();
                    prop.type = static_cast<ScriptComponent::PropertyType>(it.value()["type"].get<int>());
                    if (prop.type == ScriptComponent::PropertyType::Int) prop.intVal = it.value()["value"].get<int>();
                    else if (prop.type == ScriptComponent::PropertyType::Float)
                        prop.floatVal = it.value()["value"].get<float>();
                    else if (prop.type == ScriptComponent::PropertyType::Bool)
                        prop.boolVal = it.value()["value"].get<bool>();
                    else if (prop.type == ScriptComponent::PropertyType::String || prop.type ==
                             ScriptComponent::PropertyType::Template ||
                             prop.type == ScriptComponent::PropertyType::Image || prop.type ==
                             ScriptComponent::PropertyType::Entity)
                        prop.stringVal = it.value()["value"].get<std::string>();
                    else if (prop.type == ScriptComponent::PropertyType::Vec2)
                    {
                        if (it.value()["value"].is_object())
                        {
                            prop.floatVal = it.value()["value"].value("x", 0.f);
                            prop.vec2Y = it.value()["value"].value("y", 0.f);
                        }
                    } else if (prop.type == ScriptComponent::PropertyType::Color)
                    {
                        if (it.value()["value"].is_object())
                        {
                            prop.colorR = it.value()["value"].value("r", 255);
                            prop.colorG = it.value()["value"].value("g", 255);
                            prop.colorB = it.value()["value"].value("b", 255);
                        }
                    }

                    obj.scriptProperties[prop.name] = prop;
                    sc.SetExportedProperty(prop);
                }
            }
        }

        if (j.contains("camera"))
        {
            CameraComponent cam{true};
            if (j["camera"].is_object())
            {
                cam.active = j["camera"].value("active", true);
                cam.smoothSpeed = j["camera"].value("smoothSpeed", 0.0f);
                cam.offsetX = j["camera"].value("offsetX", 0.0f);
                cam.offsetY = j["camera"].value("offsetY", 0.0f);
                cam.zoom = j["camera"].value("zoom", 1.0f);
                cam.priority = j["camera"].value("priority", 0);
                cam.multiFollowMode = static_cast<CameraMultiFollowMode>(j["camera"].value("multiFollowMode", 1));
                cam.minZoom = j["camera"].value("minZoom", 0.3f);
                cam.maxZoom = j["camera"].value("maxZoom", 3.0f);
                cam.autoFramePadding = j["camera"].value("autoFramePadding", 200.0f);
            }
            m_Registry.AddComponent(obj.entity, cam);
        } else if (obj.objectType == ObjectType::Camera)
        {
            CameraComponent cam{true, 0.0f, 0.0f, 0.0f, 1.0f, 0, CameraMultiFollowMode::Average, 0.3f, 3.0f, 200.0f};
            m_Registry.AddComponent(obj.entity, cam);
        }

        if (j.contains("collision"))
        {
            CollisionType cType = CollisionType::Solid;
            if (j["collision"].contains("type") && j["collision"]["type"] == "static")
                cType = CollisionType::Static;
            int ch = j["collision"].value("channel", 0);
            bool isTrig = j["collision"].value("isTrigger", false);
            ColliderShape shape = ColliderShape::Box;
            if (j["collision"].contains("shape") && j["collision"]["shape"] == "circle")
                shape = ColliderShape::Circle;
            else if (obj.objectType == ObjectType::Circle)
                shape = ColliderShape::Circle;
            CollisionComponent col{ch, cType, isTrig, shape};
            col.offsetX = j["collision"].value("offsetX", 0.0f);
            col.offsetY = j["collision"].value("offsetY", 0.0f);
            col.sizeX = j["collision"].value("sizeX", 0.0f);
            col.sizeY = j["collision"].value("sizeY", 0.0f);
            col.radius = j["collision"].value("radius", 0.0f);
            m_Registry.AddComponent(obj.entity, col);
        }

        if (j.contains("rigidbody"))
        {
            Rigidbody2DComponent rb;
            std::string bt = j["rigidbody"].value("bodyType", "dynamic");
            if (bt == "kinematic") rb.bodyType = BodyType::Kinematic;
            else if (bt == "static") rb.bodyType = BodyType::Static;
            else rb.bodyType = BodyType::Dynamic;

            rb.mass = j["rigidbody"].value("mass", 1.0f);
            rb.gravityScale = j["rigidbody"].value("gravityScale", 1.0f);
            rb.restitution = j["rigidbody"].value("restitution", 0.0f);
            rb.drag = j["rigidbody"].value("drag", 0.05f);
            rb.freezeRotation = j["rigidbody"].value("freezeRotation", true);
            m_Registry.AddComponent(obj.entity, rb);
            if (!m_Registry.HasComponent<VelocityComponent>(obj.entity))
                m_Registry.AddComponent(obj.entity, VelocityComponent{0.f, 0.f});
        }

        m_Registry.AddComponent(obj.entity, HierarchyComponent{});
        m_Objects.push_back(std::move(obj));
    }

    UpdateWorldTransforms();
    std::unordered_set<std::string> loadedTemplates;
    for (const auto &obj : m_Objects)
    {
        if (!obj.templatePath.empty())
        {
            loadedTemplates.insert(obj.templatePath);
        }
    }
    for (const auto &tmpl : loadedTemplates)
    {
        SyncTemplateInstances(tmpl);
    }

    SetDirty(false);
    UpdateStatusText();
    std::cout << "[INFO] [EditorScene] Scene loaded successfully from " << path << " (" << m_Objects.size() <<
            " objects).\n";
}


void EditorScene::SnapshotState()
{
    SyncToRegistry();
    m_PlayModeSnapshot = json{};
    m_PlayModeSnapshot["name"] = "snapshot";
    m_PlayModeSnapshot["uiPath"] = UIManager::Get().GetCurrentUIPath();
    m_PlayModeSnapshot["objects"] = json::array();

    for (auto &obj: m_Objects) { m_PlayModeSnapshot["objects"].push_back(SerializeObject(obj)); }

    m_SnapshotEntityCounter = m_Registry.GetEntityCounter();
    std::cout << "[INFO] [EditorScene] Play-mode snapshot created (" << m_Objects.size() << " objects).\n";
}


void EditorScene::RestoreSnapshot()
{
    std::cout << "[INFO] [EditorScene] Restoring play-mode snapshot...\n";

    for (auto &obj: m_Objects)
        if (obj.entity != 0)
            m_Registry.DestroyEntity(obj.entity);
    m_Registry.Clear();
    m_Registry.SetEntityCounter(1);

    m_Objects.clear();
    ClearSelection();

    if (m_PlayModeSnapshot.contains("uiPath"))
    {
        std::string originalUI = m_PlayModeSnapshot["uiPath"];
        UIManager::Get().SetCurrentUIPath(originalUI);
        UIManager::Get().Load(originalUI);
    } else
    {
        std::string currentUI = UIManager::Get().GetCurrentUIPath();
        if (!currentUI.empty()) { UIManager::Get().Load(currentUI); }
    }

    if (m_PlayModeSnapshot.contains("objects") && m_PlayModeSnapshot["objects"].is_array())
    {
        for (auto &j: m_PlayModeSnapshot["objects"]) { DeserializeObject(j); }
    }

    if (m_SnapshotEntityCounter > m_Registry.GetEntityCounter())
    {
        m_Registry.SetEntityCounter(m_SnapshotEntityCounter);
    }

    UpdateWorldTransforms();
    UpdateStatusText();
    std::cout << "[INFO] [EditorScene] Play-mode snapshot restored (" << m_Objects.size() << " objects).\n";
}


json EditorScene::SerializeObject(const EditorObject &obj) const
{
    json j;
    j["id"] = obj.id;
    j["entity"] = obj.entity;
    j["tag"] = obj.tag;
    j["parent"] = obj.parentId;
    std::string typeStr = "rectangle";
    if (obj.objectType == ObjectType::Circle) typeStr = "circle";
    else if (obj.objectType == ObjectType::Triangle) typeStr = "triangle";
    else if (obj.objectType == ObjectType::Pentagon) typeStr = "pentagon";
    else if (obj.objectType == ObjectType::Hexagon) typeStr = "hexagon";
    else if (obj.objectType == ObjectType::Sprite) typeStr = "sprite";
    else if (obj.objectType == ObjectType::Camera) typeStr = "camera";
    else if (obj.objectType == ObjectType::Empty) typeStr = "empty";
    else if (obj.objectType == ObjectType::SpawnPoint) typeStr = "spawn_point";
    else if (obj.objectType == ObjectType::TriggerZone) typeStr = "trigger_zone";
    else if (obj.objectType == ObjectType::PhysicsBox) typeStr = "physics_box";
    else if (obj.objectType == ObjectType::PhysicsBall) typeStr = "physics_ball";
    else if (obj.objectType == ObjectType::StaticPlatform) typeStr = "static_platform";
    else if (obj.objectType == ObjectType::WorldText) typeStr = "world_text";
    else if (obj.objectType == ObjectType::AudioSource) typeStr = "audio_source";
    else if (obj.objectType == ObjectType::ParticleEmitter) typeStr = "particle_emitter";
    j["type"] = typeStr;
    j["x"] = obj.localPosition.x;
    j["y"] = obj.localPosition.y;
    j["rotation"] = obj.rotation;
    j["scaleX"] = obj.scaleX;
    j["scaleY"] = obj.scaleY;
    j["zIndex"] = obj.zIndex;
    j["visibleInGame"] = obj.visibleInGame;
    j["width"] = obj.shape.getSize().x;
    j["height"] = obj.shape.getSize().y;

    j["color"] = {obj.color.r, obj.color.g, obj.color.b};

    if (!obj.spritePath.empty())
    {
        std::error_code ec;
        std::filesystem::path p(obj.spritePath);
        std::filesystem::path root = std::filesystem::absolute(ASSET_PATH, ec);
        std::string rel = std::filesystem::proximate(p, root, ec).generic_string();
        j["sprite"] = ec ? obj.spritePath : rel;
    }

    if (!obj.templatePath.empty())
    {
        std::error_code ec;
        std::filesystem::path p(obj.templatePath);
        std::filesystem::path root = FindProjectRoot();
        std::string rel = std::filesystem::proximate(p, root, ec).generic_string();
        j["template"] = ec ? obj.templatePath : rel;
    }

    if (obj.entity != 0 && m_Registry.HasComponent<VelocityComponent>(obj.entity))
    {
        auto &vel = m_Registry.GetComponent<VelocityComponent>(obj.entity);
        j["velocity"] = {{"dx", vel.dx}, {"dy", vel.dy}};
    }

    if (!obj.scriptPath.empty())
    {
        std::error_code ec;
        std::filesystem::path p(obj.scriptPath);
        std::filesystem::path root = std::filesystem::absolute(ASSET_PATH, ec);
        std::string rel = std::filesystem::proximate(p, root, ec).generic_string();
        j["script"] = ec ? obj.scriptPath : rel;

        if (!obj.scriptProperties.empty())
        {
            json propsJson;
            for (const auto &pair: obj.scriptProperties)
            {
                const auto &prop = pair.second;
                json pJson;
                pJson["type"] = static_cast<int>(prop.type);
                if (prop.type == ScriptComponent::PropertyType::Int) pJson["value"] = prop.intVal;
                else if (prop.type == ScriptComponent::PropertyType::Float) pJson["value"] = prop.floatVal;
                else if (prop.type == ScriptComponent::PropertyType::Bool) pJson["value"] = prop.boolVal;
                else if (prop.type == ScriptComponent::PropertyType::String || prop.type ==
                         ScriptComponent::PropertyType::Template ||
                         prop.type == ScriptComponent::PropertyType::Image || prop.type ==
                         ScriptComponent::PropertyType::Entity)
                    pJson["value"] = prop.stringVal;
                else if (prop.type == ScriptComponent::PropertyType::Vec2)
                    pJson["value"] = {{"x", prop.floatVal}, {"y", prop.vec2Y}};
                else if (prop.type == ScriptComponent::PropertyType::Color)
                    pJson["value"] = {{"r", prop.colorR}, {"g", prop.colorG}, {"b", prop.colorB}};
                propsJson[pair.first] = pJson;
            }
            j["scriptProperties"] = propsJson;
        }
    }

    if (obj.entity != 0 && m_Registry.HasComponent<CameraComponent>(obj.entity))
    {
        auto &cam = m_Registry.GetComponent<CameraComponent>(obj.entity);
        j["camera"] = {
            {"active", cam.active},
            {"smoothSpeed", cam.smoothSpeed},
            {"offsetX", cam.offsetX},
            {"offsetY", cam.offsetY},
            {"zoom", cam.zoom},
            {"priority", cam.priority},
            {"multiFollowMode", static_cast<int>(cam.multiFollowMode)},
            {"minZoom", cam.minZoom},
            {"maxZoom", cam.maxZoom},
            {"autoFramePadding", cam.autoFramePadding}
        };
    }

    if (obj.entity != 0 && m_Registry.HasComponent<CollisionComponent>(obj.entity))
    {
        auto &col = m_Registry.GetComponent<CollisionComponent>(obj.entity);
        j["collision"] = {
            {"channel", col.channel},
            {"type", col.type == CollisionType::Solid ? "solid" : "static"},
            {"isTrigger", col.isTrigger},
            {"shape", col.shape == ColliderShape::Circle ? "circle" : "box"},
            {"offsetX", col.offsetX},
            {"offsetY", col.offsetY},
            {"sizeX", col.sizeX},
            {"sizeY", col.sizeY},
            {"radius", col.radius}
        };
    }

    if (obj.entity != 0 && m_Registry.HasComponent<Rigidbody2DComponent>(obj.entity))
    {
        auto &rb = m_Registry.GetComponent<Rigidbody2DComponent>(obj.entity);
        std::string bt = "dynamic";
        if (rb.bodyType == BodyType::Kinematic) bt = "kinematic";
        else if (rb.bodyType == BodyType::Static) bt = "static";
        j["rigidbody"] = {
            {"bodyType", bt},
            {"mass", rb.mass},
            {"gravityScale", rb.gravityScale},
            {"restitution", rb.restitution},
            {"drag", rb.drag},
            {"freezeRotation", rb.freezeRotation}
        };
    }

    if (obj.entity != 0 && m_Registry.HasComponent<TextComponent>(obj.entity))
    {
        auto &tc = m_Registry.GetComponent<TextComponent>(obj.entity);
        j["text"] = {
            {"text", tc.text},
            {"size", tc.characterSize},
            {"align", tc.alignment},
            {"color", {tc.color.r, tc.color.g, tc.color.b}},
            {"outlineThickness", tc.outlineThickness}
        };
    } else if (obj.objectType == ObjectType::WorldText)
    {
        j["text"] = {
            {"text", obj.textString},
            {"size", obj.textFontSize},
            {"align", obj.textAlignment},
            {"color", {obj.textColor.r, obj.textColor.g, obj.textColor.b}},
            {"outlineThickness", 0.0f}
        };
    }

    if (obj.entity != 0 && m_Registry.HasComponent<AudioSourceComponent>(obj.entity))
    {
        auto &ac = m_Registry.GetComponent<AudioSourceComponent>(obj.entity);
        j["audioSource"] = {
            {"soundPath", ac.soundPath},
            {"volume", ac.volume},
            {"pitch", ac.pitch},
            {"loop", ac.loop},
            {"playOnStart", ac.playOnStart},
            {"isSpatial", ac.isSpatial},
            {"minDistance", ac.minDistance},
            {"attenuation", ac.attenuation}
        };
    } else if (obj.objectType == ObjectType::AudioSource)
    {
        j["audioSource"] = {
            {"soundPath", obj.audioClipPath},
            {"volume", obj.audioVolume},
            {"pitch", obj.audioPitch},
            {"loop", obj.audioLoop},
            {"playOnStart", obj.audioPlayOnStart},
            {"isSpatial", obj.audioIsSpatial},
            {"minDistance", 150.0f},
            {"attenuation", 1.0f}
        };
    }

    if (obj.entity != 0 && m_Registry.HasComponent<ParticleEmitterComponent>(obj.entity))
    {
        auto &pec = m_Registry.GetComponent<ParticleEmitterComponent>(obj.entity);
        j["particleEmitter"] = {
            {"emitting", pec.emitting},
            {"maxParticles", pec.maxParticles},
            {"rate", pec.emissionRate},
            {"lifetime", pec.lifetime},
            {"speed", pec.speed},
            {"speedVariance", pec.speedVariance},
            {"angle", pec.angle},
            {"spread", pec.spreadAngle},
            {"startSize", pec.startSize},
            {"endSize", pec.endSize},
            {"gravityX", pec.gravityX},
            {"gravityY", pec.gravityY},
            {"startColor", {pec.startColor.r, pec.startColor.g, pec.startColor.b}},
            {"endColor", {pec.endColor.r, pec.endColor.g, pec.endColor.b}}
        };
    } else if (obj.objectType == ObjectType::ParticleEmitter)
    {
        j["particleEmitter"] = {
            {"emitting", obj.particleEmitting},
            {"maxParticles", obj.particleMaxParticles},
            {"rate", obj.particleRate},
            {"lifetime", obj.particleLifetime},
            {"speed", obj.particleSpeed},
            {"speedVariance", 40.0f},
            {"angle", obj.particleAngle},
            {"spread", obj.particleSpread},
            {"startSize", obj.particleStartSize},
            {"endSize", obj.particleEndSize},
            {"gravityX", obj.particleGravityX},
            {"gravityY", obj.particleGravityY},
            {"startColor", {obj.particleStartColor.r, obj.particleStartColor.g, obj.particleStartColor.b}},
            {"endColor", {obj.particleEndColor.r, obj.particleEndColor.g, obj.particleEndColor.b}}
        };
    }

    if (obj.entity != 0 && m_Registry.HasComponent<SpriteAnimationComponent>(obj.entity))
    {
        auto &anim = m_Registry.GetComponent<SpriteAnimationComponent>(obj.entity);
        json aJson;
        aJson["columns"] = anim.columns;
        aJson["rows"] = anim.rows;
        aJson["frameWidth"] = anim.frameWidth;
        aJson["frameHeight"] = anim.frameHeight;
        aJson["currentClip"] = anim.currentClip;
        aJson["currentFrame"] = anim.currentFrame;
        aJson["speed"] = anim.playbackSpeed;
        aJson["isPlaying"] = anim.isPlaying;

        json clipsArr = json::array();
        for (const auto &c : anim.clips)
        {
            clipsArr.push_back({
                {"name", c.name},
                {"start", c.startFrame},
                {"count", c.frameCount},
                {"fps", c.fps},
                {"loop", c.loop}
            });
        }
        aJson["clips"] = clipsArr;
        j["animation"] = aJson;
    }

    return j;
}


void EditorScene::DeserializeObject(const json &j)
{
    EditorObject obj;
    obj.id = j["id"];
    if (j.contains("tag")) obj.tag = j["tag"];
    obj.parentId = j.value("parent", "");
    obj.color = sf::Color(j["color"][0], j["color"][1], j["color"][2]);
    obj.shape.setSize({j["width"], j["height"]});
    obj.localPosition = {j.value("x", 0.f), j.value("y", 0.f)};
    obj.shape.setPosition(obj.localPosition);
    if (j.contains("rotation")) obj.rotation = j["rotation"];
    if (j.contains("scaleX")) obj.scaleX = j["scaleX"];
    if (j.contains("scaleY")) obj.scaleY = j["scaleY"];
    obj.zIndex = j.value("zIndex", 0);
    obj.shape.setRotation(obj.rotation);
    obj.shape.setScale(obj.scaleX, obj.scaleY);
    obj.shape.setFillColor(obj.color);

    const std::string typeStr = j.value("type", "rectangle");
    if (typeStr == "circle") obj.objectType = ObjectType::Circle;
    else if (typeStr == "triangle") obj.objectType = ObjectType::Triangle;
    else if (typeStr == "pentagon") obj.objectType = ObjectType::Pentagon;
    else if (typeStr == "hexagon") obj.objectType = ObjectType::Hexagon;
    else if (typeStr == "sprite") obj.objectType = ObjectType::Sprite;
    else if (typeStr == "camera") obj.objectType = ObjectType::Camera;
    else if (typeStr == "empty") obj.objectType = ObjectType::Empty;
    else if (typeStr == "spawn_point") obj.objectType = ObjectType::SpawnPoint;
    else if (typeStr == "trigger_zone") obj.objectType = ObjectType::TriggerZone;
    else if (typeStr == "physics_box") obj.objectType = ObjectType::PhysicsBox;
    else if (typeStr == "physics_ball") obj.objectType = ObjectType::PhysicsBall;
    else if (typeStr == "static_platform") obj.objectType = ObjectType::StaticPlatform;
    else if (typeStr == "world_text") obj.objectType = ObjectType::WorldText;
    else if (typeStr == "audio_source") obj.objectType = ObjectType::AudioSource;
    else if (typeStr == "particle_emitter") obj.objectType = ObjectType::ParticleEmitter;
    else obj.objectType = ObjectType::Rectangle;

    bool defVisible = !(obj.objectType == ObjectType::SpawnPoint || obj.objectType == ObjectType::AudioSource || obj.
                        objectType == ObjectType::ParticleEmitter || obj.objectType == ObjectType::Camera || obj.
                        objectType == ObjectType::Empty || obj.objectType == ObjectType::TriggerZone);
    obj.visibleInGame = j.value("visibleInGame", defVisible);

    if (IsPolygonType(obj.objectType))
    {
        obj.circleShape.setPointCount(GetPolygonPointCount(obj.objectType));
        float rx = obj.shape.getSize().x * 0.5f;
        float ry = obj.shape.getSize().y * 0.5f;
        if (rx > 0.001f && ry > 0.001f)
        {
            obj.circleShape.setRadius(rx);
            obj.circleShape.setScale(obj.scaleX, obj.scaleY * (ry / rx));
        }
        obj.circleShape.setPosition(obj.shape.getPosition());
        obj.circleShape.setRotation(obj.rotation);
        obj.circleShape.setFillColor(obj.color);
    }

    Entity desiredId = j.value("entity", static_cast<Entity>(0));
    obj.entity = m_Registry.CreateEntity(desiredId);

    TransformComponent t;
    t.x = j["x"];
    t.y = j["y"];
    t.rotation = obj.rotation;
    t.scaleX = obj.scaleX;
    t.scaleY = obj.scaleY;
    m_Registry.AddComponent(obj.entity, t);
    m_Registry.AddComponent(obj.entity, RenderComponent{
                                obj.color, obj.shape.getSize(), MapToShapeType(obj.objectType), obj.zIndex,
                                obj.visibleInGame
                            });
    if (!obj.id.empty()) { m_Registry.AddComponent(obj.entity, NameComponent{obj.id}); }
    if (!obj.tag.empty()) { m_Registry.AddComponent(obj.entity, TagComponent{obj.tag}); }

    if (j.contains("sprite"))
    {
        std::string sp = j["sprite"].get<std::string>();
        std::filesystem::path p(sp);
        if (!p.is_absolute()) { sp = (std::filesystem::path(ASSET_PATH) / p).string(); }
        ApplySpriteToObject(obj, sp);
    }

    if (j.contains("template"))
    {
        std::string tp = j["template"].get<std::string>();
        std::filesystem::path p(tp);
        if (!p.is_absolute()) { tp = (FindProjectRoot() / p).string(); }
        obj.templatePath = tp;
    }

    if (j.contains("velocity") && j["velocity"].is_object())
    {
        float dx = j["velocity"].value("dx", 0.f);
        float dy = j["velocity"].value("dy", 0.f);
        m_Registry.AddComponent(obj.entity, VelocityComponent{dx, dy});
    }

    if (j.contains("script"))
    {
        std::string sp = j["script"].get<std::string>();
        std::filesystem::path p(sp);
        if (!p.is_absolute()) { sp = (std::filesystem::path(ASSET_PATH) / p).string(); }
        auto &sc = m_Registry.AddComponent(obj.entity, ScriptComponent(LuaState::GetLua(), sp, obj.entity));
        sc.SetEntity(obj.entity);
        obj.scriptPath = sp;
        for (const auto &prop: sc.GetExportedProperties()) { obj.scriptProperties[prop.name] = prop; }
        if (j.contains("scriptProperties"))
        {
            for (auto it = j["scriptProperties"].begin(); it != j["scriptProperties"].end(); ++it)
            {
                ScriptComponent::Property prop;
                prop.name = it.key();
                prop.type = static_cast<ScriptComponent::PropertyType>(it.value()["type"].get<int>());
                if (prop.type == ScriptComponent::PropertyType::Int) prop.intVal = it.value()["value"].get<int>();
                else if (prop.type == ScriptComponent::PropertyType::Float)
                    prop.floatVal = it.value()["value"].get<float>();
                else if (prop.type == ScriptComponent::PropertyType::Bool)
                    prop.boolVal = it.value()["value"].get<bool>();
                else if (prop.type == ScriptComponent::PropertyType::String || prop.type ==
                         ScriptComponent::PropertyType::Template ||
                         prop.type == ScriptComponent::PropertyType::Image || prop.type ==
                         ScriptComponent::PropertyType::Entity)
                    prop.stringVal = it.value()["value"].get<std::string>();
                else if (prop.type == ScriptComponent::PropertyType::Vec2)
                {
                    if (it.value()["value"].is_object())
                    {
                        prop.floatVal = it.value()["value"].value("x", 0.f);
                        prop.vec2Y = it.value()["value"].value("y", 0.f);
                    }
                } else if (prop.type == ScriptComponent::PropertyType::Color)
                {
                    if (it.value()["value"].is_object())
                    {
                        prop.colorR = it.value()["value"].value("r", 255);
                        prop.colorG = it.value()["value"].value("g", 255);
                        prop.colorB = it.value()["value"].value("b", 255);
                    }
                }

                obj.scriptProperties[prop.name] = prop;
                sc.SetExportedProperty(prop);
            }
        }
    }

    if (j.contains("camera"))
    {
        CameraComponent cam{true};
        if (j["camera"].is_object())
        {
            cam.active = j["camera"].value("active", true);
            cam.smoothSpeed = j["camera"].value("smoothSpeed", 0.0f);
            cam.offsetX = j["camera"].value("offsetX", 0.0f);
            cam.offsetY = j["camera"].value("offsetY", 0.0f);
            cam.zoom = j["camera"].value("zoom", 1.0f);
            cam.priority = j["camera"].value("priority", 0);
            cam.multiFollowMode = static_cast<CameraMultiFollowMode>(j["camera"].value("multiFollowMode", 1));
            cam.minZoom = j["camera"].value("minZoom", 0.3f);
            cam.maxZoom = j["camera"].value("maxZoom", 3.0f);
            cam.autoFramePadding = j["camera"].value("autoFramePadding", 200.0f);
        }
        m_Registry.AddComponent(obj.entity, cam);
    } else if (obj.objectType == ObjectType::Camera)
    {
        CameraComponent cam{true, 0.0f, 0.0f, 0.0f, 1.0f, 0, CameraMultiFollowMode::Average, 0.3f, 3.0f, 200.0f};
        m_Registry.AddComponent(obj.entity, cam);
    }

    if (j.contains("collision"))
    {
        CollisionType cType = CollisionType::Solid;
        if (j["collision"].contains("type") && j["collision"]["type"] == "static")
            cType = CollisionType::Static;
        int ch = j["collision"].value("channel", 0);
        bool isTrig = j["collision"].value("isTrigger", false);
        ColliderShape shape = ColliderShape::Box;
        if (j["collision"].contains("shape") && j["collision"]["shape"] == "circle")
            shape = ColliderShape::Circle;
        else if (obj.objectType == ObjectType::Circle)
            shape = ColliderShape::Circle;
        CollisionComponent col{ch, cType, isTrig, shape};
        col.offsetX = j["collision"].value("offsetX", 0.0f);
        col.offsetY = j["collision"].value("offsetY", 0.0f);
        col.sizeX = j["collision"].value("sizeX", 0.0f);
        col.sizeY = j["collision"].value("sizeY", 0.0f);
        col.radius = j["collision"].value("radius", 0.0f);
        m_Registry.AddComponent(obj.entity, col);
    }

    if (j.contains("rigidbody"))
    {
        Rigidbody2DComponent rb;
        std::string bt = j["rigidbody"].value("bodyType", "dynamic");
        if (bt == "kinematic") rb.bodyType = BodyType::Kinematic;
        else if (bt == "static") rb.bodyType = BodyType::Static;
        else rb.bodyType = BodyType::Dynamic;

        rb.mass = j["rigidbody"].value("mass", 1.0f);
        rb.gravityScale = j["rigidbody"].value("gravityScale", 1.0f);
        rb.restitution = j["rigidbody"].value("restitution", 0.0f);
        rb.drag = j["rigidbody"].value("drag", 0.05f);
        rb.freezeRotation = j["rigidbody"].value("freezeRotation", true);
        m_Registry.AddComponent(obj.entity, rb);
        if (!m_Registry.HasComponent<VelocityComponent>(obj.entity))
            m_Registry.AddComponent(obj.entity, VelocityComponent{0.f, 0.f});
    }

    if (j.contains("text") && (j["text"].is_object() || j["text"].is_string()))
    {
        TextComponent tc;
        if (j["text"].is_string()) { tc.text = j["text"].get<std::string>(); } else
        {
            tc.text = j["text"].value("text", "World Text");
            tc.characterSize = j["text"].value("size", 28u);
            tc.alignment = j["text"].value("align", 0);
            if (j["text"].contains("color") && j["text"]["color"].is_array() && j["text"]["color"].size() >= 3)
            {
                tc.color = sf::Color(j["text"]["color"][0], j["text"]["color"][1], j["text"]["color"][2]);
            }
            tc.outlineThickness = j["text"].value("outlineThickness", 0.0f);
        }
        obj.textString = tc.text;
        obj.textFontSize = tc.characterSize;
        obj.textColor = tc.color;
        obj.textAlignment = tc.alignment;
        m_Registry.AddComponent(obj.entity, tc);
    }

    if (j.contains("audioSource") && j["audioSource"].is_object())
    {
        AudioSourceComponent ac;
        ac.soundPath = j["audioSource"].value("soundPath", "");
        ac.volume = j["audioSource"].value("volume", 100.0f);
        ac.pitch = j["audioSource"].value("pitch", 1.0f);
        ac.loop = j["audioSource"].value("loop", false);
        ac.playOnStart = j["audioSource"].value("playOnStart", true);
        ac.isSpatial = j["audioSource"].value("isSpatial", false);
        ac.minDistance = j["audioSource"].value("minDistance", 150.0f);
        ac.attenuation = j["audioSource"].value("attenuation", 1.0f);
        obj.audioClipPath = ac.soundPath;
        obj.audioVolume = ac.volume;
        obj.audioPitch = ac.pitch;
        obj.audioLoop = ac.loop;
        obj.audioPlayOnStart = ac.playOnStart;
        obj.audioIsSpatial = ac.isSpatial;
        m_Registry.AddComponent(obj.entity, ac);
    }

    if (j.contains("particleEmitter") && j["particleEmitter"].is_object())
    {
        ParticleEmitterComponent pec;
        pec.emitting = j["particleEmitter"].value("emitting", true);
        pec.maxParticles = j["particleEmitter"].value("maxParticles", 120);
        pec.emissionRate = j["particleEmitter"].value("rate", 25.0f);
        pec.lifetime = j["particleEmitter"].value("lifetime", 1.5f);
        pec.speed = j["particleEmitter"].value("speed", 120.0f);
        pec.speedVariance = j["particleEmitter"].value("speedVariance", 40.0f);
        pec.angle = j["particleEmitter"].value("angle", -90.0f);
        pec.spreadAngle = j["particleEmitter"].value("spread", 45.0f);
        pec.startSize = j["particleEmitter"].value("startSize", 8.0f);
        pec.endSize = j["particleEmitter"].value("endSize", 2.0f);
        pec.gravityX = j["particleEmitter"].value("gravityX", 0.0f);
        pec.gravityY = j["particleEmitter"].value("gravityY", 60.0f);
        if (j["particleEmitter"].contains("startColor") && j["particleEmitter"]["startColor"].is_array() && j[
                "particleEmitter"]["startColor"].size() >= 3)
        {
            pec.startColor = sf::Color(j["particleEmitter"]["startColor"][0], j["particleEmitter"]["startColor"][1],
                                       j["particleEmitter"]["startColor"][2]);
        }
        if (j["particleEmitter"].contains("endColor") && j["particleEmitter"]["endColor"].is_array() && j[
                "particleEmitter"]["endColor"].size() >= 3)
        {
            pec.endColor = sf::Color(j["particleEmitter"]["endColor"][0], j["particleEmitter"]["endColor"][1],
                                     j["particleEmitter"]["endColor"][2]);
        }
        obj.particleEmitting = pec.emitting;
        obj.particleMaxParticles = pec.maxParticles;
        obj.particleRate = pec.emissionRate;
        obj.particleLifetime = pec.lifetime;
        obj.particleSpeed = pec.speed;
        obj.particleAngle = pec.angle;
        obj.particleSpread = pec.spreadAngle;
        obj.particleStartSize = pec.startSize;
        obj.particleEndSize = pec.endSize;
        obj.particleGravityX = pec.gravityX;
        obj.particleGravityY = pec.gravityY;
        obj.particleStartColor = pec.startColor;
        obj.particleEndColor = pec.endColor;
        m_Registry.AddComponent(obj.entity, pec);
    }

    if (j.contains("animation") && j["animation"].is_object())
    {
        const auto &aj = j["animation"];
        SpriteAnimationComponent anim;
        anim.columns = aj.value("columns", 1);
        anim.rows = aj.value("rows", 1);
        anim.frameWidth = aj.value("frameWidth", 0);
        anim.frameHeight = aj.value("frameHeight", 0);
        anim.currentClip = aj.value("currentClip", "default");
        anim.currentFrame = aj.value("currentFrame", 0);
        anim.playbackSpeed = aj.value("speed", 1.0f);
        anim.isPlaying = aj.value("isPlaying", true);

        if (aj.contains("clips") && aj["clips"].is_array() && !aj["clips"].empty())
        {
            anim.clips.clear();
            for (const auto &cj : aj["clips"])
            {
                AnimationClip clip;
                clip.name = cj.value("name", "default");
                clip.startFrame = cj.value("start", 0);
                clip.frameCount = cj.value("count", 1);
                clip.fps = cj.value("fps", 10.0f);
                clip.loop = cj.value("loop", true);
                anim.clips.push_back(clip);
            }
        }
        else
        {
            float fps = aj.value("fps", 10.0f);
            bool loop = aj.value("loop", true);
            anim.clips.clear();
            anim.clips.push_back(AnimationClip{"default", 0, anim.columns * anim.rows, fps, loop});
        }
        m_Registry.AddComponent(obj.entity, anim);
    }

    m_Registry.AddComponent(obj.entity, HierarchyComponent{});

    m_Objects.push_back(std::move(obj));
    UpdateWorldTransforms();
    UpdateStatusText();
}


void EditorScene::ApplyState(EditorObject &obj, const json &j)
{
    RemoveObject(obj.id);
    DeserializeObject(j);
}

