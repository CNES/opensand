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

template <class Chan>
using has_one_input = negation<std::is_base_of<RtChannelMux, Chan>>;

template <class Chan>
using has_one_output = disjunction<std::is_base_of<RtChannelMux, Chan>::value,
                                   std::is_base_of<RtChannel, Chan>::value>;

template <class Chan>
using has_n_inputs = negation<has_one_input<Chan>>;

template <class Chan>
using has_n_outputs = negation<has_one_output<Chan>>;

