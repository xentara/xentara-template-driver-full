// Copyright (c) embedded ocean GmbH
#include "PerValueReadState.hpp"

#include "Attributes.hpp"

#include <xentara/memory/WriteSentinel.hpp>

namespace xentara::plugins::templateDriver
{

template <std::regular DataType>
auto PerValueReadState<DataType>::resolveAttribute(std::u16string_view name) -> const model::Attribute *
{
	// Check all the attributes we support
	return model::Attribute::resolve(name,
		model::Attribute::kChangeTime);
}

template <std::regular DataType>
auto PerValueReadState<DataType>::resolveEvent(std::u16string_view name, std::shared_ptr<void> parent) -> std::shared_ptr<process::Event>
{
	// Check all the events we support
	if (name == model::Attribute::kValue)
	{
		return std::shared_ptr<process::Event>(parent, &_valueChangedEvent);
	}
	else if (name == process::Event::kChanged)
	{
		return std::shared_ptr<process::Event>(parent, &_changedEvent);
	}

	// The event name is not known
	return nullptr;
}

template <std::regular DataType>
auto PerValueReadState<DataType>::readHandle(const DataBlock &dataBlock,
	const model::Attribute &attribute) const noexcept -> std::optional<data::ReadHandle>
{
	// Try reach readable attribute
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
	const utils::eh::Failable<DataType> &valueOrError,
	const CommonReadState::Changes &commonChanges,
	PendingEventList &eventsToFire) -> void
{
	// Get the correct array entry
	auto &state = writeSentinel[_stateHandle];
	const auto &oldState = writeSentinel.oldValues()[_stateHandle];

	// Set the value, replacing errors with a default constructed value
	state._value = valueOrError.valueOr(DataType());

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

} // namespace xentara::plugins::templateDriver