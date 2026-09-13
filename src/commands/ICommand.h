#pragma once
// =============================================================================
//  ICommand.h — Interface de base du patron Commande (Undo / Redo)
// =============================================================================

#include <string>

namespace stabileo::commands {

class ICommand {
public:
    virtual ~ICommand() = default;
    virtual bool execute() = 0;
    virtual bool undo() = 0;
    virtual std::string getName() const = 0;
};

} // namespace stabileo::commands
