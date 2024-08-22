#include "ShaderNode.h"

namespace Helios
{
  ShaderNode::ShaderNode(const std::string& name, NodeType type)
    : m_Name(name), m_Type(type)
  {
  }

  void ShaderNode::AddInputPort(const std::string& name, DataType type)
  {
    if (std::find_if(m_InputPorts.begin(), m_InputPorts.end(), [&name](const NodePort& port) {
      return port.name == name;
      }) != m_InputPorts.end())
    {
      CORE_ERROR("Input port with this name already exists!");
      return;
    }

    m_InputPorts.push_back(NodePort{ name, type });
  }

  void ShaderNode::AddOutputPort(const std::string& name, DataType type)
  {
    if (std::find_if(m_OutputPorts.begin(), m_OutputPorts.end(), [&name](const NodePort& port) {
      return port.name == name;
      }) != m_OutputPorts.end())
    {
      CORE_ERROR("Output port with this name already exists!");
      return;
    }

    m_OutputPorts.push_back(NodePort{ name, type });
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

    m_InputPorts.erase(std::remove_if(m_InputPorts.begin(), m_InputPorts.end(), [&name](const NodePort& port) {
      return port.name == name;
    }));
  }

  void ShaderNode::RemoveOutputPort(const std::string& name)
  {
    m_InputPorts.erase(std::remove_if(m_OutputPorts.begin(), m_OutputPorts.end(), [&name](const NodePort& port) {
      return port.name == name;
    }));
  }

  MathNode::MathNode(const std::string& name, Operation op)
    : ShaderNode(name, NodeType::Math), m_Operation(op)
  {
  }
}