#include <appliance_state.h>

void PowerOff::activate()
{
    Serial.println("Transitioning to Power On state (Power is available to run the appliance).");
    stateMachine->setState(new PowerOn(stateMachine));
}
void PowerOff::deactivate()
{
    Serial.println("Already in Power Off state.");
}
void PowerOff::inhibit()
{
    Serial.println("Transitioning to Inhibited state (If inhibited, inhibit).");
    stateMachine->setState(new Inhibited(stateMachine));
}
void PowerOff::unInhibit()
{
    Serial.println("Cannot un-inhibit; not in Inhibited state.");
}
void PowerOff::overridePower()
{
    Serial.println("Transitioning to Power On Override state (If overridden, power on).");
    stateMachine->setState(new PowerOnOverride(stateMachine));
}
void PowerOff::unOverride()
{
    Serial.println("Cannot un-override; not in Power On Override state.");
}
void PowerOff::printCurrentState() const
{
    Serial.println("Current State: Power Off");
}

void PowerOn::activate()
{
    Serial.println("Already in Power On state.");
}
void PowerOn::deactivate()
{
    Serial.println("Transitioning to Power Off state (Power is no longer available).");
    stateMachine->setState(new PowerOff(stateMachine));
}
void PowerOn::inhibit()
{
    Serial.println("Transitioning to Inhibited state (If inhibited, inhibit).");
    stateMachine->setState(new Inhibited(stateMachine));
}
void PowerOn::unInhibit()
{
    Serial.println("Cannot un-inhibit; not in Inhibited state.");
}
void PowerOn::overridePower()
{
    Serial.println("Cannot override; already powered on.");
}
void PowerOn::unOverride()
{
    Serial.println("Cannot un-override; not in Power On Override state.");
}
void PowerOn::printCurrentState() const
{
    Serial.println("Current State: Power On");
}

void Inhibited::activate()
{
    Serial.println("Cannot activate from Inhibited state.");
}
void Inhibited::deactivate()
{
    Serial.println("Transitioning to Power Off state (Power is no longer available).");
    stateMachine->setState(new PowerOff(stateMachine));
}
void Inhibited::inhibit()
{
    Serial.println("Already in Inhibited state.");
}
void Inhibited::unInhibit()
{
    Serial.println("Transitioning to Power Off state (The appliance is no longer inhibited).");
    stateMachine->setState(new PowerOff(stateMachine));
}
void Inhibited::overridePower()
{
    Serial.println("Cannot override; not in Power Off state.");
}
void Inhibited::unOverride()
{
    Serial.println("Cannot un-override; not in Power On Override state.");
}
void Inhibited::printCurrentState() const
{
    Serial.println("Current State: Inhibited");
}

void PowerOnOverride::activate()
{
    Serial.println("Already in Power On Override state.");
}
void PowerOnOverride::deactivate()
{
    Serial.println("Transitioning to Power Off state from Power On Override.");
    stateMachine->setState(new PowerOff(stateMachine));
}
void PowerOnOverride::inhibit()
{
    Serial.println("Cannot inhibit from Power On Override state.");
}
void PowerOnOverride::unInhibit()
{
    Serial.println("Cannot un-inhibit; not in Inhibited state.");
}
void PowerOnOverride::overridePower()
{
    Serial.println("Already in Power On Override state.");
}
void PowerOnOverride::unOverride()
{
    Serial.println("Transitioning to Power Off state (When no longer overridden).");
    stateMachine->setState(new PowerOff(stateMachine));
}
void PowerOnOverride::printCurrentState() const
{
    Serial.println("Current State: Power On Override");
}

// StateMachine implementation
StateMachine::StateMachine()
{
    currentState = new PowerOff(this);
}

void StateMachine::setState(State *newState)
{
    delete currentState; // Clean up the previous state
    currentState = newState;
}

void StateMachine::activate() { currentState->activate(); }
void StateMachine::deactivate() { currentState->deactivate(); }
void StateMachine::inhibit() { currentState->inhibit(); }
void StateMachine::unInhibit() { currentState->unInhibit(); }
void StateMachine::overridePower() { currentState->overridePower(); }
void StateMachine::unOverride() { currentState->unOverride(); }
void StateMachine::printCurrentState() const { currentState->printCurrentState(); }

/*
void setup()
{
    Serial.begin(9600);
    StateMachine sm;

    sm.printCurrentState();
    sm.activate();
    sm.printCurrentState();
    sm.inhibit();
    sm.printCurrentState();
    sm.deactivate();
    sm.printCurrentState();
    sm.overridePower();
    sm.printCurrentState();
    sm.unOverride();
    sm.printCurrentState();
    sm.unInhibit();
    sm.printCurrentState();
    sm.deactivate();
    sm.printCurrentState();
}
*/
