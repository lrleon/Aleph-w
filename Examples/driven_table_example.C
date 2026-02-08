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
 * @brief Example demonstrating both legacy and modern use of Event Tables.
 *
 * This example simulates a simple "Smart Home" system where different devices
 * (Lights, Thermostat, Security System) respond to events.
 *
 * It demonstrates:
 * 1. Legacy interface: Using void* function pointers (for backward compatibility)
 * 2. Modern interface: Using type-safe lambdas with templates
 * 3. Passing context data to events
 * 4. Executing events by ID
 *
 * @author Leandro Rabindranath León
 */

#include <iostream>
#include <string>
#include <vector>
#include <driven_table.H>

using namespace std;

// =============================================================================
// Part 1: Legacy Example (void* interface for backward compatibility)
// =============================================================================

// =============================================================================
// Simulation Context
// =============================================================================

struct SmartHomeState
{
  bool lights_on = false;
  int temperature = 22; // Celsius
  bool alarm_armed = false;
  string last_log;

  void log(const string & msg)
  {
    last_log = msg;
    cout << "[SmartHome] " << msg << endl;
  }
};

// =============================================================================
// Event Handlers
// =============================================================================

// Event: Turn Lights On
void * on_lights_on(void *data)
{
  auto *state = static_cast<SmartHomeState *>(data);
  state->lights_on = true;
  state->log("Lights turned ON");
  return nullptr;
}

// Event: Turn Lights Off
void * on_lights_off(void *data)
{
  auto *state = static_cast<SmartHomeState *>(data);
  state->lights_on = false;
  state->log("Lights turned OFF");
  return nullptr;
}

// Event: Increase Temperature
void * on_temp_up(void *data)
{
  auto *state = static_cast<SmartHomeState *>(data);
  state->temperature++;
  state->log("Temperature increased to " + to_string(state->temperature) + "C");
  return nullptr;
}

// Event: Arm Alarm
void * on_arm_alarm(void *data)
{
  auto *state = static_cast<SmartHomeState *>(data);
  state->alarm_armed = true;
  state->log("Security System ARMED");
  return nullptr;
}

// Event: Emergency (Panic Button)
void * on_panic(void *data)
{
  auto *state = static_cast<SmartHomeState *>(data);
  state->lights_on = true;
  state->alarm_armed = true;
  state->log("PANIC! Lights ON, Alarm ARMED, Police Notified!");
  return nullptr;
}

// =============================================================================
// Legacy Simulation
// =============================================================================

void run_legacy_example()
{
  cout << "\n=== LEGACY EXAMPLE: Smart Home Event Simulation (void* interface) ===" << endl;

  // 1. Create the Event Table (using legacy alias)
  Legacy_Dynamic_Event_Table event_system;

  // 2. Register Events
  // The table assigns a unique ID to each registered event.
  cout << "Registering events..." << endl;

  size_t EVT_LIGHTS_ON = event_system.register_event(on_lights_on);
  size_t EVT_LIGHTS_OFF = event_system.register_event(on_lights_off);
  size_t EVT_TEMP_UP = event_system.register_event(on_temp_up);
  size_t EVT_ARM_ALARM = event_system.register_event(on_arm_alarm);
  size_t EVT_PANIC = event_system.register_event(on_panic);

  cout << "Events registered. Total events: " << event_system.size() << endl;
  cout << "-----------------------------------" << endl;

  // 3. Initialize System State
  SmartHomeState home;

  // 4. Simulate a sequence of actions (Event Loop)
  struct Action
  {
    size_t event_id;
    string description;
  };

  vector<Action> scenario = {
        {EVT_LIGHTS_ON, "User arrives home"},
        {EVT_TEMP_UP, "User feels cold"},
        {EVT_TEMP_UP, "User feels still cold"},
        {EVT_LIGHTS_OFF, "User goes to bed"},
        {EVT_ARM_ALARM, "User arms security"},
        {EVT_PANIC, "Intruder detected!"}
      };

  for (const auto & [event_id, description]: scenario)
    {
      cout << "\n> Scenario: " << description << endl;

      // Execute the event, passing the shared state
      event_system.execute_event(event_id, &home);

      // Verify state (optional)
      if (event_id == EVT_PANIC)
        if (home.lights_on && home.alarm_armed)
          cout << "  (System responded correctly to panic)" << endl;
    }

  cout << "\n=== Legacy Example Finished ===" << endl;
}


// =============================================================================
// Part 2: Modern Example (Type-Safe Templates with Lambdas)
// =============================================================================

// State for modern example
struct ModernSmartHome
{
  bool lights_on = false;
  int temperature = 22;
  bool alarm_armed = false;

  void log(const string & msg)
  {
    cout << "[ModernHome] " << msg << endl;
  }
};

void run_modern_example()
{
  cout << "\n=== MODERN EXAMPLE: Type-Safe Event System with Lambdas ===" << endl;

  ModernSmartHome home;

  // Create type-safe event tables for different event signatures

  // 1. Events that modify state (void return, take state reference)
  Dynamic_Event_Table<void(ModernSmartHome &)> state_events;

  auto EVT_LIGHTS_ON = state_events.register_event([](ModernSmartHome & h)
                                                     {
                                                       h.lights_on = true;
                                                       h.log("Lights turned ON (lambda)");
                                                     });

  auto EVT_LIGHTS_OFF = state_events.register_event([](ModernSmartHome & h)
                                                      {
                                                        h.lights_on = false;
                                                        h.log("Lights turned OFF (lambda)");
                                                      });

  auto EVT_TEMP_UP = state_events.register_event([](ModernSmartHome & h)
                                                   {
                                                     h.temperature++;
                                                     h.log("Temperature increased to " + to_string(h.temperature) +
                                                           "C (lambda)");
                                                   });

  auto EVT_ARM_ALARM = state_events.register_event([](ModernSmartHome & h)
                                                     {
                                                       h.alarm_armed = true;
                                                       h.log("Security System ARMED (lambda)");
                                                     });

  // 2. Events that query state (return bool, take const reference)
  Dynamic_Event_Table<bool(const ModernSmartHome &)> query_events;

  auto QUERY_IS_SECURE = query_events.register_event([](const ModernSmartHome & h)
                                                       {
                                                         return h.alarm_armed && ! h.lights_on;
                                                       });

  auto QUERY_IS_COMFORTABLE = query_events.register_event([](const ModernSmartHome & h)
                                                            {
                                                              return h.temperature >= 20 && h.temperature <= 24;
                                                            });

  // 3. Events with multiple parameters (temperature control)
  Dynamic_Event_Table<void(ModernSmartHome &, int)> temp_control_events;

  auto EVT_SET_TEMP = temp_control_events.register_event([](ModernSmartHome & h, int target)
                                                           {
                                                             h.temperature = target;
                                                             h.log("Temperature set to " + to_string(target) +
                                                                   "C (lambda with param)");
                                                           });

  // Run simulation scenario
  cout << "\n--- Scenario: Evening Routine ---" << endl;

  state_events.execute_event(EVT_LIGHTS_ON, home);
  cout << "Is secure? " << (query_events.execute_event(QUERY_IS_SECURE, home) ? "Yes" : "No") << endl;

  temp_control_events.execute_event(EVT_SET_TEMP, home, 23);
  cout << "Is comfortable? " << (query_events.execute_event(QUERY_IS_COMFORTABLE, home) ? "Yes" : "No") << endl;

  state_events.execute_event(EVT_LIGHTS_OFF, home);
  state_events.execute_event(EVT_ARM_ALARM, home);

  cout << "Is secure? " << (query_events.execute_event(QUERY_IS_SECURE, home) ? "Yes" : "No") << endl;

  // Demonstrate stateful lambdas
  cout << "\n--- Advanced: Stateful Event (Counter) ---" << endl;

  int event_count = 0;
  Dynamic_Event_Table<void()> simple_events;

  auto EVT_COUNT = simple_events.register_event([&event_count]()
                                                  {
                                                    event_count++;
                                                    cout << "Event executed " << event_count << " time(s)" << endl;
                                                  });

  simple_events.execute_event(EVT_COUNT);
  simple_events.execute_event(EVT_COUNT);
  simple_events.execute_event(EVT_COUNT);

  cout << "Total event executions: " << event_count << endl;

  cout << "\n=== Modern Example Finished ===" << endl;
}


// =============================================================================
// Main
// =============================================================================

int main()
{
  // Run both examples to show legacy and modern usage
  run_legacy_example();
  run_modern_example();

  cout << "\n=== All Examples Completed ===" << endl;
  return 0;
}
