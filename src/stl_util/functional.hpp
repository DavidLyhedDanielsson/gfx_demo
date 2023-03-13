#pragma once

#include <optional>

template<typename Collection, typename Val, typename Func>
bool ifFound(Collection& collection, const Val& val, const Func& func)
{
    auto iter = std::find(collection.begin(), collection.end(), val);
    if(iter != collection.end())
    {
        func();
        return true;
    }
    else
        return false;
};

template<typename Collection, typename Val, typename Func, typename Func2>
bool ifFound(Collection& collection, const Val& val, const Func& found, const Func2& notFound)
{
    auto iter = std::find(collection.begin(), collection.end(), val);
    if(iter != collection.end())
    {
        found();
        return true;
    }
    else
    {
        notFound();
        return false;
    }
};

template<typename Collection, typename Val, typename Func>
bool ifFoundKey(Collection& collection, const Val& key, const Func& func)
{
    auto iter = collection.find(key);
    if(iter != collection.end())
    {
        func(iter->second);
        return true;
    }
    else
        return false;
};

template<typename Collection, typename Val, typename Func, typename Func2>
bool ifFoundKey(Collection& collection, const Val& key, const Func& found, const Func2& notFound)
{
    auto iter = collection.find(key);
    if(iter != collection.end())
    {
        found(iter->second);
        return true;
    }
    else
    {
        notFound();
        return false;
    }
};

template<typename T, typename... Rest, typename Func>
void ifNullopt(Func func, std::optional<T>& opt, Rest... rest)
{
    if(!opt)
    {
        if constexpr(sizeof...(Rest) > 0)
            ifNullopt(func, rest...);
        else
            func();
    }
};

template<typename T, typename... Rest, typename Func>
void ifOptional(Func func, std::optional<T>& opt, Rest... rest)
{
    if(opt)
    {
        if constexpr(sizeof...(Rest) > 0)
            ifNullopt(func, rest...);
        else
            func();
    }
};