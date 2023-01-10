#include <type_traits>


namespace Rt
{


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

template <class Chan, class = void>
struct has_one_input
    : std::is_base_of<Channel, Chan>
{};

template <class Chan>
struct has_one_input<Chan, std::void_t<typename Chan::DemuxKey>>
    : std::is_base_of<ChannelDemux<typename Chan::DemuxKey>, Chan>
{};

template <class Chan>
using has_one_output = disjunction<std::is_base_of<Channel, Chan>::value,
                                   std::is_base_of<ChannelMux, Chan>::value>;

/* */
template <class Chan>
using has_n_inputs = negation<has_one_input<Chan>>;

template <class Chan>
using has_n_outputs = negation<has_one_output<Chan>>;


/*/ // C++20 concepts
template<typename Bl>
concept SimpleUpper = has_one_input<typename Bl::ChannelUpward>::value && has_one_output<typename Bl::ChannelDownward>::value;


template<typename Bl>
concept SimpleLower = has_one_input<typename Bl::ChannelDownward>::value && has_one_output<typename Bl::ChannelUpwad>::value;


template<typename Bl>
concept MultipleUpper = has_n_inputs<typename Bl::ChannelUpward>::value && has_n_outputs<typename Bl::ChannelDownward>::value;


template<typename Bl>
concept MultipleLower = has_n_inputs<typename Bl::ChannelDownward>::value && has_n_outputs<typename Bl::ChannelUpward>::value;
/* */


};
