#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace engine::editor
{
    class ICommand
    {
    public:
        virtual ~ICommand() = default;
        virtual void Execute() = 0;
        virtual void Undo() = 0;
        virtual void Redo() { Execute(); }
        virtual std::string GetName() const = 0;
    };

    class CommandHistory
    {
    public:
        explicit CommandHistory(size_t maxHistory = 128);
        ~CommandHistory() = default;

        // Executes the command, adds it to the undo stack, and clears the redo stack
        void Execute(std::shared_ptr<ICommand> command);

        // Pushes an already-executed command onto the undo stack and clears the redo stack
        void Push(std::shared_ptr<ICommand> command);

        // Undo the most recent command
        bool Undo();

        // Redo the next command
        bool Redo();

        bool CanUndo() const;
        bool CanRedo() const;

        std::string GetUndoName() const;
        std::string GetRedoName() const;

        void Clear();

        const std::vector<std::shared_ptr<ICommand>>& GetUndoStack() const { return undoStack; }
        const std::vector<std::shared_ptr<ICommand>>& GetRedoStack() const { return redoStack; }

        size_t GetCurrentIndex() const { return undoStack.size(); }
        size_t GetTotalSteps() const { return undoStack.size() + redoStack.size(); }

        // Allows jumping directly to any step in the history
        void JumpToIndex(size_t targetUndoCount);

        size_t GetMaxHistory() const { return maxHistorySize; }
        void SetMaxHistory(size_t maxHistory) { maxHistorySize = maxHistory; }

    private:
        std::vector<std::shared_ptr<ICommand>> undoStack;
        std::vector<std::shared_ptr<ICommand>> redoStack;
        size_t maxHistorySize = 128;
    };
}
