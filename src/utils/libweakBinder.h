// http://blog.csdn.net/wangji163163/article/details/73698662

#include <memory>
#include <functional>
namespace stdx = std;
template<typename _Class, typename... _Args>
class Weak_Binder0
{
    typedef stdx::weak_ptr<_Class>				WP;
    typedef stdx::shared_ptr<_Class>			SP;
    typedef stdx::function<void(_Class*, _Args...)>FN;
public:
    Weak_Binder0(const WP& wp, const FN& f) :wp_(wp), f_(f) {}
    void operator()(_Args&&... args) const { if (SP sp = wp_.lock())f_(sp.get(), stdx::forward<_Args>(args)...); }
private:
    WP wp_;
    FN f_;
};
template<typename _Ret, typename _Class, typename... _Args>
class Weak_Binder
{
    typedef _Ret								RT;
    typedef stdx::weak_ptr<_Class>				WP;
    typedef stdx::shared_ptr<_Class>			SP;
    typedef stdx::function<RT(_Class*, _Args...)>FN;
public:
    Weak_Binder(const RT& fail, const WP& wp, const FN& f) :fail_(fail), wp_(wp), f_(f) {}
    _Ret operator()(_Args&&... args) const
    {
        if (SP sp = wp_.lock())
            return f_(sp.get(), stdx::forward<_Args>(args)...);
        return fail_;
    }
private:
    RT fail_;
    WP	wp_;
    FN f_;
};
template<typename _Class, typename... _Args>
Weak_Binder0<_Class, _Args...> wbind(const stdx::shared_ptr<_Class>& sp, void (_Class::*f)(_Args...))
{
    return Weak_Binder0<_Class, _Args...>(sp, f);
}
template<typename _Class, typename... _Args>
Weak_Binder0<_Class, _Args...> wbind(const stdx::shared_ptr<_Class>& sp, void (_Class::*f)(_Args...) const)
{
    return Weak_Binder0<_Class, _Args...>(sp, f);
}
template<typename _Ret, typename _Class, typename... _Args>
Weak_Binder<_Ret, _Class, _Args...> wbind(const _Ret&& fail, const stdx::shared_ptr<_Class>& sp, _Ret(_Class::*f)(_Args...))
{
    return Weak_Binder<_Ret, _Class, _Args...>(fail, sp, f);
}
template<typename _Ret, typename _Class, typename... _Args>
Weak_Binder<_Ret, _Class, _Args...> wbind(const _Ret&& fail, const stdx::shared_ptr<_Class>& sp, _Ret(_Class::*f)(_Args...) const)
{
    return Weak_Binder<_Ret, _Class, _Args...>(fail, sp, f);
}
//////////////////////////////////////////////////////////////////////////
template<typename _Class, typename... _Args>
Weak_Binder0<_Class, _Args...> wbind(const stdx::weak_ptr<_Class>& sp, void (_Class::*f)(_Args...))
{
    return Weak_Binder0<_Class, _Args...>(sp, f);
}
template<typename _Class, typename... _Args>
Weak_Binder0<_Class, _Args...> wbind(const stdx::weak_ptr<_Class>& sp, void (_Class::*f)(_Args...) const)
{
    return Weak_Binder0<_Class, _Args...>(sp, f);
}
template<typename _Ret, typename _Class, typename... _Args>
Weak_Binder<_Ret, _Class, _Args...> wbind(const _Ret& fail, const stdx::weak_ptr<_Class>& sp, _Ret(_Class::*f)(_Args...))
{
    return Weak_Binder<_Ret, _Class, _Args...>(fail, sp, f);
}
template<typename _Ret, typename _Class, typename... _Args>
Weak_Binder<_Ret, _Class, _Args...> wbind(const _Ret& fail, const stdx::weak_ptr<_Class>& sp, _Ret(_Class::*f)(_Args...) const)
{
    return Weak_Binder<_Ret, _Class, _Args...>(fail, sp, f);
}