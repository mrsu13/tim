#pragma once

#include <memory>
#include <type_traits>


namespace tim
{

/**
 * \brief Класс, хранящий флаги на основании перечисления.
 * Ответственность за то, чтобы перечисление использовало правильные значения,
 * лежит на авторе перечисления.
 */
template<typename Enum>
class flags
{

public:

    using value_type = unsigned;
    using enum_type = Enum;

    constexpr inline flags(value_type value = 0);
    constexpr inline flags(Enum f);

    constexpr inline operator value_type() const;
    constexpr inline operator bool() const;

    constexpr inline bool test(Enum f) const;
    inline void set(value_type mask);
    inline void set(Enum f, bool on = true);
    inline void clear(Enum f);
    inline void reset();

    constexpr inline tim::flags<Enum> operator|(Enum f) const;
    inline tim::flags<Enum> &operator=(value_type mask);
    inline tim::flags<Enum> &operator|=(Enum f);
    inline tim::flags<Enum> &operator|=(const tim::flags<Enum> &f);

private:

    value_type _flags;
};


#define TIM_DECL_OPERATORS_FOR_FLAGS(F) \
constexpr inline F operator|(F::enum_type f1, F::enum_type f2) noexcept \
{ \
    return F(f1) | f2; \
}


/**
 * Проверка на равенство двух наборов флагов.
 */
template<typename Enum>
constexpr inline bool operator==(const tim::flags<Enum> &f1, const tim::flags<Enum> &f2)
{
    return static_cast<typename tim::flags<Enum>::value_type>(f1)
                == static_cast<typename tim::flags<Enum>::value_type>(f2);
}

}


namespace std
{

template<typename Enum>
struct hash<tim::flags<Enum>>
{

public:

    inline std::size_t operator()(const tim::flags<Enum> &f) const
    {
        return std::hash<typename tim::flags<Enum>::value_type>()((typename tim::flags<Enum>::value_type)f);
    }
};

}


// Implementation

// Public

/**
 * Конструктор.
 * \param value Целочисленное значение флагов.
 */
template<typename Enum>
constexpr tim::flags<Enum>::flags(value_type value)
    : _flags(value)
{
    static_assert(std::is_enum<Enum>::value, "Enum must be an enum type.");
}

/**
 * Конструктор.
 * \param f Значение флага из перечисления.
 */
template<typename Enum>
constexpr tim::flags<Enum>::flags(Enum f)
    : _flags((value_type)f)
{
    static_assert(std::is_enum<Enum>::value, "Enum must be an enum type.");
}

/**
 * \return Значение флагов в виде целого числа.
 */
template<typename Enum>
constexpr tim::flags<Enum>::operator value_type() const
{
    return _flags;
}

/**
 * Приведение значения флагов к булеву типу для проверку на неравенство нулю.
 */
template<typename Enum>
constexpr tim::flags<Enum>::operator bool() const
{
    return _flags;
}

/**
 * \return \c true, если флаг \a f установлен и \c false --- в противном случае.
 */
template<typename Enum>
constexpr bool tim::flags<Enum>::test(Enum f) const
{
    return (_flags & (value_type)f);
}

template<typename Enum>
void tim::flags<Enum>::set(value_type mask)
{
    _flags = mask;
}

/**
 * Установить флаг \a f в значение \a on.
 */
template<typename Enum>
void tim::flags<Enum>::set(Enum f, bool on)
{
    if (on)
        _flags |= (value_type)f;
    else
        _flags &= ~(value_type)f;
}

/**
 * Сбросить флаг \a f.
 */
template<typename Enum>
void tim::flags<Enum>::clear(Enum f)
{
    _flags &= ~(value_type)f;
}

/**
 * Сбросить все флаги. После вызова этого метода объект класса хранит нулевое значение.
 */
template<typename Enum>
void tim::flags<Enum>::reset()
{
    _flags = 0;
}

/**
 * \return Объект с копией флагов из этого объекта, в этой копии также будет
 * установлен флаг \a f.
 */
template<typename Enum>
constexpr tim::flags<Enum> tim::flags<Enum>::operator|(Enum f) const
{
    return tim::flags<Enum>(_flags | (value_type)f);
}

/**
 * Set flags defined by mask \a mask.
 *
 * \see set()
 */
template<typename Enum>
tim::flags<Enum> &tim::flags<Enum>::operator=(value_type mask)
{
    _flags = mask;
    return *this;
}

/**
 * Set flag \a f.
 *
 * \see set()
 */
template<typename Enum>
tim::flags<Enum> &tim::flags<Enum>::operator|=(Enum f)
{
    _flags |= (value_type)f;
    return *this;
}

/**
 * Установить флаги, установленные в \a f.
 */
template<typename Enum>
tim::flags<Enum> &tim::flags<Enum>::operator|=(const tim::flags<Enum> &f)
{
    _flags |= f._flags;
    return *this;
}
