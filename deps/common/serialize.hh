/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include <algorithm>
#include <concepts>
#include <cstdlib>
#include <span>

namespace common {

template <typename T>
concept MyInt = std::is_integral_v<T> || std::is_integral_v<std::underlying_type_t<T>>;

/// All of these are little endian of course.
template <typename Iter, typename Int>
concept IntParams = requires (Iter i) {
        { *i } -> std::convertible_to<Int>;
} || requires (Iter i) {
        { *i } -> std::convertible_to<std::underlying_type_t<Int>>;
};

/*--------------------------------------------------------------------------*/

template <typename Iter, MyInt Int>
        requires IntParams<Iter, Int>
auto getSpan (Iter const &iterator, Int *val)
{
        using IterVal = std::decay_t<decltype (*iterator)>;
        static_assert (sizeof (*val) >= sizeof (IterVal));
        static_assert (sizeof (*val) % sizeof (IterVal) == 0);
        size_t s = sizeof (*val) / sizeof (IterVal);
        auto *p = reinterpret_cast<IterVal *> (val);
        return std::span{p, s};
}

/*--------------------------------------------------------------------------*/

template <typename Iter, MyInt Int>
        requires IntParams<Iter, Int>
Iter getInt (Iter const &iterator, Int *val)
{
        auto sp = getSpan (iterator, val);
        std::copy_n (iterator, sp.size (), sp.data ());
        return std::next (iterator, sp.size ());
}

/*--------------------------------------------------------------------------*/

template <typename Iter, MyInt Int>
        requires IntParams<Iter, Int>
Iter setInt (Int val, Iter &iterator)
{
        auto sp = getSpan (iterator, &val);
        std::copy_n (sp.data (), sp.size (), iterator);
        return std::next (iterator, sp.size ());
}

/**
 * Serializes to an iterator, but without size validation.
 */
template <typename Iterator> class SerializeToIterator {
public:
        SerializeToIterator (Iterator *iter) : iter_{iter} {}

        template <MyInt Int> SerializeToIterator &set (Int u)
        {
                *iter_ = setInt (u, *iter_);
                return *this;
        }

        template <MyInt Int> SerializeToIterator &get (Int *u)
        {
                *iter_ = getInt (*iter_, u);
                return *this;
        }

protected:
        Iterator *iter_;
};

/**
 * Helper function.
 */
auto serialize (auto iter) { return SerializeToIterator{iter}; }

/**
 * Serialize to a fixed size collection that has to be created and provided
 * beforehand validating the size of the collection.
 */
template <typename Collection> class SerializeToCollection {
public:
        using Base = SerializeToIterator<typename Collection::iterator>;

        SerializeToCollection (Collection *data) : data_{data}, iter{data_->begin ()}, base{&iter} {}

        template <MyInt T> SerializeToCollection &set (T u)
        {
                if (!check (&u)) {
                        return *this;
                }

                base.set (u);
                return *this;
        }

        template <MyInt T> SerializeToCollection &get (T *u)
        {
                if (!check (u)) {
                        return *this;
                }

                base.get (u);
                return *this;
        }

        Collection const &data () const { return *data_; }
        bool isSuccess () const { return success; }

        void clear ()
        {
                data_->clear ();
                success = true;
        }

private:
        bool check (MyInt auto *u)
        {
                auto sp = getSpan (iter, u);

                if (auto dist = std::distance (iter, data_->end ()); dist < int (sp.size ())) {
                        success = false;
                }

                return success;
        }

        Collection *data_{};
        Collection::iterator iter;
        Base base;
        bool success = true;
};

/**
 * Helper function.
 *
 * @tparam Collection
 * @param c
 * @return requires
 */
template <typename Collection>
        requires requires (Collection c) {
                { c.begin () }; // Crude way of detecting a STL collection
                { c.end () };
        }
auto serialize (Collection *col)
{
        return SerializeToCollection{col};
}

} // namespace common