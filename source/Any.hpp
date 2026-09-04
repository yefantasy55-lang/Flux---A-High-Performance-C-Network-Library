#include <iostream>
#include <typeinfo>
#include <algorithm> // for std::swap

class Any
{
private:
    class holder
    {
    public:
        virtual ~holder() {}
        virtual const std::type_info &type() const = 0;
        virtual holder *clone() const = 0;
    };

    template <class T>
    class placeholder : public holder
    {
    public:
        placeholder(const T &val) : _val(val) {}
        const std::type_info &type() const override { return typeid(T); }
        holder *clone() const override { return new placeholder(_val); }
        T _val;
    };

    holder *_content;

public:
    Any() : _content(nullptr) {}

    template <class T>
    Any(const T &val) : _content(new placeholder<T>(val)) {}

    Any(const Any &other)
        : _content(other._content ? other._content->clone() : nullptr) {}

    ~Any() { delete _content; }

    Any &operator=(const Any &other)
    {
        if (this != &other)
        {
            Any tmp(other); // 拷贝构造
            swap(tmp);      // 交换内容
        }
        return *this;
    }

    template <class T>
    Any &operator=(const T &val)
    {
        Any(val).swap(*this);
        return *this;
    }

    void swap(Any &other) noexcept
    {
        std::swap(_content, other._content);
    }

    template <class T>
    T *get()
    {
        if (!_content)
            return nullptr;
        if (typeid(T) != _content->type())
            return nullptr;
        return &static_cast<placeholder<T> *>(_content)->_val;
    }

    template <class T>
    const T *get() const
    {
        if (!_content)
            return nullptr;
        if (typeid(T) != _content->type())
            return nullptr;
        return &static_cast<const placeholder<T> *>(_content)->_val;
    }

    bool has_value() const noexcept { return _content != nullptr; }

    const std::type_info &type() const
    {
        if (!_content)
            throw std::bad_cast();
        return _content->type();
    }

    // 清空
    void reset()
    {
        delete _content;
        _content = nullptr;
    }
};