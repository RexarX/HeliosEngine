#pragma once

#include "Core.h"

#include "pch.h"

namespace Helios 
{
	class Layer;

	class HELIOSENGINE_API LayerStack
	{
	public:
		LayerStack() = default;
		~LayerStack();

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* overlay);
		void PopLayer(Layer* layer);
		void PopOverlay(Layer* overlay);

		inline std::vector<Layer*>::iterator begin() { return m_Layers.begin(); }
		inline std::vector<Layer*>::iterator end() { return m_Layers.end(); }

		inline std::vector<Layer*>::reverse_iterator rbegin() { return m_Layers.rbegin(); }
		inline std::vector<Layer*>::reverse_iterator rend() { return m_Layers.rend(); }

	private:
		std::vector<Layer*> m_Layers;
		uint32_t m_LayerInsertIndex = 0;
	};
}