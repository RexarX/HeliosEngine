#pragma once

#include "pch.h"

#include <toml++/toml.hpp>

#define CONFIG_CLASS(ClassName) \
  public: \
    ClassName() = default; \
    virtual ~ClassName() = default; \
    inline const char* GetConfigName() const override { return #ClassName; } \

namespace Helios {
  class HELIOSENGINE_API Config {
  public:
    virtual ~Config() = default;

    virtual void Serialize(toml::table& output) const = 0;
    virtual void Deserialize(const toml::table& input) = 0;

    virtual void LoadDefaults() = 0;

    virtual inline const char* GetConfigName() const = 0;
    
    void MarkDirty() { m_IsDirty = true; }
    void ClearDirty() { m_IsDirty = false; }

    inline bool IsDirty() const { return m_IsDirty; }

  protected:
    template <typename T>
    void SetValue(T& currentValue, const T& newValue);

  private:
    bool m_IsDirty = false;
  };

  template <typename T>
  void Config::SetValue(T& currentValue, const T& newValue) {
    if (currentValue != newValue) {
      currentValue = newValue;
      MarkDirty();
    }
  }

  template <typename T>
  concept ConfigTrait = std::derived_from<T, Config>;
}