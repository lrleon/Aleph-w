/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

/**
 * @file driven_table_example.C
 * @brief Example demonstrating the use of Event Tables for a simple simulation.
 *
 * This example simulates a simple "Smart Home" system where different devices
 * (Lights, Thermostat, Security System) respond to events.
 *
 * It demonstrates:
 * 1. Defining event handler functions.
 * 2. Using `Dynamic_Event_Table` to register events dynamically.
 * 3. Passing context data to events.
 * 4. Executing events by ID.
 *
 * @author Leandro Rabindranath León
 */

#include <iostream>
#include <string>
#include <vector>
#include <driven_table.H>

using namespace std;

// =============================================================================
// Simulation Context
// =============================================================================

struct SmartHomeState {
    bool lights_on = false;
    int temperature = 22; // Celsius
    bool alarm_armed = false;
    string last_log;

    void log(const string& msg) {
        last_log = msg;
        cout << "[SmartHome] " << msg << endl;
    }
};

// =============================================================================
// Event Handlers
// =============================================================================

// Event: Turn Lights On
void* on_lights_on(void* data) {
    auto* state = static_cast<SmartHomeState*>(data);
    state->lights_on = true;
    state->log("Lights turned ON");
    return nullptr;
}

// Event: Turn Lights Off
void* on_lights_off(void* data) {
    auto* state = static_cast<SmartHomeState*>(data);
    state->lights_on = false;
    state->log("Lights turned OFF");
    return nullptr;
}

// Event: Increase Temperature
void* on_temp_up(void* data) {
    auto* state = static_cast<SmartHomeState*>(data);
    state->temperature++;
    state->log("Temperature increased to " + to_string(state->temperature) + "C");
    return nullptr;
}

// Event: Arm Alarm
void* on_arm_alarm(void* data) {
    auto* state = static_cast<SmartHomeState*>(data);
    state->alarm_armed = true;
    state->log("Security System ARMED");
    return nullptr;
}

// Event: Emergency (Panic Button)
void* on_panic(void* data) {
    auto* state = static_cast<SmartHomeState*>(data);
    state->lights_on = true;
    state->alarm_armed = true;
    state->log("PANIC! Lights ON, Alarm ARMED, Police Notified!");
    return nullptr;
}

// =============================================================================
// Main Simulation
// =============================================================================

int main() {
    cout << "=== Smart Home Event Simulation ===" << endl;

    // 1. Create the Event Table
    // We use a Dynamic_Event_Table because we might add more devices later.
    Dynamic_Event_Table event_system;

    // 2. Register Events
    // The table assigns a unique ID to each registered event.
    cout << "Registering events..." << endl;

    size_t EVT_LIGHTS_ON = event_system.register_event(on_lights_on);
    size_t EVT_LIGHTS_OFF = event_system.register_event(on_lights_off);
    size_t EVT_TEMP_UP    = event_system.register_event(on_temp_up);
    size_t EVT_ARM_ALARM  = event_system.register_event(on_arm_alarm);
    size_t EVT_PANIC      = event_system.register_event(on_panic);

    cout << "Events registered. Total events: " << event_system.size() << endl;
    cout << "-----------------------------------" << endl;

    // 3. Initialize System State
    SmartHomeState home;

    // 4. Simulate a sequence of actions (Event Loop)
    struct Action {
        size_t event_id;
        string description;
    };

    vector<Action> scenario = {
        {EVT_LIGHTS_ON, "User arrives home"},
        {EVT_TEMP_UP,   "User feels cold"},
        {EVT_TEMP_UP,   "User feels still cold"},
        {EVT_LIGHTS_OFF, "User goes to bed"},
        {EVT_ARM_ALARM, "User arms security"},
        {EVT_PANIC,     "Intruder detected!"}
    };

    for (const auto& action : scenario) {
        cout << "\n> Scenario: " << action.description << endl;

        // Execute the event, passing the shared state
        event_system.execute_event(action.event_id, &home);

        // Verify state (optional)
        if (action.event_id == EVT_PANIC) {
            if (home.lights_on && home.alarm_armed) {
                cout << "  (System responded correctly to panic)" << endl;
            }
        }
    }

    cout << "\n=== Simulation Finished ===" << endl;

    return 0;
}
