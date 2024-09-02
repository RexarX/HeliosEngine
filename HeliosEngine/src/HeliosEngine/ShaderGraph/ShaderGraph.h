#pragma once

#include "ShaderNode.h"

namespace Helios
{
  struct Connection
  {
    std::string outputPortName;
    std::shared_ptr<ShaderNode> inputNode = nullptr;
    std::string inputPortName;

    bool operator==(const Connection& other) const
    {
      return outputPortName == other.outputPortName &&
             inputNode == other.inputNode &&
             inputPortName == other.inputPortName;
    }
  };

  class HELIOSENGINE_API ShaderGraph
  {
  public:
    ShaderGraph(std::string_view name);
    ~ShaderGraph() = default;

    void AddNode(const std::shared_ptr<ShaderNode>& node);
    void RemoveNode(const std::shared_ptr<ShaderNode>& node);

    void Connect(const std::shared_ptr<ShaderNode>& outputNode, const std::string& outputPortName,
                 const std::shared_ptr<ShaderNode>& inputNode, const std::string& inputPortName);

    void Disconnect(const std::shared_ptr<ShaderNode>& outputNode, const std::string& outputPortName,
                    const std::shared_ptr<ShaderNode>& inputNode, const std::string& inputPortName);

    std::string GenerateShader() const;

  private:
    std::string m_Name;

    std::vector<std::shared_ptr<ShaderNode>> m_Nodes;
    std::unordered_map<std::shared_ptr<ShaderNode>, std::vector<Connection>> m_Connections;
  };
}