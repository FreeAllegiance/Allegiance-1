
#pragma once

#include "ui.h"
#include "items.hpp"
#include "UiState.h"
#include <list>
#include <numeric>
#include <algorithm>

template <typename OriginalEntryType>
class ListCount : public Number {

public:
    ListCount(TRef<UiList<OriginalEntryType>> list) :
        Number(list)
    {
    }

    TRef<UiList<OriginalEntryType>> GetSourceList() {
        return (TRef<UiList<OriginalEntryType>>)(UiList<OriginalEntryType>*)GetChild(0);
    }

    void Evaluate() override {
        const std::vector<OriginalEntryType>& list = GetSourceList()->GetList();
        GetValueInternal() = (float)(list.size());
    }
};

template <typename T>
std::vector<int> sortReturnIndices(const std::vector<T> &v) {
    std::vector<int> indices(v.size());
    std::iota(indices.begin(), indices.end(), 0);

    std::sort(indices.begin(), indices.end(), [&v](int a, int b) {
        return v[a] < v[b];
    });

    return indices;
}

template <typename EntryType>
class SortedList : public UiList<EntryType> {

private:
    std::function<TRef<Number>(EntryType)> m_callback;

    std::vector<TRef<Number>> m_vNumbers;

public:
    SortedList(const TRef<UiList<EntryType>>& list, std::function<TRef<Number>(EntryType)> callback) :
        m_callback(callback),
        m_vNumbers({}),
        UiList({}, list)
    {
    }

    ~SortedList() {
        for (auto numberEntry : m_vNumbers) {
            numberEntry->RemoveParent(this);
        }
    }

    TRef<UiList<EntryType>> GetSourceList() {
        return (TRef<UiList<EntryType>>)(UiList<EntryType>*)GetChild(0);
    }

    void Evaluate() override {
        const std::vector<EntryType>& listSource = GetSourceList()->GetList();

        {
            //Can be improved: The following re-evaluation should only be done when the source list gets a Changed event
            for (auto numberEntry : m_vNumbers) {
                numberEntry->RemoveParent(this);
            }
            m_vNumbers.resize(0);
            for (auto sourceEntry : listSource) {
                TRef<Number> numberNewEntry = m_callback(sourceEntry);
                numberNewEntry->AddParent(this);
                m_vNumbers.push_back(numberNewEntry);
            }
        }

        //get the scores for each entry
        auto vFloats = std::vector<float>({});
        for (const TRef<Number>& value : m_vNumbers) {
            vFloats.push_back(value->GetValue());
        }

        //sort the scores
        std::vector<int> vSortedIndices = sortReturnIndices(vFloats);

        //get the result based on the sorted indices
        std::vector<EntryType> listResult({});
        for (int index : vSortedIndices) {
            listResult.push_back(listSource[index]);
        }

        GetListInternal() = listResult;
    }
};

class ListNamespace {
public:
    static void AddNamespace(LuaScriptContext& context) {
        sol::table table = context.GetLua().create_table();

        context.GetLua().new_usertype<TRef<UiList<sol::object>>>("TRef<UiList<sol::object>>");

        table["Map"] = [&context](TRef<UiList<sol::object>> list, sol::function callback) {
            auto wrapped_callback = context.WrapCallback<sol::object, sol::object, TRef<Number>>(callback, sol::nil);

            return (TRef<UiList<sol::object>>)new MappedList<sol::object, sol::object>(list, wrapped_callback);
        };
        table["MapToImages"] = table["Map"];

        table["Sort"] = [&context](TRef<UiList<sol::object>> list, sol::function callback) {
            auto wrapped_callback = context.WrapCallback<TRef<Number>, sol::object>(callback, new Number(0.0f));

            return (TRef<UiList<sol::object>>)new SortedList<sol::object>(list, wrapped_callback);
        };

        table["Count"] = [](TRef<UiList<sol::object>> list) {
            return (TRef<Number>)new ListCount<sol::object>(list);
        };

        context.GetLua().set("List", table);
    }
};
