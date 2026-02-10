//
// Created by Jannik Kluge on 10.02.26.
//

#include "Registry.h"

Entity Registry::CreateEntity()
{
    return m_EntityCounter++;
}
