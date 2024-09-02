#include "ShaderNode.h"

namespace Helios
{
  ShaderNode::ShaderNode(std::string_view name, NodeType type)
    : m_Name(name), m_Type(type)
  {
  }

  void ShaderNode::AddInputPort(const std::string& name, DataType type)
  {
    if (std::find_if(m_InputPorts.begin(), m_InputPorts.end(),
      [&name](const NodePort& port) { return port.name == name; })
      != m_InputPorts.end())
    {
      CORE_ERROR("Input port with this name already exists!");
      return;
    }

    m_InputPorts.emplace_back(name, type);
  }

  void ShaderNode::AddOutputPort(const std::string& name, DataType type)
  {
    if (std::find_if(m_OutputPorts.begin(), m_OutputPorts.end(),
      [&name](const NodePort& port) { return port.name == name; })
      != m_OutputPorts.end())
    {
      CORE_ERROR("Output port with this name already exists!");
      return;
    }

    m_OutputPorts.emplace_back(name, type);
  }

  void ShaderNode::RemoveInputPort(const std::string& name)
  {
    if (std::find_if(m_InputPorts.begin(), m_InputPorts.end(), [&name](const NodePort& port) {
      return port.name == name;
      }) == m_InputPorts.end())
    {
      CORE_ERROR("Input port with this name does not exist!");
      return;
    }

    std::erase_if(m_InputPorts, [&name](const NodePort& port) {
      return port.name == name;
    });
  }

  void ShaderNode::RemoveOutputPort(const std::string& name)
  {
    std::erase_if(m_OutputPorts, [&name](const NodePort& port) {
      return port.name == name;
    });
  }

  MathNode::MathNode(std::string_view name, Operation op)
    : ShaderNode(name, NodeType::Math), m_Operation(op)
  {
  }
}