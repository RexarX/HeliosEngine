#pragma once

#include "Types.h"

namespace Helios
{
  class HELIOSENGINE_API ShaderNode
  {
  public:
    ShaderNode(std::string_view name, NodeType type);
    virtual ~ShaderNode() = default;

    void AddInputPort(const std::string& name, DataType type);
    void AddOutputPort(const std::string& name, DataType type);

    void RemoveInputPort(const std::string& name);
    void RemoveOutputPort(const std::string& name);

    inline const std::string& GetName() const { return m_Name; }
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