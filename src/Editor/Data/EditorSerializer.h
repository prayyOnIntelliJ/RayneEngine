#ifndef EDITORSERIALIZER_H
#define EDITORSERIALIZER_H

#include <string>

class EditorScene;

class EditorSerializer
{
public:
    explicit EditorSerializer(EditorScene *scene);

    ~EditorSerializer() = default;

    void SaveToJson(const std::string &path);

    void LoadFromJson(const std::string &path);

private:
    EditorScene *m_Scene = nullptr;
};

#endif
