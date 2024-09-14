#pragma once

#include "Core.h"
#include "Timestep.h"

#include "Events/Event.h"

struct ImGuiContext;

namespace Helios
{
	class HELIOSENGINE_API Layer
	{
	public:
		Layer(std::string_view name = "Layer");
		virtual ~Layer() = default;

		virtual void OnAttach() {}
		virtual void OnDetach() {}

		virtual void OnUpdate(Timestep ts) {}
		virtual void OnEvent(Event& event) {}

		virtual void Draw() {}

    virtual void OnImGuiRender(ImGuiContext* context) {}

		inline const std::string& GetName() const { return m_Name; }

	protected:
		std::string m_Name;
	};
}