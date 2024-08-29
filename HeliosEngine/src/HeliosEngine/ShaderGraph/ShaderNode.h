#pragma once

#include "Types.h"

namespace Helios
{
  class HELIOSENGINE_API ShaderNode
  {
  public:
    ShaderNode(std::string_view name, NodeType type);
    virtual ~ShaderNode() = default;

    void AddInputPort(std::string_view name, DataType type);
    void AddOutputPort(std::string_view name, DataType type);

    void RemoveInputPort(std::string_view name);
    void RemoveOutputPort(std::string_view name);

    inline std::string_view GetName() const { return m_Name; }
    inline NodeType GetType() const { return m_Type; }

    inline const std::vector<NodePort>& GetInputPorts() const { return m_InputPorts; }
    inline const std::vector<NodePort>& GetOutputPorts() const { return m_OutputPorts; }

  protected:
    std::string m_Name;
    NodeType m_Type;
    std::vector<NodePort> m_InputPorts;
    std::vector<NodePort> m_OutputPorts;
  };

  class HELIOSENGINE_API MathNode : public ShaderNode
  {
  public:
    enum class Operation { Add, Subtract, Multiply, Divide };

    MathNode(std::string_view name, Operation op);
    virtual ~MathNode() = default;
  
  protected:
    Operation m_Operation;
  };
}