// Copyright (c) embedded ocean GmbH
#include "TemplateOutput.hpp"

#include "AbstractTemplateOutputHandler.hpp"
#include "TemplateOutputHandler.hpp"
#include "TemplateBatchTransaction.hpp"

#include <xentara/config/FallbackHandler.hpp>
#include <xentara/config/Resolver.hpp>
#include <xentara/data/DataType.hpp>
#include <xentara/data/ReadHandle.hpp>
#include <xentara/data/WriteHandle.hpp>
#include <xentara/model/Attribute.hpp>
#include <xentara/model/ForEachAttributeFunction.hpp>
#include <xentara/model/ForEachEventFunction.hpp>
#include <xentara/utils/json/decoder/Object.hpp>
#include <xentara/utils/json/decoder/Errors.hpp>

namespace xentara::plugins::templateDriver
{
	
using namespace std::literals;

TemplateOutput::Class TemplateOutput::Class::_instance;

auto TemplateOutput::loadConfig(const ConfigIntializer &initializer,
		utils::json::decoder::Object &jsonObject,
		config::Resolver &resolver,
		const config::FallbackHandler &fallbackHandler) -> void
{
	// Get a reference that allows us to modify our own config attributes
    auto &&configAttributes = initializer[Class::instance().configHandle()];

	// Go through all the members of the JSON object that represents this object
	bool ioBatchLoaded = false;
	for (auto && [name, value] : jsonObject)
    {
		if (name == "dataType"sv)
		{
			// Create the handler
			_handler = createHandler(value);
		}
		/// @todo use a more descriptive keyword, e.g. "poll"
		else if (name == "batchTransaction"sv)
		{
			resolver.submit<TemplateBatchTransaction>(value, [this](std::reference_wrapper<TemplateBatchTransaction> batchTransaction)
				{ 
					_batchTransaction = &batchTransaction.get();
					batchTransaction.get().addInput(*this);
					batchTransaction.get().addOutput(*this);
				});
			ioBatchLoaded = true;
		}
		/// @todo load custom configuration parameters
		else if (name == "TODO"sv)
		{
			/// @todo parse the value correctly
			auto todo = value.asNumber<std::uint64_t>();

			/// @todo check that the value is valid
			if (!"TODO")
			{
				/// @todo use an error message that tells the user exactly what is wrong
				utils::json::decoder::throwWithLocation(value, std::runtime_error("TODO is wrong with TODO parameter of template output"));
			}

			/// @todo set the appropriate member variables, and update configAttributes accordingly (if necessary) 
		}
		else
		{
			// Pass any unknown parameters on to the fallback handler, which will load the built-in parameters ("id" and "uuid"),
			// and throw an exception if the key is unknown
            fallbackHandler(name, value);
		}
    }

	// Make sure that a data type was specified
	if (!_handler)
	{
		/// @todo replace "template output" with a more descriptive name
		utils::json::decoder::throwWithLocation(jsonObject, std::runtime_error("Missing data type in template output"));
	}
	// Make sure that a batch transaction was specified
	if (!ioBatchLoaded)
	{
		/// @todo replace "batch transaction" and "template output" with more descriptive names
		utils::json::decoder::throwWithLocation(jsonObject, std::runtime_error("missing batch transaction in template output"));
	}
	/// @todo perform consistency and completeness checks
	if (!"TODO")
	{
		/// @todo use an error message that tells the user exactly what is wrong
		utils::json::decoder::throwWithLocation(jsonObject, std::runtime_error("TODO is wrong with template output"));
	}
}

auto TemplateOutput::createHandler(utils::json::decoder::Value &value) -> std::unique_ptr<AbstractTemplateOutputHandler>
{
	// Get the keyword from the value
	auto keyword = value.asString<std::string>();
	
	/// @todo use keywords that are appropriate to the I/O component
	if (keyword == "bool"sv)
	{
		return std::make_unique<TemplateOutputHandler<bool>>();
	}
	else if (keyword == "uint8"sv)
	{
		return std::make_unique<TemplateOutputHandler<std::uint8_t>>();
	}
	else if (keyword == "uint16"sv)
	{
		return std::make_unique<TemplateOutputHandler<std::uint16_t>>();
	}
	else if (keyword == "uint32"sv)
	{
		return std::make_unique<TemplateOutputHandler<std::uint32_t>>();
	}
	else if (keyword == "uint64"sv)
	{
		return std::make_unique<TemplateOutputHandler<std::uint64_t>>();
	}
	else if (keyword == "int8"sv)
	{
		return std::make_unique<TemplateOutputHandler<std::int8_t>>();
	}
	else if (keyword == "int16"sv)
	{
		return std::make_unique<TemplateOutputHandler<std::int16_t>>();
	}
	else if (keyword == "int32"sv)
	{
		return std::make_unique<TemplateOutputHandler<std::int32_t>>();
	}
	else if (keyword == "int64"sv)
	{
		return std::make_unique<TemplateOutputHandler<std::int64_t>>();
	}
	else if (keyword == "float32"sv)
	{
		return std::make_unique<TemplateOutputHandler<float>>();
	}
	else if (keyword == "float64"sv)
	{
		return std::make_unique<TemplateOutputHandler<double>>();
	}
	else if (keyword == "string"sv)
	{
		return std::make_unique<TemplateOutputHandler<std::string>>();
	}

	// The keyword is not known
	else
	{
		/// @todo replace "template output" with a more descriptive name
		utils::json::decoder::throwWithLocation(value, std::runtime_error("unknown data type in template output"));
	}

	return std::unique_ptr<AbstractTemplateOutputHandler>();
}

auto TemplateOutput::dataType() const -> const data::DataType &
{
	// dataType() must not be called before the configuration was loaded, so the handler should have been
	// created already.
	if (!_handler) [[unlikely]]
	{
		throw std::logic_error("internal error: xentara::plugins::templateDriver::TemplateOutput::dataType() called before configuration has been loaded");
	}

	// Forward the request to the handler
	return _handler->dataType();
}

auto TemplateOutput::directions() const -> io::Directions
{
	return io::Direction::Input | io::Direction::Output;
}

auto TemplateOutput::forEachAttribute(const model::ForEachAttributeFunction &function) const -> bool
{
	// forEachAttribute() must not be called before the configuration was loaded, so the handler should have been
	// created already.
	if (!_handler) [[unlikely]]
	{
		throw std::logic_error("internal error: xentara::plugins::templateDriver::TemplateOutput::forEachAttribute() called before configuration has been loaded");
	}
	// forEachAttribute() must not be called before references have been resolved, so the batch transaction should have been
	// set already.
	if (!_batchTransaction) [[unlikely]]
	{
		throw std::logic_error("internal error: xentara::plugins::templateDriver::TemplateOutput::forEachAttribute() called before cross references have been resolved");
	}

	return
		// Handle the handler attributes
		_handler->forEachAttribute(function, *_batchTransaction);

	/// @todo handle any additional attributes this class supports, including attributes inherited from the I/O component and the batch transaction
}

auto TemplateOutput::forEachEvent(const model::ForEachEventFunction &function) -> bool
{
	// forEachAttribute() must not be called before the configuration was loaded, so the handler should have been
	// created already.
	if (!_handler) [[unlikely]]
	{
		throw std::logic_error("internal error: xentara::plugins::templateDriver::TemplateOutput::forEachEvent() called before configuration has been loaded");
	}
	// forEachEvent() must not be called before references have been resolved, so the batch transaction should have been
	// set already.
	if (!_batchTransaction) [[unlikely]]
	{
		throw std::logic_error("internal error: xentara::plugins::templateDriver::TemplateOutput::forEachEvent() called before cross references have been resolved");
	}

	return
		// Handle the handler events
		_handler->forEachEvent(function, *_batchTransaction, sharedFromThis());

	/// @todo handle any additional events this class supports, including events inherited from the I/O component and the batch transaction
}

auto TemplateOutput::makeReadHandle(const model::Attribute &attribute) const noexcept -> std::optional<data::ReadHandle>
{
	// makeReadHandle() must not be called before the configuration was loaded, so the handler should have been
	// created already.
	if (!_handler) [[unlikely]]
	{
		// Don't throw an exception, because this function is noexcept
		return std::make_error_code(std::errc::invalid_argument);
	}
	// makeReadHandle() must not be called before references have been resolved, so the batch transaction should have been
	// set already.
	if (!_batchTransaction) [[unlikely]]
	{
		// Don't throw an exception, because this function is noexcept
		return std::make_error_code(std::errc::invalid_argument);
	}

	// Handle the handler attributes
	if (auto handle = _handler->makeReadHandle(attribute, *_batchTransaction))
	{
		return handle;
	}

	/// @todo handle any additional readable attributes this class supports, including attributes inherited from the I/O component and the batch transaction

	return std::nullopt;
}

auto TemplateOutput::makeWriteHandle(const model::Attribute &attribute) noexcept -> std::optional<data::WriteHandle>
{
	// makeWriteHandle() must not be called before the configuration was loaded, so the handler should have been
	// created already.
	if (!_handler) [[unlikely]]
	{
		// Don't throw an exception, because this function is noexcept
		return std::make_error_code(std::errc::invalid_argument);
	}
	// makeReadHandle() must not be called before references have been resolved, so the batch transaction should have been
	// set already.
	if (!_batchTransaction) [[unlikely]]
	{
		// Don't throw an exception, because this function is noexcept
		return std::make_error_code(std::errc::invalid_argument);
	}

	// Handle the handler attributes
	if (auto handle = _handler->makeWriteHandle(attribute, *_batchTransaction, sharedFromThis()))
	{
		return handle;
	}

	/// @todo handle any additional writable attributes this class supports, including attributes inherited from the I/O component and the batch transaction

	return std::nullopt;
}

auto TemplateOutput::attachInput(memory::Array &dataArray, std::size_t &eventCount) -> void
{
	// attachInput() must not be called before the configuration was loaded, so the handler should have been
	// created already.
	if (!_handler) [[unlikely]]
	{
		throw std::logic_error("internal error: xentara::plugins::templateDriver::TemplateOutput::attachInput() called before configuration has been loaded");
	}

	// Attach the read state of the handler
	_handler->attachReadState(dataArray, eventCount);
}

auto TemplateOutput::updateReadState(WriteSentinel &writeSentinel,
	std::chrono::system_clock::time_point timeStamp,
	const utils::eh::expected<std::reference_wrapper<const ReadCommand::Payload>, std::error_code> &payloadOrError,
	const CommonReadState::Changes &commonChanges,
	PendingEventList &eventsToRaise) -> void
{
	// updateReadState() must not be called before the configuration was loaded, so the handler should have been
	// created already.
	if (!_handler) [[unlikely]]
	{
		throw std::logic_error("internal error: xentara::plugins::templateDriver::TemplateOutput::updateReadState() called before configuration has been loaded");
	}

	// Forward the request to the handler
	_handler->updateReadState(writeSentinel, timeStamp, payloadOrError, commonChanges, eventsToRaise);
}

auto TemplateOutput::addToWriteCommand(WriteCommand &command) -> bool
{
	// addToWriteCommand() must not be called before the configuration was loaded, so the handler should have been
	// created already.
	if (!_handler) [[unlikely]]
	{
		throw std::logic_error("internal error: xentara::plugins::templateDriver::TemplateOutput::addToWriteCommand() called before configuration has been loaded");
	}

	// Forward the request to the handler
	return _handler->addToWriteCommand(command);
}

auto TemplateOutput::attachOutput(memory::Array &dataArray, std::size_t &eventCount) -> void
{
	// attachOutput() must not be called before the configuration was loaded, so the handler should have been
	// created already.
	if (!_handler) [[unlikely]]
	{
		throw std::logic_error("internal error: xentara::plugins::templateDriver::TemplateOutput::attachOutput() called before configuration has been loaded");
	}

	// Attach the write state of the handler
	_handler->attachWriteState(dataArray, eventCount);
}

auto TemplateOutput::updateWriteState(WriteSentinel &writeSentinel,
	std::chrono::system_clock::time_point timeStamp,
	std::error_code error,
	PendingEventList &eventsToRaise) -> void
{
	// updateWriteState() must not be called before the configuration was loaded, so the handler should have been
	// created already.
	if (!_handler) [[unlikely]]
	{
		throw std::logic_error("internal error: xentara::plugins::templateDriver::TemplateOutput::updateWriteState() called before configuration has been loaded");
	}

	// Forward the request to the handler
	_handler->updateWriteState(writeSentinel, timeStamp, error, eventsToRaise);
}

} // namespace xentara::plugins::templateDriver