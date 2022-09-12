/*
    neat - simple graphics engine
    This library is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <map>

namespace neat {

template <typename Event, typename State>
class StateMachine {
  public:
    using Transitions = std::map<std::pair<State, Event>, State>;

    /** To simplify this interface we set 'Error State',
     in case transition is not possible 'emit' returns error state */
    StateMachine(Transitions* table, State initial, State error) :
        transitionTable_(table), currentState_(initial), error_(error) {
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
