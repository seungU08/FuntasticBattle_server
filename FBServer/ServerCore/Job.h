#pragma once

class Job : public RefCountable
{
public:
    Job(function<void()> callback) : _callback(move(callback)) {}

    template<typename T, typename Ret, typename... Args>
    Job(TSharedPtr<T> owner, Ret(T::* func)(Args...), Args&&... args)
    {
        _callback = [owner, func, args...]() mutable {
            ((*owner).*func)(args...);
        };
    }

    void Execute() { _callback(); }

private:
    function<void()> _callback;
};

using JobRef = TSharedPtr<Job>;
