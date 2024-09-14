#include "ShaderGraph.h"

#include "ShaderNode.h"

namespace Helios
{
  ShaderGraph::ShaderGraph(std::string_view name)
    : m_Name(name)
  {
  }

  void ShaderGraph::AddNode(const std::shared_ptr<ShaderNode>& node)
  {
    if (node == nullptr) { CORE_ERROR("Attempting to add null node!"); return; }

    if (std::find(m_Nodes.begin(), m_Nodes.end(), node) != m_Nodes.end()) {
      CORE_ERROR("Node already exists in the graph!");
      return;
    }

    m_Nodes.push_back(node);
  }

  void ShaderGraph::RemoveNode(const std::shared_ptr<ShaderNode>& node)
  {
    if (node == nullptr) { CORE_ERROR("Attempting to remove null node!"); return; }

    auto nodeIt = std::find(m_Nodes.begin(), m_Nodes.end(), node);
    if (nodeIt == m_Nodes.end()) {
      CORE_ERROR("Attempting to remove a node not present in the graph!");
      return;
    }

    m_Connections.erase(node);
    for (auto& [outputNode, connections] : m_Connections) {
      std::erase_if(connections, [&node](const Connection& conn) {
        return conn.inputNode == node;
      });
    }

    for (auto it = m_Connections.begin(); it != m_Connections.end();) {
      if (it->second.empty()) { it = m_Connections.erase(it); }
      else { ++it; }
    }

    m_Nodes.erase(nodeIt);
  }

  void ShaderGraph::Connect(const std::shared_ptr<ShaderNode>& outputNode, const std::string& outputPortName,
                            const std::shared_ptr<ShaderNode>& inputNode, const std::string& inputPortName)
  {
    if (outputNode == nullptr || inputNode == nullptr) {
      CORE_ERROR("Attempting to connect null nodes!");
      return;
    }

    if (std::find(m_Nodes.begin(), m_Nodes.end(), outputNode) == m_Nodes.end() ||
        std::find(m_Nodes.begin(), m_Nodes.end(), inputNode) == m_Nodes.end()) {
      CORE_ERROR("Attempting to connect nodes not present in the graph!");
      return;
    }

    auto& inputPorts = inputNode->GetInputPorts();
    auto& outputPorts = outputNode->GetOutputPorts();

    auto outputPortIt = std::find_if(inputPorts.begin(), inputPorts.end(),
      [&outputPortName](const NodePort& port) { return port.name == outputPortName; });

    auto inputPortIt = std::find_if(outputPorts.begin(), outputPorts.end(),
      [&inputPortName](const NodePort& port) { return port.name == inputPortName; });

    if (outputPortIt == outputPorts.end() || inputPortIt == inputPorts.end()) {
      CORE_ERROR("Invalid port name!");
      return;
    }

    if (outputPortIt->type != inputPortIt->type) {
      CORE_ERROR("Incompatible port types!");
      return;
    }

    Connection newConnection{ outputPortName, inputNode, inputPortName };

    auto& connections = m_Connections[outputNode];
    if (std::find(connections.begin(), connections.end(), newConnection) != connections.end()) {
      return;
    }

    connections.push_back(newConnection);
  }

  void ShaderGraph::Disconnect(const std::shared_ptr<ShaderNode>& outputNode, const std::string& outputPortName,
                               const std::shared_ptr<ShaderNode>& inputNode, const std::string& inputPortName)
  {
    if (outputNode == nullptr || inputNode == nullptr) {
      CORE_ERROR("Attempting to disconnect null nodes!");
      return;
    }

    auto it = m_Connections.find(outputNode);
    if (it == m_Connections.end()) { return; }

    auto& connections = it->second;
    std::erase_if(connections, [&outputPortName, &inputNode, &inputPortName](const Connection& conn) {
      return conn.outputPortName == outputPortName &&
             conn.inputNode == inputNode &&
             conn.inputPortName == inputPortName;
    });

    if (connections.empty()) { m_Connections.erase(it); }
  }

  std::string ShaderGraph::GenerateShader() const
  {
    std::string code;
    


    return code;
  }
}