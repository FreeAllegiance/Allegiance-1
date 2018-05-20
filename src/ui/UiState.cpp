
#include "UiState.h"

void Write(IMDLBinaryFile* pmdlFile, const std::shared_ptr<UiState>& value) {}
ZString GetString(int indent, const std::shared_ptr<UiState>& value) {
    return "";
}
ZString GetFunctionName(const std::shared_ptr<UiState>& value) {
    return "";
}

/*
template <class PropertiesContainerType : UiObjectContainer>
void Write(IMDLBinaryFile* pmdlFile, const UiState<PropertiesContainerType>& value) {

}
template <class PropertiesContainerType : UiObjectContainer>
ZString GetString(int indent, const UiState<PropertiesContainerType>& value) {
    auto state_name = ((UiState<PropertiesContainerType>&)value).GetName();
    return ZString(("State: " + state_name).c_str());
}
template <class PropertiesContainerType : UiObjectContainer>
ZString GetFunctionName(const UiState<PropertiesContainerType>& value) {
    return "UiState";
}

template <class PropertiesContainerType : UiObjectContainer>
UiState<PropertiesContainerType>::UiState(std::string name, PropertiesContainerType properties) :
    m_name(name),
    m_properties(properties)
{
}

template <class PropertiesContainerType : UiObjectContainer>
std::string UiState<PropertiesContainerType>::GetName() const
{
    return m_name;
}

template <class PropertiesContainerType : UiObjectContainer>
PropertiesContainerType& UiState<PropertiesContainerType>::GetProperties() const
{
    return (PropertiesContainerType&)m_properties;
}
*/


