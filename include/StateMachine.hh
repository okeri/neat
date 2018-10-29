// Copyright 2018 Keri Oleg

#pragma once

#include <map>

namespace neat {

template <typename Event, typename State>
class StateMachine {
  public:
    using Transitions = std::map<std::pair<State, Event>, State>;

    /** To simplify this interface we set 'Error State',
     in case transition is not possible invoke returns error state */
    StateMachine(Transitions* table, State initial, State error) :
        transitionTable_(table),
        currentState_(initial),
        error_(error) {
    }

    State emit(Event event) {
        std::pair<State, Event> key{currentState_, event};
        const auto& newState = transitionTable_->find(key);
        if (newState != transitionTable_->end()) {
            currentState_ = newState->second;
            return currentState_;
        }
        return error_;
    }

    State state() const {
        return currentState_;
    }

  private:
    Transitions* transitionTable_;
    State currentState_;
    State error_;
};

}  // namespace neat
