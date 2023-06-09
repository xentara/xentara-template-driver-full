// Copyright (c) embedded ocean GmbH
#include "PerValueReadState.hpp"

#include "Attributes.hpp"

#include <xentara/memory/WriteSentinel.hpp>

namespace xentara::plugins::templateDriver
{

template <std::regular DataType>
auto PerValueReadState<DataType>::forEachAttribute(const model::ForEachAttributeFunction &function) const -> bool
{
	// Handle all the attributes we support
	return
		function(model::Attribute::kChangeTime);
}

template <std::regular DataType>
auto PerValueReadState<DataType>::forEachEvent(const model::ForEachEventFunction &function, std::shared_ptr<void> parent) -> bool
{
	// Handle all the events we support
	return
		function(model::Attribute::kValue, std::shared_ptr<process::Event>(parent, &_valueChangedEvent)) ||
		function(process::Event::kChanged, std::shared_ptr<process::Event>(parent, &_changedEvent));
}

template <std::regular DataType>
auto PerValueReadState<DataType>::makeReadHandle(const DataBlock &dataBlock,
	const model::Attribute &attribute) const noexcept -> std::optional<data::ReadHandle>
{
	// Try each readable attribute
	if (attribute == model::Attribute::kChangeTime)
	{
		return dataBlock.member(_stateHandle, &State::_changeTime);
	}

	return std::nullopt;
}

template <std::regular DataType>
auto PerValueReadState<DataType>::valueReadHandle(const DataBlock &dataBlock) const noexcept -> data::ReadHandle
{
	return dataBlock.member(_stateHandle, &State::_value);
}

template <std::regular DataType>
auto PerValueReadState<DataType>::attach(memory::Array &dataArray, std::size_t &eventCount) -> void
{
	// Add the state to the array
	_stateHandle = dataArray.appendObject<State>();

	// Add the number of events that can be triggered at once, which is all of them.
	eventCount += 2;
}

template <std::regular DataType>
auto PerValueReadState<DataType>::update(
	WriteSentinel &writeSentinel,
	std::chrono::system_clock::time_point timeStamp,
	const utils::eh::expected<DataType, std::error_code> &valueOrError,
	const CommonReadState::Changes &commonChanges,
	PendingEventList &eventsToFire) -> void
{
	// Get the correct array entry
	auto &state = writeSentinel[_stateHandle];
	const auto &oldState = writeSentinel.oldValues()[_stateHandle];

	// Set the value, replacing errors with a default constructed value
	state._value = valueOrError.value_or(DataType());

	// Detect changes
	const auto valueChanged = state._value != oldState._value;
	const auto changed = valueChanged || commonChanges;

	// Update the change time, if necessary. We always need to write the change time, even if it is the same as before,
	// because the memory resource might use swap-in.
	state._changeTime = changed ? timeStamp : oldState._changeTime;

	// Cause the correct events to be fired
	if (valueChanged)
	{
		eventsToFire.push_back(_valueChangedEvent);
	}
	if (changed)
	{
		eventsToFire.push_back(_changedEvent);
	}
}

/// @class xentara::plugins::templateDriver::PerValueReadState
/// @todo change list of template instantiations to the supported types
template class PerValueReadState<bool>;
template class PerValueReadState<std::uint8_t>;
template class PerValueReadState<std::uint16_t>;
template class PerValueReadState<std::uint32_t>;
template class PerValueReadState<std::uint64_t>;
template class PerValueReadState<std::int8_t>;
template class PerValueReadState<std::int16_t>;
template class PerValueReadState<std::int32_t>;
template class PerValueReadState<std::int64_t>;
template class PerValueReadState<float>;
template class PerValueReadState<double>;
template class PerValueReadState<std::string>;

} // namespace xentara::plugins::templateDriver