/*
stateDiagram
    [*] --> PowerOff
    PowerOff --> PowerOn : Activate - Power is available to run the appliance
    PowerOn --> PowerOff : Deactivate - Power is no longer available
    PowerOn --> Inhibited : Inhibit - If inhibited, inhibit
    PowerOff --> Inhibited : Inhibit - If inhibited, inhibit
    Inhibited --> PowerOff : Un-inhibit - The appliance is no longer inhibited
    PowerOff --> PowerOnOverride : Override - If overriden, power on
    PowerOnOverride --> PowerOff : Un-override - When no longer overridden
*/

#include <Arduino.h>

// Abstract State class
class State
{
public:
    virtual ~State() = default;
    virtual void activate() = 0;
    virtual void deactivate() = 0;
    virtual void inhibit() = 0;
    virtual void unInhibit() = 0;
    virtual void overridePower() = 0;
    virtual void unOverride() = 0;
    virtual String currentStateName() = 0;
};

// Context class
class StateMachine
{
public:
    StateMachine();

    void setState(State *newState);
    void activate();
    void deactivate();
    void inhibit();
    void unInhibit();
    void overridePower();
    void unOverride();
    String currentStateName();

private:
    State *currentState;
};

class PowerOn : public State
{
public:
    PowerOn(StateMachine *sm) : stateMachine(sm) {};
    void activate() override;
    void deactivate() override;
    void inhibit() override;
    void unInhibit() override;
    void overridePower() override;
    void unOverride() override;
    String currentStateName() override;

private:
    StateMachine *stateMachine;
};

class PowerOff : public State
{
public:
    PowerOff(StateMachine *sm) : stateMachine(sm) {};
    void activate() override;
    void deactivate() override;
    void inhibit() override;
    void unInhibit() override;
    void overridePower() override;
    void unOverride() override;
    String currentStateName() override;

private:
    StateMachine *stateMachine;
};

class PowerOnOverride : public State
{
public:
    PowerOnOverride(StateMachine *sm) : stateMachine(sm) {};
    void activate() override;
    void deactivate() override;
    void inhibit() override;
    void unInhibit() override;
    void overridePower() override;
    void unOverride() override;
    String currentStateName() override;

private:
    StateMachine *stateMachine;
};

class Inhibited : public State
{
public:
    Inhibited(StateMachine *sm) : stateMachine(sm) {};
    void activate() override;
    void deactivate() override;
    void inhibit() override;
    void unInhibit() override;
    void overridePower() override;
    void unOverride() override;
    String currentStateName() override;

private:
    StateMachine *stateMachine;
};