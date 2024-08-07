// Copyright (c) embedded ocean GmbH
#include "TemplateIoComponent.hpp"

#include "Attributes.hpp"
#include "TemplateIoTransaction.hpp"
#include "TemplateInput.hpp"
#include "TemplateOutput.hpp"

#include <xentara/config/Errors.hpp>
#include <xentara/data/ReadHandle.hpp>
#include <xentara/memory/memoryResources.hpp>
#include <xentara/memory/WriteSentinel.hpp>
#include <xentara/model/Attribute.hpp>
#include <xentara/model/ForEachAttributeFunction.hpp>
#include <xentara/model/ForEachEventFunction.hpp>
#include <xentara/model/ForEachTaskFunction.hpp>
#include <xentara/process/EventList.hpp>
#include <xentara/process/ExecutionContext.hpp>
#include <xentara/skill/ElementFactory.hpp>
#include <xentara/utils/eh/currentErrorCode.hpp>
#include <xentara/utils/json/decoder/Object.hpp>
#include <xentara/utils/json/decoder/Errors.hpp>

#include <string_view>

#ifdef _WIN32
#	include <Windows.h>
#else
#	include <errno.h>
#endif

namespace xentara::plugins::templateDriver
{

using namespace std::literals;

auto TemplateIoComponent::load(utils::json::decoder::Object &jsonObject, config::Context &context) -> void
{
	// Go through all the members of the JSON object that represents this object
	for (auto && [name, value] : jsonObject)
    {
		/// @todo load configuration parameters
		if (name == "TODO"sv)
		{
			/// @todo parse the value correctly
			auto todo = value.asNumber<std::uint64_t>();

			/// @todo check that the value is valid
			if (!"TODO")
			{
				/// @todo use an error message that tells the user exactly what is wrong
				utils::json::decoder::throwWithLocation(value, std::runtime_error("TODO is wrong with TODO parameter of template I/O component"));
			}

			/// @todo set the appropriate member variables
		}
		else
		{
            config::throwUnknownParameterError(name);
		}
    }

	/// @todo perform consistency and completeness checks
	if (!"TODO")
	{
		/// @todo use an error message that tells the user exactly what is wrong
		utils::json::decoder::throwWithLocation(jsonObject, std::runtime_error("TODO is wrong with template I/O component"));
	}
}

auto TemplateIoComponent::performReconnectTask(const process::ExecutionContext &context) -> void
{
	// Only perform the reconnect if we are supposed to be connected in the first place
	if (_connectionRequestCount.load(std::memory_order_relaxed) == 0)
	{
		return;
	}
	// Also don't reconnect if we are already connected
	if (_handle)
	{
		return;
	}

	/// @todo check _lastError to see if a reconnect can succeed at all, and bail if it can't. A reconnect need not be attempted
	// if it requires non-existent hardware, like a missing network adapter or I/O card, for example.
	// see isConnectionError() for an example on how to check error codes.

	// Attempt a connection
	connect(context.scheduledTime());
}

auto TemplateIoComponent::connect(std::chrono::system_clock::time_point timeStamp) -> void
{
	try
	{
		/// @todo try to establish the connection, and set the _handle object

		/// @todo if the connect function does not throw errors, but uses return types or internal handle state,
		// throw an std::system_error here on failure, or call updateState() directly.
		
		// Note: If your connect function uses normal system error codes (errno on Linux or GetLastError() on Windows), you
		// should create std::error_codes using std::system_category(). If you are using a library and/or protocol that provides
		// its own error codes, you should define a custom error category.

		// The connection was successful
		updateState(timeStamp, std::error_code());
	}
	/// @todo if your connection function throws exceptions that are not derived from std::system_error, but that
	// still provide some sort of error code, you should catch those exceptions separately and wrap the error code in a custom
	// error category.
	catch (const std::exception &)
	{
		// Get the error from the current exception using this special utility function
		const auto error = utils::eh::currentErrorCode();
		
		// Update the state
		updateState(timeStamp, error);
	}
}

auto TemplateIoComponent::disconnect(std::chrono::system_clock::time_point timeStamp) -> void
{
	// Reset the handle in any case, even if we fail, because the connection state should be false after this
	auto handle = std::exchange(_handle, Handle());

	/// @todo close the connection, ignoring any errors. If the disconnect function can throw exceptions,
	// these shoudl be caucht and ignored.

	// This is always a graceful disconnect, regardless of what happened, so never include an error code.
	updateState(timeStamp, CustomError::NotConnected);
}

auto TemplateIoComponent::updateState(std::chrono::system_clock::time_point timeStamp, std::error_code error, const ErrorSink *excludeErrorSink)
	-> void
{
	// First, check if anything changed
	if (error == _lastError)
	{
		return;
	}

	// Get the old and new state
	const auto wasConnected = !_lastError;
	const auto connected = !error;
	
	// Make a write sentinel
	memory::WriteSentinel sentinel { _stateDataBlock };
	auto &state = *sentinel;
	const auto &oldState = sentinel.oldValue();

	// Update the state
	state._deviceState = !error;

	// Update the change time, if necessary. We always need to write the change time, even if it is the same as before,
	// because memory resources use swap-in.
	state._connectionTime = wasConnected != connected ? timeStamp : oldState._connectionTime;

	// Update the error code
	state._error = error;

	// Collect the events to raise
	process::StaticEventList<1> events;
	if (!wasConnected && connected)
	{
		events.push_back(_connectedEvent);
	}
	else if (wasConnected && !connected)
	{
		events.push_back(_disconnectedEvent);
	}

	// Commit the data and raise the events
	sentinel.commit(timeStamp, events);

	// Notify all error sinks
	for (auto &&sink : _errorSinks)
	{
		if (&sink.get() != excludeErrorSink)
		{
			sink.get().ioComponentStateChanged(timeStamp, error);
		}
	}
}

auto TemplateIoComponent::isConnectionError(std::error_code error) noexcept -> bool
{
	/// @todo check if this error affects the connection as a whole, and bail if it doesn't.
	// This function should return true on errors that signal that the entire I/O component has stopped working,
	// like timeouts and network errors, and false on errors thst only affect some inputs and/or outputs, such as
	// unknown input or output, type mismatch, range errors etc.

	// Example code suitable for socket errors:

	// Check system errors
	if (error.category() == std::system_category())
	{
		switch (error.value())
		{
	#ifdef _WIN32
		case WSAEBADF:
		case WSAENOTSOCK:
		case ERROR_INVALID_HANDLE:
		case WSAECONNRESET:
		case WSAECONNABORTED:
		case WSAENETRESET:
		case WSAESHUTDOWN:
		case WSAENETUNREACH:
		case WSAEHOSTUNREACH:
		case WSAEHOSTDOWN:
		case WSAENOTCONN:
		case ERROR_BROKEN_PIPE:
	#else // _WIN32
		case EBADF:
		case ECONNRESET:
		case ECONNABORTED:
		case ENETRESET:
		case ESHUTDOWN:
		case ENETUNREACH:
		case EHOSTUNREACH:
		case EHOSTDOWN:
		case ENOTCONN:
		case EPIPE:
	#endif // _WIN32
			return true;

		default:
			return false;
		}
	}

	// Check custom errors:
	if (error.category() == customErrorCategory())
	{
		switch (CustomError(error.value()))
		{
		case CustomError::NotConnected:
		case CustomError::UnknownError:
			/// @todo add case statements for other relevant custom errors (like e.g. timeout) here
			return true;

		case CustomError::NoError:
		case CustomError::NoData:
		default:
			return false;
		}
	}

	// No other categories need apply
	else
	{
		return false;
	}
}

auto TemplateIoComponent::requestConnect(std::chrono::system_clock::time_point timeStamp) noexcept -> void
{
	// increment the count
	const auto oldCount = _connectionRequestCount++;

	// connect if the old count was 0
	if (oldCount == 0)
	{
		connect(timeStamp);
	}
}

auto TemplateIoComponent::requestDisconnect(std::chrono::system_clock::time_point timeStamp) noexcept -> void
{
	// decrement the count
	const auto newCount = --_connectionRequestCount;

	// disconnect if the new count is 0
	if (newCount == 0)
	{
		disconnect(timeStamp);
	}
}

auto TemplateIoComponent::handleError(std::chrono::system_clock::time_point timeStamp, std::error_code error, const ErrorSink *sender) noexcept -> void
{
	// Ignore any new errors if we already have an error (the first error always wins).
	if (_lastError)
	{
		return;
	}
	// Check if this error affects the connection as a whole, and bail if it doesn't. This function shoudl
	if (!isConnectionError(error))
	{
		return;
	}

	// Reset the handle
	/// @todo gracefully close the handle, if this is necessary
	_handle = Handle();

	// update the error state
	updateState(timeStamp, error, sender);
}

auto TemplateIoComponent::createChildElement(const skill::Element::Class &elementClass, skill::ElementFactory &factory)
	-> std::shared_ptr<skill::Element>
{
	if (&elementClass == &TemplateInput::Class::instance())
	{
		return factory.makeShared<TemplateInput>(*this);
	}
	else if (&elementClass == &TemplateOutput::Class::instance())
	{
		return factory.makeShared<TemplateOutput>(*this);
	}
	else if (&elementClass == &TemplateIoTransaction::Class::instance())
	{
		return factory.makeShared<TemplateIoTransaction>(*this);
	}

	/// @todo add any other supported child element types

	return nullptr;
}

auto TemplateIoComponent::forEachAttribute(const model::ForEachAttributeFunction &function) const -> bool
{
	/// @todo handle any additional attributes this class supports
	return
		function(model::Attribute::kDeviceState) ||
		function(attributes::kConnectionTime) ||
		function(attributes::kDeviceError);
}

auto TemplateIoComponent::forEachEvent(const model::ForEachEventFunction &function) -> bool
{
	// Handle all the events we support
	return
		function(process::Event::kConnected, sharedFromThis(&_connectedEvent)) ||
		function(process::Event::kDisconnected, sharedFromThis(&_disconnectedEvent));

	/// @todo handle any additional events this class supports
}

auto TemplateIoComponent::forEachTask(const model::ForEachTaskFunction &function) -> bool
{
	// Handle all the tasks we support
	return
		function(process::Task::kReconnect, sharedFromThis(&_reconnectTask));

	/// @todo handle any additional tasks this class supports
}

auto TemplateIoComponent::makeReadHandle(const model::Attribute &attribute) const noexcept -> std::optional<data::ReadHandle>
{
	// Try our attributes
	if (attribute == model::Attribute::kDeviceState)
	{
		return _stateDataBlock.member(&State::_deviceState);
	}
	else if (attribute == attributes::kConnectionTime)
	{
		return _stateDataBlock.member(&State::_connectionTime);
	}
	else if (attribute == attributes::kDeviceError)
	{
		return _stateDataBlock.member(&State::_error);
	}

	/// @todo handle any additional readable attributes this class supports

	// Nothing found
	return std::nullopt;
}

auto TemplateIoComponent::realize() -> void
{
	// Create the data block
	_stateDataBlock.create(memory::memoryResources::data());
}

auto TemplateIoComponent::ReconnectTask::preparePreOperational(const process::ExecutionContext &context) -> Status
{
	// Request a connection
	_target.get().requestConnect(context.scheduledTime());

	return Status::Ready;
}

auto TemplateIoComponent::ReconnectTask::preOperational(const process::ExecutionContext &context) -> Status
{
	// Do the same as in operational
	operational(context);

	return Status::Ready;
}

auto TemplateIoComponent::ReconnectTask::operational(const process::ExecutionContext &context) -> void
{
	_target.get().performReconnectTask(context);
}

auto TemplateIoComponent::ReconnectTask::preparePostOperational(const process::ExecutionContext &context) -> Status
{
	// Everything in the post operational stage is optional, so we can report ready right away
	return Status::Ready;
}

auto TemplateIoComponent::ReconnectTask::postOperational(const process::ExecutionContext &context) -> Status
{
	// We just do the same thing as in the operational stage
	operational(context);

	return Status::Ready;
}

auto TemplateIoComponent::ReconnectTask::finishPostOperational(const process::ExecutionContext &context) -> void
{
	// Request a disconnect
	_target.get().requestDisconnect(context.scheduledTime());
}

} // namespace xentara::plugins::templateDriver