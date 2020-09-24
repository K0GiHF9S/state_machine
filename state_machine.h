#pragma once

#include "type_traits.h"

namespace lib
{

using event_id_t = size_t;
using state_id_t = size_t;

struct ievent
{
    const event_id_t ID;
    explicit ievent(event_id_t id) : ID(id) {}
};

template <event_id_t _ID>
struct event_base : ievent
{
    static constexpr event_id_t ID = _ID;
    event_base(void) : ievent{ID} {}
};

struct istate
{
    const state_id_t ID;

    explicit istate(state_id_t id) : ID(id) {}
    virtual ~istate(void) = default;
    virtual state_id_t on_enter(void) { return (ID); }
    virtual void       on_exit(void) {}
    virtual state_id_t on_event(const ievent& event) = 0;
};

template <state_id_t _ID>
struct state_base : istate
{
    static constexpr state_id_t ID = _ID;
    state_base(void) : istate{ID} {}

    state_id_t on_event(const ievent& event) override
    {
        (void)event;
        //
    }
};

class state_machine
{
    istate** const   _states;
    const state_id_t _states_count;
    state_id_t       _current_state_id;

public:
    state_machine(istate** states, state_id_t states_count, state_id_t first_state_id) :
        _states(states), _states_count(states_count), _current_state_id(first_state_id)
    {}

    void on_event(const ievent& event)
    {
        (void)_states_count;
        (void)event;
        state_id_t next_id = _states[_current_state_id]->on_event(event);
        while (next_id != _current_state_id)
        {
            _states[_current_state_id]->on_exit();
            _current_state_id = next_id;
            next_id           = _states[_current_state_id]->on_enter();
        }
    }
};
}
