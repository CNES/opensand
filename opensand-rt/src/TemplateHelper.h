#include <type_traits>

template <bool B>
using bool_constant = std::integral_constant<bool, B>;

// NOT
template <class B>
struct negation: bool_constant<!bool(B::value)>
{};

template <bool...>
struct bool_pack
{};

// AND
template <bool... Bs>
using conjunction = std::is_same<bool_pack<true, Bs...>, bool_pack<Bs..., true>>;

// OR
template <bool... Bs>
struct disjunction: bool_constant<!conjunction<!Bs...>::value>
{};

// void_t (C++17)
template <typename... Ts>
struct make_void
{
	typedef void type;
};
template <typename... Ts>
using void_t = typename make_void<Ts...>::type;

template <class Chan, class = void>
struct has_one_input
    : std::is_base_of<RtChannel, Chan>
{};

template <class Chan>
struct has_one_input<Chan, void_t<typename Chan::DemuxKey>>
    : std::is_base_of<RtChannelDemux<typename Chan::DemuxKey>, Chan>
{};

template <class Chan>
using has_one_output = disjunction<std::is_base_of<RtChannel, Chan>::value,
                                   std::is_base_of<RtChannelMux, Chan>::value>;

template <class Chan>
using has_n_inputs = negation<has_one_input<Chan>>;

template <class Chan>
using has_n_outputs = negation<has_one_output<Chan>>;
