#ifndef EDITORCOMMAND_H
#define EDITORCOMMAND_H

#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>

class EditorScene;

class EditorCommand
{
public:
    virtual ~EditorCommand() = default;

    virtual void Execute(EditorScene *scene) = 0;

    virtual void Undo(EditorScene *scene) = 0;
};

class ObjectStateCommand : public EditorCommand
{
public:
    std::string objectId;
    nlohmann::json beforeState;
    nlohmann::json afterState;

    ObjectStateCommand(std::string id, nlohmann::json before, nlohmann::json after)
        : objectId(std::move(id)), beforeState(std::move(before)), afterState(std::move(after)) {}

    void Execute(EditorScene *scene) override;

    void Undo(EditorScene *scene) override;
};

class MacroCommand : public EditorCommand
{
public:
    std::vector<std::shared_ptr<EditorCommand> > commands;
    void Execute(EditorScene *scene) override { for (auto &c: commands) c->Execute(scene); }

    void Undo(EditorScene *scene) override
    {
        for (auto it = commands.rbegin(); it != commands.rend(); ++it) (*it)->Undo(scene);
    }
};

#endif
