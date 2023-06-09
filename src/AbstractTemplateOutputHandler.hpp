// Copyright (c) embedded ocean GmbH
#pragma once

#include "Types.hpp"
#include "CommonReadState.hpp"
#include "ReadCommand.hpp"

#include <xentara/data/DataType.hpp>
#include <xentara/data/ReadHandle.hpp>
#include <xentara/data/WriteHandle.hpp>
#include <xentara/memory/Array.hpp>
#include <xentara/memory/WriteSentinel.hpp>
#include <xentara/model/Attribute.hpp>
#include <xentara/model/ForEachAttributeFunction.hpp>
#include <xentara/model/ForEachEventFunction.hpp>
#include <xentara/process/Event.hpp>

#include <functional>
#include <string_view>
#include <memory>
#include <chrono>
#include <optional>

namespace xentara::plugins::templateDriver
{

class TemplateBatchTransaction;
class WriteCommand;

// Base class for data type specific functionality for TemplateOutput.
/// @todo rename this class to something more descriptive
class AbstractTemplateOutputHandler
{
public:
	/// @brief Virtual destructor
	/// @note The destructor is pure virtual (= 0) to ensure that this class will remain abstract, even if we should remove all
	/// other pure virtual functions later. This is not necessary, of course, but prevents the abstract class from becoming
	/// instantiable by accident as a result of refactoring.
	virtual ~AbstractTemplateOutputHandler() = 0;

	/// @brief Returns the data type
	virtual auto dataType() const -> const data::DataType & = 0;

	/// @brief Iterates over all the attributes.
	/// @param function The function that should be called for each attribute
	/// @param batchTransaction The batch transaction this output is attached to. This is used to handle inherited attributes.
	/// @return The return value of the last function call
	virtual auto forEachAttribute(const model::ForEachAttributeFunction &function, TemplateBatchTransaction &batchTransaction) const -> bool = 0;

	/// @brief Iterates over all the events that belong to this state.
	/// @param function The function that should be called for each events
	/// @param batchTransaction The batch transaction this output is attached to. This is used to handle inherited events.
	/// @param parent
	/// @parblock
	/// A shared pointer to the containing object.
	/// 
	/// The pointer is used in the aliasing constructor of std::shared_ptr when constructing the event pointers,
	/// so that they will share ownership information with pointers to the parent object.
	/// @endparblock
	/// @return The return value of the last function call
	virtual auto forEachEvent(const model::ForEachEventFunction &function, TemplateBatchTransaction &batchTransaction, std::shared_ptr<void> parent) -> bool = 0;

	/// @brief Creates a read-handle for an attribute.
	/// @param attribute The attribute to create the handle for
	/// @param batchTransaction The batch transaction this output is attached to. This is used to handle inherited attributes.
	/// @return A read handle for the attribute, or std::nullopt if the attribute is unknown
	virtual auto makeReadHandle(const model::Attribute &attribute, TemplateBatchTransaction &batchTransaction) const noexcept -> std::optional<data::ReadHandle> = 0;

	/// @brief Creates a write-handle for an attribute that belong to this handler.
	/// @param attribute The attribute to create the handle for
	/// @param batchTransaction The batch transaction this output is attached to. This is used to handle inherited attributes.
	/// @param parent
	/// @parblock
	/// A shared pointer to the containing object.
	/// 
	/// The pointer is used in the aliasing constructor of std::shared_ptr when constructing the write handle
	/// so that any contained pointers will share ownership information with pointers to the parent object.
	/// @endparblock
	/// @return A write handle for the attribute, or std::nullopt if the attribute is unknown
	virtual auto makeWriteHandle(const model::Attribute &attribute, TemplateBatchTransaction &batchTransaction, std::shared_ptr<void> parent) noexcept -> std::optional<data::WriteHandle> = 0;
	
	/// @brief Attaches the read state to a batch transaction
	/// @param dataArray The data array that the attributes should be added to. The caller will use the information in this array
	/// to allocate the data block.
	/// @param eventCount A variable that counts the total number of events than can be fired for a single update.
	/// The maximum number of events that update() will request to be fired will be added to this variable. The caller will use this
	/// event count to preallocate a buffer when collecting the events to fire after an update.
	virtual auto attachReadState(memory::Array &dataArray, std::size_t &eventCount) -> void = 0;

	/// @brief Updates the read state and collects the events to send
	/// @param writeSentinel A write sentinel for the data block the data is stored in
	/// @param timeStamp The update time stamp
	/// @param payloadOrError This is a variant-like type that will hold either the payload of the read command, or an std::error_code object
	/// containing a read error.
	/// @param commonChanges An object containing information about which parts of the common read state changed, if any.
	/// @param eventsToFire Any events that need to be fired as a result of the update will be added to this
	/// list. The events will not be fired directly, because the write sentinel needs to be commited first,
	/// which is done by the caller.
	/// @todo add parameters needed to decode the value from the payload of a read command, like e.g. a data offset.
	virtual auto updateReadState(WriteSentinel &writeSentinel,
		std::chrono::system_clock::time_point timeStamp,
		const utils::eh::expected<std::reference_wrapper<const ReadCommand::Payload>, std::error_code> &payloadOrError,
		const CommonReadState::Changes &commonChanges,
		PendingEventList &eventsToFire) -> void = 0;
		
	/// @brief Attaches the write state to a batch transaction
	/// @param dataArray The data array that the attributes should be added to. The caller will use the information in this array
	/// to allocate the data block.
	/// @param eventCount A variable that counts the total number of events than can be fired for a single update.
	/// The maximum number of events that update() will request to be fired will be added to this variable. The caller will use this
	/// event count to preallocate a buffer when collecting the events to fire after an update.
	virtual auto attachWriteState(memory::Array &dataArray, std::size_t &eventCount) -> void = 0;

	/// @brief Adds any pending output value to a write command.
	/// @param command The write command to add the value to.
	/// @return This function must return *true* if data was added, or *false* if no value was pending.
	virtual auto addToWriteCommand(WriteCommand &command) -> bool = 0;

	/// @brief Updates the write state and collects the events to send
	/// @param writeSentinel A write sentinel for the data block the data is stored in
	/// @param timeStamp The update time stamp
	/// @param error The error code, or a default constructed std::error_code object if no error occurred
	/// @param eventsToFire Any events that need to be fired as a result of the update will be added to this
	/// list. The events will not be fired directly, because the write sentinel needs to be commited first,
	/// which is done by the caller.
	virtual auto updateWriteState(
		WriteSentinel &writeSentinel,
		std::chrono::system_clock::time_point timeStamp,
		std::error_code error,
		PendingEventList &eventsToFire) -> void = 0;
};

inline AbstractTemplateOutputHandler::~AbstractTemplateOutputHandler() = default;

} // namespace xentara::plugins::templateDriver