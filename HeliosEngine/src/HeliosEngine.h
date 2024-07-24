#pragma once

#include "HeliosEngine/Core.h"
#include "HeliosEngine/Application.h"
#include "HeliosEngine/Layer.h"
#include "HeliosEngine/Input.h"
#include "HeliosEngine/KeyCodes.h"
#include "HeliosEngine/MouseButtonCodes.h"
#include "HeliosEngine/Log.h"
#include "HeliosEngine/Timestep.h"

#include "HeliosEngine/Scene/SceneManager.h"

#include "HeliosEngine/EntityComponentSystem/Systems/EventSystem.h"
#include "HeliosEngine/EntityComponentSystem/Systems/InputSystem.h"
#include "HeliosEngine/EntityComponentSystem/Systems/RenderingSystem.h"
#include "HeliosEngine/EntityComponentSystem/Systems/ScriptSystem.h"

#include "HeliosEngine/EntityComponentSystem/Components/Camera.h"
#include "HeliosEngine/EntityComponentSystem/Components/Renderable.h"
#include "HeliosEngine/EntityComponentSystem/Components/Transform.h"
#include "HeliosEngine/EntityComponentSystem/Components/Script.h"

#include "HeliosEngine/ImGui/ImGuiLayer.h"

#include "HeliosEngine/Events/Event.h"
#include "HeliosEngine/Events/ApplicationEvent.h"
#include "HeliosEngine/Events/InputEvent.h"
#include "HeliosEngine/Events/KeyEvent.h"
#include "HeliosEngine/Events/MouseEvent.h"