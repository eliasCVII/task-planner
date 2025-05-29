#ifndef UNDOMANAGER_H
#define UNDOMANAGER_H

#include <memory>
#include <vector>
#include <string>

// Forward declarations
class TaskManager;
class Act;

/**
 * Abstract base class for all undoable commands
 */
class UndoableCommand {
public:
    virtual ~UndoableCommand() = default;

    /**
     * Execute the command (perform the operation)
     */
    virtual void execute() = 0;

    /**
     * Undo the command (reverse the operation)
     */
    virtual void undo() = 0;

    /**
     * Get a human-readable description of the command
     */
    virtual std::string getDescription() const = 0;

    /**
     * Get the approximate memory footprint of this command in bytes
     */
    virtual size_t getMemoryFootprint() const = 0;
};

/**
 * Base class for TaskManager-related commands
 */
class TaskManagerCommand : public UndoableCommand {
protected:
    TaskManager* manager;
    std::string description;

public:
    TaskManagerCommand(TaskManager* mgr, const std::string& desc);
    std::string getDescription() const override;

protected:
    void setDescription(const std::string& desc) { description = desc; }
};

/**
 * Command for adding a task
 */
class AddTaskCommand : public TaskManagerCommand {
private:
    std::string name;
    std::string startTime;
    int length;
    bool isRigid;
    bool hasStartTime;
    int insertIndex; // Where the task was added

public:
    // Constructor for task with start time (fixed task)
    AddTaskCommand(TaskManager* mgr, const std::string& taskName,
                   const std::string& start, int len, bool rigid);

    // Constructor for task without start time (flexible task)
    AddTaskCommand(TaskManager* mgr, const std::string& taskName,
                   int len, bool rigid);

    void execute() override;
    void undo() override;
    size_t getMemoryFootprint() const override;
};

/**
 * Command for deleting a task
 */
class DeleteTaskCommand : public TaskManagerCommand {
private:
    int index;
    // Store deleted task data for undo
    std::string deletedName;
    std::string deletedStartTime;
    int deletedLength;
    bool deletedRigid;
    bool deletedFixed;
    bool taskWasDeleted; // Track if deletion was successful

public:
    DeleteTaskCommand(TaskManager* mgr, int taskIndex);

    void execute() override;
    void undo() override;
    size_t getMemoryFootprint() const override;
};

/**
 * Command for editing task name
 */
class EditTaskNameCommand : public TaskManagerCommand {
private:
    int taskIndex;
    std::string oldName;
    std::string newName;
    bool wasExecuted;

public:
    EditTaskNameCommand(TaskManager* mgr, int index, const std::string& oldValue, const std::string& newValue);

    void execute() override;
    void undo() override;
    size_t getMemoryFootprint() const override;
};

/**
 * Command for editing task start time
 */
class EditTaskStartTimeCommand : public TaskManagerCommand {
private:
    int taskIndex;
    std::string oldStartTime;
    std::string newStartTime;
    bool oldFixed;
    bool newFixed;
    bool wasExecuted;

public:
    EditTaskStartTimeCommand(TaskManager* mgr, int index, const std::string& oldValue, const std::string& newValue, bool oldFixedState, bool newFixedState);

    void execute() override;
    void undo() override;
    size_t getMemoryFootprint() const override;
};

/**
 * Command for editing task length
 */
class EditTaskLengthCommand : public TaskManagerCommand {
private:
    int taskIndex;
    int oldLength;
    int newLength;
    bool wasExecuted;

public:
    EditTaskLengthCommand(TaskManager* mgr, int index, int oldValue, int newValue);

    void execute() override;
    void undo() override;
    size_t getMemoryFootprint() const override;
};

/**
 * Command for toggling task fixed status
 */
class ToggleTaskFixedCommand : public TaskManagerCommand {
private:
    int taskIndex;
    bool oldFixed;
    bool wasExecuted;

public:
    ToggleTaskFixedCommand(TaskManager* mgr, int index, bool oldValue);

    void execute() override;
    void undo() override;
    size_t getMemoryFootprint() const override;
};

/**
 * Command for toggling task rigid status
 */
class ToggleTaskRigidCommand : public TaskManagerCommand {
private:
    int taskIndex;
    bool oldRigid;
    bool wasExecuted;

public:
    ToggleTaskRigidCommand(TaskManager* mgr, int index, bool oldValue);

    void execute() override;
    void undo() override;
    size_t getMemoryFootprint() const override;
};

/**
 * Command for moving task up one position
 */
class MoveTaskUpCommand : public TaskManagerCommand {
private:
    int taskIndex;
    bool wasFixed; // Store if task was fixed before movement
    bool wasExecuted;

public:
    MoveTaskUpCommand(TaskManager* mgr, int index, bool taskWasFixed);

    void execute() override;
    void undo() override;
    size_t getMemoryFootprint() const override;
};

/**
 * Command for moving task down one position
 */
class MoveTaskDownCommand : public TaskManagerCommand {
private:
    int taskIndex;
    bool wasFixed; // Store if task was fixed before movement
    bool wasExecuted;

public:
    MoveTaskDownCommand(TaskManager* mgr, int index, bool taskWasFixed);

    void execute() override;
    void undo() override;
    size_t getMemoryFootprint() const override;
};

/**
 * Command for starting a task timer (sets current time and makes task fixed)
 * Includes cascading time propagation for subsequent tasks
 */
class StartTaskTimerCommand : public TaskManagerCommand {
private:
    struct TaskState {
        int index;
        std::string oldStartTime;
        std::string newStartTime;
        bool oldFixed;
        bool newFixed;
    };

    std::vector<TaskState> affectedTasks;
    std::string timerStartTime;
    bool wasExecuted;

public:
    StartTaskTimerCommand(TaskManager* mgr, int index);

    void execute() override;
    void undo() override;
    size_t getMemoryFootprint() const override;

private:
    void calculateCascadingUpdates(TaskManager* mgr, int startIndex);
    std::string calculateNextAvailableTime(const std::string& startTime, int durationMinutes);
    int timeStringToMinutes(const std::string& timeStr);
    std::string minutesToTimeString(int minutes);
};

/**
 * Command group that can contain multiple commands to be undone/redone as a unit
 */
class CommandGroup : public UndoableCommand {
private:
    std::vector<std::unique_ptr<UndoableCommand>> commands;
    std::string groupDescription;

public:
    CommandGroup(const std::string& description);

    void addCommand(std::unique_ptr<UndoableCommand> command);
    void execute() override;
    void undo() override;
    std::string getDescription() const override;
    size_t getMemoryFootprint() const override;
    bool isEmpty() const;
};

/**
 * Manages undo/redo operations with memory limits
 */
class UndoManager {
private:
    static constexpr size_t MAX_UNDO_HISTORY = 100;
    static constexpr size_t MAX_MEMORY_USAGE = 1024 * 1024; // 1MB

    std::vector<std::unique_ptr<UndoableCommand>> undoStack;
    std::vector<std::unique_ptr<UndoableCommand>> redoStack;
    size_t currentMemoryUsage;

    // Command grouping support
    std::unique_ptr<CommandGroup> currentGroup;
    bool groupingEnabled;

    /**
     * Enforce memory limits by removing oldest commands if necessary
     */
    void enforceMemoryLimits();

    /**
     * Clear the redo stack (called when a new command is executed)
     */
    void clearRedoStack();

public:
    UndoManager();
    ~UndoManager() = default;

    /**
     * Execute a command and add it to the undo stack
     */
    void executeCommand(std::unique_ptr<UndoableCommand> command);

    /**
     * Check if undo is possible
     */
    bool canUndo() const;

    /**
     * Check if redo is possible
     */
    bool canRedo() const;

    /**
     * Undo the last command
     */
    void undo();

    /**
     * Redo the last undone command
     */
    void redo();

    /**
     * Get description of the last undone operation
     */
    std::string getLastUndoDescription() const;

    /**
     * Get description of the last redoable operation
     */
    std::string getLastRedoDescription() const;

    /**
     * Get current memory usage in bytes
     */
    size_t getCurrentMemoryUsage() const;

    /**
     * Get number of operations in undo stack
     */
    size_t getUndoStackSize() const;

    /**
     * Get number of operations in redo stack
     */
    size_t getRedoStackSize() const;

    /**
     * Start a command group - subsequent commands will be grouped together
     */
    void startCommandGroup(const std::string& groupDescription);

    /**
     * End the current command group and add it to the undo stack
     */
    void endCommandGroup();

    /**
     * Check if currently in a command group
     */
    bool isGrouping() const;
};

#endif // UNDOMANAGER_H
