#pragma once

namespace Helios {
  class ShaderNode;

  class HELIOSENGINE_API ShaderGraph {
  public:
    struct Connection {
      std::string_view outputPortName;
      std::shared_ptr<ShaderNode> inputNode = nullptr;
      std::string_view inputPortName;

      inline bool operator==(const Connection& other) const {
        return outputPortName == other.outputPortName && inputNode == other.inputNode
               && inputPortName == other.inputPortName;
      }
    };

    ShaderGraph(std::string_view name);
    ~ShaderGraph() = default;

    void AddNode(const std::shared_ptr<ShaderNode>& node);
    void RemoveNode(const std::shared_ptr<ShaderNode>& node);

    void Connect(const std::shared_ptr<ShaderNode>& outputNode, std::string_view outputPortName,
                 const std::shared_ptr<ShaderNode>& inputNode, std::string_view inputPortName);

    void Disconnect(const std::shared_ptr<ShaderNode>& outputNode, std::string_view outputPortName,
                    const std::shared_ptr<ShaderNode>& inputNode, std::string_view inputPortName);

    inline const std::string& GetName() const { return m_Name; }

    std::string GenerateShader() const;

  private:
    std::string m_Name;

    std::vector<std::shared_ptr<ShaderNode>> m_Nodes;
    std::unordered_map<std::shared_ptr<ShaderNode>, std::vector<Connection>> m_Connections;
  };
}