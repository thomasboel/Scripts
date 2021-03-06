#include <iostream>
#include <tuple>
#include <vector>
#include <list>
#include <unordered_map>
#include <array>
#include <string>

#include <tuple>
#include <iterator>
#include <type_traits>

#include <boost/range.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/mpl/lambda.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/erase.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/less.hpp>

namespace DETAIL
{
    template
        <class T>
    class EXTRACT_ITERATOR_VALUE_TYPE
    {
    public:
        using type = typename std::iterator_traits<T>::value_type;
    };

    template
        <class T>
    class EXTRACT_ITERATOR_REF_TYPE
    {
    public:
        using type = typename std::iterator_traits<T>::reference;
    };

    template
        <class MPL_VECTOR_T>
    class MPL_VECTOR_TO_TUPLE
    {
    private:

        template <typename Seq, typename T>
        struct add_to_types;

        template <typename T, typename... Ts>
        struct add_to_types<std::tuple<Ts...>, T>
        {
            typedef std::tuple<Ts..., T> type;
        };

    public:
        using type = typename boost::mpl::fold <MPL_VECTOR_T, std::tuple<>, add_to_types<boost::mpl::_1, boost::mpl::_2>>::type;
    };

    template
        <class T, class REF_MPL_ARGS_T, std::size_t N>
    class DEREF_ITER_TUPLE_MAKER
    {
        template <typename ERASE_T>
        struct ERASE_AFTER_N
        {
            using BEGIN_T = typename boost::mpl::begin<ERASE_T>::type;
            using END_T = typename boost::mpl::end<ERASE_T>::type;
            using BEGIN_ERASE_T = typename boost::mpl::advance<BEGIN_T, boost::mpl::int_<N>>::type;
            using type = typename boost::mpl::erase<ERASE_T, BEGIN_ERASE_T, END_T>::type;
        };

        using PARTIAL_MPL_ARGS = typename boost::mpl::eval_if<boost::mpl::less<boost::mpl::int_<N>, boost::mpl::size<REF_MPL_ARGS_T>>,
            ERASE_AFTER_N<REF_MPL_ARGS_T>, REF_MPL_ARGS_T> ::type;

    public:
        static auto get(const T& t) -> typename MPL_VECTOR_TO_TUPLE<PARTIAL_MPL_ARGS>::type
        {
            return std::tuple_cat(
                DEREF_ITER_TUPLE_MAKER<T, REF_MPL_ARGS_T, N - 1>::get(t),
                std::forward_as_tuple(
                *std::get<N - 1>(t)
                )
                );
        }
    };

    template
        <class T, class REF_MPL_ARGS_T>
    class DEREF_ITER_TUPLE_MAKER<T, REF_MPL_ARGS_T, 1>
    {
    public:
        static auto get(const T& t) -> std::tuple<typename std::iterator_traits<typename std::tuple_element<0, T>::type>::reference>
        {
            return std::forward_as_tuple(*std::get<0>(t));
        }
    };

    template
        <class T, std::size_t N>
    class ITER_TUPLE_CMP
    {
    public:
        static bool get(const T& lhs, const T& rhs)
        {
            return
                ITER_TUPLE_CMP<T, N - 1>::get(lhs, rhs) ||
                (std::get<N - 1>(lhs) == std::get<N - 1>(rhs));
        }
    };

    template
        <class T>
    class ITER_TUPLE_CMP<T, 1>
    {
    public:
        static bool get(const T& lhs, const T& rhs)
        {
            return std::get<0>(lhs) == std::get<0>(rhs);
        }
    };

    template
        <class T, std::size_t N>
    class ITER_TUPLE_INCR
    {
    public:
        static void incr(T& t)
        {
            std::get<N - 1>(t)++;
            ITER_TUPLE_INCR<T, N - 1>::incr(t);
        }
    };

    template
        <class T>
    class ITER_TUPLE_INCR<T, 1>
    {
    public:
        static void incr(T& t)
        {
            std::get<0>(t)++;
        }
    };

    template
        <typename INNER_ITERS_T, typename REF_MPL_ARGS_T, typename REF_T, typename VAL_T>
    class ZIPPED_ITER_IMPL : public boost::iterator_facade<
        ZIPPED_ITER_IMPL<INNER_ITERS_T, REF_MPL_ARGS_T, REF_T, VAL_T>,
        VAL_T,
        boost::forward_traversal_tag,
        REF_T>
    {
        friend class boost::iterator_core_access;
        template<typename... ARGS> friend class ZIPPED_RANGE_IMPL;

        ZIPPED_ITER_IMPL(INNER_ITERS_T iter_tuple) : m_iter_tuple(iter_tuple) { }

        REF_T dereference() const
        {
            return DEREF_ITER_TUPLE_MAKER<INNER_ITERS_T, REF_MPL_ARGS_T, std::tuple_size<INNER_ITERS_T>::value>::get(m_iter_tuple);
        }

        using MY_TYPE = ZIPPED_ITER_IMPL<INNER_ITERS_T, REF_MPL_ARGS_T, REF_T, VAL_T>;
        bool equal(const MY_TYPE& other) const
        {
            return ITER_TUPLE_CMP<INNER_ITERS_T, std::tuple_size<INNER_ITERS_T>::value>::get(m_iter_tuple, other.m_iter_tuple);
        }

        void increment()
        {
            ITER_TUPLE_INCR<INNER_ITERS_T, std::tuple_size<INNER_ITERS_T>::value>::incr(m_iter_tuple);
        }

        INNER_ITERS_T m_iter_tuple;
    };

    template
        <typename... ARGS>
    class ZIPPED_RANGE_IMPL
    {
    private:
        using BOOST_MPL_ARGS = boost::mpl::vector<ARGS...>;
        using NO_REF_MPL_ARGS = typename boost::mpl::transform<BOOST_MPL_ARGS, std::remove_reference<boost::mpl::_1>>::type;
        using ITERATOR_MPL_ARGS = typename boost::mpl::transform<NO_REF_MPL_ARGS, boost::range_iterator<boost::mpl::_1>>::type;
        using VALUE_TYPE_MPL_ARGS = typename boost::mpl::transform<ITERATOR_MPL_ARGS, EXTRACT_ITERATOR_VALUE_TYPE<boost::mpl::_1>>::type;
        using REF_TYPE_MPL_ARGS = typename boost::mpl::transform<ITERATOR_MPL_ARGS, EXTRACT_ITERATOR_REF_TYPE<boost::mpl::_1>>::type;
    public:
        using REF_TYPE_TUPLE = typename DETAIL::MPL_VECTOR_TO_TUPLE<REF_TYPE_MPL_ARGS>::type;
        using VALUE_TYPE_TUPLE = typename DETAIL::MPL_VECTOR_TO_TUPLE<VALUE_TYPE_MPL_ARGS>::type;
        using ITER_TYPE_TUPLE = typename DETAIL::MPL_VECTOR_TO_TUPLE<ITERATOR_MPL_ARGS>::type;
        using ZIPPED_ITER = ZIPPED_ITER_IMPL<ITER_TYPE_TUPLE, REF_TYPE_MPL_ARGS, REF_TYPE_TUPLE, VALUE_TYPE_TUPLE>;

        ZIPPED_RANGE_IMPL(ITER_TYPE_TUPLE begin, ITER_TYPE_TUPLE end) : m_begin(begin), m_end(end) { }

        ZIPPED_ITER begin() const { return m_begin; }
        ZIPPED_ITER end() const { return m_end; }
    private:
        ZIPPED_ITER m_begin;
        ZIPPED_ITER m_end;
    };

} /* namespace DETAIL */

template
<typename... ARGS>
DETAIL::ZIPPED_RANGE_IMPL<ARGS...>
make_zipped_range(ARGS&&... args)
{
    auto begin_iters = std::make_tuple(std::begin(args)...);
    auto end_iters = std::make_tuple(std::end(args)...);
    return DETAIL::ZIPPED_RANGE_IMPL<ARGS...>(
        begin_iters,
        end_iters
        );
}

int main()
{
    std::cout << "Hello World\n";
    std::list<int> il = { 0, 1, 2, 3 };
    std::vector<int> iv = { 3, 2, 1, 0, 5 };
    std::array<int, 6> ia = { 7, 8, 9, 10, 11, 12 };
    int ica[] = { 100, 200, 300, 400, 500, 600 };
    std::unordered_map<int, std::string> umap = { { 0, "abcd" }, { 1, "cbca" }, { 2, "bcbc" }, { 3, "foo" } };

    auto zipped_range = make_zipped_range(il, iv, ia, ica, umap);
    for (const auto& t : zipped_range)
    {
        auto first = std::get<0>(t);
        auto second = std::get<1>(t);
        auto third = std::get<2>(t);
        auto fourth = std::get<3>(t);
        auto fifth = std::get<4>(t);
        std::cout << first << ", " << second << ", " << third << ", " << fourth << ", "
            << "(" << fifth.first << " -> " << fifth.second << ")\n";
    }
    return 0;
}
