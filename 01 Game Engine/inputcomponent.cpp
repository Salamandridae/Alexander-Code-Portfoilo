#include "inputcomponent.h"
#include "gameobject.h"
#include "input.h"

RightCommand::RightCommand()
{

}

RightCommand::~RightCommand()
{

}

LeftCommand::LeftCommand()
{

}

LeftCommand::~LeftCommand()
{

}

InputComponent::InputComponent()
{

}

Command* InputComponent::handleInput(Input* mInput)
{
    if (mInput->D)
    {
        return new RightCommand;
    }
    if (mInput->A)
    {
        return new LeftCommand;
    }
    return nullptr;
}

void InputComponent::update(GameObject &parent, Input* mInput)
{
    Command* tempCommand;
    tempCommand = handleInput(mInput);
    if (tempCommand != nullptr)
    {
        tempCommand->execute(parent);
        delete tempCommand;
    }
}


