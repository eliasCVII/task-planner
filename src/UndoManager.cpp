#include "UndoManager.h"
#include "TaskManager.h"
#include "Act.h"
#include <ctime>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <algorithm>

// Helper function to get current time in HH:MM format
std::string getCurrentTimeString() {
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M");
    return oss.str();
}

// TaskManagerCommand Implementation
TaskManagerCommand::TaskManagerCommand(TaskManager* mgr, const std::string& desc)
    : manager(mgr), description(desc) {
}

std::string TaskManagerCommand::getDescription() const {
    return description;
}

// AddTaskCommand Implementation
AddTaskCommand::AddTaskCommand(TaskManager* mgr, const std::string& taskName,
                               const std::string& start, int len, bool rigid)
    : TaskManagerCommand(mgr, "Add task '" + taskName + "'"),
      name(taskName), startTime(start), length(len), isRigid(rigid),
      hasStartTime(true), insertIndex(-1) {
}

AddTaskCommand::AddTaskCommand(TaskManager* mgr, const std::string& taskName,
                               int len, bool rigid)
    : TaskManagerCommand(mgr, "Add task '" + taskName + "'"),
      name(taskName), startTime(""), length(len), isRigid(rigid),
      hasStartTime(false), insertIndex(-1) {
}

void AddTaskCommand::execute() {
    if (hasStartTime) {
        manager->addTask(name, startTime, length, isRigid);
    } else {
        manager->addTask(name, length, isRigid);
    }
    insertIndex = manager->taskSize() - 1; // Task was added at the end
}

void AddTaskCommand::undo() {
    if (insertIndex >= 0 && insertIndex < manager->taskSize()) {
        manager->deleteTask(insertIndex);
    }
}

size_t AddTaskCommand::getMemoryFootprint() const {
    return sizeof(*this) + name.size() + startTime.size();
}

// DeleteTaskCommand Implementation
DeleteTaskCommand::DeleteTaskCommand(TaskManager* mgr, int taskIndex)
    : TaskManagerCommand(mgr, "Delete task"),
      index(taskIndex), deletedLength(0), deletedRigid(false),
      deletedFixed(false), taskWasDeleted(false) {
}

void DeleteTaskCommand::execute() {
    if (index >= 0 && index < manager->taskSize()) {
        // Store the task data before deletion
        try {
            Act task = manager->getTask(index);
            deletedName = task.getName();
            deletedStartTime = task.getStartStr();
            deletedLength = task.getLength();
            deletedRigid = task.isRigid();
            deletedFixed = task.isFixed();

            // Update description with actual task name
            description = "Delete task '" + deletedName + "'";

            // Perform the deletion
            taskWasDeleted = manager->deleteTask(index);
        } catch (const std::exception& e) {
            std::cerr << "Error in DeleteTaskCommand::execute(): " << e.what() << std::endl;
            taskWasDeleted = false;
        }
    } else {
        taskWasDeleted = false;
    }
}

void DeleteTaskCommand::undo() {
    if (taskWasDeleted) {
        try {
            if (deletedFixed && !deletedStartTime.empty()) {
                // Restore fixed task with start time
                manager->insertTask(index, deletedName, deletedStartTime, deletedLength, deletedRigid);
            } else {
                // Restore flexible task
                manager->insertTask(index, deletedName, deletedLength, deletedRigid);
            }
        } catch (const std::exception& e) {
            std::cerr << "Error in DeleteTaskCommand::undo(): " << e.what() << std::endl;
        }
    }
}

size_t DeleteTaskCommand::getMemoryFootprint() const {
    return sizeof(*this) + deletedName.size() + deletedStartTime.size();
}

// EditTaskNameCommand Implementation
EditTaskNameCommand::EditTaskNameCommand(TaskManager* mgr, int index, const std::string& oldValue, const std::string& newValue)
    : TaskManagerCommand(mgr, "Changed task name from '" + oldValue + "' to '" + newValue + "'"),
      taskIndex(index), oldName(oldValue), newName(newValue), wasExecuted(false) {
}

void EditTaskNameCommand::execute() {
    if (taskIndex >= 0 && taskIndex < manager->taskSize()) {
        auto& task = manager->getTaskRef(taskIndex);
        task.setName(newName);
        wasExecuted = true;

        // Trigger recalculation
        bool hasWarnings = false;
        manager->calcActLen(hasWarnings);
        manager->calcStartTimes();
    }
}

void EditTaskNameCommand::undo() {
    if (wasExecuted && taskIndex >= 0 && taskIndex < manager->taskSize()) {
        auto& task = manager->getTaskRef(taskIndex);
        task.setName(oldName);

        // Trigger recalculation
        bool hasWarnings = false;
        manager->calcActLen(hasWarnings);
        manager->calcStartTimes();
    }
}

size_t EditTaskNameCommand::getMemoryFootprint() const {
    return sizeof(*this) + oldName.size() + newName.size();
}

// EditTaskStartTimeCommand Implementation
EditTaskStartTimeCommand::EditTaskStartTimeCommand(TaskManager* mgr, int index, const std::string& oldValue, const std::string& newValue, bool oldFixedState, bool newFixedState)
    : TaskManagerCommand(mgr, ""), taskIndex(index), oldStartTime(oldValue), newStartTime(newValue),
      oldFixed(oldFixedState), newFixed(newFixedState), wasExecuted(false) {
    // Set description with task name
    if (index >= 0 && index < mgr->taskSize()) {
        std::string taskName = mgr->getTaskRef(index).getName();
        std::string oldDisplay = oldValue.empty() ? "flexible" : oldValue;
        std::string newDisplay = newValue.empty() ? "flexible" : newValue;
        setDescription("Changed task '" + taskName + "' start time from " + oldDisplay + " to " + newDisplay);
    } else {
        setDescription("Changed task start time from '" + oldValue + "' to '" + newValue + "'");
    }
}

void EditTaskStartTimeCommand::execute() {
    if (taskIndex >= 0 && taskIndex < manager->taskSize()) {
        auto& task = manager->getTaskRef(taskIndex);

        if (newStartTime.empty()) {
            // Make task flexible
            if (task.isFixed()) {
                task.setFixed(); // Toggle to make it flexible
            }
        } else {
            // Set start time and make task fixed
            task.setStartTime(newStartTime);
            if (!task.isFixed()) {
                task.setFixed(); // Toggle to make it fixed
            }
        }

        wasExecuted = true;

        // Trigger recalculation
        bool hasWarnings = false;
        manager->calcActLen(hasWarnings);
        manager->calcStartTimes();
    }
}

void EditTaskStartTimeCommand::undo() {
    if (wasExecuted && taskIndex >= 0 && taskIndex < manager->taskSize()) {
        auto& task = manager->getTaskRef(taskIndex);

        // Restore old start time
        if (oldStartTime.empty()) {
            // Make task flexible
            if (task.isFixed()) {
                task.setFixed(); // Toggle to make it flexible
            }
        } else {
            // Set old start time
            task.setStartTime(oldStartTime);
        }

        // Restore old fixed state
        if (task.isFixed() != oldFixed) {
            task.setFixed(); // Toggle to match old state
        }

        // Trigger recalculation
        bool hasWarnings = false;
        manager->calcActLen(hasWarnings);
        manager->calcStartTimes();
    }
}

size_t EditTaskStartTimeCommand::getMemoryFootprint() const {
    return sizeof(*this) + oldStartTime.size() + newStartTime.size();
}

// EditTaskLengthCommand Implementation
EditTaskLengthCommand::EditTaskLengthCommand(TaskManager* mgr, int index, int oldValue, int newValue)
    : TaskManagerCommand(mgr, ""), taskIndex(index), oldLength(oldValue), newLength(newValue), wasExecuted(false) {
    // Set description with task name
    if (index >= 0 && index < mgr->taskSize()) {
        std::string taskName = mgr->getTaskRef(index).getName();
        setDescription("Changed task '" + taskName + "' length from " + std::to_string(oldValue) + " to " + std::to_string(newValue) + " minutes");
    } else {
        setDescription("Changed task length from " + std::to_string(oldValue) + " to " + std::to_string(newValue));
    }
}

void EditTaskLengthCommand::execute() {
    if (taskIndex >= 0 && taskIndex < manager->taskSize()) {
        auto& task = manager->getTaskRef(taskIndex);
        task.setLength(newLength);
        wasExecuted = true;

        // Trigger recalculation
        bool hasWarnings = false;
        manager->calcActLen(hasWarnings);
        manager->calcStartTimes();
    }
}

void EditTaskLengthCommand::undo() {
    if (wasExecuted && taskIndex >= 0 && taskIndex < manager->taskSize()) {
        auto& task = manager->getTaskRef(taskIndex);
        task.setLength(oldLength);

        // Trigger recalculation
        bool hasWarnings = false;
        manager->calcActLen(hasWarnings);
        manager->calcStartTimes();
    }
}

size_t EditTaskLengthCommand::getMemoryFootprint() const {
    return sizeof(*this);
}

// ToggleTaskFixedCommand Implementation
ToggleTaskFixedCommand::ToggleTaskFixedCommand(TaskManager* mgr, int index, bool oldValue)
    : TaskManagerCommand(mgr, ""), taskIndex(index), oldFixed(oldValue), wasExecuted(false) {
    // Set description with task name
    if (index >= 0 && index < mgr->taskSize()) {
        std::string taskName = mgr->getTaskRef(index).getName();
        setDescription("Toggled task '" + taskName + "' fixed status from " +
                      std::string(oldValue ? "Yes" : "No") + " to " +
                      std::string(!oldValue ? "Yes" : "No"));
    } else {
        setDescription("Toggled task fixed status from " + std::string(oldValue ? "Yes" : "No") + " to " + std::string(!oldValue ? "Yes" : "No"));
    }
}

void ToggleTaskFixedCommand::execute() {
    if (taskIndex >= 0 && taskIndex < manager->taskSize()) {
        auto& task = manager->getTaskRef(taskIndex);
        task.setFixed(); // Toggle fixed state
        wasExecuted = true;

        // Trigger recalculation
        bool hasWarnings = false;
        manager->calcActLen(hasWarnings);
        manager->calcStartTimes();
    }
}

void ToggleTaskFixedCommand::undo() {
    if (wasExecuted && taskIndex >= 0 && taskIndex < manager->taskSize()) {
        auto& task = manager->getTaskRef(taskIndex);

        // Toggle back to original state
        if (task.isFixed() != oldFixed) {
            task.setFixed(); // Toggle to restore original state
        }

        // Trigger recalculation
        bool hasWarnings = false;
        manager->calcActLen(hasWarnings);
        manager->calcStartTimes();
    }
}

size_t ToggleTaskFixedCommand::getMemoryFootprint() const {
    return sizeof(*this);
}

// ToggleTaskRigidCommand Implementation
ToggleTaskRigidCommand::ToggleTaskRigidCommand(TaskManager* mgr, int index, bool oldValue)
    : TaskManagerCommand(mgr, ""), taskIndex(index), oldRigid(oldValue), wasExecuted(false) {
    // Set description with task name
    if (index >= 0 && index < mgr->taskSize()) {
        std::string taskName = mgr->getTaskRef(index).getName();
        setDescription("Toggled task '" + taskName + "' rigid status from " +
                      std::string(oldValue ? "Yes" : "No") + " to " +
                      std::string(!oldValue ? "Yes" : "No"));
    } else {
        setDescription("Toggled task rigid status from " + std::string(oldValue ? "Yes" : "No") + " to " + std::string(!oldValue ? "Yes" : "No"));
    }
}

void ToggleTaskRigidCommand::execute() {
    if (taskIndex >= 0 && taskIndex < manager->taskSize()) {
        auto& task = manager->getTaskRef(taskIndex);
        task.setRigid(!task.isRigid()); // Toggle rigid state
        wasExecuted = true;

        // Trigger recalculation
        bool hasWarnings = false;
        manager->calcActLen(hasWarnings);
        manager->calcStartTimes();
    }
}

void ToggleTaskRigidCommand::undo() {
    if (wasExecuted && taskIndex >= 0 && taskIndex < manager->taskSize()) {
        auto& task = manager->getTaskRef(taskIndex);
        task.setRigid(oldRigid); // Restore original state

        // Trigger recalculation
        bool hasWarnings = false;
        manager->calcActLen(hasWarnings);
        manager->calcStartTimes();
    }
}

size_t ToggleTaskRigidCommand::getMemoryFootprint() const {
    return sizeof(*this);
}

// MoveTaskUpCommand Implementation
MoveTaskUpCommand::MoveTaskUpCommand(TaskManager* mgr, int index, bool taskWasFixed)
    : TaskManagerCommand(mgr, ""), taskIndex(index), wasFixed(taskWasFixed), wasExecuted(false) {
    // Set description with task name
    if (index >= 0 && index < mgr->taskSize()) {
        std::string taskName = mgr->getTaskRef(index).getName();
        setDescription("Moved task '" + taskName + "' up");
    } else {
        setDescription("Move task up");
    }
}

void MoveTaskUpCommand::execute() {
    if (taskIndex > 0 && taskIndex < manager->taskSize()) {
        // Remove fixed status if task was fixed (as per existing behavior)
        auto& task = manager->getTaskRef(taskIndex);
        if (task.isFixed()) {
            task.setFixed(); // Toggle to remove fixed status
        }

        if (manager->moveTaskUp(taskIndex)) {
            wasExecuted = true;

            // Trigger recalculation
            bool hasWarnings = false;
            manager->calcActLen(hasWarnings);
            manager->calcStartTimes();
        }
    }
}

void MoveTaskUpCommand::undo() {
    if (wasExecuted && taskIndex > 0 && taskIndex < manager->taskSize()) {
        // Move task back down to original position
        if (manager->moveTaskDown(taskIndex - 1)) {
            // Restore fixed status if it was originally fixed
            if (wasFixed) {
                auto& task = manager->getTaskRef(taskIndex);
                if (!task.isFixed()) {
                    task.setFixed(); // Toggle to restore fixed status
                }
            }

            // Trigger recalculation
            bool hasWarnings = false;
            manager->calcActLen(hasWarnings);
            manager->calcStartTimes();
        }
    }
}

size_t MoveTaskUpCommand::getMemoryFootprint() const {
    return sizeof(*this);
}

// MoveTaskDownCommand Implementation
MoveTaskDownCommand::MoveTaskDownCommand(TaskManager* mgr, int index, bool taskWasFixed)
    : TaskManagerCommand(mgr, ""), taskIndex(index), wasFixed(taskWasFixed), wasExecuted(false) {
    // Set description with task name
    if (index >= 0 && index < mgr->taskSize()) {
        std::string taskName = mgr->getTaskRef(index).getName();
        setDescription("Moved task '" + taskName + "' down");
    } else {
        setDescription("Move task down");
    }
}

void MoveTaskDownCommand::execute() {
    if (taskIndex >= 0 && taskIndex < manager->taskSize() - 1) {
        // Remove fixed status if task was fixed (as per existing behavior)
        auto& task = manager->getTaskRef(taskIndex);
        if (task.isFixed()) {
            task.setFixed(); // Toggle to remove fixed status
        }

        if (manager->moveTaskDown(taskIndex)) {
            wasExecuted = true;

            // Trigger recalculation
            bool hasWarnings = false;
            manager->calcActLen(hasWarnings);
            manager->calcStartTimes();
        }
    }
}

void MoveTaskDownCommand::undo() {
    if (wasExecuted && taskIndex >= 0 && taskIndex < manager->taskSize() - 1) {
        // Move task back up to original position
        if (manager->moveTaskUp(taskIndex + 1)) {
            // Restore fixed status if it was originally fixed
            if (wasFixed) {
                auto& task = manager->getTaskRef(taskIndex);
                if (!task.isFixed()) {
                    task.setFixed(); // Toggle to restore fixed status
                }
            }

            // Trigger recalculation
            bool hasWarnings = false;
            manager->calcActLen(hasWarnings);
            manager->calcStartTimes();
        }
    }
}

size_t MoveTaskDownCommand::getMemoryFootprint() const {
    return sizeof(*this);
}

// StartTaskTimerCommand Implementation
StartTaskTimerCommand::StartTaskTimerCommand(TaskManager* mgr, int index)
    : TaskManagerCommand(mgr, ""), wasExecuted(false) {

    if (index >= 0 && index < mgr->taskSize()) {
        timerStartTime = getCurrentTimeString();

        // Calculate all cascading updates needed
        calculateCascadingUpdates(mgr, index);

        // Set description based on affected tasks
        if (affectedTasks.size() == 1) {
            std::string taskName = mgr->getTaskRef(index).getName();
            setDescription("Started timer for task '" + taskName + "' at " + timerStartTime);
        } else {
            std::string taskName = mgr->getTaskRef(index).getName();
            setDescription("Started timer for task '" + taskName + "' at " + timerStartTime +
                          " (updated " + std::to_string(affectedTasks.size() - 1) + " subsequent tasks)");
        }
    } else {
        setDescription("Start task timer");
    }
}

void StartTaskTimerCommand::execute() {
    if (!affectedTasks.empty()) {
        // Apply all calculated updates
        for (const auto& taskState : affectedTasks) {
            if (taskState.index >= 0 && taskState.index < manager->taskSize()) {
                auto& task = manager->getTaskRef(taskState.index);

                // Set the new start time
                if (taskState.newStartTime.empty()) {
                    // Make task flexible
                    if (task.isFixed()) {
                        task.setFixed(); // Toggle to make flexible
                    }
                } else {
                    // Set specific start time
                    task.setStartTime(taskState.newStartTime);
                }

                // Set the new fixed status
                if (task.isFixed() != taskState.newFixed) {
                    task.setFixed(); // Toggle to match desired state
                }
            }
        }

        wasExecuted = true;

        // Trigger recalculation
        bool hasWarnings = false;
        manager->calcActLen(hasWarnings);
        manager->calcStartTimes();
    }
}

void StartTaskTimerCommand::undo() {
    if (wasExecuted && !affectedTasks.empty()) {
        // Restore all affected tasks to their original states
        for (const auto& taskState : affectedTasks) {
            if (taskState.index >= 0 && taskState.index < manager->taskSize()) {
                auto& task = manager->getTaskRef(taskState.index);

                // Restore old start time
                if (taskState.oldStartTime.empty()) {
                    // Task was originally flexible - make it flexible again
                    if (task.isFixed()) {
                        task.setFixed(); // Toggle to make it flexible
                    }
                } else {
                    // Task had a specific start time - restore it
                    task.setStartTime(taskState.oldStartTime);
                }

                // Restore old fixed state
                if (task.isFixed() != taskState.oldFixed) {
                    task.setFixed(); // Toggle to match old state
                }
            }
        }

        // Trigger recalculation
        bool hasWarnings = false;
        manager->calcActLen(hasWarnings);
        manager->calcStartTimes();
    }
}

size_t StartTaskTimerCommand::getMemoryFootprint() const {
    size_t total = sizeof(*this) + timerStartTime.size();
    for (const auto& taskState : affectedTasks) {
        total += sizeof(taskState) + taskState.oldStartTime.size() + taskState.newStartTime.size();
    }
    return total;
}

void StartTaskTimerCommand::calculateCascadingUpdates(TaskManager* mgr, int startIndex) {
    if (startIndex < 0 || startIndex >= mgr->taskSize()) {
        return;
    }

    // First, add the target task (the one Alt+B was pressed on)
    auto& targetTask = mgr->getTaskRef(startIndex);
    TaskState targetState;
    targetState.index = startIndex;
    targetState.oldStartTime = targetTask.getStartStr();
    targetState.oldFixed = targetTask.isFixed();
    targetState.newStartTime = timerStartTime;
    targetState.newFixed = true; // Timer start always makes task fixed
    affectedTasks.push_back(targetState);

    // Now check subsequent tasks for conflicts
    std::string currentEndTime = calculateNextAvailableTime(timerStartTime, targetTask.getLength());

    for (int i = startIndex + 1; i < mgr->taskSize(); ++i) {
        auto& task = mgr->getTaskRef(i);
        std::string taskStartTime = task.getStartStr();

        // Check if this task has a start time that would create a conflict
        bool hasConflict = false;
        if (!taskStartTime.empty()) {
            // Task has a specific start time - check if it's before the previous task ends
            int taskStartMinutes = timeStringToMinutes(taskStartTime);
            int currentEndMinutes = timeStringToMinutes(currentEndTime);

            if (taskStartMinutes < currentEndMinutes) {
                hasConflict = true;
            }
        }

        if (hasConflict) {
            // This task needs to be updated to resolve the conflict
            TaskState conflictState;
            conflictState.index = i;
            conflictState.oldStartTime = task.getStartStr();
            conflictState.oldFixed = task.isFixed();
            conflictState.newStartTime = currentEndTime; // Start when previous task ends
            conflictState.newFixed = true; // Make it fixed to maintain the sequence
            affectedTasks.push_back(conflictState);

            // Update currentEndTime for the next iteration
            currentEndTime = calculateNextAvailableTime(currentEndTime, task.getLength());
        } else if (!taskStartTime.empty()) {
            // Task has a valid start time that doesn't conflict - use it as the new baseline
            currentEndTime = calculateNextAvailableTime(taskStartTime, task.getLength());
        } else {
            // Task is flexible - it will be handled by calcStartTimes(), so we can stop cascading
            break;
        }
    }
}

std::string StartTaskTimerCommand::calculateNextAvailableTime(const std::string& startTime, int durationMinutes) {
    if (startTime.empty()) {
        return "";
    }

    int startMinutes = timeStringToMinutes(startTime);
    int endMinutes = startMinutes + durationMinutes;

    // Handle day overflow (24-hour wrap)
    if (endMinutes >= 24 * 60) {
        endMinutes = endMinutes % (24 * 60);
    }

    return minutesToTimeString(endMinutes);
}

int StartTaskTimerCommand::timeStringToMinutes(const std::string& timeStr) {
    if (timeStr.empty()) {
        return 0;
    }

    size_t colonPos = timeStr.find(':');
    if (colonPos == std::string::npos) {
        return 0;
    }

    try {
        int hours = std::stoi(timeStr.substr(0, colonPos));
        int minutes = std::stoi(timeStr.substr(colonPos + 1));
        return hours * 60 + minutes;
    } catch (const std::exception&) {
        return 0;
    }
}

std::string StartTaskTimerCommand::minutesToTimeString(int minutes) {
    // Handle negative minutes and day overflow
    while (minutes < 0) {
        minutes += 24 * 60;
    }
    minutes = minutes % (24 * 60);

    int hours = minutes / 60;
    int mins = minutes % 60;

    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << hours << ":"
        << std::setfill('0') << std::setw(2) << mins;
    return oss.str();
}

// CommandGroup Implementation
CommandGroup::CommandGroup(const std::string& description) : groupDescription(description) {
}

void CommandGroup::addCommand(std::unique_ptr<UndoableCommand> command) {
    if (command) {
        commands.push_back(std::move(command));
    }
}

void CommandGroup::execute() {
    // Execute all commands in order
    for (auto& command : commands) {
        command->execute();
    }
}

void CommandGroup::undo() {
    // Undo all commands in reverse order
    for (auto it = commands.rbegin(); it != commands.rend(); ++it) {
        (*it)->undo();
    }
}

std::string CommandGroup::getDescription() const {
    if (commands.size() == 1) {
        return commands[0]->getDescription();
    } else if (commands.size() > 1) {
        return groupDescription + " (" + std::to_string(commands.size()) + " operations)";
    }
    return groupDescription;
}

size_t CommandGroup::getMemoryFootprint() const {
    size_t total = sizeof(*this) + groupDescription.size();
    for (const auto& command : commands) {
        total += command->getMemoryFootprint();
    }
    return total;
}

bool CommandGroup::isEmpty() const {
    return commands.empty();
}

// UndoManager Implementation
UndoManager::UndoManager() : currentMemoryUsage(0), groupingEnabled(false) {
}

void UndoManager::executeCommand(std::unique_ptr<UndoableCommand> command) {
    if (!command) {
        return;
    }

    // Execute the command
    command->execute();

    if (groupingEnabled && currentGroup) {
        // Add to current group
        currentGroup->addCommand(std::move(command));
    } else {
        // Clear redo stack since we're adding a new command
        clearRedoStack();

        // Add to undo stack
        currentMemoryUsage += command->getMemoryFootprint();
        undoStack.push_back(std::move(command));

        // Enforce memory limits
        enforceMemoryLimits();
    }
}

bool UndoManager::canUndo() const {
    return !undoStack.empty();
}

bool UndoManager::canRedo() const {
    return !redoStack.empty();
}

void UndoManager::undo() {
    if (!canUndo()) {
        return;
    }

    // Get the last command
    auto command = std::move(undoStack.back());
    undoStack.pop_back();
    currentMemoryUsage -= command->getMemoryFootprint();

    // Undo the command
    command->undo();

    // Move to redo stack
    redoStack.push_back(std::move(command));
}

void UndoManager::redo() {
    if (!canRedo()) {
        return;
    }

    // Get the last undone command
    auto command = std::move(redoStack.back());
    redoStack.pop_back();

    // Re-execute the command
    command->execute();

    // Move back to undo stack
    currentMemoryUsage += command->getMemoryFootprint();
    undoStack.push_back(std::move(command));
}

std::string UndoManager::getLastUndoDescription() const {
    if (canUndo()) {
        return undoStack.back()->getDescription();
    }
    return "";
}

std::string UndoManager::getLastRedoDescription() const {
    if (canRedo()) {
        return redoStack.back()->getDescription();
    }
    return "";
}

size_t UndoManager::getCurrentMemoryUsage() const {
    return currentMemoryUsage;
}

size_t UndoManager::getUndoStackSize() const {
    return undoStack.size();
}

size_t UndoManager::getRedoStackSize() const {
    return redoStack.size();
}

void UndoManager::enforceMemoryLimits() {
    // Calculate total memory usage including redo stack
    size_t totalMemoryUsage = currentMemoryUsage;
    for (const auto& cmd : redoStack) {
        totalMemoryUsage += cmd->getMemoryFootprint();
    }

    // Remove oldest undo commands if over limits
    while ((undoStack.size() > MAX_UNDO_HISTORY || totalMemoryUsage > MAX_MEMORY_USAGE)
           && !undoStack.empty()) {
        size_t removedSize = undoStack.front()->getMemoryFootprint();
        currentMemoryUsage -= removedSize;
        totalMemoryUsage -= removedSize;
        undoStack.erase(undoStack.begin());
    }

    // If still over memory limit, clear some redo commands
    while (totalMemoryUsage > MAX_MEMORY_USAGE && !redoStack.empty()) {
        totalMemoryUsage -= redoStack.front()->getMemoryFootprint();
        redoStack.erase(redoStack.begin());
    }
}

void UndoManager::clearRedoStack() {
    redoStack.clear();
}

void UndoManager::startCommandGroup(const std::string& groupDescription) {
    // End any existing group first
    if (groupingEnabled && currentGroup && !currentGroup->isEmpty()) {
        endCommandGroup();
    }

    // Start new group
    currentGroup = std::make_unique<CommandGroup>(groupDescription);
    groupingEnabled = true;
}

void UndoManager::endCommandGroup() {
    if (groupingEnabled && currentGroup) {
        if (!currentGroup->isEmpty()) {
            // Clear redo stack since we're adding a new command
            clearRedoStack();

            // Add the group to undo stack
            currentMemoryUsage += currentGroup->getMemoryFootprint();
            undoStack.push_back(std::move(currentGroup));

            // Enforce memory limits
            enforceMemoryLimits();
        }

        currentGroup.reset();
        groupingEnabled = false;
    }
}

bool UndoManager::isGrouping() const {
    return groupingEnabled && currentGroup != nullptr;
}
