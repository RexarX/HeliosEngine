#pragma once

#include "pch.h"

namespace Helios {
  class HELIOSENGINE_API ShaderNode {
  public:
    enum class DataType {
      Int, Int2, Int3, Int4,
      Float, Vec2, Vec3, Vec4,
      Mat3, Mat4,
      Bool,
      Sampler2D
    };

    enum class Type {
      Input,
      Output,
      Math,
      Texture2D
    };

    struct Port {
      std::string name;
      DataType type;
    };

    ShaderNode(std::string_view name, Type type);
    virtual ~ShaderNode() = default;

    void AddInputPort(std::string_view name, DataType type);
    void AddOutputPort(std::string_view name, DataType type);

    void RemoveInputPort(std::string_view name);
    void RemoveOutputPort(std::string_view name);

    inline const std::string& GetName() const { return m_Name; }
    inline Type GetType() const { return m_Type; }

    inline const std::vector<Port>& GetInputPorts() const { return m_InputPorts; }
    inline const std::vector<Port>& GetOutputPorts() const { return m_OutputPorts; }

  protected:
    std::string m_Name;
    Type m_Type;
    std::vector<Port> m_InputPorts;
    std::vector<Port> m_OutputPorts;
  };

  class HELIOSENGINE_API MathNode : public ShaderNode {
  public:
    enum class Operation {
      Add, Subtract, Multiply, Divide
    };

    MathNode(std::string_view name, Operation op);
    virtual ~MathNode() = default;
  
  protected:
    Operation m_Operation;
  };
}