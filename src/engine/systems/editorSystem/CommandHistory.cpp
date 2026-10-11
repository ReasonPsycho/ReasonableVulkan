#include "CommandHistory.hpp"

namespace engine::editor
{
    CommandHistory::CommandHistory(size_t maxHistory)
        : maxHistorySize(maxHistory)
    {
    }

    void CommandHistory::Execute(std::shared_ptr<ICommand> command)
    {
        if (!command) return;

        command->Execute();
        Push(command);
    }

    void CommandHistory::Push(std::shared_ptr<ICommand> command)
    {
        if (!command) return;

        undoStack.push_back(command);
        redoStack.clear();

        if (undoStack.size() > maxHistorySize)
        {
            undoStack.erase(undoStack.begin());
        }
    }

    bool CommandHistory::Undo()
    {
        if (undoStack.empty())
        {
            return false;
        }

        auto command = undoStack.back();
        undoStack.pop_back();

        command->Undo();
        redoStack.push_back(command);

        return true;
    }

    bool CommandHistory::Redo()
    {
        if (redoStack.empty())
        {
            return false;
        }

        auto command = redoStack.back();
        redoStack.pop_back();

        command->Redo();
        undoStack.push_back(command);

        return true;
    }

    bool CommandHistory::CanUndo() const
    {
        return !undoStack.empty();
    }

    bool CommandHistory::CanRedo() const
    {
        return !redoStack.empty();
    }

    std::string CommandHistory::GetUndoName() const
    {
        if (undoStack.empty()) return "";
        return undoStack.back()->GetName();
    }

    std::string CommandHistory::GetRedoName() const
    {
        if (redoStack.empty()) return "";
        return redoStack.back()->GetName();
    }

    void CommandHistory::Clear()
    {
        undoStack.clear();
        redoStack.clear();
    }

    void CommandHistory::JumpToIndex(size_t targetUndoCount)
    {
        while (undoStack.size() > targetUndoCount && CanUndo())
        {
            Undo();
        }

        while (undoStack.size() < targetUndoCount && CanRedo())
        {
            Redo();
        }
    }
}
