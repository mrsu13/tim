#pragma once

#include <string>
#include <vector>
#include <list>
#include <unordered_map>
#include <unordered_set>


namespace tim
{

class uuid
{

public:

    /** This enum defines the values used in the variant field of the UUID.
        The value in the variant field determines the layout of the 128-bit value.

    */
    enum class variant
    {
        Unknown = -1, ///< Variant is unknown.
        Ncs        =  0, ///< (0 - -) Reserved for Ncs (Network Computing System) backward compatibility.
        Dce        =  2, ///< (1 0 -) Distributed Computing Environment, the scheme used by tim::uuid.
        Microsoft  =  6, ///< (1 1 0) Reserved for Microsoft backward compatibility (GUID).
        Reserved   =  7  ///< (1 1 1) Reserved for future definition.
    };

    /** This enum defines the values used in the version field of the UUID.
        The version field is meaningful only if the value in the variant field
        is tim::uuid::Dce.
    */
    enum class version
    {
        Unknown       = -1, ///< Version is unknown.
        Time          =  1, ///< (0 0 0 1) Time-based, by using timestamp, clock sequence, and
                            ///< MAC network card address (if available) for the node sections.
        EmbeddedPosix =  2, //< (0 0 1 0) Dce Security version, with embedded POSIX UUIDs.
        Name          =  3, //< (0 0 1 1) Name-based, by using values from a name for all sections.
        Random        =  4  //< (0 1 0 0) Random Random-based, by using random numbers for all sections.
    };

    uuid();
    uuid(const tim::uuid &other);
    uuid(unsigned int l, unsigned short w1, unsigned short w2,
         unsigned char b1, unsigned char b2, unsigned char b3, unsigned char b4,
         unsigned char b5, unsigned char b6, unsigned char b7, unsigned char b8);
    uuid(const std::string &text);
    uuid(const char *text);

    enum class format
    {
        Canonical  = 1, // Like {943b573e-7a1d-4419-81b1-3308455be5f7}
        NoBrackets = 2, // Like 943b573e-7a1d-4419-81b1-3308455be5f7
        Compact    = 3  // Like 943b573e7a1d441981b13308455be5f7
    };

    inline std::string to_string() const;
    std::string to_string(const format format) const;
    bool from_string(const std::string &text);
    inline operator std::string() const;

    bool is_null() const;
    inline bool valid() const;
    inline operator bool() const;
    void clear();

    tim::uuid &operator=(const tim::uuid &other);
    inline tim::uuid &operator=(const std::string &text);

    bool operator==(const tim::uuid &orig) const;

    inline bool operator!=(const tim::uuid &orig) const;

    bool operator<(const tim::uuid &other) const;
    bool operator>(const tim::uuid &other) const;

    static tim::uuid create();
    variant uuid_variant() const;
    version uuid_version() const;

private:

    bool _valid;

    unsigned int   data1;
    unsigned short data2;
    unsigned short data3;
    unsigned char  data4[8];
};

using uuid_vector = std::vector<tim::uuid>;
using uuid_list = std::list<tim::uuid>;
using uuid_map = std::unordered_map<tim::uuid, tim::uuid>;
using uuid_set = std::unordered_set<tim::uuid>;

}


namespace std
{

template<>
struct hash<tim::uuid>
{

public:

    inline std::size_t operator()(const tim::uuid &uuid) const
    {
        return std::hash<std::string>()(uuid.to_string());
    }
};

}


namespace tim
{

inline std::size_t hash_value(const tim::uuid &uuid)
{
    return std::hash<tim::uuid>()(uuid);
}

}


// Implementation

// Public

std::string tim::uuid::to_string() const
{
    return to_string(format::Canonical);
}

/** \return The string representation of the UUID.

    \sa to_string()
*/
tim::uuid::operator std::string() const
{
    return to_string();
}

bool tim::uuid::valid() const
{
    return _valid;
}

tim::uuid::operator bool() const
{
    return valid() && !is_null();
}

tim::uuid &tim::uuid::operator=(const std::string &text)
{
    from_string(text);
    return *this;
}

/** \return true if this tim::uuid and the \a other tim::uuid are different;
    otherwise returns false.
*/
bool tim::uuid::operator!=(const tim::uuid &orig) const
{
    return !(*this == orig);
}
