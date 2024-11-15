#pragma once

#include "HeliosEngine/Core.h"
#include "HeliosEngine/Application.h"
#include "HeliosEngine/Layer.h"
#include "HeliosEngine/Input.h"
#include "HeliosEngine/KeyCodes.h"
#include "HeliosEngine/MouseButtonCodes.h"
#include "HeliosEngine/Log.h"
#include "HeliosEngine/Timestep.h"

#include "HeliosEngine/Scene/Scene.h"
#include "HeliosEngine/Scene/SceneManager.h"

#include "HeliosEngine/ImGui/ImGuiLayer.h"

#include "EntityComponentSystem/Entity.h"
#include "EntityComponentSystem/Entity.inl" // Workaround
#include "EntityComponentSystem/Components.h"

#include "HeliosEngine/EntityComponentSystem/Systems/ScriptSystem.h"

#include "HeliosEngine/Events/Event.h"
#include "HeliosEngine/Events/ApplicationEvent.h"
#include "HeliosEngine/Events/KeyEvent.h"
#include "HeliosEngine/Events/MouseEvent.h"